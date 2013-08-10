/* main.c
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

#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <stdlib.h>
#include "ccache_config.h"

#include "organizer/bio.h"
#include "net/ae.h"
#include "net/anet.h"
#include "net/client.h"
#include "cache/mcache.h"
#include "signal_handler.h"
#include "http/request_handler.h"

char *program_name;

struct aeEventLoop server; /* server global state */
void usage (int status);

/* These enum values cannot possibly conflict with the option values
   ordinarily used by commands, including CHAR_MAX + 1, etc.  Avoid
   CHAR_MIN - 1, as it may equal -1, the getopt end-of-options value.  */

enum
{
  GETOPT_HELP_CHAR = (CHAR_MIN - 2),
  GETOPT_VERSION_CHAR = (CHAR_MIN - 3)
};

#define GETOPT_HELP_OPTION_DECL \
  "help", no_argument, NULL, GETOPT_HELP_CHAR
#define GETOPT_VERSION_OPTION_DECL \
  "version", no_argument, NULL, GETOPT_VERSION_CHAR
#define GETOPT_SELINUX_CONTEXT_OPTION_DECL \
  "context", required_argument, NULL, 'Z'

#define HELP_OPTION_DESCRIPTION \
  _("      --help     display this help and exit\n")
#define VERSION_OPTION_DESCRIPTION \
  _("      --version  output version information and exit\n")


static struct option const longopts[] =
{
  {GETOPT_SELINUX_CONTEXT_OPTION_DECL},
  {"port", required_argument, NULL, 'p'},
  {"src", required_argument, NULL, 's'},
  {"tmp", required_argument, NULL, 't'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* We received a SIGTERM,  shuttingdown here in a safe way, as it is
 * not ok doing so inside the signal handler. */
int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientDat){
    (void)id;
    (void)clientDat;
    /*if (server.shutdown_asap) {
        if (prepareForShutdown() == CCACHE_OK) exit(0);
        redisLog(CCACHE_WARNING,"SIGTERM received but errors trying to shut down the server, check the logs for more information");
    }
    */    
    eventLoop->loop++;
#ifdef AE_MAX_CLIENT_IDLE_TIME
    if (eventLoop->maxidletime && AE_LOOP_ELLAPSED(eventLoop->loop,1))
        closeTimedoutClients(eventLoop);
#endif
    if(AE_LOOP_ELLAPSED(eventLoop->loop,2)) {
        if(shouldIFreeSomeData()) {
            cacheDeleteStaleEntries(eventLoop->cache,1);
        }
    }        
    return AE_LOOP_INTERVAL;
}

void *aeMainThread(void *eventloop)
{
   aeEventLoop *el = eventloop;
   aeMain(el);
   aeDeleteEventLoop(el);
   free(el);
   pthread_exit(NULL);
}

void initMaster(aeEventLoop *el, int numslave)
{
    el->myid = 0;
    el->nextSlaveID = 0;
    el->slaves = malloc(sizeof(aeEventLoop)*numslave);
    el->numslave = numslave;
}

struct ccache_options {
    int port;
    char *srcd;
    char *tmpd;
};

int main(int argc, char* argv[])
{
     char *search = strchr(argv[0],'/');
     program_name = (search == NULL)? argv[0]: (search+1);
     setupSignalHandlers();
     char neterr[ANET_ERR_LEN];
     struct ccache_options options;
     options.srcd = ".";
     options.tmpd = ".";
     int optc;
     while ((optc = getopt_long (argc, argv, "ps:tZ:", longopts, NULL)) != -1)
       {
         switch (optc)
           {
           case 'p':
             if(!optarg || (options.port = atoi(optarg)) < 0) {
                 printf("ERROR: Invalid port [%s].\nThe port must be a number greater than 0.\n",optarg);
                 usage(EXIT_FAILURE);
             }
             break;
           case 's':
             options.srcd = optarg;
             break;
           case 't': /* --verbose  */
             options.tmpd = optarg;
             break;
           case GETOPT_HELP_CHAR:
             usage (EXIT_SUCCESS);
             break;
           case GETOPT_VERSION_CHAR:
             DISPLAY_DESCRIPTION;
             exit (EXIT_SUCCESS);
           default:
             usage (EXIT_FAILURE);
           }
       }



     bioSetDirs(options.srcd,options.tmpd);

     requestHandleInitializeGlobalCache();
     cacheMasterInit();
     aeEventLoop *el = aeCreateEventLoop();
     initMaster(el,CCACHE_NUM_WORKER_THREADS);
     pthread_t threads[CCACHE_NUM_WORKER_THREADS];
     int rc;
     long t;
     for(t=0; t<CCACHE_NUM_WORKER_THREADS; t++){
       aeEventLoop *slave = aeCreateEventLoop();
       slave->myid = t+1;
       aeCreateTimeEvent(slave, 2000, serverCron, NULL, NULL);
       slave->cache = cacheAddSlave(slave);
       el->slaves[t] = slave;       
       rc = pthread_create(&threads[t], NULL, aeMainThread, slave);
       if (rc){
          printf("ERROR: return code from pthread_create() is %d\n", rc);
          exit(-1);
       }
    }


    char *bindaddr = "0.0.0.0";
    int ipfd = anetTcpServer(neterr,options.port,bindaddr);
    if (ipfd == ANET_ERR) {
        printf("ERROR: Opening port %d: %s",options.port, neterr);
        exit(1);
    }
    //aeCreateTimeEvent(el, 2000, serverCron, NULL, NULL);
    if (ipfd > 0 && aeCreateFileEvent(el,ipfd,AE_READABLE,acceptTcpHandler,NULL) == AE_ERR)
        printf("creating file event\n");
    if (ipfd > 0)
        printf("The server is now ready to accept connections on port %d\n",options.port);
    aeMain(el);
    aeDeleteEventLoop(el);
    free(el);
    return 0;
}


void usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, ("Try `%s --help' for more information.\n"),program_name);
  else
    {
      printf (("Usage: %s [PORT]... [SRC_DIR]... [TMP_DIR]\n" \
              "With no SRC_DIR, the current directory is used as input dir.\n"\
              "With no TMP_DIR, the /tmp directory is used as tmp dir.\n"\
              "\n"),program_name);
    }

  exit (status);
}
