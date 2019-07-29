#ifndef	DNC_THREAD_H
#define	DNC_THREAD_H

#include "types.h"
#include <pthread.h>

typedef pthread_t dnc_thread_t;

struct Thread
{
    dnc_thread_t iThread;                   ///< pthread Thread info.
    struct Mutex * mMutex;                  ///< mutex protecting this struct
    bool bRunning;                          ///< true if executing, false if not:  must be managed explicitly by thread function
    void *pUserData;                        ///< Additional info for the thread
    char *sName;                            ///< The thread name
    void (*mainFunction) (struct Thread *); ///< Thread Main Function
    bool bShutdown;                         ///< Shutdown Flag
    int refs;                               ///< Reference count
    void * pContext;
    //void (*interrupt)(struct Thread *);       ///< Cancellation handler for a blocking function.
};

/** Create a new thread
 * @param *function The function the thread will execute
 * @param *userData The thread user data
 * @param *name The name of the thread
 * @param *context The initial context of the thread
 * @return Null on error a new Thread on success.
 */
struct Thread *Thread_Launch (void (*function) (struct Thread *),
                                     void *userData, char *name,
                                     void *context);

/** Change the registered context of a running thread.
 * @param thread the thread to change
 * @param context the new context
 * @return The old context
 */
void * Thread_ChangeContext(struct Thread *thread,
                                    void *context);

/** Get the registered context of a running thread.
 * @param thread the thread to change
 * @return The current context for the thread.
 */
void * Thread_GetContext(struct Thread *p_pThread);

/** Get the running context for the current thread.
 * @return The current context.
 */
void * Thread_GetCurrentContext(void);

/** Destroy a threads data
 * @param *thread The thread to destroy
 */
void Thread_Destroy (struct Thread *thread);

/** Checks whether a thread is running or not
 * @param *thread The thread the test
 * @return true if running, false if not
 */
bool Thread_IsRunning (struct Thread *thread);

/** Check if a thread has finished.
 * @param thread The thread to test.
 * @return true if the thread has exited, false if its still running.
 */
bool Thread_IsStopped (struct Thread *thread);

/** Suspend execution of the calling thread until the target thread therminates.
 * @param thread The target thread.
 */
void Thread_Join(struct Thread *thread);
/** Interrupt the target thread.
 * @param thread The target thread.
 */
void Thread_Interrupt(struct Thread *thread);
/** Stop the target thread.
 * @param thread The target thread.
 */
void Thread_Stop (struct Thread *thread);
void Thread_StopAndJoin (struct Thread *thread);
void Thread_InterruptAndJoin (struct Thread *thread);
/** Cause the current thread to yield execution to other runnable threads.
 */
void Thread_Yield(void);

/** Get the number of currently running threads.
 * @return the number of currently running threads.
 */
uint32_t Thread_getCount (void);

/** Get the current thread.
 */
struct Thread *Thread_GetCurrent(void);
/** Get the current thread ID.
 */
dnc_thread_t Thread_GetCurrentId(void);

int Thread_KeyCmp(void *a, void *id);
int Thread_Cmp(void *a, void *b);

#endif // DNC_THREAD_H
