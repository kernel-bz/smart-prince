/*
 *  TU Eindhoven
 *  Eindhoven, The Netherlands
 *
 *  Name            :   haar.cpp
 *
 *  Author          :   Francesco Comaschi (f.comaschi@tue.nl)
 *
 *  Date            :   November 12, 2012
 *
 *  Function        :   Haar features evaluation for face detection
 *
 *  History         :
 *      12-11-12    :   Initial version.
 *      2017-09-01  :   Editted by JungJaeJoon(rgbi3307@nate.com)
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include "common/types.h"
#include "common/debug.h"
#include "common/config.h"
#include "list/list.h"
#include "haar/haar.h"
#include "haar/image.h"
#include "haar/nn_cnn.h"

/* TODO: use matrices */
/* classifier parameters */
/************************************
 * Notes:
 * To paralleism the filter,
 * these monolithic arrays may
 * need to be splitted or duplicated
 ***********************************/
static int *stages_array;
static int *rectangles_array;
static int *weights_array;
static int *alpha1_array;
static int *alpha2_array;
static int *tree_thresh_array;
static int *stages_thresh_array;
static int **scaled_rectangles_array;

int myMax(int a, int b)
{
  if (a >= b)
    return a;
  else
    return b;
}

int myMin(int a, int b)
{
  if (a <= b)
    return a;
  else
    return b;
}

int  myRound( float value )
{
  return (int)(value + (value >= 0 ? 0.5 : -0.5));
}

int myAbs(int n)
{
  if (n >= 0)
    return n;
  else
    return -n;
}

