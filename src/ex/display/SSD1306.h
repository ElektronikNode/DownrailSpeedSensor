/*
 * SSD1306.h
 *
 *  Created on: 13.04.2017
 *      Author: thomas
 */

#ifndef SRC_EX_DISPLAY_SSD1306_H_
#define SRC_EX_DISPLAY_SSD1306_H_

#include "hal.h"
#include "hal_i2c.h"

struct {
	uint16_t width;
	uint16_t height;

	i2caddr_t address;

} ssd1306_driver_t;


void ssd1306_init(void);
void ssd1306_startup(I2CDriver *i2cp);

#endif /* SRC_EX_DISPLAY_SSD1306_H_ */
