#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sockets.h>
#include <v7.h>
#include <misc.h>
#include <string.h>

char esc_clear[128] = "\x1b[H\x1b[2J\x1b[3J";
char esc_reset[16] = "\x1b[0m";
char esc_bold[16] = "\x1b[1m";
char esc_dim[16] = "\x1b[2m";
char esc_italic[16] = "\x1b[3m";
char esc_unbold[16] = "\x1b[22m";
char esc_clrline[16] = "\x1b[2K";
char esc_goup[16] = "\x1b[1A";

#ifndef DEFAULT_IP
    #define DEFAULT_IP "::FFFF:144.217.26.87"
#endif

#ifndef DEFAULT_PORT
    #define DEFAULT_PORT 7070
#endif

void setupIP(char *ip, short *port, size_t ip_n) {
    char buffer[16] = {0};
    printf("Setup IP [Y/N]? ");
    fgets(buffer, sizeof(buffer) - 1, stdin);
    switch(buffer[0]) {
        case 'Y':
        case 'y':
            printf("Enter IP (IPv4 must be prefixed with ::FFFF:): ");
            fgets(ip, ip_n - 1, stdin);
            misc_trimlf(ip);
            printf("Enter Port (7070 by default): ");
            scanf("%hd", port);
            fgets(buffer, sizeof(buffer) - 1, stdin);
        break;

        default:
            strncpy(ip, DEFAULT_IP, ip_n - 1);
            *port = DEFAULT_PORT;
    }
}

int showRules(SOCKBUF_T *sb, const char *servername) {
    printf("%s[ %s - Rules ]\n\n", esc_clear, servername);
    char buffer[16384] = {0};
    char buffer2[64] = {0};
    v7_sendRulesRequest(sb->socket);
    switch(v7_getRulesResponse(sb, buffer, sizeof(buffer))) {
        case 1:
            printf("v7 rules error: %s\n", socket_error());
            socket_destroy(sb->socket);
            return 1;

        case 2:
            printf("v7 rules error! (is this a v7 server?)\n");
            socket_destroy(sb->socket);
            return 1;

        default: {
            int linecounter = 0;
            for(char *token = strtok(buffer, "\n"); token; token = strtok(NULL, "\n")) {
                printf("%s\n", token);
                linecounter++;
                if(linecounter % 12 == 0) {
                    printf("\n[ Press enter ]\n");
                    fgets(buffer2, sizeof(buffer2) - 1, stdin);
                    printf("%s", esc_clear);
                }
            }
            printf("\n[ Press enter ]\n");
            fgets(buffer2, sizeof(buffer2) - 1, stdin);
        }
    }

    printf("%s[ AuroraChat Unix ]\nUser guide:\n/quit - Exit AuroraChat\n/help - List all available commands\n\n[ Press enter ]\n", esc_clear);
    fgets(buffer2, sizeof(buffer2) - 1, stdin);

    return 0;
}

void showMOTD(const char *servername, const char *motd) {
    char buffer[64] = {0};
    misc_nonblock_disable();
    printf("%s%s[ %s - Message of the day ]\n\n%s\n\n[ Press enter ]\n", esc_reset, esc_clear, servername, motd);
    fgets(buffer, sizeof(buffer) - 1, stdin);
    misc_nonblock_enable();
    printf("%s", esc_clear);
}

int loginScreen(char *cmd, char *login, char *passwd, size_t cmd_n, size_t login_n, size_t passwd_n) {
    printf("Enter your username: ");
    fgets(login, login_n - 1, stdin);
    misc_trimlf(login);

    misc_noecho();
    printf("Enter your password: ");
    fgets(passwd, passwd_n - 1, stdin);
    misc_trimlf(passwd);
    misc_echo();
    printf("\n");

    char buffer[16] = {0};
    printf("[L]ogin or [R]egister? ");
    fgets(buffer, sizeof(buffer) - 1, stdin);
    switch(buffer[0]) {
        case 'L':
        case 'l':
            strncpy(cmd, "login", cmd_n - 1);
            return 0;

        case 'R':
        case 'r':
            strncpy(cmd, "register", cmd_n - 1);
            return 0;

        default:
            return 1;
    }
}

