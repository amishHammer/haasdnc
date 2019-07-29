#include "config.h"

#include "debug.h"
#include "thread.h"
#include "list.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>


static struct List *sg_threadList;

#include <pthread.h>
#include <signal.h>

static pthread_attr_t g_attr;
static pthread_once_t g_once_control = PTHREAD_ONCE_INIT;

static void initThreading_pthreads(void);


static void handler(int sig);
static void Thread_Lock(void *);
static void Thread_Unlock(void *);

static void
initThreading (void)
{
    sg_threadList = List_Create(LIST_MODE_GENERIC, 
            Thread_Cmp, 
            Thread_KeyCmp, 
            NULL, NULL, 
            Thread_Lock, 
            Thread_Unlock);
    initThreading_pthreads();
}

static void *
Thread_MainWrapper (void *arg)
{
    struct Thread *l_pThread = (struct Thread *) arg;
    List_Lock(sg_threadList);
    Thread_Lock (l_pThread);

    l_pThread->bRunning = true;

    Thread_Unlock (l_pThread);
    List_Unlock(sg_threadList);

    l_pThread->mainFunction (l_pThread);

    List_Lock(sg_threadList);
    Thread_Lock (l_pThread);

    l_pThread->bRunning = false;

    Thread_Unlock (l_pThread);
    List_Unlock(sg_threadList);

    // Don't destroy the thread structure, it may still be referenced
    // We should probably ref count this struct
    Thread_Destroy(l_pThread);

    return NULL; // Implicit pthread_exit()
}

struct Thread *
Thread_Launch (void (*fpFunction) (struct Thread *), void *userData,
        char *name, void *context)
{
    struct Thread *thread;
    
    ASSERT (fpFunction != NULL);
    if (fpFunction == NULL)
        return NULL;

    pthread_once (&g_once_control, initThreading);

    // allocate memory for thread structure
    thread = (struct Thread *)calloc (1, sizeof (struct Thread));
    if (thread == NULL)
    {
        dnc_log (LOG_ERR,
                 "%s: Failed to launch thread in Thread_Launch due to out of memory for Thread", __func__);
        return NULL;
    }

    // initialize running indicator
    thread->bRunning = false;
    thread->pContext = context;
    thread->pUserData = userData;
    thread->sName = name;
    thread->bShutdown = false;
    // Init ref count, once for the list, once for the caller.
    thread->refs =2;

    thread->mainFunction = fpFunction;
    // initialize running mutex
    if ((thread->mMutex = Mutex_Create(MUTEX_MODE_RECURSIVE)) == NULL)
    {
        free(thread);
        return NULL;
    }

#ifdef _MSC_VER
    thread->hThread = CreateThread(NULL, 0, Thread_MainWrapper, thread, 0, &thread->iThread);
#else //_MSC_VER
    // start thread, check for error
    if (pthread_create
        (&thread->iThread, &g_attr, Thread_MainWrapper, thread) != 0)
    {
        free (thread);
        dnc_log (LOG_ERR,
                 "%s: Failed to launch thread in Thread_Launch due to pthread_create error (%i)", __func__,
                 errno);
        return NULL;
    }
#endif //_MSC_VER
    List_Push(sg_threadList, thread);

    // done
    return thread;
}

void
Thread_Destroy (struct Thread *thread)
{
    ASSERT(thread != NULL);
    if (thread == NULL)
        return;

    List_Lock(sg_threadList);
    Thread_Lock(thread);

    // Reference count should not drop below 1 as the list holds a ref.
    ASSERT(thread->refs >= 1);

    if (thread->refs > 1)
    {
        thread->refs--;
        Thread_Unlock(thread);
        List_Unlock(sg_threadList);
        return;
    }

    List_Remove(sg_threadList, thread);
    // destroy running mutex
    Thread_Unlock(thread);
    List_Unlock(sg_threadList);

    Mutex_Destroy (thread->mMutex);
    
    free (thread);
}

 bool
Thread_IsRunning (struct Thread *thread)
{
    bool l_bRunning;
    ASSERT (thread != NULL);
    if (thread == NULL)
        return false;

    Thread_Lock (thread);

    l_bRunning = thread->bRunning;

    Thread_Unlock (thread);

    return l_bRunning;
}

bool
Thread_IsStopped (struct Thread *thread)
{
    bool l_bShutdown;

    ASSERT (thread != NULL);
    if (thread == NULL)
        return false;

    Thread_Lock (thread);

    l_bShutdown = thread->bShutdown;

    Thread_Unlock (thread);

    return l_bShutdown;
}


void
Thread_Stop (struct Thread *thread)
{
    ASSERT (thread != NULL);
    if (thread == NULL)
        return;

    Thread_Lock (thread);

    thread->bShutdown = true;

    Thread_Unlock (thread);

}
void
Thread_Interrupt(struct Thread *thread)
{
    ASSERT (thread != NULL);
    if (thread == NULL)
        return;

    Thread_Lock(thread);
    Thread_Stop(thread);
#ifdef _MSC_VER
    //UNIMPLEMENTED();
#else //_MSC_VER
    pthread_kill(thread->iThread, SIGUSR1);
#endif //_MSC_VER
    //thread->interrupt(thread);
    Thread_Unlock(thread);

}

