// Client program.
#define _LARGEFILE64_SOURCE
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#define ERR_EXIT(arg, error_msg)    \
    close(sfd);                     \
    if (arg == 0) {                 \
        perror(error_msg);          \
    } else {                        \
        fprintf(stderr, error_msg); \
    }                               \
    exit(1);
int wtos(int fd, void *buf, size_t len,
         int nl) {  // write to socket with optional newline delimiting
    size_t written = 0;
    ssize_t w;
    while (written < len) {  // loop over to handle partial writes
        if ((w = write(fd, buf + written, len - written)) <= 0) {
            return -1;
        }
        written += w;
    }
    if (nl) {
        if (write(fd, "\n", 1) <= 0) {
            return -1;
        }
        written++;
    } else {  // no newline => need null terminator for file
        if (write(fd, "\0", 1) <= 0) {
            return -1;
        }
        written++;
    }
    return written;
}
// cmdline reminder: IP-address, portnum, username, filename
int main(int argc, char *argv[]) {
    // get socket, fill in sockaddr, connect
    int sfd;
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_in sck;

    memset(&sck, 0, sizeof(sck));
    sck.sin_family = AF_INET;

    char *endd;
    long lport = strtol(argv[2], &endd, 10);
    if (*endd != 0 ||
        !isdigit(argv[2][0])) {  // strtol ignores leading whitespace, +, -
        ERR_EXIT(1, "invalid portnum\n");
    }

    uint16_t port = (uint16_t)lport;
    sck.sin_port = htons(port);
    if (inet_pton(AF_INET, argv[1], &sck.sin_addr) <= 0) {
        ERR_EXIT(1, "invalid ip addr\n");
    }

    if (connect(sfd, (struct sockaddr *)&sck, sizeof(sck)) == -1) {
        ERR_EXIT(0, "connect");
    }

    // get file size, contents
    int ffd;
    if ((ffd = open(argv[4], O_RDONLY)) < 0) {
        ERR_EXIT(0, "open");
    }

    off64_t fs = lseek64(ffd, 0, SEEK_END);
    lseek(ffd, 0, SEEK_SET);

    // store fs in string
    char fsbuf[10];  // max filesize 2^32 => 10 digits in string format
    int fsbuflen = sprintf(fsbuf, "%ld", fs);

    if (wtos(sfd, argv[3], strlen(argv[3]), 1) < 0 ||
        wtos(sfd, argv[4], strlen(argv[4]), 1) < 0 ||
        wtos(sfd, fsbuf, fsbuflen, 1) < 0) {
        ERR_EXIT(0, "write data");
    }

    // write file content to socket
    char buf[15];
    ssize_t r;
    ssize_t w = 0;
    while (1) {
        if ((r = read(ffd, buf, 15)) < 0) {
            close(ffd);
            ERR_EXIT(0, "read file content");
        }
        if (r == 0) {
            break;
        }
        if ((w = write(sfd, &buf, r)) != r) {
            close(ffd);
            ERR_EXIT(1, "write data err or eof\n");
        }
    }
    close(ffd);

    r = 0;
    w = 0;
    char serbuf[11] = {0};
    while (r < 11) {
        if ((w = read(sfd, serbuf + r, 11 - r)) <
            0) {  // read blocks until serial sent
            ERR_EXIT(0, "receiving serial");
        }
        if (w == 0) {
            break;
        }
        r += w;
    }

    char *end;
    unsigned long long ser = strtoull(serbuf, &end, 10);
    if (!(*end == '\n') || !(isdigit(serbuf[0]))) {
        ERR_EXIT(1, "invalid serial\n");
    }
    printf("%lld\n", ser);
    close(sfd);
    exit(0);
}