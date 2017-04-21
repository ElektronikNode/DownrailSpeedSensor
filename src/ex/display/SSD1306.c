/*
 * SSD1306.c
 *
 *  Created on: 13.04.2017
 *      Author: thomas
 */


#include "SSD1306.h"

#include <string.h>

#define COMMAND 0x80
#define DATA 0x40

 // Hardware parameters
const uint8_t __osc_freq = 0x08;
const uint8_t __clk_div = 0x00;
const uint8_t __mux_ratio = 0x3F;

const uint8_t __contrast = 0x8F;

void ssd1306ObjectInit(SSD1306Driver *devp) {
	devp->config = NULL;
}

void ssd1306Start(SSD1306Driver *devp, SSD1306Config *config) {
	devp->config = config;

	// for more informations look at Page 64 of the datasheet
	ssd1306SendCommand(devp, SSD1306_COMMAND_ENTIRE_DISPLAY_ON);
	ssd1306SendCommandData(devp, SSD1306_COMMAND_SET_DISPLAY_CLOCK, __osc_freq << 4 | __clk_div);
	ssd1306SendCommandData(devp, SSD1306_COMMAND_SET_MULTIPLEX_RATIO, __mux_ratio);
	ssd1306SendCommandData(devp, SSD1306_COMMAND_SET_DISPLAY_OFFSET, 0x00);
	ssd1306SendCommandData(devp, SSD1306_COMMAND_SET_CHARGE_PUMP, 0x14); // Enable Charge Pump
	ssd1306SendCommand(devp, 0x40); // Set Display Start Line
	ssd1306SendCommand(devp, SSD1306_COMMAND_SET_NORMAL_DISPLAY);
	ssd1306SendCommand(devp, SSD1306_COMMAND_ENTIRE_DISPLAY_ON);
	ssd1306SendCommand(devp, SSD1306_COMMAND_SET_SEGMENT_REMAP_127);
	ssd1306SendCommand(devp, SSD1306_COMMAND_SET_COM_OUTPUT_SCAN_DIR_TD);

	// Horizontal mode
	ssd1306SendCommand(devp, SSD1306_COMMAND_SET_MEMORY_HORIZONTAL);
	ssd1306SendCommand(devp, 0x00); // Set Lower Column Start Address
	ssd1306SendCommandData(devp, SSD1306_COMMAND_SET_COM_PINS, 0x12);
	ssd1306SendCommandData(devp, SSD1306_COMMAND_SET_CONTRAST_CONTROL, __contrast);
	ssd1306SendCommandData(devp, SSD1306_COMMAND_SET_PRECHARGE_PERIOD, 0xf1);
	ssd1306SendCommandData(devp, SSD1306_COMMAND_SET_VCOMH_LEVEL, 0x40);
	ssd1306SendCommand(devp, SSD1306_COMMAND_SET_DISPLAY_ON);
}

void set_page_address(SSD1306Driver *devp)  {
	ssd1306SendCommand(devp, SSD1306_COMMAND_SET_MEMORY_PAGE);
	ssd1306SendCommand(devp, 0x00);
	ssd1306SendCommand(devp, 7);
}

void set_column_address(SSD1306Driver *devp) {
	ssd1306SendCommand(devp, SSD1306_COMMAND_SET_MEMORY_VERTICAL);
	ssd1306SendCommand(devp, 0x00);
	ssd1306SendCommand(devp, 127);
}

void ssd1306Update(SSD1306Driver *devp) {
	set_column_address(devp);
	set_page_address(devp);

	for(int j = 0; j < 64; j++) {
		uint8_t data[17];
		data[0] = DATA;
		memcpy(&data[1], &(devp)->config->fb->fb[16*j], 16 * sizeof(uint8_t));

		i2cAcquireBus((devp)->config->i2cp);
		i2cStart((devp)->config->i2cp, (devp)->config->i2ccfg);

		i2cMasterTransmitTimeout(devp->config->i2cp,
				devp->config->slaveaddress,
				data, 17,
				NULL, 0,
				1000 //TIME_INFINITE
		);

		i2cReleaseBus((devp)->config->i2cp);
	}
}

msg_t ssd1306SendCommand(SSD1306Driver *devp, ssd1306_command command) {
	uint8_t txbuf[2] = {0x80, command};
	msg_t res;

	i2cAcquireBus((devp)->config->i2cp);
	i2cStart((devp)->config->i2cp, (devp)->config->i2ccfg);

	res = i2cMasterTransmitTimeout(devp->config->i2cp,
			devp->config->slaveaddress,
			txbuf, 2,
			NULL, 0,
			1000 //TIME_INFINITE
	);

	i2cReleaseBus((devp)->config->i2cp);

	return res;
}

msg_t ssd1306SendCommandData(SSD1306Driver *devp, ssd1306_command command, uint8_t data) {
	uint8_t txbuf[4] = {0x80, command, 0x80, data};
	msg_t res;

	i2cAcquireBus((devp)->config->i2cp);
	i2cStart((devp)->config->i2cp, (devp)->config->i2ccfg);

	res = i2cMasterTransmitTimeout(devp->config->i2cp,
			devp->config->slaveaddress,
			txbuf, 4,
			NULL, 0,
			1000 //TIME_INFINITE
	);

	i2cReleaseBus((devp)->config->i2cp);

	return res;
}
