// unix headers
#include <unistd.h>
#include <limits.h>
#include <poll.h>
// libc header
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// network headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 8888 // server listening port
#define SERVER_IP   INADDR_ANY // server listening ip
#define OPEN_MAX    1024

struct request {
    int n;
    char cmd[1024];
};

struct pollfd client_fd[OPEN_MAX];
int maxi;
struct request req[OPEN_MAX];

void print_client_info(struct sockaddr_in *client_addr) {
    printf("%s:%d connected\n", inet_ntoa(client_addr->sin_addr),
        ntohs(client_addr->sin_port));
}

int main() {
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = htonl(SERVER_IP);

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    bind(listen_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(listen_sock, 10);

    client_fd[0].fd = listen_sock;
    client_fd[0].events = POLLRDNORM;
    int i;
    for (i = 1; i < OPEN_MAX; i++) {
        client_fd[i].fd = -1;
        req[i].n = -1; 
    }
    maxi = 0;

    int nready;
    struct sockaddr_in client_addr;
    int len, n, ncmd;
    int conn;
    char buf[1024], err[64];
    int sock;
    FILE* fp;
    for ( ; ; ) {
        nready = poll(client_fd, maxi + 1, -1);

        // check listening socket
        if (client_fd[0].revents & POLLRDNORM) {
            len = sizeof(client_addr);
            conn = accept(listen_sock, (struct sockaddr*)&client_addr, &len);
            for (i = 1; i < OPEN_MAX; i++)
                if (client_fd[i].fd < 0) {
                    client_fd[i].fd = conn;
                    break;
                }
            if (i == OPEN_MAX) {
                perror("OPEN_MAX reached");
                exit(0);
            }
            client_fd[i].events = POLLRDNORM;
            if (i > maxi)
                maxi = i;

            print_client_info(&client_addr);
            if (--nready <= 0)
                continue;
        }

        // check connected socket
        for (i = 1; i <= maxi; i++) {
            if ((sock = client_fd[i].fd) < 0)
                continue;
            if (client_fd[i].revents & (POLLRDNORM | POLLERR)) {
                n = read(sock, &req[i].n, 4);
                if (n == 0) {
                    printf("client closed\n");
                    close(client_fd[i].fd);
                    client_fd[i].fd = -1;
                    req[i].n = -1;
                } else if (n < 0) {
                    perror("read error");
                    close(client_fd[i].fd);
                    client_fd[i].fd = -1;
                    req[i].n = -1;
                } else if (n < 4) {
                    // deal with partial read 
                    sprintf(err, "read count error: read %d, return %d", 4, n);
                    perror(err);
                    exit(0);
                } else {
                    n = read(sock, req[i].cmd, req[i].n);
                    if (n != req[i].n) {
                        sprintf(err, "read cmd error: read %d, return %d", req[i].n, n);
                        perror(err);
                        exit(0);
                    }
                    req[i].cmd[req[i].n] = '\0';
                    printf("receive command: %s\n", req[i].cmd);
                    fp = popen(req[i].cmd, "r");
                    if (!fp) {
                        perror("popen error");
                        exit(0);
                    }
                    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
                        write(client_fd[i].fd, buf, n);                     
                    }
                    pclose(fp);
                }

                if (--nready <= 0)
                    break;
            }
        }
    }

    return 0;
}

