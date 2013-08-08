/* img.c - Resize image
 *
 * Copyright (c) 2013, Nguyen Truong Minh <nguyentrminh at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of CCACHE nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include "img.h"
#include "ufile.h"
#include "util.h"

#define IMG_ENCODE_DEFAULT ".jpg"

static void saveImage(sds dstpath, int baseoffset, uchar *buf, size_t len);
static int img_parse_uri(const char *uri, sds *filename, int *w, int *h);

typedef enum {
    uri_start,
    fn_start,
    width_start,
    height_start,
    parse_error
} uri_parse_state;

void imgAuto(safeQueue *sq, struct bio_job *job, sds dstpath)
{
    char *uri = job->name+strlen(SERVICE_IMG_ZOOM)+1;
    int baseoffset = tmpDirLen() + strlen(SERVICE_IMG_ZOOM)+1;
    int width = 0, height = 0;
    sds fn = NULL;
    if(img_parse_uri(uri,&fn,&width,&height) == parse_error) {
        goto clean;
    }

    uchar *buf = NULL;
    size_t len = 0;
    int notpushed = 1;
    // initializations
    sds srcpath = ufilePathInSrcDir(fn);
    IplImage* src = cvLoadImage(srcpath, CV_LOAD_IMAGE_COLOR);
    IplImage* dst = NULL;
    CvMat* enImg = NULL;
    /* validate that everything initialized properly */
    if(!src)
    {
        ulog(CCACHE_VERBOSE,"can't load image file: %s\n",fn);
        goto clean;
    }
    /* NOTICE: dangerous type conversion */
    double ratio = (double)width/height;
    int new_width = src->height*ratio;
    int new_height = src->width/ratio;
    if(new_width>src->width) new_width = src->width;
    if(new_height>src->height) new_height = src->height;

    #ifdef IMG_CROP_AVAILABLE
    int x = (src->width - new_width)/2;
    int y = (src->height - new_height)/2;
    // Say what the source region is
    cvSetImageROI( src, cvRect(x,y,new_width,new_height));
    #endif

    // Must have dimensions of output image
    dst = cvCreateImage(cvSize(width,height), src->depth, src->nChannels);
    if(!dst) goto clean;

    cvResize(src,dst,CV_INTER_CUBIC);

#ifdef IMG_CROP_AVAILABLE
    cvResetImageROI( src );
#endif

    int p[3];
    p[0] = CV_IMWRITE_JPEG_QUALITY;
    p[1] = 80; /* JPEG QUALITY */
    p[2] = 0;
    enImg = cvEncodeImage(IMG_ENCODE_DEFAULT, dst, p );
    buf = enImg->data.ptr;
    len = enImg->rows*enImg->cols;
    job->result = ufilMakettpReplyFromBuffer(buf,len);
    job->type |= BIO_WRITE_FILE; /* Remind master of new written file  */
    safeQueuePush(sq,job);
    saveImage(dstpath, baseoffset, buf, len);
    notpushed = 0;

  /* clean up and release resources */
clean:
    if(notpushed) {
        job->result = NULL;
        safeQueuePush(sq,job);
    }
    sdsfree(fn);
    sdsfree(srcpath);    
    if(src) cvReleaseImage(&src);
    if(enImg) cvReleaseMat(&enImg);
    if(dst) cvReleaseImage(&dst);
    return;
}

void saveImage(sds dstpath, int baseoffset, uchar *buf, size_t len)
{
    /* create subdirs */
    if(utilMkSubDirs(dstpath,baseoffset)) return;
    FILE *outfile = fopen(dstpath, "wb");
    if (outfile == NULL) {
        ulog(CCACHE_WARNING, "can't write to %s\n", dstpath);
    }
    else {
        fwrite(buf,len,1,outfile);
    }
}

int img_parse_uri(const char *uri, sds *filename, int *w, int *h)
{
    uri_parse_state state = uri_start;
    int width = 0, height = 0;
    const char *ptr = uri;
    /* uri is of the form: fn?w=x&h=y */
    while(*ptr!='?'&&*ptr++);
    if(!*ptr) return parse_error; /* No '?' */
    /* *ptr now is '?' */
    *filename = sdsnewlen(uri,ptr-uri);
    ptr++;
    do {
        switch(*ptr) {
        case 'w':
            state = width_start;
            break;
        case 'h':
            state = height_start;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if(state == width_start) {
                width = width*10+(*ptr) - '0';
            }
            else if(state == height_start) {
                height = height*10+(*ptr) - '0';
            }
            break;
        case '&':
        case '=':
            break;
        default:
            state = parse_error;
            break;
        }
    }while(*++ptr&&(state!=parse_error));
    if(state == parse_error) {
        ulog(CCACHE_VERBOSE,"parse uri error %s", uri);
        return parse_error;
    }
    else if(width<1 || width>IMG_MAX_WIDTH || height<1 || height>IMG_MAX_HEIGHT) {
        ulog(CCACHE_VERBOSE,"Not conform %d %d %s",width,height,*filename);
        return parse_error;
    }
    *w = width;
    *h = height;
    return state;
}
