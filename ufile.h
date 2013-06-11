#ifndef UFILE_H
#define UFILE_H
#include "sds.h"

/* This structure represents a background Job. It is only used locally to this
 * file as the API deos not expose the internals at all. */
struct bio_job {
    time_t time; /* Time at which the job was created. */
    sds name;
    sds result;
};

#define CCACHE_NUM_IO_THREADS 4

sds getFileContent(char* fn);
sds makeHttpReplyFromFile(char* fn);
void bioInit(void);
void bioCreateBackgroundJob(int tid, sds name) ;
unsigned int bioPendingJobsOfThread(int tid);
int bioGetResult(int tid, sds *name, sds *result);

#endif // UFILE_H
