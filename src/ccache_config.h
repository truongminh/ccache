#ifndef CCACHE_CONFIG_H
#define CCACHE_CONFIG_H

/* Error codes */
#define CCACHE_OK                0
#define CCACHE_ERR               -1

/* Static server configuration */
#define CCACHE_SERVERPORT        6379    /* TCP port */
#define CCACHE_MAXIDLETIME       0       /* default client timeout: infinite */
#define CCACHE_IOBUF_LEN         (1024*4)

/* Asynchronous I/O Options */
#define AE_MAX_PENDING_CLIENT 30000 /* Number of client pending at acceptor */
#define AE_MAX_CLIENT_IDLE_TIME 5 /* seconds */
#define AE_LOOP_INTERVAL 100 /* milliseconds */
#define AE_LOOP_ELLAPSED(loop,second) (loop%(second*1000/AE_LOOP_INTERVAL)==0)

#define CCACHE_ENCODING_RAW 0     /* Raw representation */
#define CCACHE_ENCODING_INT 1     /* Encoded as integer */

/* Log levels */
#define CCACHE_DEBUG 0
#define DEBUG CCACHE_DEBUG
#define CCACHE_VERBOSE 1
#define CCACHE_NOTICE 2
#define CCACHE_WARNING 3


#define CCACHE_LOG_LEVEL CCACHE_DEBUG
#define CCACHE_LOG_FILE "ccache.log"

/* Anti-warning macro... */
#define CCACHE_NOTUSED(V) ((void) V)

/* Caching Options */
#define MASTER_STATUS_REFRESH_PERIOD 5 /* 10 seconds */
#define MASTER_MAX_AVAIL_MEM (10L<<20) /* 100MB */
#define MASTER_MAX_AVAIL_DISK (100L<<20) /* 100MB */
#define ZOOM_MAX_ON_DISK (5L<<20) /* 5MB */

#define ONE_MEGABYTE (1<<20)
#define BYTES_TO_MEGABYTES(d) ((double)d/ONE_MEGABYTE)

/* Threads serving clients ordered by the accepting thread */
#define CCACHE_NUM_WORKER_THREADS    4
/* Threads doing background jobs ordered by the master cache */
#define CCACHE_NUM_BIO_THREADS 4

#define SERVICE_STATIC_FILE "/static"
#define SERVICE_ZOOM "/zoom"
#define CCACHE_MAX_URI_LEN 1024

/* Image Service Options */
#define IMG_CROP_AVAILABLE 1
#define IMG_MAX_WIDTH 1000
#define IMG_MAX_HEIGHT 1000

void ulog(int level, const char *fmt, ...);

#endif // CCACHE_CONFIG_H
