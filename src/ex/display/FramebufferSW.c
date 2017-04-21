/*
 * FramebufferSW.c
 *
 *  Created on: 15.04.2017
 *      Author: thomas
 */

#include "FramebufferSW.h"

#include <string.h>
#include <stdlib.h>

static uint8_t _framebuffer[1024]; // TODO: do not use fixed size and create one framebuffer per object

void FramebufferSWObjectInit(FramebufferSW *fb, size_t x_res, size_t y_res) {
	fb->x_res = x_res;
	fb->y_res = y_res;
	fb->buffer_size = (x_res * y_res) / 8;
	fb->fb = _framebuffer;
	fb->pixel_color = COLOR_SW_WHITE;
}

void FramebufferSWClear(FramebufferSW *fb) {
	memset(fb->fb, 0x00, fb->buffer_size); //
}

void FramebufferSWSetColor(FramebufferSW *fb, colorsw_t color) {
	fb->pixel_color = color;
}

void FramebufferSWDrawPixel(FramebufferSW *fb, int x, int y) {
	size_t index;

	// TODO: start with 0|0 or 1|1

	// check for out off range
	if (x >= (int)fb->x_res || x < 0 || y >= (int)fb->y_res || y < 0) {
		return;
	}

	index = (size_t)x + ((size_t)(y / 8)) * fb->x_res;

	if(fb->pixel_color == COLOR_SW_BLACK) {
		fb->fb[index] &= ~ (1 << (y % 8));
	} else {
		fb->fb[index] |= 1 << (y % 8);
	}
}

void FramebufferSWDrawLine(FramebufferSW *fb, int x1, int y1, int x2, int y2) {
   int dx, dy, sx, sy, err, e2;
   dx = abs (x2-x1);
   dy = abs (y2-y1);
   if (x1<x2) sx = 1;
      else sx = -1;
   if (y1<y2) sy = 1;
      else sy = -1;
   err = dx-dy;
   do {
	   FramebufferSWDrawPixel(fb, x1, y1);
      if ((x1 == x2) && (y1 == y2))
         break;
      e2 = 2*err;
      if (e2 > -dy) {
         err = err - dy;
    x1 = x1+sx;
      }
      if (e2 < dx) {
         err = err + dx;
     y1 = y1 + sy;
      }
   } while (1);
   return;
}
