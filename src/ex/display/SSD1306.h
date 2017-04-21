/*
 * SSD1306.h
 *
 *  Created on: 13.04.2017
 *      Author: thomas
 */

#ifndef SSD1306_H
#define SSD1306_H

#include "hal.h"
#include "hal_i2c.h"

#include "FramebufferSW.h"


#if !HAL_USE_I2C
#error "SSD1306 requires HAL_USE_I2C"
#endif

#if !I2C_USE_MUTUAL_EXCLUSION
#error "SSD1306 requires I2C_USE_MUTUAL_EXCLUSION"
#endif


// https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf

// page 36

typedef enum {
	// 10.1.3 - Set Memory Addressing Mode
	SSD1306_COMMAND_SET_MEMORY_HORIZONTAL 	= 0x20,
	SSD1306_COMMAND_SET_MEMORY_VERTICAL 	= 0x21,
	SSD1306_COMMAND_SET_MEMORY_PAGE 		= 0x22,

	// 10.1.7 - Set Contrast Control for BANK0
	SSD1306_COMMAND_SET_CONTRAST_CONTROL	= 0x81,

	// 2.1
	SSD1306_COMMAND_SET_CHARGE_PUMP			= 0x8D,

	// 10.1.8 - Set Segment Re-map
	SSD1306_COMMAND_SET_SEGMENT_REMAP_0		= 0xA0,
	SSD1306_COMMAND_SET_SEGMENT_REMAP_127	= 0xA1,

	// 10.1.9 - Entire Display ON
	SSD1306_COMMAND_ENTIRE_DISPLAY_ON 		= 0xA4,
	SSD1306_COMMAND_ENTIRE_DISPLAY_ON_FORCE = 0xA5,

	// 10.1.10 - Set Normal/Inverse Display
	SSD1306_COMMAND_SET_NORMAL_DISPLAY		= 0xA6,
	SSD1306_COMMAND_SET_INVERSE_DISPLAY		= 0xA7,

	// 10.1.11 - Set Multiplex Ratio
	SSD1306_COMMAND_SET_MULTIPLEX_RATIO		= 0xA8,

	// 10.1.12 - Set Display ON/OFF
	SSD1306_COMMAND_SET_DISPLAY_OFF			= 0xAE,
	SSD1306_COMMAND_SET_DISPLAY_ON			= 0xAF,

	// 10.1.13 - Set Page Start Address for Page Addressing Mode
	SSD1306_COMMAND_SET_PAGE_START_ADDR_0	= 0xB0,
	SSD1306_COMMAND_SET_PAGE_START_ADDR_1	= 0xB1,
	SSD1306_COMMAND_SET_PAGE_START_ADDR_2	= 0xB2,
	SSD1306_COMMAND_SET_PAGE_START_ADDR_3	= 0xB3,
	SSD1306_COMMAND_SET_PAGE_START_ADDR_4	= 0xB4,
	SSD1306_COMMAND_SET_PAGE_START_ADDR_5	= 0xB5,
	SSD1306_COMMAND_SET_PAGE_START_ADDR_6	= 0xB6,
	SSD1306_COMMAND_SET_PAGE_START_ADDR_7	= 0xB7,

	// 10.1.14 - Set COM Output Scan Direction
	SSD1306_COMMAND_SET_COM_OUTPUT_SCAN_DIR_BU = 0xC0, // Botton-Up
	SSD1306_COMMAND_SET_COM_OUTPUT_SCAN_DIR_TD = 0xC8, // Top-Down

	// 10.1.15 - Set Display Offset
	SSD1306_COMMAND_SET_DISPLAY_OFFSET		= 0xD3,

	// 10.1.16 - Set Display Clock Divide Ratio/ Oscillator Frequency
	SSD1306_COMMAND_SET_DISPLAY_CLOCK		= 0xD5,

	// 10.1.17 - Set Pre-charge Period
	SSD1306_COMMAND_SET_PRECHARGE_PERIOD	= 0xD9,

	// 10.1.18 - Set COM Pins Hardware Configuration
	SSD1306_COMMAND_SET_COM_PINS			= 0xDA,

	// 10.1.19 - Set V_COMH Deselect Level
	SSD1306_COMMAND_SET_VCOMH_LEVEL			= 0xDB,

	// 10.1.20 - NOP
	SSD1306_COMMAND_NOP						= 0xE3,
} ssd1306_command;

/**
 * @brief  OLED Controller Slave Address.
 */
typedef enum {
  SSD1306_SA0_GND = 0x3C,           /**< SA0 pin connected to GND.          */
  SSD1306_SA0_VCC = 0x3D            /**< SA0 pin connected to VCC.          */
} ssd1306_sa0_t;

typedef struct {
	/**
	* @brief I2C driver associated to this SSD1306.
	*/
	I2CDriver                 *i2cp;

	/**
	* @brief I2C configuration associated to this SSD1306 display
	*        subsystem.
	*/
	const I2CConfig           *i2ccfg;

	/**
	* @brief Framebuffer which is drawn
	*/
	const FramebufferSW       *fb;

	/**
	* @brief  Slave Address
	*/
	ssd1306_sa0_t             slaveaddress;
} SSD1306Config;

typedef struct {
	/* Current configuration data.*/
	const SSD1306Config       *config;
} SSD1306Driver;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

	void ssd1306ObjectInit(SSD1306Driver *devp);
	void ssd1306Start(SSD1306Driver *devp, SSD1306Config *config);

	void ssd1306Update(SSD1306Driver *devp);

	msg_t ssd1306SendCommand(SSD1306Driver *devp, ssd1306_command command);
	msg_t ssd1306SendCommandData(SSD1306Driver *devp, ssd1306_command command, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /* SSD1306_H */
