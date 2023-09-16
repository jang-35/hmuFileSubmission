// Server program.

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define ERR_EXIT(arg, error_msg) \
    perror(error_msg);           \
    close(sfd);                  \
    if (arg == 0) {              \
        exit(1);                 \
    }                            \
    _exit(1);

int main(int argc, char *argv[]) {
    int sfd, cfd;
    unsigned long long serial = 0;
    pid_t p;
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ERR_EXIT(0, "socket");
    }
    uint16_t port = (uint16_t)strtol(argv[1], NULL, 10);
    struct sockaddr_in sck;
    memset(&sck, 0, sizeof(sck));
    sck.sin_family = AF_INET;
    sck.sin_port = htons(port);
    sck.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sfd, (struct sockaddr *)&sck, sizeof(struct sockaddr_in)) < 0) {
        ERR_EXIT(0, "bind");
    }

    listen(sfd, 5);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    while (1) {  // pass in serial, cfd
        if ((cfd = accept(sfd, NULL, 0)) < 0) {
            perror("accept");
        }
        if ((p = fork()) < 0) {
            perror("fork");
        }
        if (p != 0) {
            close(cfd);
            serial++;
        } else {
            char serbuf[11];
            sprintf(serbuf, "%lld\n", serial);
            char cfdbuf[6];  // int fd => 5 digit max
            sprintf(cfdbuf, "%d", cfd);
            execlp(argv[2], argv[2], cfdbuf, serbuf, (char *)NULL);

            close(cfd);
            ERR_EXIT(1, "exec");
        }
    }
    close(sfd);
    return 0;
}
