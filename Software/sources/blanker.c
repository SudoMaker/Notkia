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

#define PERIPH_BASE		0x10000000

volatile void *periph_phys = NULL;

volatile XHAL_GPIO_TypeDef *xhgpioa = NULL;
volatile XHAL_GPIO_TypeDef *xhgpiob = NULL;

XHAL_SPI_HandleTypeDef xhspi = {0};


int main(int argc, char **argv) {
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

	return 0;
}