void readTextClassifier()   //(myCascade * cascade)
{
  /*number of stages of the cascade classifier*/
  int stages=0;
  /*total number of weak classifiers (one node each)*/
  int total_nodes = 0;
  int i, j, k, l;
  char mystring [12];
  int r_index = 0;
  int w_index = 0;
  int tree_index = 0;


  FILE *finfo = fopen(FNAME_HAAR_INFO, "r");

  /**************************************************
   how many stages are in the cascaded filter?
   the first line of info.txt is the number of stages
   (in the 5kk73 example, there are 25 stages)
  **************************************************/
  if ( fgets (mystring , 12 , finfo) != NULL )
  {
      stages = atoi(mystring);  ///cascade->n_stages=25
  }
  i = 0;

  stages_array = (int *)malloc(sizeof(int)*stages);

  /**************************************************
   * how many filters in each stage?
   * They are specified in info.txt,
   * starting from second line.
   * (in the 5kk73 example, from line 2 to line 26)
   *************************************************/
  while ( fgets (mystring , 12 , finfo) != NULL )
  {
      stages_array[i] = atoi(mystring);  ///9,16,27,32,52,53,62,72,83,91,99,...,200
      total_nodes += stages_array[i];   ///cascade->total_nodes=2913;
      i++;
  }
  fclose(finfo);

    printf("readed stages[%d], total nodes[%d] from info file(%s)\n", stages, total_nodes, FNAME_HAAR_INFO);

  /* TODO: use matrices where appropriate */
  /***********************************************
   * Allocate a lot of array structures
   * Note that, to increase parallelism,
   * some arrays need to be splitted or duplicated
   **********************************************/
  rectangles_array = (int *)malloc(sizeof(int)*total_nodes*12);
  scaled_rectangles_array = (int **)malloc(sizeof(int*)*total_nodes*12);
  weights_array = (int *)malloc(sizeof(int)*total_nodes*3);
  alpha1_array = (int*)malloc(sizeof(int)*total_nodes);
  alpha2_array = (int*)malloc(sizeof(int)*total_nodes);
  tree_thresh_array = (int*)malloc(sizeof(int)*total_nodes);
  stages_thresh_array = (int*)malloc(sizeof(int)*stages);

  FILE *fp = fopen(FNAME_HAAR_CLASS, "r");

  /******************************************
   * Read the filter parameters in class.txt
   *
   * Each stage of the cascaded filter has:
   * 18 parameter per filter x tilter per stage
   * + 1 threshold per stage
   *
   * For example, in 5kk73,
   * the first stage has 9 filters,
   * the first stage is specified using
   * 18 * 9 + 1 = 163 parameters
   * They are line 1 to 163 of class.txt
   *
   * The 18 parameters for each filter are:
   * 1 to 4: coordinates of rectangle 1
   * 5: weight of rectangle 1
   * 6 to 9: coordinates of rectangle 2
   * 10: weight of rectangle 2
   * 11 to 14: coordinates of rectangle 3
   * 15: weight of rectangle 3
   * 16: threshold of the filter
   * 17: alpha 1 of the filter
   * 18: alpha 2 of the filter
   * +1: stages_thresh_array
   ******************************************/

  /* loop over n of stages */
  for (i = 0; i < stages; i++)  ///25
   {    /* loop over n of trees */
        for (j = 0; j < stages_array[i]; j++)   ///9,16,27,32,52,53,62,72,83,91,99,...,200
        {	/* loop over n of rectangular features */
            for(k = 0; k < 3; k++)
            {	/* loop over the n of vertices */
                for (l = 0; l <4; l++)
                {
                      if (fgets (mystring , 12 , fp) != NULL)
                        rectangles_array[r_index] = atoi(mystring);
                      else
                        break;
                      r_index++;
                } /* end of l loop */

                if (fgets (mystring , 12 , fp) != NULL)
                {
                  weights_array[w_index] = atoi(mystring);
                  /* Shift value to avoid overflow in the haar evaluation */
                  /*TODO: make more general */
                  /*weights_array[w_index]>>=8; */
                } else break;

                w_index++;

            } /* end of k loop */

          if (fgets (mystring , 12 , fp) != NULL)
            tree_thresh_array[tree_index]= atoi(mystring);
          else
            break;

          if (fgets (mystring , 12 , fp) != NULL)
            alpha1_array[tree_index]= atoi(mystring);
          else
            break;

          if (fgets (mystring , 12 , fp) != NULL)
            alpha2_array[tree_index]= atoi(mystring);
          else
            break;

          tree_index++;

          /**
          if (j == stages_array[i]-1)   ///end of stages_array[]
          {
              if (fgets (mystring , 12 , fp) != NULL)
                stages_thresh_array[i] = atoi(mystring);
              else
                break;
          }
          */

        } /* end of j loop */

        if (j == stages_array[i])   ///end of stages_array[]
          {
              if (fgets (mystring , 12 , fp) != NULL)
                stages_thresh_array[i] = atoi(mystring);
              else
                break;
          }

    } /* end of i loop */

    printf("readed the filter parameters in each stages[%d] from class file(%s)\n"
                , stages, FNAME_HAAR_CLASS);

  fclose(fp);
}

void releaseTextClassifier()
{
  free(stages_array);
  free(rectangles_array);
  free(scaled_rectangles_array);
  free(weights_array);
  free(tree_thresh_array);
  free(alpha1_array);
  free(alpha2_array);
  free(stages_thresh_array);
}


/***********************************************
 * Note:
 * The int_sqrt is softwar integer squre root.
 * GPU has hardware for floating squre root (sqrtf).
 * In GPU, it is wise to convert an int variable
 * into floating point, and use HW sqrtf function.
 * More info:
 * http://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#standard-functions
 **********************************************/
/*****************************************************
 * The int_sqrt is only used in runCascadeClassifier
 * If you want to replace int_sqrt with HW sqrtf in GPU,
 * simple look into the runCascadeClassifier function.
 *****************************************************/
static unsigned int int_sqrt (unsigned int value)
{
  int i;
  unsigned int a = 0, b = 0, c = 0;
  for (i=0; i < (32 >> 1); i++)
    {
      c<<= 2;
#define UPPERBITS(value) (value>>30)
      c += UPPERBITS(value);
#undef UPPERBITS
      value <<= 2;
      a <<= 1;
      b = (a<<1) | 1;
      if (c >= b)
	{
	  c -= b;
	  a++;
	}
    }
  return a;
}

