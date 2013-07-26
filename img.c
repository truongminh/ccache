#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>

#include "img.h"
#include "sds.h"

int imgAuto(sds url)
{
    int width,height;
    int count;
    sds *tokens = sdssplitlen(url,sdslen(url),"/",1,&count);
    if (count != 5 || strcmp(tokens[1],"zoom") != 0) {
        sdsfreesplitres(tokens,count);
        return IMG_ERR;
    }

    width = atoi(tokens[2]);
    height = atoi(tokens[3]);
    char *fn = tokens[4];
    if(width<1 || width>1000 || height<1 || height>1000||fn[0]=='.') {
        ulog(CCACHE_VERBOSE,"Not conform %d %d %s",width,height,fn);
        return IMG_ERR;
    }
    sdsfreesplitres(tokens, count-1);
    return resize(fn,width,height);
}

int resize( sds fn,  unsigned int width, unsigned int height)
{
    int err = IMG_ERR;
    // initializations
    sds srcpath = sdsnew(IMG_SRC_STRING);
    sds dstpath = NULL;
    srcpath = sdscatprintf(srcpath,"/%s",fn);
    IplImage* src = cvLoadImage(srcpath, CV_LOAD_IMAGE_COLOR);
    IplImage* dst = NULL;
    // validate that everything initialized properly
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

  dstpath = sdsnew(IMG_ZOOM_STRING);
  dstpath = sdscatprintf(dstpath,"/%d",width);
  /*TODO: use a dict of created dirs to the reduce number of syscalls 'mkdir'*/
  if(mkdir(dstpath,IMG_ZOOM_DIR_MODE) == 0 || errno == EEXIST) {
      ;
  }
  else {
      ulog(CCACHE_WARNING,"Error making directory [%s]: %s\n", dstpath, strerror(errno));
      goto clean;
  }
  dstpath = sdscatprintf(dstpath,"/%d",height);
  if(mkdir(dstpath,IMG_ZOOM_DIR_MODE) == 0 || errno == EEXIST) {
      /* add to dict */ ;
  }
  else {
      ulog(CCACHE_WARNING,"Error making directory [%s]: %s\n", dstpath, strerror(errno));
      goto clean;
  }
  dstpath = sdscatprintf(dstpath,"/%s",fn);
  printf("%d",cvSaveImage (dstpath, dst, 0));
  cvReleaseImage(&src);
  err = IMG_OK;
  // clean up and release resources
clean:
    sdsfree(srcpath);
    sdsfree(fn);
    if(dstpath) sdsfree(dstpath);
    if(dst) cvReleaseImage(&dst);
    cvReleaseImage(&src);
    return err;
}
