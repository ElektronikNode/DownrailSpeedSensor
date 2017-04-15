/*
 * SSD1306.c
 *
 *  Created on: 13.04.2017
 *      Author: thomas
 */


#include "SSD1306.h"

#define COMMAND 0x80
#define DATA 0x40

 // Hardware parameters
const uint8_t __osc_freq = 0x08;
const uint8_t __clk_div = 0x00;
const uint8_t __mux_ratio = 0x3F;

 // Fundamental commands
const uint8_t __entire_display_on = 0xa4;
const uint8_t __normal_display = 0xa6;

const uint8_t __contrast = 0x8F;

void ssd1306_init(void) {

}

void ssd1306_startup(I2CDriver *i2cp) {
//	send_command(0xA4);
//	send_command_data(0xD5, __osc_freq << 4 | __clk_div);
//	send_command_data(0xA8, __mux_ratio);
//	send_command_data(0xd3, 0x00);
//	send_command_data(0x8d, 0x14);
//	send_command(0x40);
//	send_command(0xa6);
//	send_command(0xa4);
//	send_command(0xa1);
//	send_command(0xc8);
//	// Horizontal mode
//	send_command(0x20);
//	send_command(0x00);
//
//	send_command_data(0xda, 0x12);
//	send_command_data(0x81, __contrast);
//	send_command_data(0xd9, 0xf1);
//	send_command_data(0xdb, 0x40);
//	send_command(0xaf);
}
