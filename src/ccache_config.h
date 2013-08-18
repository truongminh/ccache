#ifndef CCACHE_CONFIG_H
#define CCACHE_CONFIG_H

#define VERSION "1.0"
#define YEAR "2013"
#define COPYRIGHT_HOLDER "Nguyen Truong Minh"
#define AUTHORS "Nguyen Truong Minh (nguyentrminh@gmail.com)"
#define DESCRIPTION "An image caching server with image resizing, crop and quality on the fly "
#define PROGRAM_NAME "CCACHE"
#define LICENSE "GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n" \
                " This is free software: you are free to change and redistribute it.\n" \
                " There is NO WARRANTY, to the extent permitted by law.\n"
#define DISPLAY_DESCRIPTION printf("%s %s\n %s\nCopyright (C) %s %s\nLicense %s\nWritten by %s.\n" \
                            ,PROGRAM_NAME, VERSION, DESCRIPTION, COPYRIGHT_HOLDER, YEAR, LICENSE, AUTHORS);

/* Error codes */
#define CCACHE_OK                0
#define CCACHE_ERR               -1

/* Static server configuration */
#define CCACHE_SERVERPORT        6379    /* TCP port */
#define CCACHE_MAXIDLETIME       0       /* default client timeout: infinite */
#define CCACHE_IOBUF_LEN         (1024*4)

#define CCACHE_ENCODING_RAW 0     /* Raw representation */
#define CCACHE_ENCODING_INT 1     /* Encoded as integer */

/* Log levels */
#define CCACHE_DEBUG 0
#define DEBUG CCACHE_DEBUG
#define CCACHE_VERBOSE 1
#define CCACHE_NOTICE 2
#define CCACHE_WARNING 3


#define CCACHE_LOG_LEVEL CCACHE_WARNING
#define CCACHE_LOG_FILE "ccache.log"

/* Anti-warning macro... */
#define CCACHE_NOTUSED(V) ((void) V)

/* Caching Options */
/* When the number of entries exceeds the preserved entries,
 * the cache will be re-index, causing huge-performance penalty.
 * Another important factor is the efficiency of hash.
 * As we take only n-least significant bits,
 * n should be big enough to avoid as much as hash collision.
 *
 * Therefore, it is advisable to provide an appropriate preserved entries,
 * which is 16 times of the typical used number of cache entries.
 *
 * An image caching server use 4GB RAM for cache, each entries consume 16KB.
 * The number of used cache entries is (2^32)/(2^14) = (2^18)
 * The number of preverved entries should be 2^22
 */

#define PRESERVED_CACHE_ENTRIES (2<<22)

#define MASTER_STATUS_REFRESH_PERIOD 5 /* 10 seconds */
#define MASTER_MAX_AVAIL_MEM (50L<<20) /* 100MB */
#define ZOOM_MAX_ON_DISK (10L<<30) /* 10GB */



#define ONE_MEGABYTE (1<<20)
#define BYTES_TO_MEGABYTES(d) ((double)d/ONE_MEGABYTE)

/* Threads serving clients ordered by the accepting thread */
#define CCACHE_NUM_WORKER_THREADS    4
/* Threads doing background jobs ordered by the master cache */
#define CCACHE_NUM_BIO_THREADS 4


/* Asynchronous I/O Options */
#define AE_MAX_CLIENT_PER_WORKER 16000 /* Number of client pending at acceptor */
#define AE_MAX_EPOLL_EVENTS 1024
#define AE_FD_SET_SIZE (CCACHE_NUM_BIO_THREADS*AE_MAX_CLIENT_PER_WORKER)    /* Max number of fd supported */
#define AE_MAX_CLIENT_IDLE_TIME 5 /* seconds */


#define SERVICE_STATIC_FILE "/static"
#define SERVICE_ZOOM "/zoom"
#define CCACHE_MAX_URI_LEN 1024

/* Image Service Options */
#define IMG_CROP_AVAILABLE 1
#define IMG_MAX_WIDTH 1000
#define IMG_MAX_HEIGHT 1000

void ulog(int level, const char *fmt, ...);

#endif // CCACHE_CONFIG_H
