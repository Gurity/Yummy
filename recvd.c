#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT 8042

void show_client_info(struct sockaddr_in* client_addr) {
    printf("%s : %d\n", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
}

void do_service(int sock) {
    char buf[1024] = {0};
    int n;
    /*
    int n = read(sock, buf, sizeof(buf));
    if (n > 0) {
        printf("%d recveived: %s\n", n, buf);
    }
    */
    n = 0;
    char c;
    while (read(sock, &c, 1) > 0) {
        buf[n++] = c;
        if (c == '\n')
            break;
    }
    if (n > 0)
        write(sock, buf, n);
}

int main() {
    // create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    // set socket to share port
    int optval = 1; 
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

    // bind socket to a local interface
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    // put socket into listen mode
    listen(sock, 1024);

    // accept new connection
    struct sockaddr_in client_addr; 
    int sock_to_client;
    socklen_t len_client_addr;
    for ( ; ; ) {
        sock_to_client = accept(sock, (struct sockaddr*)&client_addr, &len_client_addr);
        if (sock_to_client < 0)
            continue;
        show_client_info(&client_addr);
        do_service(sock_to_client);
        close(sock_to_client);
    }

	return 0;
}
