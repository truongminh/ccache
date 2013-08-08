
#include <pthread.h>
#include <stdlib.h>
#include <string.h> /* strerror */
#include <errno.h>
#include "lib/ufile.h"
#include "lib/adlist.h"
#include "lib/safe_queue.h"
#include "service/img.h"
#include "lib/util.h" /* for stringstartwith */
#include "bio.h"

/* Thread-synchronization variable for each thread */
static pthread_mutex_t bio_mutex[CCACHE_NUM_BIO_THREADS];
static pthread_cond_t bio_condvar[CCACHE_NUM_BIO_THREADS];

/* Each thread has a list of io-pending jobs */
static list *bio_jobs[CCACHE_NUM_BIO_THREADS];
static unsigned long long bio_pending[CCACHE_NUM_BIO_THREADS];
static safeQueue *bio_job_results[CCACHE_NUM_BIO_THREADS];


void *bioProcessBackgroundJobs(void *arg);

/* Make sure we have enough stack to perform all the things we do in the
 * main thread. */

/* Initialize the background system, spawning the thread. */
void bioInit(void) {
    pthread_attr_t attr;
    pthread_t thread;
    size_t stacksize;
    int j;

    /* Initialization of state vars and objects */
    for (j = 0; j < CCACHE_NUM_BIO_THREADS; j++) {
        pthread_mutex_init(&bio_mutex[j],NULL);
        pthread_cond_init(&bio_condvar[j],NULL);
        bio_jobs[j] = listCreate();
        bio_pending[j] = 0;
        bio_job_results[j] = safeQueueCreate();
    }

    /* Set the stack size as by default it may be small in some system */
    pthread_attr_init(&attr);
    pthread_attr_getstacksize(&attr,&stacksize);
    if (!stacksize) stacksize = 1; /* The world is full of Solaris Fixes */
    while (stacksize < CCACHE_THREAD_STACK_SIZE) stacksize *= 2;
    pthread_attr_setstacksize(&attr, stacksize);

    /* Ready to spawn our threads. We use the single argument the thread
     * function accepts in order to pass the job ID the thread is
     * responsible of. */
    for (j = 0; j < CCACHE_NUM_BIO_THREADS; j++) {
        void *arg = (void*)(unsigned long) j;
        if (pthread_create(&thread,&attr,bioProcessBackgroundJobs,arg) != 0) {
            ulog(CCACHE_WARNING,"Fatal: Can't initialize Background Threads.");
            exit(1);
        }
    }
}

static int ctid = 0;
void bioPushGeneralJob(sds name) {
    bioCreateBackgroundJob(ctid,name,BIO_GENERAL);
    ctid++;
    if(ctid == CCACHE_NUM_BIO_THREADS) ctid = 0;
}
void bioPushRemoveFileJob(sds name) {
    bioCreateBackgroundJob(ctid,name,BIO_REMOVE_FILE);
    ctid++;
    if(ctid == CCACHE_NUM_BIO_THREADS) ctid = 0;
}

/* This function is thread-safe */
void bioCreateBackgroundJob(int tid, sds name,int type) {
    struct bio_job *job = malloc(sizeof(*job));

    job->time = time(NULL);
    job->name = name;
    job->type = type;
    pthread_mutex_lock(&bio_mutex[tid]);
    listAddNodeTail(bio_jobs[tid],job);
    bio_pending[tid]++;
    pthread_cond_signal(&bio_condvar[tid]);
    pthread_mutex_unlock(&bio_mutex[tid]);
}

void *bioProcessBackgroundJobs(void *arg) {
    struct bio_job *job;
    unsigned long tid = (unsigned long) arg;

    pthread_detach(pthread_self());
    pthread_mutex_lock(&bio_mutex[tid]);
    while(1) {
        listNode *ln;

        /* The loop always starts with the lock hold. */
        if (listLength(bio_jobs[tid]) == 0) {
            pthread_cond_wait(&bio_condvar[tid],&bio_mutex[tid]);
            continue;
        }
        /* Pop the job from the queue. */
        ln = listFirst(bio_jobs[tid]);
        job = ln->value;
        /* It is now possible to unlock the background system as we know have
         * a stand alone job structure to process.*/
        pthread_mutex_unlock(&bio_mutex[tid]);

        /* NOTICE: path must be safe before used */
        if(notsafePath(job->name)) {
            job->result = NULL;
            safeQueuePush(bio_job_results[tid],job); /* the current job will be freed by master */
            ulog(CCACHE_VERBOSE,"Invalid uri: too long or contain [..]");
            goto finish;
        }
        if(job->type&BIO_GENERAL) {
            if(stringstartwith(job->name,SERVICE_STATIC_FILE)) {
                /* This is static file job */
                sds fn = sdsnew(job->name+strlen(SERVICE_STATIC_FILE));
                sds path = ufilePathInSrcDir(fn);
                job->result = ufileMakeHttpReplyFromFile(path);
                sdsfree(fn);
                sdsfree(path);
                safeQueuePush(bio_job_results[tid],job); /* the current job will be freed by master */
                goto finish;
            }
            else if(stringstartwith(job->name,SERVICE_IMG_ZOOM)) {
                /* Search tmp folder */
                sds path = ufilePathInTmpDir(SERVICE_IMG_ZOOM,job->name+strlen(SERVICE_IMG_ZOOM));
                printf("path %s\n",path);
                job->result = ufileMakeHttpReplyFromFile(path);                
                if(job->result) {
                    safeQueuePush(bio_job_results[tid],job); /* the current job will be freed by master */
                }
                else {
                    /* Resize an existing image if possible */
                    imgAuto(bio_job_results[tid],job,path);
                }
                sdsfree(path);
                goto finish;
            }
        }
        if(job->type&BIO_REMOVE_FILE) {
            // remove file
            sds path = ufilePathInTmpDirSds(job->name);
            if(remove(path)==0) {
                ulog(CCACHE_DEBUG,"%s deleted\n",path);
            }
            else {
                 ulog(CCACHE_WARNING," remove [%s] %s",path,strerror(errno));
            }
            free(job); /* Ignore job result */
            goto finish;
        }
        /* NOTICE: never push the same job twice */
        finish:
        /* Lock again before reiterating the loop, if there are no longer
         * jobs to process we'll block again in pthread_cond_wait(). */
        pthread_mutex_lock(&bio_mutex[tid]);
        /* NOTICE */
        listDelNode(bio_jobs[tid],ln);
        bio_pending[tid]--;
    }
}

/* Return the number of pending jobs of the specified thread. */
unsigned int bioPendingJobsOfThread(int tid) {
    unsigned long long val;
    pthread_mutex_lock(&bio_mutex[tid]);
    val = bio_pending[tid];
    pthread_mutex_unlock(&bio_mutex[tid]);
    return val;
}

int bioGetResult(int tid, sds *name, sds *result) {
    struct bio_job *job = safeQueuePop(bio_job_results[tid]);
    int res = 0;
    if(job)
    {
        *name = job->name;
        *result = job->result;        
        res = job->type;
        free(job);
    }
    return res;
}
