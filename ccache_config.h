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

#define CCACHE_ENCODING_RAW 0     /* Raw representation */
#define CCACHE_ENCODING_INT 1     /* Encoded as integer */

/* Hash table parameters */
#define CCACHE_HT_MINFILL        10      /* Minimal hash table fill 10% */

/* Log levels */
#define CCACHE_DEBUG 0
#define CCACHE_VERBOSE 1
#define CCACHE_NOTICE 2
#define CCACHE_WARNING 3


#define CCACHE_LOG_LEVEL CCACHE_VERBOSE
#define CCACHE_LOG_FILE "ccache.log"

/* Anti-warning macro... */
#define CCACHE_NOTUSED(V) ((void) V)

#define CCACHE_NUM_WORKER_THREADS    4
#define CCACHE_NUM_BIO_THREADS 4


/* Image Directories and Options */

#define IMG_CROP_AVAILABLE 1
#define IMG_SRC_STRING "src"
#define IMG_ZOOM_STRING "zoom"

void ulog(int level, const char *fmt, ...);

#endif // CCACHE_CONFIG_H
