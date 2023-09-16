// Optional helper program for server-side child process.

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

#define WRITE_HDERR write(cfd, "HDERR\n", 6);
#define ERR_EXIT \
    close(cfd);  \
    _exit(1);

/* read until newline, up to max bytes
add null terminator
return -1 on eof, 0 if no newline is found within max bytes, otherwise return
1*/
int runtnl(int fd, char *buf, size_t max) {
    size_t done = 0;
    ssize_t r;
    char c;
    while (done < max) {
        if ((r = read(fd, &c, 1)) < 0) {
            perror("read");
        } else if (r == 0) {
            return -1;
        }
        if (c == '\n') {
            *(buf + done) = '\0';
            return 1;
        }
        *(buf + done) = c;
        done += r;
    }
    return 0;
}
int stralnum(char *str, int len) {
    for (int i = 0; i < len; i++) {
        if (!isalnum(*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}
int strdigit(char *str, int len) {
    for (int i = 0; i < len; i++) {
        if (!isdigit(*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}
int main(int argc, char *argv[]) {  // cfd, serial
    int cfd = (int)strtol(argv[1], NULL, 10);
    // get username
    char uname[9];
    int r;
    if ((r = runtnl(cfd, uname, 9)) == 0 || !stralnum(uname, strlen(uname))) {
        WRITE_HDERR;
        ERR_EXIT;
    }
    if (r == -1) {  // premature eof
        ERR_EXIT;
    }

    // get filename
    char fname[101];
    if ((r = runtnl(cfd, fname, 101)) == 0) {
        WRITE_HDERR;
        ERR_EXIT;
    }
    if (r == -1) {
        ERR_EXIT;
    }

    // get filesize
    unsigned long long fs;
    char fsbuf[11];
    fsbuf[0] = 0;
    if ((r = runtnl(cfd, fsbuf, 11)) == 0 || !strdigit(fsbuf, strlen(fsbuf))) {
        WRITE_HDERR;
        ERR_EXIT;
    }
    if (r == -1) {
        ERR_EXIT;
    }
    fs = strtoull(fsbuf, NULL, 10);

    // remove newline from serial for name
    char *nl = strchr(argv[2], 10);
    *nl = 0;

    unsigned int fnlen = strlen(uname) + strlen(argv[2]) + strlen(fname) + 2;
    char fcname[fnlen + 1];
    if (fnlen > 0) {
        fcname[0] = '\0';
    }

    sprintf(fcname, "%s-%s-%s", uname, argv[2], fname);

    int ffd = open(fcname, O_CREAT | O_APPEND | O_WRONLY);
    char buf[15];
    unsigned long long rr;
    for (unsigned long long i = 0; i < ceil(fs / 15); i++) {
        if ((rr = read(cfd, buf, 1)) <= 0) {
            fprintf(stderr, "premature EOF or error\n");
            remove(fcname);
            close(ffd);
            ERR_EXIT;
        }
        write(ffd, buf, rr);
    }

    // send serial
    *nl = 10;
    write(cfd, argv[2], strlen(argv[2]));

    close(ffd);
    close(cfd);
    exit(0);
}