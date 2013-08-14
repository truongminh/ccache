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
#include "service/zoom.h"
#include "lib/ufile.h"
#include "lib/util.h"
#include "organizer/bio.h"
#include "lib/adlist.h"

#define IMG_ENCODE_DEFAULT ".jpg"
#define IMG_DEFAULT_QUALITY 100 /* percent */


static sds zoomSrcDir;
static sds zoomTmpDir;
static list *zoomTmpFiles;
static double zoomTotalTmpFile;
static int baseoffset; /* len of zoomTmpDir */
static void saveImage(char *dstpath, int baseoffset, uchar *buf, size_t len);
static int img_parse_uri(const char *uri, sds *filename, int *w, int *h, int *c, int *q);

typedef enum {
    uri_start,
    fn_start,
    width_start,
    height_start,
    crop_start,
    quality_start,
    parse_success_no_params,
    parse_error
} uri_parse_state;

void zoomServiceInit(sds srcDir)
{
    /* service */
    zoomSrcDir = sdsdup(srcDir);
    zoomTmpDir = bioPathInTmpDirCharPtr(SERVICE_ZOOM+1);
    if(utilMkdir(zoomTmpDir)) exit(EXIT_FAILURE);
    baseoffset = sdslen(zoomTmpDir);
    zoomTmpFiles = listCreate();
    listSetFreeMethod(zoomTmpFiles,freeFileInfo);

    if(ufilescanFolder(zoomTmpFiles,zoomTmpDir,3)) {
        printf("scan fail \n");
        exit(EXIT_FAILURE);
    }
    zoomTotalTmpFile = 0;
    listIter li;
    listNode *ln;
    listRewind(zoomTmpFiles,&li);
    while ((ln = listNext(&li)) != NULL) {
        struct FileInfo *fi = listNodeValue(ln);
        zoomTotalTmpFile += fi->size;
    }
}

void zoomImg(safeQueue *sq, struct bio_job *job)
{
    /* Search tmp folder */

    char *uri = job->name+strlen(SERVICE_ZOOM)+1;
    sds dstpath = bioPathInTmpDir(SERVICE_ZOOM,uri);
    printf("path %s\n",dstpath);
    //job->result = ufileMakeHttpReplyFromFile(dstpath);
    job->result = ufileMmapHttpReply(dstpath);
    if(job->result) {
        sdsfree(dstpath);
        safeQueuePush(sq,job); /* the current job will be freed by master */
        return;
    }

    int width = 0, height = 0;
    sds fn = NULL;
    sds srcpath = NULL;
    IplImage* src = NULL;
    IplImage* dst = NULL;
    IplImage* toencode = NULL;
    CvMat* enImg = NULL;
    int notpushed = 1;
    int iscrop = 1;
    int p[3];
    p[0] = CV_IMWRITE_JPEG_QUALITY;
    p[1] = IMG_DEFAULT_QUALITY;
    p[2] = 0;
    uchar *buf = NULL;
    size_t len = 0;
    uri_parse_state state = img_parse_uri(uri,&fn,&width,&height, &iscrop, &p[1]);
    if(state == parse_error) goto clean;
    // initializations
    srcpath = bioPathInSrcDir(fn);
    printf("src %s\n",srcpath);
    src = cvLoadImage(srcpath, CV_LOAD_IMAGE_COLOR);
    /* validate that everything initialized properly */
    if(!src)
    {
        ulog(CCACHE_VERBOSE,"can't load image file: %s\n",srcpath);
        goto clean;
    }

    int src_width = src->width;
    int src_height = src->height;
    int roi_src_width = src_width;
    int roi_src_height = src_height;


    if(width&&height) {
        /* Preserve origial ratio */
        /* NOTICE: dangerous type conversion */
        roi_src_width = src_height*width/height;
        roi_src_height = src_width*height/width;
        if(roi_src_width>src_width) roi_src_width = src_width;
        if(roi_src_height>src_height) roi_src_height = src_height;
    }
    else if(!width&&height) {
        width = src_width;
    }
    else if(width&&!height) {
        height = src_height;
    }
    else {
        toencode = src;
    }

    if(!toencode) {
        if(iscrop) {
            int x = (src_width - roi_src_width)/2;
            int y = (src_height - roi_src_height)/2;
            // Say what the source region is
            cvSetImageROI( src, cvRect(x,y,roi_src_width,roi_src_height));
        }

        dst = cvCreateImage(cvSize(width,height), src->depth, src->nChannels);
        if(!dst) goto clean;

        cvResize(src,dst,CV_INTER_CUBIC);

        if(iscrop) {
            cvResetImageROI( src );
        }

        toencode = dst;
    }


    enImg = cvEncodeImage(IMG_ENCODE_DEFAULT, toencode, p );
    buf = enImg->data.ptr;
    len = enImg->rows*enImg->cols;
    job->result = ufilMakettpReplyFromBuffer(buf,len);
    job->type |= BIO_WRITE_FILE; /* Remind master of new written file  */
    safeQueuePush(sq,job);    
    notpushed = 0;

  /* clean up and release resources */
clean:
    if(notpushed) {
        job->result = NULL;
        safeQueuePush(sq,job);
    }
    if(fn) sdsfree(fn);
    if(srcpath) sdsfree(srcpath);
    if(src) cvReleaseImage(&src);
    if(enImg){
        saveImage(dstpath, baseoffset, buf, len);
        cvReleaseMat(&enImg);
    }
    sdsfree(dstpath);
    if(dst) cvReleaseImage(&dst);
    return;
}