static void _haar_set_image_classifier( myCascade* _cascade, ImageInt* _sum, ImageInt* _sqsum)
{
  ImageInt *sum = _sum;
  ImageInt *sqsum = _sqsum;
  myCascade* cascade = _cascade;
  int i, j, k;
  MyRect equRect;
  int r_index = 0;
  int w_index = 0;
  MyRect tr;

  cascade->sum = *sum;
  cascade->sqsum = *sqsum;

  equRect.x = equRect.y = 0;
  equRect.width = cascade->orig_window_size.width;      ///24
  equRect.height = cascade->orig_window_size.height;    ///24

  cascade->inv_window_area = equRect.width*equRect.height;

  cascade->p0 = (sum->data) ;
  cascade->p1 = (sum->data +  equRect.width - 1) ;
  cascade->p2 = (sum->data + sum->width*(equRect.height - 1));
  cascade->p3 = (sum->data + sum->width*(equRect.height - 1) + equRect.width - 1);

  cascade->pq0 = (sqsum->data);
  cascade->pq1 = (sqsum->data +  equRect.width - 1) ;
  cascade->pq2 = (sqsum->data + sqsum->width*(equRect.height - 1));
  cascade->pq3 = (sqsum->data + sqsum->width*(equRect.height - 1) + equRect.width - 1);

  /****************************************
   * Load the index of the four corners
   * of the filter rectangle
   **************************************/

  /* loop over the number of stages */
  for( i = 0; i < cascade->n_stages; i++ )  ///25
    {
      /* loop over the number of haar features */
      for( j = 0; j < stages_array[i]; j++ )    ///9,16,27,32,52,53,62,72,83,91,99,...,200
        {
          int nr = 3;
          /* loop over the number of rectangles */
          for( k = 0; k < nr; k++ )
            {
                  tr.x = rectangles_array[r_index + k*4];
                  tr.width = rectangles_array[r_index + 2 + k*4];
                  tr.y = rectangles_array[r_index + 1 + k*4];
                  tr.height = rectangles_array[r_index + 3 + k*4];
                  if (k < 2)
                    {
                      ///HaarRect data
                      scaled_rectangles_array[r_index + k*4] = (sum->data + sum->width*(tr.y ) + (tr.x )) ;
                      scaled_rectangles_array[r_index + k*4 + 1] = (sum->data + sum->width*(tr.y ) + (tr.x  + tr.width)) ;
                      scaled_rectangles_array[r_index + k*4 + 2] = (sum->data + sum->width*(tr.y  + tr.height) + (tr.x ));
                      scaled_rectangles_array[r_index + k*4 + 3] = (sum->data + sum->width*(tr.y  + tr.height) + (tr.x  + tr.width));
                    }
                      else
                    {
                          if ((tr.x == 0)&& (tr.y == 0) &&(tr.width == 0) &&(tr.height == 0))
                            {
                              scaled_rectangles_array[r_index + k*4] = NULL ;
                              scaled_rectangles_array[r_index + k*4 + 1] = NULL ;
                              scaled_rectangles_array[r_index + k*4 + 2] = NULL;
                              scaled_rectangles_array[r_index + k*4 + 3] = NULL;
                            }
                          else
                            {
                              scaled_rectangles_array[r_index + k*4] = (sum->data + sum->width*(tr.y ) + (tr.x )) ;
                              scaled_rectangles_array[r_index + k*4 + 1] = (sum->data + sum->width*(tr.y ) + (tr.x  + tr.width)) ;
                              scaled_rectangles_array[r_index + k*4 + 2] = (sum->data + sum->width*(tr.y  + tr.height) + (tr.x ));
                              scaled_rectangles_array[r_index + k*4 + 3] = (sum->data + sum->width*(tr.y  + tr.height) + (tr.x  + tr.width));
                            }
                    } ///end of branch if(k<2)

            } ///end of k loop
          r_index+=12;
          w_index+=3;

        } ///end of j loop

    } ///end i loop

}

/****************************************************
 * evalWeakClassifier:
 * the actual computation of a haar filter.
 * More info:
 * http://en.wikipedia.org/wiki/Haar-like_features
 ***************************************************/
