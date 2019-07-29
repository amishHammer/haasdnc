#ifndef DNC_DNCTTY_H
#define DNC_DNCTTY_H
#include <termios.h>
#include <stdio.h>

typedef enum
{
    DNC_TYPE_HAAS,
    DNC_TYPE_MORI,
} DNC_TYPE_t;



int tty_setRaw(DNC_TYPE_t type, int fd, struct termios *ttystate_orig, speed_t speed);

int tty_mori_setRaw(int fd, struct termios *ttystate_orig, speed_t speed);
int tty_haas_setRaw(int fd, struct termios *ttystate_orig, speed_t speed);



#endif // DNC_DNCTTY_H
