/* bio.h
 */

#ifndef BIO_H
#define BIO_H

#include "lib/sds.h"
#include "ccache_config.h"

#define BIO_ZOOM_IMAGE 16
#define BIO_REMOVE_FILE 8
#define BIO_WRITE_FILE 4
#define BIO_READ_FILE 2
#define BIO_GENERAL BIO_READ_FILE
#define BIO_FINISHED 1

#define CCACHE_THREAD_STACK_SIZE (1024*1024*4)

/* This structure represents a background Job. It is only used locally to this
 * file as the API deos not expose the internals at all. */
struct bio_job {
    time_t time; /* Time at which the job was created. */
    int type;
    sds name;
    sds result;
};

void bioSetDirs(char *sdn, char *tdn);
sds bioPathInSrcDir(sds fn);
sds bioPathInTmpDirCharPtr(char *str);
sds bioPathInTmpDirSds(sds fn);
sds bioPathInTmpDir(char *base, char *str);

void bioInit(void);
void bioPushGeneralJob(sds name); /* reserved for master  */
void bioPushRemoveFileJob(sds name);
void bioPushWriteFileJob(sds name);
void bioCreateBackgroundJob(int tid, sds name, int type) ;
unsigned int bioPendingJobsOfThread(int tid);
int bioGetResult(int tid, sds *name, sds *result);

#endif // BIO_H
