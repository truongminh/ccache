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
#include <sys/mman.h>
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

sds ufileMmapHttpReply(char *fn)
{
    int fdin;
    struct stat fs;
    if((fdin = open(fn,O_RDONLY)) < 0) {
        ulog(CCACHE_WARNING,"ufile open[%s] %s",fn,strerror(errno));
        return NULL;
    }
    if (fstat(fdin, &fs)) {
        ulog(CCACHE_WARNING,"ufile fstat[%s] %s",fn,strerror(errno));
        return NULL;
    }
    sds content = sdsempty();
    size_t size = fs.st_size;
    content = sdscatprintf(content,"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n",size);
    content = sdsMakeRoomFor(content,size);
    size_t before = sdslen(content);
    char* dst = content + before;
    char *src;
    if ((src = mmap(0, size, PROT_READ, MAP_SHARED, fdin, 0)) == MAP_FAILED) {
        ulog(CCACHE_WARNING,"ufile mmap[%s] %s",fn,strerror(errno));
        sdsfree(content);
        close(fdin);
        return NULL;
    }
    memcpy(dst,src,size);
    munmap(src,size);
    sdsaddlen(content,size);
    close(fdin);
    return content;

}

/* Use native UNIX API syscall */
sds ufileMakeHttpReplyFromFile(char* fn)
{
    int fd;
    struct stat fs;
    if((fd = open(fn,O_RDONLY)) < 0) {
        ulog(CCACHE_WARNING,"ufile open[%s] %s",fn,strerror(errno));
        return NULL;
    }
    if (fstat(fd, &fs)) {
        ulog(CCACHE_WARNING,"ufile fstat[%s] %s",fn,strerror(errno));
        return NULL;
    }
    sds content = sdsempty();
    size_t size = fs.st_size;
    content = sdscatprintf(content,"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n",size);
    content = sdsMakeRoomFor(content,size);
    size_t before = sdslen(content);
    char* ptr = content + before;
    size_t nleft = size;
    ssize_t nread;
    while(nleft>0) {
        if((nread = read(fd, content+sdslen(content), nleft)) < 0) {
            if(errno == EINTR) /* Interrupted by sighandler */
                nread = 0; /* call read again */
            else {
                ulog(CCACHE_WARNING,"ufile read[%s] %s",fn,strerror(errno));
                sdsfree(content);
                close(fd);
                return NULL;
            }
        }
        else if(nread == 0) {
            break; /* End-Of-File */
        }
        nleft -= nread;
        ptr += nread;
    }
    sdsaddlen(content,size);
    close(fd);
    return content;
}

/* Use C standard I/O */
sds _ufileMakeHttpReplyFromFile(char *filepath)
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

/* The write function from RIO package */
ssize_t ufileWriteFile(char *fn, void *src, size_t size)
{
    int fd;    
    if((fd = open(fn, O_RDWR | O_CREAT | O_TRUNC, FILE_MODE)) < 0) {
        ulog(CCACHE_WARNING,"ufile open[%s] %s",fn,strerror(errno));
        return -1;
    }
    size_t nleft = size;
    ssize_t nwritten;
    char *bufp = src;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR) /* Interrupted by sig handler return */
                nwritten = 0; /* and call write() again */
            else {
                ulog(CCACHE_WARNING,"ufile write[%s] %s",fn,strerror(errno));
                return -1; /* errno set by write() */
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return 0;
}

ssize_t ufileMmapWrite(char *fn, void *src, size_t size)
{
    int fdout;
    if ((fdout = open(fn, O_RDWR | O_CREAT | O_TRUNC, FILE_MODE)) < 0) {
        ulog(CCACHE_WARNING,"ufile open[%s] %s",fn,strerror(errno));
        return -1;
    }
    /* set size of output file */
    if (lseek(fdout, size - 1, SEEK_SET) == -1) {
       ulog(CCACHE_WARNING,"ufile lseek[%s] %s",fn,strerror(errno));
       return -1;
    }
    if (write(fdout, "", 1) != 1){
       ulog(CCACHE_WARNING,"ufile test write[%s] %s",fn,strerror(errno));
       return -1;
    }
    void *dst;
    if ((dst = mmap(0, size, PROT_READ | PROT_WRITE,MAP_SHARED, fdout, 0)) == MAP_FAILED){
        ulog(CCACHE_WARNING,"ufile mmap[%s] %s",fn,strerror(errno));
        return -1;
     }
    memcpy(dst, src, size); /* does the file copy */
    munmap(dst,size);
    return 0;
}

/*
 * get_filetype - derive file type from file name.
 * Source code in TINY web server
 */

void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}
