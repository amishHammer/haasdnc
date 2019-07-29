#include "config.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "debug.h"
#include "log.h"
#include "thread.h"
//#include "runtime_config.h"


//#include "init.h"

#define LOG_CONF_FILE "logging.conf"

static char level_strings[][9] = {
    "Debug",
    "Alert",
    "Critical",
    "Error",
    "Warning",
    "Notice",
    "Info",
    "Debug"
};


bool
configureLogging (void)
{
#if 0
    if (Config_getLogDest() == DNC_LOG_DEST_SYSLOG)
        openlog (NULL, LOG_PID, Config_getLogFacility());
#endif //_MSC_VER

    return true;
}

void
b_perror (const char *fmt)
{
    dnc_log (LOG_ERR, fmt, strerror (errno));
}

void
dnc_log (unsigned level, const char *fmt, ...)
{
    char *msg = NULL;
	va_list argp;
    DNC_LOG_DEST_t log_dest = DNC_LOG_DEST_ERR;//Config_getLogDest();
/*
    if (level > (unsigned) Config_getLogLevel())
    {
        return;
    }
*/  
    va_start (argp, fmt);
    
    if (log_dest != DNC_LOG_DEST_SYSLOG)
    {
        if (vasprintf (&msg, fmt, argp) == -1)
            return;
    }

    switch (log_dest)
    {
    case DNC_LOG_DEST_SYSLOG:
#ifdef _MSC_VER
		UNIMPLEMENTED();
#else
        vsyslog (level, fmt, argp);
#endif
        break;
    case DNC_LOG_DEST_FILE:
        break;
    case DNC_LOG_DEST_ERR:
    default:
        fprintf (stderr, "%s: %s\n", level_strings[level], msg);
        break;
    }
    va_end (argp);
    if (msg != NULL)
        free (msg);
}

int
dnc_get_log_level ()
{
    return LOG_DEBUG;//Config_getLogLevel();
}

DNC_LOG_DEST_t
dnc_get_log_dest ()
{
    return DNC_LOG_DEST_ERR;//Config_getLogDest();
}

void
dnc_debug_logging ()
{
//    Config_setLogLevel(LOG_DEBUG);
//    Config_setLogDest(RZB_LOG_DEST_ERR);
}
