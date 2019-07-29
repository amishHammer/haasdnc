#ifndef DNC_DNC_THREAD_H
#define DNC_DNC_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <termios.h>

#include "thread.h"
#include "dnctty.h"
#define CLINE_SIZE 4096

struct dnc_context {
    char cline[CLINE_SIZE];
    int cpos;
    char *port;
    speed_t speed;
    DNC_TYPE_t dnc_type;
    char * gcode_file;
}; 

void dnc_thread(struct Thread *thread);

#ifdef __cplusplus
}
#endif
#endif // DNC_DNC_THREAD_H
