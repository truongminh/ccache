#ifndef UFILE_H
#define UFILE_H
#include <stdint.h>
#include "sds.h"
#include "ccache_config.h"

typedef unsigned char uchar;


void ufileSetDirs(char *sdn, char *tdn);
sds ufileMakeHttpReplyFromFile(sds filepath);
sds ufilMakettpReplyFromBuffer(uchar *buf, size_t len);


sds ufilePathInSrcDir(sds fn);
sds ufilePathInTmpDirCharPtr(char *str);
sds ufilePathInTmpDir(sds fn);


#endif // UFILE_H
