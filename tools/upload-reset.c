/* Copyright (C) 2015 Roger Clark <www.rogerclark.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Utility to send the reset sequence on RTS and DTR and chars
 * which resets the libmaple and causes the bootloader to be run
 *
 * Terminal control code by Heiko Noordhof (see copyright below)
 */

/* Copyright (C) 2003 Heiko Noordhof <heikyAusers.sf.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

enum { EXIT_USAGE = 64, EXIT_SERIAL = 1 };

static int set_rts(unsigned short level);
static int set_dtr(unsigned short level);

static int fd = -1;
static struct termios oldterminfo;

static void closeserial(void)
{
    if (fd < 0) {
        return;
    }
    (void)tcsetattr(fd, TCSANOW, &oldterminfo);
    (void)close(fd);
    fd = -1;
}

static int openserial(const char *devicename)
{
    struct termios attr;

    fd = open(devicename, O_RDWR);
    if (fd < 0) {
        return 0;
    }
    if (atexit(closeserial) != 0) {
        perror("atexit");
        closeserial();
        return 0;
    }

    if (tcgetattr(fd, &oldterminfo) < 0) {
        perror("tcgetattr");
        return 0;
    }
    attr = oldterminfo;
    attr.c_cflag |= CRTSCTS | CLOCAL;
    attr.c_oflag = 0;
    if (tcflush(fd, TCIOFLUSH) < 0) {
        perror("tcflush");
        return 0;
    }
    if (tcsetattr(fd, TCSANOW, &attr) < 0) {
        perror("tcsetattr");
        return 0;
    }

    return set_rts(0) && set_dtr(0);
}

/* level=0: LOW, level=1: HIGH */
static int set_rts(unsigned short level)
{
    int status;

    if (ioctl(fd, TIOCMGET, &status) < 0) {
        perror("set_rts: TIOCMGET");
        return 0;
    }
    if (level) {
        status |= TIOCM_RTS;
    } else {
        status &= ~TIOCM_RTS;
    }
    if (ioctl(fd, TIOCMSET, &status) < 0) {
        perror("set_rts: TIOCMSET");
        return 0;
    }
    return 1;
}

static int set_dtr(unsigned short level)
{
    int status;

    if (ioctl(fd, TIOCMGET, &status) < 0) {
        perror("set_dtr: TIOCMGET");
        return 0;
    }
    if (level) {
        status |= TIOCM_DTR;
    } else {
        status &= ~TIOCM_DTR;
    }
    if (ioctl(fd, TIOCMSET, &status) < 0) {
        perror("set_dtr: TIOCMSET");
        return 0;
    }
    return 1;
}

static void sleep_us(useconds_t usec)
{
    (void)usleep(usec);
}

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3) {
        fprintf(stderr,
                "Usage: %s <serial_device> [delay_ms]\n",
                argc > 0 ? argv[0] : "upload-reset");
        return EXIT_USAGE;
    }

    if (!openserial(argv[1])) {
        fprintf(stderr, "Failed to open serial device: %s\n", argv[1]);
        return EXIT_SERIAL;
    }

    /* Magic DTR/RTS sequence then word "1EAF" (Maple / LeafLabs style) */
    (void)set_rts(0);
    (void)set_dtr(0);
    (void)set_dtr(1);

    sleep_us(50000);

    (void)set_dtr(0);
    (void)set_rts(1);
    (void)set_dtr(1);

    sleep_us(50000);

    (void)set_dtr(0);

    sleep_us(50000);

    {
        const char magic[] = "1EAF";
        ssize_t n = write(fd, magic, sizeof(magic) - 1U);
        if (n < 0) {
            perror("write");
            closeserial();
            return EXIT_SERIAL;
        }
        if ((size_t)n != sizeof(magic) - 1U) {
            fprintf(stderr, "write: short write\n");
            closeserial();
            return EXIT_SERIAL;
        }
    }

    closeserial();

    if (argc == 3) {
        char *end = NULL;
        errno = 0;
        long delay_ms = strtol(argv[2], &end, 10);
        if (errno != 0 || end == argv[2] || *end != '\0' || delay_ms < 0 ||
            delay_ms > 1000000L) {
            fprintf(stderr, "Invalid delay_ms: %s\n", argv[2]);
            return EXIT_USAGE;
        }
        sleep_us((useconds_t)(delay_ms * 1000L));
    }

    return EXIT_SUCCESS;
}
