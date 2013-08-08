/* ufile.c - build http reply from file content
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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ufile.h"
#include "lib/util.h"
#include "mhash.h"

static sds srcDir;
static sds tmpDir;

int tmpDirLen() {
    return sdslen(tmpDir);
}

void freeFileInfo(void *ptr)
{
    if(ptr) {
        struct FileInfo *fi = (struct FileInfo*)ptr;
        sdsfree(fi->fn);
        free(fi);
    }
}

static inline sds ufilePathTrailingSlash(char *fn) {
    sds path = sdsnew(fn);
    if(path&&path[sdslen(path)-1] != '/') {
            path = sdscat(path,"/");
    }
    return path;
}

sds ufilePathInSrcDir(sds fn) {
    sds path = sdsdup(srcDir);
    path = sdscatsds(path,fn);
    return path;
}

sds ufilePathInTmpDirCharPtr(char *str) {
    sds path = sdsdup(tmpDir);
    path = sdscat(path,str);
    return path;
}

sds ufilePathInTmpDirSds(sds fn) {
    sds path = sdsdup(tmpDir);
    path = sdscatsds(path,fn);
    return path;
}

sds ufilePathInTmpDir(char *base, char *str) {
    sds path = sdsdup(tmpDir);
    char *dn = mhashFunction((unsigned char*)str,strlen(str));
    char *fn = fast_url_encode(str);
    path = sdscatprintf(path,"%s/%s/%s",base,dn,fn);
    free(dn);free(fn);
    return path;
}

void ufileSetDirs(char *sdn, char *tdn)
{
    srcDir = ufilePathTrailingSlash(sdn);
    tmpDir = ufilePathTrailingSlash(tdn);
    if(notsafePath(srcDir)){
        printf("SAFE DIR MUST NOT HAVE 2 CONSECUTIVE '.' :  %s\n",srcDir);
        exit(EXIT_FAILURE);
    }
    if(notsafePath(tmpDir)){
        printf("SAFE DIR MUST NOT HAVE 2 CONSECUTIVE '.' :  %s\n",tmpDir);
        exit(EXIT_FAILURE);
    }
    if(utilMkdir(tmpDir)) exit(EXIT_FAILURE);
    ulog(CCACHE_WARNING, "Src Dir: %s\n",srcDir);
    ulog(CCACHE_WARNING, "Tmp Dir: %s\n",tmpDir);
    /* service */
    char zoomservice[2048];
    sprintf(zoomservice,"%s%s",tmpDir,SERVICE_IMG_ZOOM);
    if(utilMkdir(zoomservice)) exit(EXIT_FAILURE);
}

sds getFileContent(char* fn)
{
    int fd;
    struct stat fs;
    if((fd=open(fn,O_RDONLY)) < 0) return NULL;
    if (fstat(fd, &fs)) {
        perror ("stat");
        return NULL;
    }
    sds content = sdsempty();
    sdsMakeRoomFor(content,fs.st_size);
    int nread = read(fd, content+sdslen(content), fs.st_size);
    sdsaddlen(content,nread);
    printf("Size %ld",sdslen(content));
    close(fd);
    if(nread == 0) return NULL;
    return content;
}

sds ufileMakeHttpReplyFromFile(char *filepath)
{
    FILE* fp;
    fp = fopen (filepath, "r");
    if (!fp) {
        ulog(CCACHE_VERBOSE,"ufile fopen[%s] %s ",filepath,strerror(errno));
        return NULL;
    }
    /* if(size>1024*1024) return NULL; */
    struct stat fs;

    if(fstat(fileno(fp),&fs) != 0)
    {
        ulog(CCACHE_WARNING,"ufile fstat[%s] %s",filepath,strerror(errno));
        return NULL;
    }
    sds content = sdsempty();
    size_t size = fs.st_size;
    content = sdscatprintf(content,"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n",size);
    content = sdsMakeRoomFor(content,size);
    size_t before = sdslen(content);
    char* ptr = content + before;
    if (!fread (ptr, size, 1, fp)) {
        ulog(CCACHE_WARNING,"ufile fread[%s] %s",filepath,strerror(errno));
        sdsfree(content);
        return NULL;
    }
    if (fclose (fp)) {
        ulog(CCACHE_WARNING,"ufile fclose[%s] %s",filepath,strerror(errno));
    }

    sdsaddlen(content,size);
    //printf("==== Content ====  \n%s\n Content Length: %ld \n",content,sdslen(content));
    return content;
}

sds ufilMakettpReplyFromBuffer(uchar *buf, size_t len)
{
    sds content = sdsempty();
    content = sdscatprintf(content,"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n",len);
    content = sdscatlen(content,buf,len);
    return content;
}
