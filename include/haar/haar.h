/*
 *  TU Eindhoven
 *  Eindhoven, The Netherlands
 *
 *  Name            :   haar.h
 *
 *  Author          :   Francesco Comaschi (f.comaschi@tue.nl)
 *
 *  Date            :   November 12, 2012
 *
 *  Function        :   Haar features evaluation for face detection
 *
 *  History         :
 *      12-11-12    :   Initial version.
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

#ifndef __HAAR_H__
#define __HAAR_H__

#include <stdio.h>
#include <stdlib.h>
#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef  int sumtype;
typedef int sqsumtype;

#define FNAME_HAAR_INFO     "../data/info.txt"
#define FNAME_HAAR_CLASS    "../data/class.txt"

///214, 178, 124, 103, 86, 72, 58
#define FACE_WIDTH_124  124
#define FACE_WIDTH_086   86
#define FACE_WIDTH_072   72
#define FACE_WIDTH_058   58

typedef struct myCascade
{
// number of stages (22)
    int  n_stages;
    int total_nodes;
    float scale;

    // size of the window used in the training set (20 x 20)
    MySize orig_window_size;
//    MySize real_window_size;

    int inv_window_area;

    ImageInt sum;
    ImageInt sqsum;

    // pointers to the corner of the actual detection window
    sqsumtype *pq0, *pq1, *pq2, *pq3;
    sumtype *p0, *p1, *p2, *p3;

} myCascade;


void readTextClassifier();//(myCascade* cascade);
void releaseTextClassifier();
void haar_run (ImageByte *image, char *path, char *type, uint32_t flag);

#ifdef __cplusplus
}

#endif

#endif
