/*
    e07 bootloader suite
    Copyright (c) 2008 Daniel Mack <daniel@caiaq.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "regs.h"
#include "types.h"
#include "wikireader.h"
#include "spi.h"
#include "misc.h"
#include "crc.h"
#include "sdcard.h"

#define CMD_GO_IDLE_STATE	0x00
#define CMD_READ_CSD		0x09
#define CMD_READ_SECTOR		0x11
#define CMD_SEND_OP_COND	0x29
#define CMD_APP			0x37

static u8 csd[16];

static u8 sdcard_response(void)
{
	u8 ret, retry = 8;

	while (retry--) {
		SDCARD_CS_LO();
		ret = spi_transmit(0xff);
		SDCARD_CS_HI();

		if (ret != 0xff)
			break;
	}

	return ret;
}

static void sdcard_cmd(u8 cmd, u32 param)
{
	u8 i, c[] = {
		0x40 | cmd,
		param >> 24,
		param >> 16,
		param >> 8,
		param
	};

	SDCARD_CS_LO();
	spi_transmit(0xff);
	for (i = 0; i < sizeof(c); i++)
		spi_transmit(c[i]);

	/* checksum */
	spi_transmit((crc7(c, sizeof(c)) << 1) | 1);
	SDCARD_CS_HI();
}

int sdcard_read_sector(u32 sector, u8 *buf)
{
	u8 ret;
	u32 i, retry;

	/* only valid for non-SDHC cards! */
	sector *= BYTES_PER_SECTOR;


	for (i = 0; i < BYTES_PER_SECTOR; i++)
		buf[i] = 0;

	sdcard_cmd(CMD_READ_SECTOR, sector);
	ret = sdcard_response();

	if (ret != 0) {
		print("bad card response\n");
		return -1;
	}

	for (retry = 0; retry < 1000; retry++) {
		ret = sdcard_response();
		if (ret == 0xfe)
			break;
	}

	if (ret != 0xfe) {
		print("read timeout\n");
		return -2;
	}

	SDCARD_CS_LO();
	for (i = 0; i < BYTES_PER_SECTOR; i++)
		buf[i] = spi_transmit(0xff);
	SDCARD_CS_HI();

	/* swallow checksum */
	spi_transmit(0xff);
	spi_transmit(0xff);

	return 0;
}

static int sdcard_read_csd(void)
{
	u32 retry, ret, i;

	sdcard_cmd(CMD_READ_CSD, 0);

	for (retry = 0; retry < 1000; retry++) {
		ret = sdcard_response();
		if (ret == 0xfe)
			break;
	}

	if (ret != 0xfe)
		return -1;

	SDCARD_CS_LO();
	for (i = 0; i < sizeof(csd); i++)
		csd[i] = spi_transmit(0xff);
	SDCARD_CS_HI();

	print("dumping CSD:\n");
	hex_dump(csd, sizeof(csd));

	return 0;
}

int sdcard_init(void)
{
	u8 ret;
	u32 retry;

	for (retry = 100; retry; retry--) {
		sdcard_cmd(CMD_GO_IDLE_STATE, 0);
		ret = sdcard_response();

		if (ret == 1)
			break;
	}

	if (ret != 1) {
		print("unable to set SD card to IDLE state\n");
		return -1;
	}

	for (retry = 1000; retry; retry--) {
		sdcard_cmd(CMD_APP, 0);
		ret = sdcard_response();

		sdcard_cmd(CMD_SEND_OP_COND, 0);
		ret = sdcard_response();

		if (ret == 0)
			break;

		delay(10000);
	}

	if (ret) {
		print("SD card failed to init.\n");
		return -1;
	}

	print("SD card initialized.\n");

	if (sdcard_read_csd() < 0) {
		print("unable to read CSD\n");
		return -1;
	}

	return 0;
}

