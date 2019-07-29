#include "dnctty.h"
#include <termios.h>
#include <stdio.h>

int tty_mori_setRaw(int fd, struct termios *ttystate_orig, speed_t speed) 
{
/* set tty into raw mode */

    struct termios    tty_state;

    if (tcgetattr(fd, &tty_state) < 0) return 0;
    /* save original settings */
    *ttystate_orig = tty_state;
    /* set raw mode */
    tty_state.c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO);
    tty_state.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT);
    tty_state.c_oflag &= ~OPOST;
    tty_state.c_cflag &= ~(CSIZE | PARODD);
    tty_state.c_cflag |= CS7 | PARENB | CSTOPB;
    tty_state.c_cc[VMIN]  = 1;
    tty_state.c_cc[VTIME] = 0;
    cfsetispeed(&tty_state, speed);
    cfsetospeed(&tty_state, speed);
    if (tcsetattr(fd, TCSAFLUSH, &tty_state) < 0) return 0;
    return 1;
}
