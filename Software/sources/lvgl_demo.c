/*
    This file is part of jz-diag-tools.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <ctype.h>

#include <pthread.h>

#include "st7789.h"

#include <lvgl.h>
#include <lv_demos.h>
#define PERIPH_BASE		0x10000000

volatile void *periph_phys = NULL;

volatile XHAL_GPIO_TypeDef *xhgpioa = NULL;
volatile XHAL_GPIO_TypeDef *xhgpiob = NULL;

XHAL_SPI_HandleTypeDef xhspi = {0};

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
//	printf("f\n");

	int xs, ys, xe, ye;

	xs = area->x1 + 8;
	ys = area->y1 + 32;
	xe = area->x2 + 8;
	ye = area->y2 + 32;

	ST7789_DrawImage(xs, ys, xe - xs + 1, ye - ys + 1, (const uint16_t *)color_p);

	/* IMPORTANT!!!
	 * Inform the graphics library that you are ready with the flushing*/
	lv_disp_flush_ready(disp_drv);
}

void *tick_thread(void *data)
{
	(void)data;

	while (1) {
		usleep(5 * 1000);
		lv_tick_inc(5);
	}

	return 0;
}

int main(int argc, char **argv) {
	if (!argv[1]) {
		puts("Usage: lvgl_demo <m|s>\n"
		     "    m: Music player\n"
		     "    s: Stress test");
		puts("Warning: Unbind FBTFT driver before use.");
		exit(1);
	}

	int fd = open("/dev/mem", O_RDWR|O_SYNC);

	if (fd < 0) {
		perror("error: failed to open /dev/mem");
		return 2;
	}

	periph_phys = mmap(NULL, 0x8000000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, PERIPH_BASE);

	volatile uint32_t *reg_cpm_clkgr = periph_phys + 0x20;

	*reg_cpm_clkgr &= ~(1 << 19);

	if (periph_phys == MAP_FAILED) {
		perror("error: mmap failed");
		return 2;
	}

	puts("mmap done");

	xhgpioa = periph_phys + (XHAL_PHYSADDR_GPIO - PERIPH_BASE) + XHAL_GPIO_PORT_WIDTH * ('A' - 'A');
	xhgpiob = periph_phys + (XHAL_PHYSADDR_GPIO - PERIPH_BASE) + XHAL_GPIO_PORT_WIDTH * ('B' - 'A');

	// D_C
	XHAL_GPIO_SetAsGPIO(xhgpiob, 7, 0);
	puts("pb07 as output");

	// RST
	XHAL_GPIO_SetAsGPIO(xhgpiob, 10, 0);
	puts("pb10 as output");

	memset(&xhspi, 0, sizeof(XHAL_SPI_HandleTypeDef));
	xhspi.periph = periph_phys + (XHAL_PHYSADDR_SPI - PERIPH_BASE);

	XHAL_SPI_Init(&xhspi);
	puts("XHAL_SPI_Init, wait");

//	sleep(1);
//	puts("waited");

	ST7789_Init();
	puts("ST7789_Init");

	ST7789_Fill_Color(BLACK);

	lv_init();

	static lv_disp_draw_buf_t disp_buf;

	/*Static or global buffer(s). The second buffer is optional*/
	static lv_color_t buf_1[224 * 280];
	static lv_color_t buf_2[224 * 280];

	/*Initialize `disp_buf` with the buffer(s). With only one buffer use NULL instead buf_2 */
	lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, 224 * 280);

	static lv_disp_drv_t disp_drv;          /*A variable to hold the drivers. Must be static or global.*/
	lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
	disp_drv.draw_buf = &disp_buf;          /*Set an initialized buffer*/
	disp_drv.flush_cb = my_flush_cb;        /*Set a flush callback to draw to the display*/
	disp_drv.hor_res = 224;                 /*Set the horizontal resolution in pixels*/
	disp_drv.ver_res = 280;                 /*Set the vertical resolution in pixels*/

	lv_disp_t *disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/

	if (argv[1][0] == 'm')
		lv_demo_music();
	else
		lv_demo_stress();

	pthread_t tid_tick;
	pthread_create(&tid_tick, NULL, tick_thread, NULL);

	puts("lvgl init");

	size_t cccnt = 1;

	while (1) {
		lv_task_handler();

		usleep(7500);
	}
}