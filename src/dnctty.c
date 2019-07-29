#include "dnctty.h"

int tty_setRaw(DNC_TYPE_t type, int fd, struct termios *ttystate_orig, speed_t speed) {
    if (type == DNC_TYPE_HAAS) {
        return tty_haas_setRaw(fd, ttystate_orig, speed);
    } else if (type == DNC_TYPE_MORI) {
        return tty_mori_setRaw(fd, ttystate_orig, speed);
    } else {
        return -1;
    }
}

