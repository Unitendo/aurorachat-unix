#ifndef MISC_H
#define MISC_H

extern int misc_nonblock_enable();
extern int misc_nonblock_disable();
extern int misc_noecho();
extern int misc_echo();
extern void misc_rest();
extern void misc_trimlf(char *s);
extern int misc_startswith(const char *s1, const char *s2);

#endif