int evalWeakClassifier(int variance_norm_factor, int p_offset, int tree_index, int w_index, int r_index )
{
  /* the node threshold is multiplied by the standard deviation of the image */
  int t = tree_thresh_array[tree_index] * variance_norm_factor;

  int sum = (*(scaled_rectangles_array[r_index] + p_offset)
	     - *(scaled_rectangles_array[r_index + 1] + p_offset)
	     - *(scaled_rectangles_array[r_index + 2] + p_offset)
	     + *(scaled_rectangles_array[r_index + 3] + p_offset))
    * weights_array[w_index];


  sum += (*(scaled_rectangles_array[r_index+4] + p_offset)
	  - *(scaled_rectangles_array[r_index + 5] + p_offset)
	  - *(scaled_rectangles_array[r_index + 6] + p_offset)
	  + *(scaled_rectangles_array[r_index + 7] + p_offset))
    * weights_array[w_index + 1];

  if ((scaled_rectangles_array[r_index+8] != NULL))
    sum += (*(scaled_rectangles_array[r_index+8] + p_offset)
	    - *(scaled_rectangles_array[r_index + 9] + p_offset)
	    - *(scaled_rectangles_array[r_index + 10] + p_offset)
	    + *(scaled_rectangles_array[r_index + 11] + p_offset))
      * weights_array[w_index + 2];

  if(sum >= t)
    return alpha2_array[tree_index];
  else
    return alpha1_array[tree_index];

}

static int _haar_run_classifier( myCascade* _cascade, MyPoint pt, int start_stage )
{

  int p_offset, pq_offset;
  int i, j;
  unsigned int mean;
  unsigned int variance_norm_factor;
  int haar_counter = 0;
  int w_index = 0;
  int r_index = 0;
  int stage_sum;
  myCascade* cascade;
  cascade = _cascade;

  p_offset = pt.y * (cascade->sum.width) + pt.x;
  pq_offset = pt.y * (cascade->sqsum.width) + pt.x;

  /**************************************************************************
   * Image normalization
   * mean is the mean of the pixels in the detection window
   * cascade->pqi[pq_offset] are the squared pixel values (using the squared integral image)
   * inv_window_area is 1 over the total number of pixels in the detection window
   *************************************************************************/

  variance_norm_factor =  (cascade->pq0[pq_offset] - cascade->pq1[pq_offset] - cascade->pq2[pq_offset] + cascade->pq3[pq_offset]);
  mean = (cascade->p0[p_offset] - cascade->p1[p_offset] - cascade->p2[p_offset] + cascade->p3[p_offset]);

  variance_norm_factor = (variance_norm_factor*cascade->inv_window_area);
  variance_norm_factor =  variance_norm_factor - mean*mean;

  /***********************************************
   * Note:
   * The int_sqrt is softwar integer squre root.
   * GPU has hardware for floating squre root (sqrtf).
   * In GPU, it is wise to convert the variance norm
   * into floating point, and use HW sqrtf function.
   * More info:
   * http://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#standard-functions
   **********************************************/
  if( variance_norm_factor > 0 )
    variance_norm_factor = int_sqrt(variance_norm_factor);
  else
    variance_norm_factor = 1;

  /**************************************************
   * The major computation happens here.
   * For each scale in the image pyramid,
   * and for each shifted step of the filter,
   * send the shifted window through cascade filter.
   *
   * Note:
   *
   * Stages in the cascade filter are independent.
   * However, a face can be rejected by any stage.
   * Running stages in parallel delays the rejection,
   * which induces unnecessary computation.
   *
   * Filters in the same stage are also independent,
   * except that filter results need to be merged,
   * and compared with a per-stage threshold.
   *************************************************/
  for( i = start_stage; i < cascade->n_stages; i++ )
    {

          /****************************************************
           * A shared variable that induces false dependency
           *
           * To avoid it from limiting parallelism,
           * we can duplicate it multiple times,
           * e.g., using stage_sum_array[number_of_threads].
           * Then threads only need to sync at the end
           ***************************************************/
          stage_sum = 0;

          for( j = 0; j < stages_array[i]; j++ )
            {
                  /**************************************************
                   * Send the shifted window to a haar filter.
                   **************************************************/
                  stage_sum += evalWeakClassifier(variance_norm_factor, p_offset, haar_counter, w_index, r_index);
                  ///n_features++;
                  haar_counter++;
                  w_index+=3;
                  r_index+=12;
            } /* end of j loop */

          /**************************************************************
           * threshold of the stage.
           * If the sum is below the threshold,
           * no faces are detected,
           * and the search is abandoned at the i-th stage (-i).
           * Otherwise, a face is detected (1)
           **************************************************************/
          /* the number "0.4" is empirically chosen for 5kk73 */
          if( stage_sum < 0.4*stages_thresh_array[i] ) {
                return -i;  ///not detected
           } /* end of the per-stage thresholding */

    } ///end of i loop

  return 1; ///detected (loop for all stages)
}

