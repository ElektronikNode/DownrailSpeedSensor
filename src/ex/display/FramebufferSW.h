/*
 * FramebufferSW.h
 *
 *  Created on: 15.04.2017
 *      Author: thomas
 */

#ifndef FRAMEBUFFERSW_H
#define FRAMEBUFFERSW_H

#include "hal.h"

typedef enum {
	COLOR_SW_BLACK,
	COLOR_SW_WHITE
} colorsw_t;

typedef uint8_t framebuffer_sw_color;

typedef struct {
	/*
	 * @brief Width of Frame
	 */
	size_t x_res;

	/*
	 * @brief Height of Frame
	 */
	size_t y_res;

	/*
	 * @brief size of Frame Buffer
	 */
	size_t buffer_size;

	/*
	 * @brief Frame Buffer which is used to store the pixel data
	 */
	uint8_t *fb;

	/*
	 * @brief Pixel color which is used for drawing event
	 */
	colorsw_t pixel_color;
} FramebufferSW;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

	void FramebufferSWObjectInit(FramebufferSW *fb, size_t x_res, size_t y_res);

	void FramebufferSWClear(FramebufferSW *fb);

	void FramebufferSWSetColor(FramebufferSW *fb, colorsw_t color);

	void FramebufferSWDrawPixel(FramebufferSW *fb, int x, int y);

	void FramebufferSWDrawLine(FramebufferSW *fb, int x1, int y1, int x2, int y2);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFERSW_H */
