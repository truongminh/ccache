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
#include <dirent.h>
#include "ufile.h"
#include "lib/util.h"



void freeFileInfo(void *ptr)
{
    if(ptr) {
        struct FileInfo *fi = (struct FileInfo*)ptr;
        sdsfree(fi->fn);
        free(fi);
    }
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

int ufilescanFolder(list *files, char *indir, int depth)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (indir)) != NULL) {
      /* print all the files and directories within directory */
      while ((ent = readdir (dir)) != NULL) {
          switch(ent->d_type) {
          case DT_REG:
          {
              sds fn = sdsnew(indir);
              fn = sdscatprintf(fn,"/%s",ent->d_name);
              int fd;
              struct stat fs;
              if((fd=open(fn,O_RDONLY)) < 0) {
                  ulog(CCACHE_WARNING,"ufile fstat[%s] %s",fn,strerror(errno));
                  return 0;
              }
              if (fstat(fd, &fs)) {
                  ulog(CCACHE_WARNING,"ufile fstat[%s] %s",fn,strerror(errno));
                  return 0;
              }

              struct FileInfo* fi = (struct FileInfo*) malloc(sizeof(struct FileInfo));
              fi->fn = fn;
              fi->size = fs.st_size;
              /* regular files */
              listAddNodeTail(files,fi);
              break;
          }
          case DT_DIR:
              if(depth > 0) {
                  if(ent->d_name[0]!='.') {
                      sds path = sdsnew(indir);
                      path = sdscatprintf(path,"/%s",ent->d_name);
                      /* folders */
                      ufilescanFolder(files, path, depth-1);
                      sdsfree(path);
                  }
              }
              break;
          case DT_LNK:
               /* ignore */
              break;
          default:
              break;
          }
      }
      closedir (dir);
      return 0;
    }
    else {
        /* could not open directory */
        ulog(CCACHE_WARNING,"cannot scan directory [%s] %s \n",indir, strerror(errno));
        return 1;
    }
}
