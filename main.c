#include <stdio.h>
#include "ae.h"
#include "anet.h"
#include <stdlib.h>
#include "client.h"
#include "signal_handler.h"
#include <pthread.h>


#define NUM_THREADS     1

struct aeEventLoop server; /* server global state */


/* We received a SIGTERM,  shuttingdown here in a safe way, as it is
 * not ok doing so inside the signal handler. */
int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientDat){
    /*if (server.shutdown_asap) {
        if (prepareForShutdown() == CCACHE_OK) exit(0);
        redisLog(CCACHE_WARNING,"SIGTERM received but errors trying to shut down the server, check the logs for more information");
    }
    */    
    eventLoop->loop++;
#ifdef AE_MAX_IDLE_TIME
    if ((eventLoop->maxidletime && !(eventLoop->loop % 10)))
        closeTimedoutClients(eventLoop);
#endif
    return 100;
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

int main(void)
{    
     setupSignalHandlers();
     aeEventLoop *el = aeCreateEventLoop();
     initMaster(el,NUM_THREADS);
     pthread_t threads[NUM_THREADS];
     int rc;
     long t;
     printf("Main pthread_self %x\n",pthread_self());
     for(t=0; t<NUM_THREADS; t++){
       printf("In main: creating thread %ld\n", t);
       aeEventLoop *slave = aeCreateEventLoop();
       slave->myid = t+1;
       aeCreateTimeEvent(slave, 2000, serverCron, NULL, NULL);
       el->slaves[t] = slave;
       rc = pthread_create(&threads[t], NULL, aeMainThread, slave);
       if (rc){
          printf("ERROR; return code from pthread_create() is %d\n", rc);
          exit(-1);
       }
    }

    char neterr[ANET_ERR_LEN];
    int port = 8888;
    char *bindaddr = "0.0.0.0";
    int ipfd = anetTcpServer(neterr,port,bindaddr);
    if (ipfd == ANET_ERR) {
        printf("Opening port %d: %s",port, neterr);
        exit(1);
    }
    //aeCreateTimeEvent(el, 1, serverCron, NULL, NULL);
    if (ipfd > 0 && aeCreateFileEvent(el,ipfd,AE_READABLE,acceptTcpHandler,NULL) == AE_ERR)
        printf("creating file event\n");
    if (ipfd > 0)
        printf("The server is now ready to accept connections on port %d",port);    
    aeMain(el);
    aeDeleteEventLoop(el);
    free(el);
    return 0;
}

