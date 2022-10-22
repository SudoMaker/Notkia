#include "common.h"

typedef enum sx126x_hal_status_e
{
	SX126X_HAL_STATUS_OK    = 0,
	SX126X_HAL_STATUS_ERROR = 3,
} sx126x_hal_status_t;

static int gpfd_nrst = -1, gpfd_busy = -1, gpfd_dio1 = -1, gpfd_dio2 = -1;
static int gpevfd_dio1 = -1, gpevfd_dio2 = -1;
static int epfd = -1;
static pthread_t tid_ev;
static pthread_mutex_t spi_lock = PTHREAD_MUTEX_INITIALIZER;
static void (*isr_funcp)(unsigned int dio1_events, unsigned int dio2_events, unsigned int dio3_events, void *userp) = NULL;
static void *isr_userp = NULL;

static int gpio_open_one(int chip, int line, uint32_t req_flags, const char *tag) {
	char pathbuf[24];
	snprintf(pathbuf, sizeof(pathbuf)-1, "/dev/gpiochip%d", chip);
	int fd = open(pathbuf, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "failed to open GPIO device `%s': %s\n", pathbuf, strerror(errno));
		abort();
	}

	struct gpiohandle_request gl_req = {0};
	gl_req.lineoffsets[0] = line;
	gl_req.default_values[0] = 1;
	gl_req.flags = req_flags;
	gl_req.lines = 1;
	strcpy(gl_req.consumer_label, tag);

retry_lock:
	if (ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &gl_req)) {
		if (errno == EBUSY) {
			goto retry_lock;
		}
		fprintf(stderr, "failed to request GPIO line %d: %s\n", line, strerror(errno));
		abort();
	}

	close(fd);

	printf("GPIO: %s @ {%d, %d}\n", tag, chip, line);

	return gl_req.fd;
}

static int gpio_open_one_event(int chip, int line, uint32_t handle_flags, uint32_t event_flags, const char *tag) {
	char pathbuf[24];
	snprintf(pathbuf, sizeof(pathbuf)-1, "/dev/gpiochip%d", chip);
	int fd = open(pathbuf, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "failed to open GPIO device `%s': %s\n", pathbuf, strerror(errno));
		abort();
	}

	struct gpioevent_request gl_req = {0};
	gl_req.lineoffset = line;
	gl_req.handleflags = handle_flags;
	gl_req.eventflags = event_flags;
	strcpy(gl_req.consumer_label, tag);

retry_lock:
	if (ioctl(fd, GPIO_GET_LINEEVENT_IOCTL, &gl_req)) {
		if (errno == EBUSY) {
			goto retry_lock;
		}
		fprintf(stderr, "failed to request GPIO event for line %d: %s\n", line, strerror(errno));
		abort();
	}

	close(fd);

	printf("GPIO Event: %s @ {%d, %d}\n", tag, chip, line);

	return gl_req.fd;
}

static inline uint8_t gpio_read(int fd) {
	struct gpiohandle_data data;

	if (ioctl(fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data)) {
		perror("failed to read value from line");
		abort();
	}

	return data.values[0];
}

static inline void gpio_write(int fd, uint8_t val) {
	struct gpiohandle_data data;
	data.values[0] = val;

	if (ioctl(fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data)) {
		perror("failed to write value to line");
		abort();
	}
}

