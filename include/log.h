/** @file log.h
 * The Logging API.
 * 
 * Log levels avaliable are the same as the standard syslog log levels and
 * definitions are imported from syslog.
 *
 * Levels are:
 *      LOG_EMERGE
 *      LOG_ALERT
 *      LOG_CRIT
 *      LOG_ERR
 *      LOG_WARNING
 *      LOG_NOTICE
 *      LOG_INFO
 *      LOG_DEBUG
 *
 */
#ifndef DNC_LOG_H
#define DNC_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <syslog.h>

#include "types.h"


/** Log Destinations
 */
typedef enum
{
    DNC_LOG_DEST_FILE,          ///< Write to a file
    DNC_LOG_DEST_SYSLOG,        ///< Write to syslog
    DNC_LOG_DEST_ERR,           ///< Write to stderr
} DNC_LOG_DEST_t;

/** Log a message to the system message log
 * @param level The message level as defined in syslog.h
 * @param fmt The format string for the message
 * @param ... The data to be formatted.
 */
void dnc_log (unsigned level, const char *fmt, ...);

/** Log a standard error.
 * @param message the log message associated with the error.
 */
void dnc_perror (const char *message);

/** Get the currently configured log level
 * @return One of the log levels defined in syslog.h
 */
int dnc_get_log_level ();

/** Get the currently configured log destination
 * @return The current log distination.
 */
DNC_LOG_DEST_t dnc_get_log_dest ();

/** Set logging to debug mode.
 */
void dnc_debug_logging ();
#ifdef __cplusplus
}
#endif
#endif // DNC_LOG_H
