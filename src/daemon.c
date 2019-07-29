#include "config.h"
#include "debug.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "daemon.h"
#include "log.h"

static bool dnc_daemonize_posix(void (*signal_handler)(int), const char *pidFile);

bool dnc_daemonize (void (*signal_handler) (int), const char *pidFile)
{
	return dnc_daemonize_posix(signal_handler, pidFile);
}

	

static const char *sg_pidFile = NULL;

static void 
unlinkPidFile(void)
{
    if (sg_pidFile != NULL)
        unlink(sg_pidFile);
}
bool
dnc_daemonize_posix (void (*signal_handler) (int), const char *pidFile)
{
    pid_t pid, sid;

    if (dnc_get_log_dest () == DNC_LOG_DEST_ERR)
    {
        dnc_log (LOG_EMERG, "%s: Can't daemonize when using stderr for logging", __func__);
        return false;
    }

    if (signal_handler != NULL)
    {
        dnc_log (LOG_DEBUG, "%s: Installing new signal handler", __func__);
        signal (SIGHUP, signal_handler);
        signal (SIGTERM, signal_handler);
        signal (SIGINT, signal_handler);
        signal (SIGQUIT, signal_handler);
    }

    pid = fork ();
    if (pid < 0)
    {
        dnc_log (LOG_EMERG, "%s: Failed to daemonize", __func__);
        return false;
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0)
    {
        exit (EXIT_SUCCESS);
    }

    /* Create a new SID for the child process */
    sid = setsid ();
    if (sid < 0)
    {
        dnc_log (LOG_EMERG, "%s: Failed to become session leader", __func__);
        return false;
    }

    /* Close out the standard file descriptors */
    close (STDIN_FILENO);
    close (STDOUT_FILENO);
    close (STDERR_FILENO);

    if (pidFile != NULL)
    {
    /* save the PID */
        pid_t mainpid = getpid ();
        FILE *fd;
        mode_t old_umask = umask (0002);
        if ((fd = fopen (pidFile, "w")) == NULL)
        {
            dnc_log (LOG_ERR, "Can't save PID in file %s", pidFile);
        }
        else
        {
            if (fprintf (fd, "%u", (unsigned int) mainpid) < 0)
            {
                dnc_log (LOG_ERR, "Can't save PID in file %s", pidFile);
            }
            fclose (fd);
        }
        umask (old_umask);
        sg_pidFile = pidFile;
        atexit(unlinkPidFile);
    }

    return true;
}


