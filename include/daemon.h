#ifndef __DNC_DAEMON_H__
#define __DNC_DAEMON_H__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Daemonize the application.
 * @return true on successful daemonisation false on error.
 */
bool dnc_daemonize (void (*sighandler) (int), const char *pidFile);
#ifdef __cplusplus
}
#endif
#endif /* __RAZORBACK_DAEMON_H__ */
