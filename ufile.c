#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h> /* strerror */
#include "ufile.h"
#include "adlist.h"
#include "safe_queue.h"


/* Thread-synchronization variable for each thread */
static pthread_mutex_t bio_mutex[CCACHE_NUM_IO_THREADS];
static pthread_cond_t bio_condvar[CCACHE_NUM_IO_THREADS];

/* Each thread has a list of io-pending jobs */
static list *bio_jobs[CCACHE_NUM_IO_THREADS];
static unsigned long long bio_pending[CCACHE_NUM_IO_THREADS];
static safeQueue *bio_job_results[CCACHE_NUM_IO_THREADS];


void *bioProcessBackgroundJobs(void *arg);

/* Make sure we have enough stack to perform all the things we do in the
 * main thread. */
#define CCACHE_THREAD_STACK_SIZE (1024*1024*4)


sds gccMakeHttpReplyFromFile(char* fn);


/* Initialize the background system, spawning the thread. */
void bioInit(void) {
    pthread_attr_t attr;
    pthread_t thread;
    size_t stacksize;
    int j;

    /* Initialization of state vars and objects */
    for (j = 0; j < CCACHE_NUM_IO_THREADS; j++) {
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
    for (j = 0; j < CCACHE_NUM_IO_THREADS; j++) {
        void *arg = (void*)(unsigned long) j;
        if (pthread_create(&thread,&attr,bioProcessBackgroundJobs,arg) != 0) {
            /*redisLog(REDIS_WARNING,"Fatal: Can't initialize Background Jobs.");*/
            exit(1);
        }
    }
}

void bioCreateBackgroundJob(int tid, sds name) {
    struct bio_job *job = malloc(sizeof(*job));

    job->time = time(NULL);
    job->name = name;
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

        /* TODO: Process the job accordingly to its type. */
        job->result = gccMakeHttpReplyFromFile(job->name+1);
        // free(job);
        safeQueuePush(bio_job_results[tid],job);
        /* Lock again before reiterating the loop, if there are no longer
         * jobs to process we'll block again in pthread_cond_wait(). */
        pthread_mutex_lock(&bio_mutex[tid]);
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
    if(job)
    {
        *name = job->name;
        *result = job->result;
        free(job);
        return 1;
    }
    return 0;
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
        perror ("fopen");
        return NULL;
    }
    /* if(size>1024*1024) return NULL; */
    struct stat fs;

    if(fstat(fileno(fp),&fs) != 0)
    {
        perror ("fstat");
        return NULL;
    }
    sds content = sdsempty();
    size_t size = fs.st_size;
    content = sdscatprintf(content,"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n",size);
    content = sdsMakeRoomFor(content,size);
    size_t before = sdslen(content);
    char* ptr = content + before;
    if (!fread (ptr, size, 1, fp)) {
        perror ("fread");
        sdsfree(content);
        return NULL;
    }
    if (fclose (fp)) {
        perror ("fclose");
    }

    sdsaddlen(content,size);
    //printf("==== Content ====  \n%s\n Content Length: %ld \n",content,sdslen(content));
    return content;
}
