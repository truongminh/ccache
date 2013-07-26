#ifndef BIO_H
#define BIO_H

#include "sds.h"
#include "ccache_config.h"


#define BIO_RESIZE_IMAGE 2
#define BIO_STATIC_FILE 1
#define CCACHE_THREAD_STACK_SIZE (1024*1024*4)

/* This structure represents a background Job. It is only used locally to this
 * file as the API deos not expose the internals at all. */
struct bio_job {
    time_t time; /* Time at which the job was created. */
    int type;
    sds name;
    sds result;
};

void bioInit(void);
void bioPushStaticFileJob(sds name); /* reserved for master  */
void bioCreateBackgroundJob(int tid, sds name, int type) ;
unsigned int bioPendingJobsOfThread(int tid);
int bioGetResult(int tid, sds *name, sds *result);

#endif // BIO_H