static int _haar_scale_image_invoker( myCascade* _cascade, float _factor, int sum_row, int sum_col, struct list_head *rect_list_head)
{
  myCascade* cascade = _cascade;
  float factor = _factor;
  MyPoint p;
  int result;
  int y1, y2, x2, x, y, step;
  ///std::vector<MyRect> *vec = &_vec;
  RectList *rlist;
  int newx, newy, oldx=0, oldy=0, detected=0;

  MySize winSize0 = cascade->orig_window_size;
  MySize winSize;

    ///scaled up size(24,29,35,41,50,60,72,86,103,124,...,920)
  winSize.width =  myRound(winSize0.width*factor);  ///24*factor
  winSize.height =  myRound(winSize0.height*factor);
  y1 = 0;

  /********************************************
  * When filter window shifts to image boarder,
  * some margin need to be kept
  *********************************************/
  y2 = sum_row - winSize0.height;   ///sz.height - 24 (scaled down)
  x2 = sum_col - winSize0.width;    ///sz.width - 24

  /********************************************
   * Step size of filter window shifting
   * Reducing step makes program faster,
   * but decreases quality of detection.
   * example:
   * step = factor > 2 ? 1 : 2;
   *
   * For 5kk73,
   * the factor and step can be kept constant,
   * unless you want to change input image.
   *
   * The step size is set to 1 for 5kk73,
   * i.e., shift the filter window by 1 pixel.
   *******************************************/
  ///step = 1;  //err2
  step = factor > 2 ? 1 : 2;    //err4
  ///step = factor > 2 ? 2 : 4; //err6

  /**********************************************
   * Shift the filter window over the image.
   * Each shift step is independent.
   * Shared data structure may limit parallelism.
   *
   * Some random hints (may or may not work):
   * Split or duplicate data structure.
   * Merge functions/loops to increase locality
   * Tiling to increase computation-to-memory ratio
   *********************************************/
  for( x = 0; x <= x2; x += step )
  {
        for( y = y1; y < y2; y += step )
         {
            p.x = x;
            p.y = y;

            /*********************************************
             * Optimization Oppotunity:
             * The same cascade filter is used each time
             ********************************************/
            result = _haar_run_classifier( cascade, p, 0 );

            /*******************************************************
             * If a face is detected,
             * record the coordinates of the filter window
             * the "push_back" function is from std:vec, more info:
             * http://en.wikipedia.org/wiki/Sequence_container_(C++)
             *
             * Note that, if the filter runs on GPUs,
             * the push_back operation is not possible on GPUs.
             * The GPU may need to use a simpler data structure,
             * e.g., an array, to store the coordinates of face,
             * which can be later memcpy from GPU to CPU to do push_back
             *******************************************************/
            if( result > 0 ) {
                //MyRect r = {myRound(x*factor), myRound(y*factor), winSize.width, winSize.height};
                //vec->push_back(r);
                newx = myRound(x*factor);
                newy = myRound(y*factor);
                if (abs(newx-oldx) > 5 && abs(newy-oldy) > 5) {
                    rlist = malloc(sizeof(*rlist));
                    rlist->rect.x = newx;
                    rlist->rect.y = newy;
                    rlist->rect.width = winSize.width;
                    rlist->rect.height= winSize.height;
                    list_add(&rlist->list, rect_list_head);

                    //pr_debug_msg("*detected: %d, %d, %d, %d\n"
                     //      , rlist->rect.x, rlist->rect.y, rlist->rect.width, rlist->rect.height);
                    detected++;

                    oldx = rlist->rect.x;
                    oldy = rlist->rect.y;
                }
              }

          } ///for y

    } ///for x

    return detected;
}

