// dnc.c
//
// Copy files between CNC control and network drive
//
// usage:
//      haas /dev/ttyUSB0 /path/haascode.txt /path/haas.log
//      mori /dev/ttyUSB1 /path/moricode.txt /path/mori.log
//
//      /dev/ttyUSB0:        serial port device
//      /path/haascode.txt:  name of CNC g-code file to download on request from
//                           Haas control.
//      /path/haas.log:      name of log file for error messages
//
// notes:
//      - Assumes that input or output files reside at a specific location.
//      - haas is run as a "daemon" in background from /etc/init.d .
//
// compiling:
//      - must be compiled with a file containing setRaw with parameters
//        for specific CNC machine.  Here are our two examples:
//
//        gcc dnc.c haastty.c -o haas
//        gcc dnc.c moritty.c -o mori
//
// Ralph Stirling, 2009-01-16 modified 2012-09-20
// stirra@wallawalla.edu

#include "thread.h"
#include "log.h"
#include "dnctty.h"
#include "dnc_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

#define DC1 0x11
#define DC2 0x12
#define DC3 0x13
#define DC4 0x14





void
dnc_thread(struct Thread *thread)
{
    // TODO call get current context
    struct dnc_context * ctx = (struct dnc_context *)thread->pContext;
    
    fd_set  rfds;
    int     rval;
    int     rfd;
    int     wfd;
    int     ofd;
    int     gfd;
    char    ch;
    char    b;
    int     i;
    int     sending = 0;
    int     pause = 0;
    int     receiving = 0;
    char    *cspeed;
    speed_t speed;
    char    line[80];
    int     linelen;
    int     len;
    struct termios  istate, ostate;
    struct timeval  timeout, *to;
    long    bytesread = 0;
    ctx->cpos = 0;


    timeout.tv_sec = 0;
    timeout.tv_usec = 0L;


    dnc_log(LOG_INFO, "DNC thread started");

    wfd = open(ctx->port, O_WRONLY);
    if (wfd < 0)
    {
        dnc_log(LOG_ERR, "open %s for write failed", ctx->port);
        exit(1);
    }

    rfd = open(ctx->port, O_RDONLY | O_NDELAY);
    if (rfd < 0)
    {
        dnc_log(LOG_ERR, "open %s for read failed", ctx->port);
        exit(1);
    }

    tty_setRaw(ctx->dnc_type, rfd, &istate, ctx->speed);
    tty_setRaw(ctx->dnc_type, wfd, &ostate, ctx->speed);

    sending = 0;
    pause = 0;
    to = NULL;

    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(rfd, &rfds);

        rval = select(rfd+1, &rfds, NULL, NULL, to);

        if (rval < 0)
        {
            dnc_log(LOG_ERR, "select failed");
            exit(1);
        }

        
        if (rval > 0) {
            read(rfd, &ch, 1);
            switch (ch)
            {
                case DC1:
                    dnc_log(LOG_DEBUG, "Received DC1");
                    if (!sending)
                    {

                        sprintf(line, "%s", ctx->gcode_file);
                        dnc_log(LOG_DEBUG, "Sending: gcode_file=%s", line);
                        gfd = open(line, O_RDONLY);

                        if (gfd < 0)
                        {
                            write(wfd, "%%", 2);
                            dnc_log(LOG_ERR, "open %s failed", line);
                            break;
                        }
                        dnc_log(LOG_DEBUG, "sending");
                        sending = 1;
                        to = &timeout;
                    }
                    if (pause) {
                        dnc_log(LOG_DEBUG, "resuming send");
                    }
                    pause = 0;
                    break;
               case DC2:
                    dnc_log(LOG_DEBUG, "Received DC2");
                    if (!receiving)
                    {

                        sprintf(line, "%s.out", ctx->gcode_file);
                        ofd = open(line, O_WRONLY | O_CREAT, 0644);
                        if (ofd < 0)
                        {
                            dnc_log(LOG_ERR,"open %s failed", line);
                            break;
                        }
                        dnc_log(LOG_DEBUG, "receiving");
                        receiving = 1;
                        to = NULL;
                    }
                    break;
               case DC3:
                    dnc_log(LOG_DEBUG, "Received DC3");
                    if (sending)
                    {
                        if (!pause) {
                            dnc_log(LOG_DEBUG, "pause");
                        }
                        pause = 1;
                    }
                    break;
               case DC4:
                    dnc_log(LOG_DEBUG, "Received DC4");
                    if (receiving)
                    {
                        dnc_log(LOG_DEBUG, "done receiving");
                        close(ofd);
                        receiving = 0;
                    }
               default:
                    if (!receiving)
                        dnc_log(LOG_ERR, "unexpected: %c (0x%0x) sending=%d pause=%d",
                                (unsigned char)ch, (unsigned char)ch, sending,
                                pause);
                    else
                        write(ofd, &ch, 1);
                    break;
            }
        }
        if (sending && !pause)
        {
            int grval = read(gfd, &b, 1);

            if (grval == 1)
            {
                if (b != '\r') {
                    if (b == '\n') {
                        ctx->cline[ctx->cpos] = '\0';
                        ctx->cpos = 0;
                        dnc_log(LOG_DEBUG, "send code line: %s", ctx->cline);
                    } else {
                        ctx->cline[ctx->cpos] = b;
                        ctx->cpos++;
                        if (ctx->cpos >= CLINE_SIZE) {
                            ctx->cline[CLINE_SIZE-1] = '\0';
                            dnc_log(LOG_DEBUG, "send (partial) code line: %s", ctx->cline);
                            ctx->cline[0] = b;
                            ctx->cpos = 1;
                        }
                    }
                    write(wfd, &b, 1);
                }
                bytesread++;
            }
            else
            {
                dnc_log(LOG_DEBUG, "done sending %ld characters", bytesread);
                bytesread = 0;
                write(wfd, "%", 1);
                close(gfd);
                sending = 0;
            }
        }
    } 
}


