#ifndef __SIGNAL_HANDLER_H
#define __SIGNAL_HANDLER_H
#include <signal.h>

#ifdef HAVE_BACKTRACE
static void *getMcontextEip(ucontext_t *uc) {
#if defined(__FreeBSD__)
    return (void*) uc->uc_mcontext.mc_eip;
#elif defined(__dietlibc__)
    return (void*) uc->uc_mcontext.eip;
#elif defined(__APPLE__) && !defined(MAC_OS_X_VERSION_10_6)
  #if __x86_64__
    return (void*) uc->uc_mcontext->__ss.__rip;
  #elif __i386__
    return (void*) uc->uc_mcontext->__ss.__eip;
  #else
    return (void*) uc->uc_mcontext->__ss.__srr0;
  #endif
#elif defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6)
  #if defined(_STRUCT_X86_THREAD_STATE64) && !defined(__i386__)
    return (void*) uc->uc_mcontext->__ss.__rip;
  #else
    return (void*) uc->uc_mcontext->__ss.__eip;
  #endif
#elif defined(__i386__)
    return (void*) uc->uc_mcontext.gregs[14]; /* Linux 32 */
#elif defined(__X86_64__) || defined(__x86_64__)
    return (void*) uc->uc_mcontext.gregs[16]; /* Linux 64 */
#elif defined(__ia64__) /* Linux IA64 */
    return (void*) uc->uc_mcontext.sc_ip;
#else
    return NULL;
#endif
}

void bugReportStart(void) {
    if (server.bug_report_start == 0) {
        redisLog(CCACHE_WARNING,
            "=== REDIS BUG REPORT START: Cut & paste starting from here ===");
        server.bug_report_start = 1;
    }
}

static void sigsegvHandler(int sig, siginfo_t *info, void *secret) {
    void *trace[100];
    char **messages = NULL;
    int i, trace_size = 0;
    ucontext_t *uc = (ucontext_t*) secret;
    sds infostring, clients;
    struct sigaction act;
    CCACHE_NOTUSED(info);

    bugReportStart();
    redisLog(CCACHE_WARNING,
        "    Redis %s crashed by signal: %d", CCACHE_VERSION, sig);
    redisLog(CCACHE_WARNING,
        "    Failed assertion: %s (%s:%d)", server.assert_failed,
                        server.assert_file, server.assert_line);

    /* Generate the stack trace */
    trace_size = backtrace(trace, 100);

    /* overwrite sigaction with caller's address */
    if (getMcontextEip(uc) != NULL) {
        trace[1] = getMcontextEip(uc);
    }
    messages = backtrace_symbols(trace, trace_size);
    redisLog(CCACHE_WARNING, "--- STACK TRACE");
    for (i=1; i<trace_size; ++i)
        redisLog(CCACHE_WARNING,"%s", messages[i]);

    /* Log INFO and CLIENT LIST */
    redisLog(CCACHE_WARNING, "--- INFO OUTPUT");
    infostring = genRedisInfoString();
    redisLog(CCACHE_WARNING, infostring);
    redisLog(CCACHE_WARNING, "--- CLIENT LIST OUTPUT");
    clients = getAllClientsInfoString();
    redisLog(CCACHE_WARNING, clients);
    /* Don't sdsfree() strings to avoid a crash. Memory may be corrupted. */

    /* Log CURRENT CLIENT info */
    if (server.current_client) {
        redisClient *cc = server.current_client;
        sds client;
        int j;

        redisLog(CCACHE_WARNING, "--- CURRENT CLIENT INFO");
        client = getClientInfoString(cc);
        redisLog(CCACHE_WARNING,"client: %s", client);
        /* Missing sdsfree(client) to avoid crash if memory is corrupted. */
        for (j = 0; j < cc->argc; j++) {
            robj *decoded;

            decoded = getDecodedObject(cc->argv[j]);
            redisLog(CCACHE_WARNING,"argv[%d]: '%s'", j, (char*)decoded->ptr);
            decrRefCount(decoded);
        }
        /* Check if the first argument, usually a key, is found inside the
         * selected DB, and if so print info about the associated object. */
        if (cc->argc >= 1) {
            robj *val, *key;
            dictEntry *de;

            key = getDecodedObject(cc->argv[1]);
            de = dictFind(cc->db->dict, key->ptr);
            if (de) {
                val = dictGetEntryVal(de);
                redisLog(CCACHE_WARNING,"key '%s' found in DB containing the following object:", key->ptr);
                redisLogObjectDebugInfo(val);
            }
            decrRefCount(key);
        }
    }

    redisLog(CCACHE_WARNING,
"=== REDIS BUG REPORT END. Make sure to include from START to END. ===\n\n"
"       Please report the crash opening an issue on github:\n\n"
"           http://github.com/antirez/redis/issues\n\n"
"  Suspect RAM error? Use redis-server --test-memory to veryfy it.\n\n"
);
    /* free(messages); Don't call free() with possibly corrupted memory. */
    if (server.daemonize) unlink(server.pidfile);

    /* Make sure we exit with the right signal at the end. So for instance
     * the core will be dumped if enabled. */
    sigemptyset (&act.sa_mask);
    /* When the SA_SIGINFO flag is set in sa_flags then sa_sigaction
     * is used. Otherwise, sa_handler is used */
    act.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND;
    act.sa_handler = SIG_DFL;
    sigaction (sig, &act, NULL);
    kill(getpid(),sig);
}
#endif /* HAVE_BACKTRACE */

static void sigtermHandler(int sig) {
    printf("Received SIGTERM, scheduling shutdown...");
    //redisLog(CCACHE_WARNING,"Received SIGTERM, scheduling shutdown...");
    //server.shutdown_asap = 1;
}

void setupSignalHandlers(void) {
    /* Ignore Signals */
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    struct sigaction act;

    /* When the SA_SIGINFO flag is set in sa_flags then sa_sigaction is used.
     * Otherwise, sa_handler is used. */
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND;
    act.sa_handler = sigtermHandler;
    sigaction(SIGTERM, &act, NULL);

#ifdef HAVE_BACKTRACE
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND | SA_SIGINFO;
    act.sa_sigaction = sigsegvHandler;
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGBUS, &act, NULL);
    sigaction(SIGFPE, &act, NULL);
    sigaction(SIGILL, &act, NULL);
#endif
    return;
}
#endif