void 
Thread_Join(struct Thread *thread)
{
    ASSERT(thread != NULL);
    if (thread == NULL)
        return;

#ifdef _MSC_VER
    WaitForSingleObject(thread->hThread,INFINITE);
#else //_MSC_VER
    void *ret;
    int res =0;
    if ((res = pthread_join(thread->iThread, &ret)) != 0)
        dnc_log(LOG_ERR, "%s: Failed to join: %i", __func__, res);
#endif //_MSC_VER
}

void
Thread_StopAndJoin(struct Thread *thread)
{
   ASSERT (thread != NULL);
   if (thread == NULL)
       return;

   Thread_Stop(thread);
   Thread_Join(thread);
}

void
Thread_InterruptAndJoin(struct Thread *thread)
{
    ASSERT (thread != NULL);
    if (thread == NULL)
        return;

    Thread_Stop(thread);
    Thread_Interrupt(thread);
    Thread_Join(thread);
}


void * 
Thread_GetContext(struct Thread *thread)
{
    void *l_pOldContext;

    ASSERT(thread != NULL);
    if (thread == NULL)
        return NULL;

    Thread_Lock (thread);

    l_pOldContext = thread->pContext;
    Thread_Unlock (thread);

    return l_pOldContext;
}

void *
Thread_GetCurrentContext(void)
{
    struct Thread *thread;
    void *cont;
    thread = Thread_GetCurrent();
    if (thread == NULL)
    {
        dnc_log(LOG_ERR, "%s: Failed to get current thread", __func__);
        return NULL;
    }
    cont = Thread_GetContext(thread);
    Thread_Destroy(thread);
    return cont;
}

void *
Thread_ChangeContext(struct Thread *thread, void *context)
{
    void *l_pOldContext;

    ASSERT(thread != NULL);
    if (thread == NULL)
        return NULL;

    Thread_Lock (thread);

    l_pOldContext = thread->pContext;
    thread->pContext = context;

    Thread_Unlock (thread);

    return l_pOldContext;
}

dnc_thread_t
Thread_GetCurrentId(void)
{
#ifdef _MSC_VER
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

struct Thread *
Thread_GetCurrent(void)
{
    struct Thread *l_pRet = NULL;
    dnc_thread_t l_tCurrent = Thread_GetCurrentId();
    l_pRet = (struct Thread *)List_Find(sg_threadList, &l_tCurrent);
    if (l_pRet == NULL)
        return NULL;

    Thread_Lock(l_pRet);
    l_pRet->refs++;
    Thread_Unlock(l_pRet);
    return l_pRet;
}

void
Thread_Yield(void)
{
#ifdef _MSC_VER
    SwitchToThread();
#else
    pthread_yield();
#endif
}

uint32_t
Thread_getCount (void)
{
    uint32_t num;
    List_Lock(sg_threadList);
    num = sg_threadList->length;
    List_Unlock(sg_threadList);
    return num;
}

int 
Thread_KeyCmp(void *a, void *id)
{
    struct Thread * cA = (struct Thread *)a;
    dnc_thread_t cId = *(dnc_thread_t *)id;

    if (cId == cA->iThread)
        return 0;
    else
        return -1;
    return -1;
}

int 
Thread_Cmp(void *a, void *b)
{
    struct Thread * cA = (struct Thread *)a;
    struct Thread * cB = (struct Thread *)b;
    if (a==b)
        return 0;
    if (cA->iThread == cB->iThread)
        return 0;
    else 
        return -1;
    return -1;
}

static void 
Thread_Lock(void *a)
{
    struct Thread *t = (struct Thread *)a;
    Mutex_Lock(t->mMutex);
}

static void 
Thread_Unlock(void *a)
{
    struct Thread *t = (struct Thread *)a;
    Mutex_Unlock(t->mMutex);
}


#ifdef _MSC_VER

static void 
initThreading_win32(void)
{
    initialized=1;
}

#else //_MSC_VER

static void
initThreading_pthreads (void)
{
    struct sigaction act;
    sigset_t mask;

    pthread_attr_init (&g_attr);
    pthread_attr_setdetachstate (&g_attr, PTHREAD_CREATE_JOINABLE);

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    act.sa_handler = handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGUSR1, &act, NULL) < 0)
        dnc_log(LOG_ERR, "%s: Failed to install signal handler", __func__);
}

static void handler(int sig)
{
    dnc_log(LOG_DEBUG, "Thread Got Signal");
    return;
}

void
Thread_Interrupt_pthread(struct Thread *thread)
{
    pthread_kill(thread->iThread, SIGUSR1);
}

#endif //_MSC_VER
