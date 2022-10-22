/*
    Copyright (C) 2022 Reimu NotMoe <reimu@sudomaker.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "common.h"

#include "sx126x.h"
#include "sx126x_hal.h"

void hexdump(const uint8_t *buf, size_t len) {
	uint8_t cbuf_raw[21] = {0};
	cbuf_raw[0] = ' ';
	cbuf_raw[1] = '|';
	cbuf_raw[18] = '|';
	cbuf_raw[19] = '\n';

	uint8_t* cbuf = &cbuf_raw[2];

	uint8_t last_mod = 0;

	for (size_t i=0; i<len; i++) {
		last_mod = i % 16;
		if (last_mod == 0) {
			printf("%08x  ", i);
		}

		uint8_t p = buf[i];

		if (last_mod == 7) {
			printf("%02x  ", p);
		} else {
			printf("%02x ", p);
		}

		if (isprint(p)) {
			cbuf[last_mod] = p;
		} else {
			cbuf[last_mod] = '.';
		}

		if (last_mod == 15) {
			printf("%s", cbuf_raw);
		}
	}

	if (len && last_mod - 15) {
		cbuf[last_mod + 1] = '|';
		cbuf[last_mod + 2] = '\n';
		cbuf[last_mod + 3] = 0;
		if (last_mod < 7) {
			printf(" ");
		}
		for (size_t i = last_mod + 1; i < 16; i += 1) {
			printf("   ");
		}

		printf("%s", cbuf_raw);


	}

	printf("%08x\n", len);
}


void Lora_Initialize() {
	// Reset
	sx126x_reset(NULL);
	printf("--- Lora reset done\n");

	sx126x_wakeup(NULL);

	sx126x_set_reg_mode(NULL, SX126X_REG_MODE_DCDC);

	printf("sx126x_wakeup done\n");

	sx126x_set_standby(NULL, SX126X_STANDBY_CFG_RC);
	printf("sx126x_set_standby done\n");

	sx126x_set_pkt_type(NULL, SX126X_PKT_TYPE_LORA);
	printf("sx126x_set_pkt_type done\n");

	sx126x_set_dio3_as_tcxo_ctrl(NULL, SX126X_TCXO_CTRL_1_8V, 32);
	printf("sx126x_set_dio3_as_tcxo_ctrl done\n");

	sx126x_cal(NULL, SX126X_CAL_ALL);
	printf("sx126x_cal done\n");

	sx126x_set_standby(NULL, SX126X_STANDBY_CFG_XOSC);
	printf("sx126x_set_standby done\n");

	sx126x_pa_cfg_params_t pa_params = {
		.pa_duty_cycle = 0x04,
		.hp_max = 0x07,
		.device_sel = 0x00,
		.pa_lut = 0x01
	};

	sx126x_set_pa_cfg(NULL, &pa_params);
	printf("sx126x_set_pa_cfg done\n");

	sx126x_set_ocp_value(NULL, 0x38);
//	sx126x_set_ocp_value(NULL, 0x3f);

	printf("sx126x_set_ocp_value done\n");

	sx126x_cfg_tx_clamp(NULL);
	printf("sx126x_cfg_tx_clamp done\n");

	sx126x_set_tx_params(NULL, 0, SX126X_RAMP_200_US);
	printf("sx126x_set_tx_params done\n");

	sx126x_set_rf_freq(NULL, 433920000);
	printf("sx126x_set_rf_freq(433920000) done\n");

	sx126x_cal_img(NULL, 433920000);
	printf("sx126x_cal_img done\n");

	sx126x_set_buffer_base_address(NULL, 0x00, 0x00);
	printf("sx126x_set_buffer_base_address done\n");

	sx126x_set_pkt_type(NULL, SX126X_PKT_TYPE_LORA);
	printf("sx126x_set_pkt_type done\n");

	sx126x_set_rx_tx_fallback_mode(NULL, SX126X_FALLBACK_FS);
	printf("sx126x_set_rx_tx_fallback_mode done\n");

	sx126x_mod_params_lora_t lora_mod_params = {
		.sf = SX126X_LORA_SF12,
		.bw = SX126X_LORA_BW_062,
		.cr = SX126X_LORA_CR_4_5,
		.ldro = 1
	};

//	sx126x_mod_params_lora_t lora_mod_params = {
//		.sf = SX126X_LORA_SF8,
//		.bw = SX126X_LORA_BW_062,
//		.cr = SX126X_LORA_CR_4_5,
//		.ldro = 0
//	};

//	sx126x_mod_params_lora_t lora_mod_params = {
//		.sf = SX126X_LORA_SF12,
//		.bw = SX126X_LORA_BW_015,
//		.cr = SX126X_LORA_CR_4_5,
//		.ldro = 1
//	};

//	sx126x_mod_params_lora_t lora_mod_params = {
//		.sf = SX126X_LORA_SF12,
//		.bw = SX126X_LORA_BW_020,
//		.cr = SX126X_LORA_CR_4_5,
//		.ldro = 1
//	};

	sx126x_set_lora_mod_params(NULL, &lora_mod_params);
	printf("sx126x_set_lora_mod_params done\n");

	sx126x_pkt_params_lora_t lora_pkt_params = {
		.preamble_len_in_symb = 64,
		.header_type = SX126X_LORA_PKT_EXPLICIT,
		.pld_len_in_bytes = 32,
		.crc_is_on = 1,
		.invert_iq_is_on = 0
	};

	sx126x_set_lora_pkt_params(NULL, &lora_pkt_params);
	printf("sx126x_set_lora_pkt_params done\n");

	static const uint16_t lora_irq_mask_dio1 = SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT | SX126X_IRQ_CRC_ERROR;
	static const uint16_t lora_irq_mask_dio2 = SX126X_IRQ_PREAMBLE_DETECTED;

	sx126x_set_dio_irq_params(NULL, lora_irq_mask_dio1 | lora_irq_mask_dio2, lora_irq_mask_dio1, lora_irq_mask_dio2, 0);

	sx126x_clear_irq_status(NULL, SX126X_IRQ_ALL);

	sx126x_cfg_rx_boosted(NULL, true);

	sx126x_set_tx_infinite_preamble(NULL);
	printf("sx126x_set_tx_infinite_preamble done\n");


//	sx126x_set_rx(NULL, SX126X_MAX_TIMEOUT_IN_MS);

//	sx126x_set_lora_symb_nb_timeout(NULL, 2);

//	Lora_Refresh_DIFS();

//	Timer_Initialize(&htimer2, TIMER_16BIT);
//	Timer_SetPeriod(&htimer2, 0xf9); // 1 ms
//	Timer_SetSpeedByPrescaler(&htimer2, TIMER_PS_1_64);
//	Timer_SetInterruptHandler(&htimer2, Interrupt_Lora_Timer, NULL);
//	Timer_SetInterrupt(&htimer2, 1);
//	Timer_Start(&htimer2);

//	Lora_RFSwitch(1, 0);
//	sx126x_set_rx(NULL, LORA_RX_TIMEOUT);

//	Lora_RFSwitch(0, 1);
//	sx126x_set_tx_cw(NULL);
//
//	while (1);
}

void Lora_Process_RX() {

	sx126x_pkt_status_lora_t last_rcvd_pkt_status;

	sx126x_get_lora_pkt_status(NULL, &last_rcvd_pkt_status);

	printf("-- Lora: Process_RX: RF status: RSSI: %d, SNR: %d, RSCP: %d\n",
	       last_rcvd_pkt_status.rssi_pkt_in_dbm,
	       last_rcvd_pkt_status.snr_pkt_in_db,
	       last_rcvd_pkt_status.signal_rssi_pkt_in_dbm);

	sx126x_rx_buffer_status_t rx_buf_stat;
	uint8_t last_pld_len = 255;

	do {
		sx126x_get_rx_buffer_status(NULL, &rx_buf_stat);
		if (rx_buf_stat.pld_len_in_bytes == last_pld_len) {
			break;
		} else {
			last_pld_len = rx_buf_stat.pld_len_in_bytes;
		}
	} while (1);

//		sx126x_get_rx_buffer_status(NULL, &rx_buf_stat);

	uint8_t buf[256];

	sx126x_read_buffer(NULL, 0, buf, rx_buf_stat.pld_len_in_bytes);

	printf("-- Lora: Process_RX: len=%u, Data:\n", rx_buf_stat.pld_len_in_bytes);
	hexdump(buf, rx_buf_stat.pld_len_in_bytes);

}

void sx126x_irq_mask_print(sx126x_irq_mask_t irq_mask) {
	if (irq_mask & SX126X_IRQ_TX_DONE) {
		printf("TX_DONE ");
	}

	if (irq_mask & SX126X_IRQ_RX_DONE) {
		printf("RX_DONE ");
	}

	if (irq_mask & SX126X_IRQ_PREAMBLE_DETECTED) {
		printf("PREAMBLE_DETECTED ");
	}

	if (irq_mask & SX126X_IRQ_SYNC_WORD_VALID) {
		printf("SYNC_WORD_VALID ");
	}

	if (irq_mask & SX126X_IRQ_HEADER_VALID) {
		printf("HEADER_VALID ");
	}

	if (irq_mask & SX126X_IRQ_HEADER_ERROR) {
		printf("HEADER_ERROR ");
	}

	if (irq_mask & SX126X_IRQ_CRC_ERROR) {
		printf("CRC_ERROR ");
	}

	if (irq_mask & SX126X_IRQ_CAD_DONE) {
		printf("CAD_DONE ");
	}

	if (irq_mask & SX126X_IRQ_CAD_DETECTED) {
		printf("CAD_DETECTED ");
	}

	if (irq_mask & SX126X_IRQ_TIMEOUT) {
		printf("TIMEOUT ");
	}
}

void Lora_ProcessInterrupts(unsigned int dio1_events, unsigned int dio2_events, unsigned int dio3_events, void *userp) {
	int process_interrupts = 0;

	if (dio1_events & GPIOEVENT_EVENT_RISING_EDGE) {
		puts("It was me, DIO1!");
		process_interrupts = 1;
	}

	if (dio2_events & GPIOEVENT_EVENT_RISING_EDGE) {
		puts("It was me, DIO2!");
		process_interrupts = 1;
	}

	sx126x_irq_mask_t irq_mask;
	sx126x_get_irq_status(NULL, &irq_mask);
	sx126x_clear_irq_status(NULL, irq_mask);


	printf("Lora IRQ: 0x%04x ( ", irq_mask);
	sx126x_irq_mask_print(irq_mask);
	printf(")\n");


	if (irq_mask & SX126X_IRQ_RX_DONE) {

		printf("Lora_IRQ: RX_DONE\n");

		Lora_Process_RX();
	}

	sx126x_set_rx(NULL, SX126X_MAX_TIMEOUT_IN_MS);
}

int main() {
	//Notkia
	static int gpio_nrst[] = {2, 23};
	static int gpio_busy[] = {2, 17};
	static int gpio_dio1[] = {2, 16};
	static int gpio_dio2[] = {2, 22};

//	static int gpio_nrst[] = {0, 16};
//	static int gpio_busy[] = {0, 17};
//	static int gpio_dio1[] = {0, 18};
//	static int gpio_dio2[] = {0, 19};

	sx126x_hal_linux_init("/dev/spidev0.0", gpio_nrst, gpio_busy, gpio_dio1, gpio_dio2);
	sx126x_hal_linux_set_interrupt_handler(Lora_ProcessInterrupts, NULL);

	Lora_Initialize();

	while (1) {
		sleep(1);
	}
	return 0;
}
