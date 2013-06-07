#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
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