void saveImage(char* dstpath, int baseoffset, uchar *buf, size_t len)
{
    /* create subdirs */
    if(utilMkSubDirs(dstpath,baseoffset)) return;   
    //if(!ufileWriteFile(dstpath,buf,len)) {
    if(!ufileMmapWrite(dstpath,buf,len)) {
        listAddNodeTail(zoomTmpFiles,sdsnew(dstpath));
        zoomTotalTmpFile += len;
    }
    printf("%s HAVE: %-6.2lfMB USED: %-6.2lf%%\n",
           SERVICE_ZOOM,
           BYTES_TO_MEGABYTES(ZOOM_MAX_ON_DISK),
           zoomTotalTmpFile/ZOOM_MAX_ON_DISK);
    listIter li;
    listNode *ln;
    listRewind(zoomTmpFiles,&li);
    while ((ln = listNext(&li)) != NULL&&zoomTotalTmpFile > ZOOM_MAX_ON_DISK) {
        struct FileInfo *fi = listNodeValue(ln);
        /* delete file on disk */
        if(remove(fi->fn)==0) {
            ulog(CCACHE_DEBUG,"%s deleted \n",fi->fn);
            zoomTotalTmpFile -= fi->size;
        }
        else {
             ulog(CCACHE_WARNING," remove [%s] %s",fi->fn,strerror(errno));
        }
        listDelNode(zoomTmpFiles,ln);
    }
}

int img_parse_uri(const char *uri, sds *filename, int *w, int *h, int *c, int *q)
{
    uri_parse_state state = uri_start;
    int width = 0, height = 0, crop = 1;
    int quality = 0;
    const char *ptr = uri;
    /* uri is of the form: fn?w=x&h=y */
    while(*ptr!='?'&&*ptr++);    
    *filename = sdsnewlen(uri,ptr-uri);
    if(!*ptr) return parse_success_no_params; /* No '?' */
    /* *ptr now is '?' */
    ptr++;
    do {
        switch(*ptr) {
        case 'w':
            state = width_start;
            break;
        case 'h':
            state = height_start;
            break;
        case 'c':
            state = crop_start;
            break;
        case 'q':
            state = quality_start;
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
                width = width*10 + (*ptr) - '0';
            }
            else if(state == height_start) {
                height = height*10 + (*ptr) - '0';
            }
            else if(state == crop_start) {
                crop = *ptr - '0';
            }
            else if(state == quality_start) {
                quality = quality*10 + (*ptr) - '0';
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
    else if(width < 0 || width>IMG_MAX_WIDTH || height< 0 || height>IMG_MAX_HEIGHT) {
        ulog(CCACHE_VERBOSE,"Not conform %d %d %s",width,height,*filename);
        return parse_error;
    }
    /* width and height is ensure to be at least 0 */
    *w = width;
    *h = height;
    *c = crop;
    if (quality> 0 && quality < 100) *q = quality;
    return state;
}