/***********************************************************
 * This function downsample( an image using nearest neighbor
 * It is used to build the image pyramid
 **********************************************************/
static void _harr_nearest_neighbor (ImageByte *src, ImageByte *dst)
{

  int y;
  int j;
  int x;
  int i;
  unsigned char* t;
  unsigned char* p;
  int w1 = src->width;      ///img
  int h1 = src->height;
  int w2 = dst->width;      ///img1
  int h2 = dst->height;

  int rat = 0;

  unsigned char* src_data = src->data;
  unsigned char* dst_data = dst->data;

  int x_ratio = (int)((w1<<16)/w2) +1;
  int y_ratio = (int)((h1<<16)/h2) +1;

  for (i=0;i<h2;i++)
   {
      t = dst_data + i*w2;
      y = ((i*y_ratio)>>16);
      p = src_data + y*w1;

      rat = 0;
      for (j=0;j<w2;j++)
       {
              x = (rat>>16);
              *t++ = p[x];
              rat += x_ratio;
       }
    }

    /**
    static int cnt=0;
    char buf[80]={0,};
    cnt++;
    ///sprintf(buf, "img0_%d.pgm", cnt);
    ///writePgm(buf, src);
    sprintf(buf, "img1_%02d.pgm", cnt);
    dst->maxgrey = src->maxgrey;
    writePgm(buf, dst);
    */
}

/*****************************************************
 * Compute the integral image (and squared integral)
 * Integral image helps quickly sum up an area.
 * More info:
 * http://en.wikipedia.org/wiki/Summed_area_table
 ****************************************************/
static void _haar_integral_images( ImageByte *src, ImageInt *sum, ImageInt *sqsum )
{
  int x, y, s, sq, t, tq;
  unsigned char it;
  int height = src->height;
  int width = src->width;
  unsigned char *data = src->data;
  int * sumData = sum->data;
  int * sqsumData = sqsum->data;

  for( y = 0; y < height; y++)
    {
          s = 0;
          sq = 0;
          /* loop over the number of columns */
          for( x = 0; x < width; x ++)
            {
                  it = data[y*width+x];
                  /* sum of the current row (integer)*/
                  s += it;
                  sq += it*it;

                  t = s;
                  tq = sq;
                  if (y != 0)
                    {
                          t += sumData[(y-1)*width+x];
                          tq += sqsumData[(y-1)*width+x];
                    }
                  sumData[y*width+x]=t;
                  sqsumData[y*width+x]=tq;
            }
    }
}

/*******************************************************
 * Function: haar_detect_objects
 * Description: It calls all the major steps
 ******************************************************/
