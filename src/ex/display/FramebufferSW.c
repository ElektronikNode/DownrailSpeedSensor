/*
 * FramebufferSW.c
 *
 *  Created on: 15.04.2017
 *      Author: thomas
 */

#include "FramebufferSW.h"
#include "fonts.h"

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

	// check for out off range
	if ((x > (int)fb->x_res)||(x < 0)) return;
	if ((y > (int)fb->y_res)||(y < 0)) return;

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

void FramebufferSWPrintText(FramebufferSW *fb, uint8_t row, char *text) {

    uint8_t offset = 0;
    int counter = 0;

    if (row < 0) return;

    if (row <= ((int)fb->y_res/16 - 1)) {
      while(*text != 0) {
        if (counter < 8) {
          uint16_t data[16];
          uint16_t temp[16];
          for(int i = 0; i < 16; i++) {
              temp[i] = (uint8_t)font_16x16[*text - 32][i*2] << 8 | (uint8_t)font_16x16[*text - 32][i*2 +1];
          }

          for(int i = 0; i < 16; i++) {
              data[i] = 0;
              for(int j = 0; j < 16; j++) {
                  data[i] |= !!(temp[j] & (1 << 15-i)) << j;
              }
          }
          for(int i = 0; i < 16; i++) {
        	  fb->fb[(row*2 * (int)fb->x_res) + i + offset] = data[i] & 0xFF;
        	  fb->fb[((row*2 + 1) * (int)fb->x_res) + i + offset] = data[i] >> 8;

          }
          counter++;
        }
        offset += 16;
        text++;
      }
    }
}

void FramebufferSWPrintSMChar(FramebufferSW *fb, char x, char y, char ch, bool scr)
{
    unsigned int index = 0;
    unsigned int i = 0;
    x -= 1;
    y -= 1; // TODO

    // check for out off range
    if ((x > (int)fb->x_res)||(x < 0)) return;
    if ((y > (int)fb->y_res)||(y < 0)) return;

    if (!scr) {
      if (x * 6 > ((int)fb->x_res - 6)) {
        x = 0;
        y += 1;
      }
    }

    index = (unsigned int) x * 6  + (unsigned int) y * (int)fb->x_res;

    for (i = 0; i < 6; i++)
    {
        if (i==5)
        	fb->fb[index++] = 0x00;
        else
        	fb->fb[index++] = font_6x8[ch - 32][i];
    }
}

void FramebufferSWPrintSMText(FramebufferSW *fb, unsigned char row, const char *dataPtr, bool scr) {   //print small font text, input is row on LCD, text to print, should the text be scrollable(0/1)
  unsigned char x = 1;         // variable for X coordinate

  if (row < 0) row = 0;
  if (row <= ((int)fb->y_res/8)) {
    while (*dataPtr) {           // loop to the end of string
    	FramebufferSWPrintSMChar(fb, x, row, *dataPtr, scr);
      if (!scr) {
        if (x * 6 > ((int)fb->x_res - 6)) {
          x = 0;
          row += 1;
        }
      }
      x++;
      dataPtr++;
    }
  }
  return;
}