void joinRoom(int s, const char *room, char *roombuf, size_t n) {
    printf("%sCurrent room: #%s\n", esc_clear, room);
    v7_joinRoom(s, room);
    strncpy(roombuf, room, n - 1);
}

enum ARGSTATE {
    ARGS_NORMAL,
    ARGS_IP,
    ARGS_PORT
};

int main(int argc, char **argv) {
    char server_ip[256] = {0};
    strncpy(server_ip, DEFAULT_IP, sizeof(server_ip) - 1);
    short server_port = DEFAULT_PORT;
    int server_ipwasspecified = 0;
    int useescape = 1;
    int showhelp = 0;
    enum ARGSTATE argstate = ARGS_NORMAL;

    for(int i=1;i<argc;i++) {
        switch(argstate) {
            case ARGS_NORMAL:
                if(!strcmp(argv[i], "-i")) argstate = ARGS_IP;
                else if(!strcmp(argv[i], "-p")) argstate = ARGS_PORT;
                else if(!strcmp(argv[i], "-n")) useescape = 0;
                else if(!strcmp(argv[i], "-h")) showhelp = 1;
                else {
                    printf("Unknown parameter: %s\n", argv[i]);
                    return 1;
                }
            break;

            case ARGS_IP:
                argstate = ARGS_NORMAL;
                strncpy(server_ip, argv[i], sizeof(server_ip) - 1);
                server_ipwasspecified = 1;
            break;

            case ARGS_PORT:
                argstate = ARGS_NORMAL;
                server_port = strtol(argv[i], NULL, 0);
                server_ipwasspecified = 1;
            break;
        }
    }

    if(showhelp) {
        printf("Usage: %s [-h] [-i IP] [-p PORT] [-n]\n", argv[0]);
        return 0;
    }

    if(!useescape) {
        for(int i=0;i<127;i++) 
            esc_clear[i] = '\n';
        esc_clear[127] = 0;
        esc_reset[0] = 0;
        esc_bold[0] = 0;
        esc_dim[0] = 0;
        esc_italic[0] = 0;
        esc_unbold[0] = 0;
        esc_clrline[0] = 0;
        esc_goup[0] = 0;
    }

    printf("%s[ AuroraChat Unix ]\n", esc_clear);
    if(!server_ipwasspecified) setupIP(server_ip, &server_port, sizeof(server_ip));

    int s = socket_create(server_ip, server_port);
    if(s == -1) {
        printf("Socket create error: %s\n", socket_error());
        return 1;
    }

    SOCKBUF_T sb;
    sockbuf_init(&sb, s);

    char servername[512] = {0};

    switch(v7_waitforhello(&sb, servername, sizeof(servername))) {
        case 1:
            printf("v7 greeting error: %s\n", socket_error());
            socket_destroy(s);
            return 1;

        case 2:
            printf("v7 greeting error, not a v7 server!\n");
            socket_destroy(s);
            return 1;

        default:
        break;
    }

    if(showRules(&sb, servername))
        return 1;

    printf("%s[ %s ]\n", esc_clear, servername);

    char login_cmd[64] = {0};
    char login_login[64] = {0};
    char login_passwd[64] = {0};

    if(loginScreen(login_cmd, login_login, login_passwd, sizeof(login_cmd), sizeof(login_login), sizeof(login_passwd)))
        return 0;
    
    v7_loginOrRegister(s, login_cmd, login_login, login_passwd);
    
    char login_errorcode[512] = {0};
    char login_banreason[512] = {0};

    switch(v7_loginOKCheck(&sb, login_errorcode, login_banreason, sizeof(login_errorcode), sizeof(login_banreason))) {
        case 1:
            printf("\nv7 login error: %s\n", socket_error());
            socket_destroy(s);
            return 1;
        
        case 2:
            printf("\nv7 login error, is this a v7 server?\n");
            socket_destroy(s);
            return 1;

        case 3:
            printf("\nv7 login error: %s\n", login_errorcode);
            socket_destroy(s);
            return 1;

        case 4:
            printf("\nYou are banned!\n%s\n", login_banreason);
            socket_destroy(s);
            return 0;

        default:
        break;
    }

    char currentroomname[512] = {0};
    joinRoom(s, "general", currentroomname, sizeof(currentroomname));

    misc_nonblock_enable();
    while(1) {
        char motd[4096] = {0};
        char buffer[4096] = {0};

        size_t recvd;
        while( (recvd = sockbuf_getline_nonblock(&sb, buffer, sizeof(buffer) - 1)) ) {
            if(recvd == -1)
                break;

            buffer[recvd] = 0;
            char *token = strtok(buffer, "|");

            char author[64] = {0};
            char content[1024] = {0};
            char time[64] = {0};

            if(token == NULL) continue;
            
            if(!strncmp(token, "motd", sizeof(buffer))) {
                token = strtok(NULL, "|");
                if(token == NULL) continue;
                v7_decode(motd, token, sizeof(motd));
            }
            
            if(strncmp(token, "msg", sizeof(buffer))) 
                continue;

            token = strtok(NULL, "|");
            if(token == NULL) continue;
            v7_decode(author, token, sizeof(author));
            token = strtok(NULL, "|");
            if(token == NULL) continue;
            v7_decode(content, token, sizeof(content));
            token = strtok(NULL, "|");
            if(token != NULL) {
                time_t timestamp = strtol(token, NULL, 10);
                struct tm *ts = localtime(&timestamp);
                if(ts != NULL) {
                    snprintf(time, 63, "%s[%02d:%02d]%s ", esc_dim, ts->tm_hour, ts->tm_min, esc_unbold);
                }
            }
            printf("%s\r%s%s<%s%s%s> %s\n", esc_clrline, esc_reset, time, esc_bold, author, esc_unbold, content);
        }

        if(recvd == 0) {
            printf("%s\nDisconnected\n", esc_reset);
            socket_destroy(s);
            misc_nonblock_disable();
            return 0;
        }

        if(!socket_stillalive(s)) {
            printf("%s\nDisconnected: %s\n", esc_reset, socket_error());
            socket_destroy(s);
            misc_nonblock_disable();
            return 0;
        }

        printf("%s", esc_italic);
        fflush(stdout);

        if(*motd) showMOTD(servername, motd);

        if(fgets(buffer, sizeof(buffer) - 1, stdin) == NULL)
            misc_rest();
        else {
            printf("%s%s%s", esc_goup, esc_clrline, esc_reset);
            misc_trimlf(buffer);
            if(!strcmp(buffer, "/help")) {
                printf(
                    "\nCommand list:\n"
                    "/clear\n"
                    "/quit\n"
                    "/room\n"
                    "/room #roomname\n"
                    "/dm @username\n"
                    "/motd\n"
                    "\n"
                );
            } else if(!strcmp(buffer, "/room")) {
                printf("\nUsage: /room #roomname\nCurrent room: #%s\n\n", currentroomname);
            } else if(!misc_startswith(buffer, "/room #")) {
                joinRoom(s, buffer + 7, currentroomname, sizeof(currentroomname));
            } else if(!misc_startswith(buffer, "/dm @")) {
                joinRoom(s, buffer + 4, currentroomname, sizeof(currentroomname));
            } else if(!strcmp(buffer, "/clear")) {
                printf("%s", esc_clear);
            } else if(!strcmp(buffer, "/quit")) {
                printf("\nGoodbye!\n");
                socket_destroy(s);
                misc_nonblock_disable();
                return 0;
            } else if(!strcmp(buffer, "/motd")) {
                v7_sendMOTDReq(s);
            } else v7_sendMsg(s, buffer);
        }
    }

    return 0;
}
