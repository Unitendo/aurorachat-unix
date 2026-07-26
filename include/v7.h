#ifndef V7_H
#define V7_H

#include <stddef.h>

extern int v7_waitforhello(int s, char *servername, size_t sname_n);
extern void v7_decode(char *dst, const char *src, size_t n);
extern void v7_sendRulesRequest(int s);
extern int v7_getRulesResponse(int s, char *rules, size_t rules_n);
extern void v7_loginOrRegister(int s, const char *cmd, const char *login, const char *passwd);
extern int v7_loginOKCheck(int s, char *err, char *banmsg, size_t err_n, size_t ban_n);
extern void v7_joinRoom(int s, const char *room);
extern void v7_sendMsg(int s, const char *msg);

#endif
