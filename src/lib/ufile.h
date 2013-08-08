#ifndef UFILE_H
#define UFILE_H
#include <stdint.h>
#include "lib/sds.h"
#include "ccache_config.h"

typedef unsigned char uchar;


void ufileSetDirs(char *sdn, char *tdn);
sds ufileMakeHttpReplyFromFile(char *filepath);
sds ufilMakettpReplyFromBuffer(uchar *buf, size_t len);
int tmpDirLen();

sds ufilePathInSrcDir(sds fn);
sds ufilePathInTmpDirCharPtr(char *str);
sds ufilePathInTmpDirSds(sds fn);
sds ufilePathInTmpDir(char *base, char *str);

struct FileInfo {
    sds fn;
    size_t size;
};

void freeFileInfo(void *ptr);


#endif // UFILE_H