static int _haar_detect_objects( ImageByte* _img, MySize minSize, MySize maxSize, myCascade* cascade,
				   float scaleFactor, int minNeighbors, struct list_head *rect_list_head)
{
    //unsigned int cnt=0;
    int detected, dsum=0;
    ///const float GROUP_EPS = 0.4f;   //group overlaping windows
    ImageByte *img = _img;    //pointer to input image
  /***********************************
   * create structs for images
   * see haar.h for details
   * img1: normal image (unsigned char)
   * sum1: integral image (int)
   * sqsum1: square integral image (int)
   **********************************/
  ImageByte image1Obj;
  ImageInt sum1Obj;
  ImageInt sqsum1Obj;
  /* pointers for the created structs */
  ImageByte *img1 = &image1Obj;
  ImageInt *sum1 = &sum1Obj;
  ImageInt *sqsum1 = &sqsum1Obj;

  /********************************************************
   * allCandidates is the preliminaray face candidate,
   * which will be refined later.
   *
   * std::vector is a sequential container
   * http://en.wikipedia.org/wiki/Sequence_container_(C++)
   *
   * Each element of the std::vector is a "MyRect" struct
   * MyRect struct keeps the info of a rectangle (see haar.h)
   * The rectangle contains one face candidate
   *****************************************************/

  ///std::vector<MyRect> allCandidates;

  /* scaling factor */
  float factor;

  /* maxSize */
  if( maxSize.height == 0 || maxSize.width == 0 )
    {
      maxSize.height = img->height;
      maxSize.width = img->width;
    }

  /* window size of the training set */
  MySize winSize0 = cascade->orig_window_size;  ///24 x 24

  /* malloc for img1: unsigned char */
  createImage(img->width, img->height, img1);
  /* malloc for sum1: unsigned char */
  createSumImage(img->width, img->height, sum1);
  /* malloc for sqsum1: unsigned char */
  createSumImage(img->width, img->height, sqsum1);

  /* initial scaling factor */
  factor = 1;

  ///iterate over the image pyramid
  for( factor = 1; ; factor *= scaleFactor )    ///1.2
  {
      ///printf("%d: factor=%f\n", ++cnt, factor);

      ///scaled up size(24,29,35,41,50,60,72,86,103,124,...,920)
      MySize winSize = { myRound(winSize0.width*factor), myRound(winSize0.height*factor) };

      ///scaled down size(1280,1066,888,740,617,514,428,...,33)
      MySize sz = { ( img->width/factor ), ( img->height/factor ) };

      ///difference between sizes of the scaled image and the original detection window
      ///(sz.w-24, sz.h-24)
      MySize sz1 = { sz.width - winSize0.width, sz.height - winSize0.height };

      //printf("winSize.width=%d, winSize.height=%d\n", winSize.width, winSize.height);
      ///printf("\t sz.width=%d, sz.height=%d\n", sz.width, sz.height);
      //printf("sz1.width=%d, sz1.height=%d\n", sz1.width, sz1.height);
      //printf("\t winSize0.width=%d, winSize0.height=%d\n", winSize0.width, winSize0.height);

      /* if the actual scaled image is smaller than the original detection window, break */
      ///if( sz1.width < 0 || sz1.height < 0 )
      if( sz1.width - winSize0.width < 0 || sz1.height - winSize0.height < 0 )
            break;

      /* if a minSize different from the original detection window is specified, continue to the next scaling */
      if( winSize.width < minSize.width || winSize.height < minSize.height )    ///minSize==20
            continue;

      /*************************************
       * Set the width and height of
       * img1: normal image (unsigned char)
       * sum1: integral image (int)
       * sqsum1: squared integral image (int)
       * see image.c for details
       ************************************/
      setImage(sz.width, sz.height, img1);
      setSumImage(sz.width, sz.height, sum1);
      setSumImage(sz.width, sz.height, sqsum1);

      /***************************************************
       * Compute-intensive step:
       * building image pyramid by downsampling(from bigger to smaller)
       * downsampling using nearest neighbor
       ***************************************************/
      _harr_nearest_neighbor(img, img1);

      /***************************************************
       * Compute-intensive step:
       * At each scale of the image pyramid,
       * compute a new integral and squared integral image
       ***************************************************/
      _haar_integral_images(img1, sum1, sqsum1);

      /* sets images for haar classifier cascade */
      /**************************************************
       * Note:
       * Summing pixels within a haar window is done by
       * using four corners of the integral image:
       * http://en.wikipedia.org/wiki/Summed_area_table
       * This function loads the four corners,
       * but does not do compuation based on four coners.
       * The computation is done next in ScaleImage_Invoker
       *************************************************/
      _haar_set_image_classifier( cascade, sum1, sqsum1);

      /****************************************************
       * Process the current scale with the cascaded fitler.
       * The main computations are invoked by this function.
       * Optimization oppurtunity:
       * the same cascade filter is invoked each time
       ***************************************************/
      ///ScaleImage_Invoker(cascade, factor, sum1->height, sum1->width, allCandidates);

      detected = _haar_scale_image_invoker(cascade, factor, sum1->height, sum1->width, rect_list_head);
      if (detected > 0) {
        dsum += detected;
        ///printf("\t *Detected Count = %d(w=%d,h=%d)\n", detected, winSize.width, winSize.height);
      }

    } //end of the factor loop, finish all scales in pyramid

    /**
   if( minNeighbors != 0) {
        ///minNeighbors==1, 0.4
        ///groupRectangles(allCandidates, minNeighbors, GROUP_EPS);
   }
   */

  freeImage(img1);
  freeSumImage(sum1);
  freeSumImage(sqsum1);

  ///return allCandidates;
  return dsum;
}