void sx126x_hal_linux_gpio_init(int reset_gpio[2], int busy_gpio[2], int dio1_gpio[2], int dio2_gpio[2]) {
	gpfd_nrst = gpio_open_one(reset_gpio[0], reset_gpio[1], GPIOHANDLE_REQUEST_OUTPUT, "SX126x NRST");
	gpfd_busy = gpio_open_one(busy_gpio[0], busy_gpio[1], GPIOHANDLE_REQUEST_INPUT, "SX126x BUSY");
//	gpfd_dio1 = gpio_open_one(dio1_gpio[0], dio1_gpio[1], GPIOHANDLE_REQUEST_INPUT, "SX126x DIO1");
//	gpfd_dio2 = gpio_open_one(dio2_gpio[0], dio2_gpio[1], GPIOHANDLE_REQUEST_INPUT, "SX126x DIO2");

	gpevfd_dio1 = gpio_open_one_event(dio1_gpio[0], dio1_gpio[1], GPIOHANDLE_REQUEST_INPUT, GPIOEVENT_REQUEST_RISING_EDGE, "SX126x DIO1");
	gpevfd_dio2 = gpio_open_one_event(dio2_gpio[0], dio2_gpio[1], GPIOHANDLE_REQUEST_INPUT, GPIOEVENT_REQUEST_RISING_EDGE, "SX126x DIO2");
}



static int spifd = -1;
static struct spi_ioc_transfer spi_tr;

void sx126x_hal_linux_spi_init(const char *dev_path) {
	int fd = open(dev_path, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "failed to open SPI device `%s': %s\n", dev_path, strerror(errno));
		abort();
	}

	int spi_mode = SPI_MODE_0;
	if (ioctl(fd, SPI_IOC_WR_MODE32, &spi_mode) < 0) {
		fprintf(stderr, "failed to set SPI mode 0x%02x: %s\n", spi_mode, strerror(errno));
		abort();
	}

	int spi_speed = 8000000;
	if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0) {
		fprintf(stderr, "failed to set SPI speed to %d: %s\n", spi_speed, strerror(errno));
		abort();
	}

	spifd = fd;

	printf("SPI: %s @ %d Hz\n", dev_path, spi_speed);
}

static inline void wait_busy() {
	while (gpio_read(gpfd_busy) == 1) {
		sched_yield();
	}
}

static void *event_thread(void *userp) {
	struct epoll_event evs[2];

	while (1) {
		unsigned int dio_events[3] = {0, 0, 0};
		int rc_ep = epoll_wait(epfd, evs, 2, 5000);

		if (rc_ep > 0) {
			for (int i=0; i<rc_ep; i++) {
				struct gpioevent_data gpio_event;

				ssize_t rrc = read(evs[i].data.fd, &gpio_event, sizeof(struct gpioevent_data));

				if (rrc == sizeof(struct gpioevent_data)) {
					if (evs[i].data.fd == gpevfd_dio1) {
						dio_events[0] |= gpio_event.id;
					} else {
						dio_events[1] |= gpio_event.id;
					}
				} else {
					printf("rrc = %zd\n", rrc);
					abort();
				}
			}

			if (isr_funcp) {
				isr_funcp(dio_events[0], dio_events[1], 0, NULL);
			}
		}
	}
}


static void sx126x_hal_linux_event_init() {
	epfd = epoll_create(43);

	if (epfd < 0) {
		perror("epoll_create");
		abort();
	}

	struct epoll_event ev = {
		.events = EPOLLIN
	};

	ev.data.fd = gpevfd_dio1;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, gpevfd_dio1, &ev)) {
		perror("epoll_ctl");
		abort();
	}

	ev.data.fd = gpevfd_dio2;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, gpevfd_dio2, &ev)) {
		perror("epoll_ctl");
		abort();
	}

	if (pthread_create(&tid_ev, NULL, event_thread, NULL)) {
		perror("pthread_create");
		abort();
	}

	printf("Event thread started\n");

}

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */


void sx126x_hal_linux_init(const char *spidev_path, int reset_gpio[2], int busy_gpio[2], int dio1_gpio[2], int dio2_gpio[2]) {
	sx126x_hal_linux_spi_init(spidev_path);
	sx126x_hal_linux_gpio_init(reset_gpio, busy_gpio, dio1_gpio, dio2_gpio);
	sx126x_hal_linux_event_init();
}

void sx126x_hal_linux_set_interrupt_handler(void (*funcp)(unsigned int, unsigned int, unsigned int, void *), void *userp) {
	isr_funcp = funcp;
	isr_userp = userp;
}

