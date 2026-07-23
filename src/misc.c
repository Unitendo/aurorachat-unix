#define _POSIX_C_SOURCE 199309L

#include <misc.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <termios.h>

int misc_nonblock_enable() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if(flags == -1) return 1;

    if(fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) return 1;
    return 0;
}

int misc_nonblock_disable() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if(flags == -1) return 1;

    if(fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK) == -1) return 1;
    return 0;
}

int misc_noecho() {
    struct termios term;
    if(tcgetattr(STDIN_FILENO, &term))
        return 1;

    term.c_lflag &= ~ECHO;
    if(tcsetattr(STDIN_FILENO, 0, &term))
        return 1;

    return 0;
}

int misc_echo() {
    struct termios term;
    if(tcgetattr(STDIN_FILENO, &term))
        return 1;

    term.c_lflag |= ECHO;
    if(tcsetattr(STDIN_FILENO, 0, &term))
        return 1;

    return 0;
}

void misc_rest() {
    struct timespec duration;
    struct timespec rem;
    duration.tv_sec = 0;
    duration.tv_nsec = 100000000; // 0.1 seconds

    nanosleep(&duration, &rem);
}

void misc_trimlf(char *s) {
    size_t l = strlen(s) - 1;
    if(s[l] == '\n') s[l] = 0;
}

int misc_startswith(const char *s1, const char *s2) {
    size_t l = strlen(s2);
    for(size_t i=0;i<l;i++) {
        if(s1[i] != s2[i]) return 1;
    }
    return 0;
}
