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

#include "ufile.h"

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

sds makeHttpReplyFromFile(char* fn)
{
    int fd;
    struct stat fs;
    if((fd=open(fn,O_RDONLY)) < 0) return NULL;
    if (fstat(fd, &fs)) {
        perror ("stat");
        return NULL;
    }
    size_t size = fs.st_size;

    /* if(size>1024*1024) return NULL; */
    sds content = sdsempty();        
    content = sdscatprintf(content,"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n",size);
    content = sdsMakeRoomFor(content,size);
    size_t before = sdslen(content);
    char* ptr = content + sdslen(content);

    size_t remain = 2;
    ssize_t nread;
    while(remain > 0 && (nread = read(fd, ptr, remain) != 0)) {
        if(nread == -1) {
            printf("ERROR: %s\n",strerror(errno));
            if(errno == EINTR) {
                /* if the read syscall is interruptted, the program reissues it */
                continue;
            }
            /* Otherise, stop reading the file */
            break;
        }

        printf("nread : %ld\n",(unsigned long)nread);
        remain -= nread;
        /* 'nread' may be smaller than the actual number of read bytes
         * This make 'remain' unsafe when it is outside of the loop
         * However, we still use 'remain' to ensure that enough data was read.
         */
        ptr += nread;
    }
    close(fd);
    printf("==== Content ====  \n%s\n %ld \n",content,sdslen(content));

    if(sdslen(content)-before < size) {
        sdsfree(content);
        return NULL;
    }
    sdsaddlen(content,size);
    return content;
}


sds gccMakeHttpReplyFromFile(char* fn)
{
    FILE* fp;
    fp = fopen (fn, "r");
    if (!fp) {
        ulog(CCACHE_VERBOSE,"ufile fopen[%s] %s ",fn,strerror(errno));
        return NULL;
    }
    /* if(size>1024*1024) return NULL; */
    struct stat fs;

    if(fstat(fileno(fp),&fs) != 0)
    {
        ulog(CCACHE_WARNING,"ufile fstat[%s] %s",fn,strerror(errno));
        return NULL;
    }
    sds content = sdsempty();
    size_t size = fs.st_size;
    content = sdscatprintf(content,"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n",size);
    content = sdsMakeRoomFor(content,size);
    size_t before = sdslen(content);
    char* ptr = content + before;
    if (!fread (ptr, size, 1, fp)) {
        ulog(CCACHE_WARNING,"ufile fread[%s] %s",fn,strerror(errno));
        sdsfree(content);
        return NULL;
    }
    if (fclose (fp)) {
        ulog(CCACHE_WARNING,"ufile fclose[%s] %s",fn,strerror(errno));
    }

    sdsaddlen(content,size);
    //printf("==== Content ====  \n%s\n Content Length: %ld \n",content,sdslen(content));
    return content;
}

