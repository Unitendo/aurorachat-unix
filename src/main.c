#include <stdio.h>
#include <sockets.h>
#include <v7.h>
#include <misc.h>
#include <string.h>

#define DEFAULT_IP "::FFFF:104.236.25.60"
// #define DEFAULT_IP "::FFFF:127.0.0.1"
#define DEFAULT_PORT 7070

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
    printf("\x1b[H\x1b[2J\x1b[3J[ %s - Rules ]\n\n", servername);
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
                    printf("\x1b[H\x1b[2J\x1b[3J");
                }
            }
            printf("\n[ Press enter ]\n");
            fgets(buffer2, sizeof(buffer2) - 1, stdin);
        }
    }
    return 0;
}

void showMOTD(const char *servername, const char *motd) {
    char buffer[64] = {0};
    misc_nonblock_disable();
    printf("\x1b[H\x1b[2J\x1b[3J[ %s - Message of the day ]\n\n%s\n\n[ Press enter ]\n", servername, motd);
    fgets(buffer, sizeof(buffer) - 1, stdin);
    misc_nonblock_enable();
    printf("\x1b[H\x1b[2J\x1b[3J");
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
        break;

        case 'R':
        case 'r':
            strncpy(cmd, "register", cmd_n - 1);
            return 0;
        break;

        default:
            return 1;
    }
}

void joinRoom(int s, const char *room) {
    printf("\x1b[H\x1b[2J\x1b[3JCurrent room: #%s\n", room);
    v7_joinRoom(s, room);
}

int main() {
    printf("\x1b[H\x1b[2J\x1b[3J[ AuroraChat Unix ]\n");

    char server_ip[256] = {0};
    short server_port = 0;

    setupIP(server_ip, &server_port, sizeof(server_ip));

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

    printf("\x1b[H\x1b[2J\x1b[3J[ %s ]\n", servername);

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

    joinRoom(s, "general");

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
            printf("<%s> %s\n", author, content);
        }

        if(!socket_stillalive(s)) {
            printf("\nDisconnected: %s\n", socket_error());
            socket_destroy(s);
            return 0;
        }

        if(*motd) showMOTD(servername, motd);

        if(fgets(buffer, sizeof(buffer) - 1, stdin) == NULL)
            misc_rest();
        else {
            printf("\x1b[1A\x1b[2K");
            misc_trimlf(buffer);
            if(!strcmp(buffer, "/help")) {
                printf(
                    "\nCommand list:\n"
                    "/clear\n"
                    "/quit\n"
                    "/room #roomname\n"
                    "/motd\n"
                );
            } else if(!strcmp(buffer, "/room")) {
                printf("\nUsage: /room #roomname\n");
            } else if(!misc_startswith(buffer, "/room #")) {
                joinRoom(s, buffer + 7);
            } else if(!strcmp(buffer, "/clear")) {
                printf("\x1b[H\x1b[2J\x1b[3J");
            } else if(!strcmp(buffer, "/quit")) {
                printf("\nGoodbye!\n");
                socket_destroy(s);
                return 0;
            } else if(!strcmp(buffer, "/motd")) {
                v7_sendMOTDReq(s);
            } else v7_sendMsg(s, buffer);
        }
    }

    return 0;
}