void haar_run (ImageByte *image, char *path, char *type, uint32_t flag)
{
	i32 detected;
	unsigned int saved=0;
	static u32 face_idx=0;
	f32 scaleFactor = 1.2;    ///detection parameters
	i32 minNeighbours = 1;
    LIST_HEAD (rect_list_head);

	myCascade cascadeObj;
	myCascade *cascade = &cascadeObj;
	MySize minSize = {20, 20};
	MySize maxSize = {0, 0};

	//classifier properties
	cascade->n_stages=25;
	cascade->total_nodes=2913;
	cascade->orig_window_size.height = 24;
	cascade->orig_window_size.width = 24;

    //read classifier array from file(info.txt and class.txt)
	//readTextClassifier();

    pr_debug_msg("%s() running...\n", __FUNCTION__);
    detected = _haar_detect_objects(image, minSize, maxSize, cascade, scaleFactor, minNeighbours, &rect_list_head);
    pr_debug_msg("%s() end.\n", __FUNCTION__);
    if (detected > 0)
    {
        struct list_head *head;
        RectList *rlist;
        ImageByte face, face2;
        ImageFloat resize;
        int ret = 0;

        /**
        struct tm *tms;
        time_t t = time(NULL);
        tms = localtime(&t);
        char sdtime[40];
        sprintf(sdtime, "%04d%02d%02d_%02d%02d%02d"
            , 2000+(tms->tm_year%100), tms->tm_mon+1, tms->tm_mday
            , tms->tm_hour, tms->tm_min, tms->tm_sec);
        */

        __list_for_each(head, &rect_list_head)
        {
            rlist = list_entry(head, RectList, list);
            ///drawRectangle(image, rlist->rect);
            img_face_to_pgm (image, rlist->rect, &face);

            if (flag & ML_SAVE) {
                char fname[100];

                if (face.width >= FACE_WIDTH_124 && RunCfg.ncnt > 4) {
                    ret = img_resize_pgm_to_pgm(face, &face2, FACE_WIDTH_124, FACE_WIDTH_124, 1);
                } else if (face.width >= FACE_WIDTH_058) {
                    ret = img_resize_pgm_to_pgm(face, &face2, FACE_WIDTH_086, FACE_WIDTH_086, 1);
                } else {
                    ret = -1;
                }
                if (ret == 0) {
                    sprintf(fname, "%s/s%03d_%s_%03d.ppm", path, face.width, type, ++face_idx);
                    writePgm(fname, &face);
                    saved++;
                    if (face.data) free(face.data);
                } else if (ret > 0) {   ///resized
                    sprintf(fname, "%s/s%03d_%s_%03d.ppm", path, face2.width, type, ++face_idx);
                    writePgm(fname, &face2);
                    saved++;
                    if (face2.data) free(face2.data);
                } else {
                    pr_info_msg("Faces save cancel(width=%d)\n", face.width);
                    if (face.data) free(face.data);
                }
            }
            if (flag & ML_LEARN) {
                float cost;
                resize = img_resize(face, CNN_WIDTH, CNN_HEIGHT, CNN_COLORS);
                cost = nn_cnn_running (resize.data, LearnParam.ans, CNN_WIDTH * CNN_HEIGHT * CNN_COLORS, ML_RATE);
                printf("Learned(ans=%d, cost=%f)\n", LearnParam.ans, cost);
                free(resize.data);
            }

        } //for

        ///uint32_t cnt=0;
        __list_for_each(head, &rect_list_head)
        //list_for_each_entry_safe_reverse(rlist, rlist, &rect_list_head, list)
        {
            rlist = list_entry(head, RectList, list);
            __list_del(rlist->list.prev, rlist->list.next);
            if (rlist) {
                free(rlist);
                ///cnt++;
            }
        }
        ///printf("list free count: %d\n", cnt);
    }
    pr_info_msg("Faces(_%s_) Saved/Detected: %d/%d\n\n", type, saved, detected);
}
