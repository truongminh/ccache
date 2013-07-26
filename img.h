#ifndef IMG_H
#define IMG_H

#include <sys/stat.h>
#include <cv.h>
#include <highgui.h>
#include "sds.h"
#include "ccache_config.h"


#define IMG_ZOOM_DIR_MODE S_IRUSR | S_IWUSR | S_IXUSR
#define IMG_OK 1
#define IMG_ERR 0

int imgAuto(sds url);
int resize(sds fn,  unsigned int width, unsigned int height);

#endif // IMG_H