/**
 * Radio data transfer - write
 *
 * @remark Shall be implemented by the user
 *
 * @param [in] context          Radio implementation parameters
 * @param [in] command          Pointer to the buffer to be transmitted
 * @param [in] command_length   Buffer size to be transmitted
 * @param [in] data             Pointer to the buffer to be transmitted
 * @param [in] data_length      Buffer size to be transmitted
 *
 * @returns Operation status
 */
sx126x_hal_status_t sx126x_hal_write(const void *context, const uint8_t *command, const uint16_t command_length,
				     const uint8_t *data, const uint16_t data_length) {
	wait_busy();

	pthread_mutex_lock(&spi_lock);

	struct spi_ioc_transfer xfer[2];

	memset(xfer, 0, sizeof(xfer));

	int msg_cnt = 1;

	xfer[0].tx_buf = (unsigned long)command;
	xfer[0].len = command_length;

	if (data_length) {
		xfer[1].tx_buf = (unsigned long)data;
		xfer[1].len = data_length;

		msg_cnt++;
	}

	if (ioctl(spifd, SPI_IOC_MESSAGE(msg_cnt), xfer) < 0) {
		perror("SPI_IOC_MESSAGE");
		abort();
	}

	pthread_mutex_unlock(&spi_lock);

	return SX126X_HAL_STATUS_OK;
}

/**
 * Radio data transfer - read
 *
 * @remark Shall be implemented by the user
 *
 * @param [in] context          Radio implementation parameters
 * @param [in] command          Pointer to the buffer to be transmitted
 * @param [in] command_length   Buffer size to be transmitted
 * @param [in] data             Pointer to the buffer to be received
 * @param [in] data_length      Buffer size to be received
 *
 * @returns Operation status
 */
sx126x_hal_status_t sx126x_hal_read (const void *context, const uint8_t *command, const uint16_t command_length,
				     uint8_t *data, const uint16_t data_length) {
	wait_busy();

	pthread_mutex_lock(&spi_lock);

	struct spi_ioc_transfer xfer[2];

	memset(xfer, 0, sizeof(xfer));

	int msg_cnt = 1;

	xfer[0].tx_buf = (unsigned long)command;
	xfer[0].len = command_length;

	if (data_length) {
		xfer[1].rx_buf = (unsigned long)data;
		xfer[1].len = data_length;

		msg_cnt++;
	}

	if (ioctl(spifd, SPI_IOC_MESSAGE(msg_cnt), xfer) < 0) {
		perror("SPI_IOC_MESSAGE");
		abort();
	}

	pthread_mutex_unlock(&spi_lock);

	return SX126X_HAL_STATUS_OK;
}

/**
 * Reset the radio
 *
 * @remark Shall be implemented by the user
 *
 * @param [in] context Radio implementation parameters
 *
 * @returns Operation status
 */
sx126x_hal_status_t sx126x_hal_reset(const void *context) {
	gpio_write(gpfd_nrst, 0);
	usleep(50 * 1000);

	gpio_write(gpfd_nrst, 1);
	usleep(50 * 1000);

	return SX126X_HAL_STATUS_OK;
}

/**
 * Wake the radio up.
 *
 * @remark Shall be implemented by the user
 *
 * @param [in] context Radio implementation parameters
 *
 * @returns Operation status
 */
sx126x_hal_status_t sx126x_hal_wakeup(const void *context) {
	const uint8_t buf[2] = {0xc0, 0x00};

	struct spi_ioc_transfer xfer[1];

	memset(xfer, 0, sizeof(xfer));

	xfer[0].tx_buf = (unsigned long)buf;
	xfer[0].len = 2;

	if (ioctl(spifd, SPI_IOC_MESSAGE(1), xfer) < 0) {
		perror("SPI_IOC_MESSAGE");
		abort();
	}

	wait_busy();

	return SX126X_HAL_STATUS_OK;
}
