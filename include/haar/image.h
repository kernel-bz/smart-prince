/*
 *  TU Eindhoven
 *  Eindhoven, The Netherlands
 *
 *  Name            :   image.h
 *
 *  Author          :   Francesco Comaschi (f.comaschi@tue.nl)
 *
 *  Date            :   November 12, 2012
 *
 *  Function        :   Functions to manage .pgm images and integral images
 *
 *  History         :
 *      12-11-12    :   Initial version.
 *    2016-07-20    :   Append LinkedList by JungJaeJoon on the www.kernel.bz
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;  If not, see <http://www.gnu.org/licenses/>
 *
 * In other words, you are welcome to use, share and improve this program.
 * You are forbidden to forbid anyone else to use, share and improve
 * what you give them.   Happy coding!
 */

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/types.h"
#include "list/list.h"

typedef struct MyPoint
{
    int x;
    int y;
} MyPoint;

typedef struct
{
    int width;
    int height;
} MySize;

typedef struct
{
    int x;
    int y;
    int width;
    int height;
} MyRect;

typedef struct
{
    MyRect rect;
    struct list_head list;
} RectList;

typedef struct
{
	int width;
	int height;
	int maxgrey;
	unsigned char* data;
	int flag;
} ImageByte;

typedef struct
{
	int width;
	int height;
	int* data;
	int flag;
} ImageInt;

typedef struct {
    int w;
    int h;
    int c;
    float *data;
} ImageFloat;

int readPgm(char *fileName, ImageByte* image);
int writePgm(char *fileName, ImageByte* image);
int cpyPgm(ImageByte *src, ImageByte *dst);
void createImage(int width, int height, ImageByte *image);
void createSumImage(int width, int height, ImageInt *image);
int freeImage(ImageByte* image);
int freeSumImage(ImageInt* image);
void setImage(int width, int height, ImageByte *image);
void setSumImage(int width, int height, ImageInt *image);

void drawRectangle(ImageByte* image, MyRect r);

int img_to_pgm (uint8_t *img, int w, int h, ImageByte *image);
void  img_face_to_pgm (ImageByte* image, MyRect r, ImageByte* face);

ImageFloat img_resize(ImageByte img, int w, int h, int c);
uint32_t img_resize_pgm_to_pgm(ImageByte pgm, ImageByte* out, int w, int h, int c);
ImageFloat img_load_from_pgm(ImageByte img, int w, int h, int c);
void img_resize_print_image(ImageFloat m);

#endif
