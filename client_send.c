#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_IP "106.186.16.124"
#define SERVER_PORT 8042

int main(int argc, char* argv[]) {
    char* server_ip = argv[1];
    short server_port = atoi(argv[2]);
    // create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // connect to recv server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_aton(server_ip, &server_addr.sin_addr); 
    int res = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (res < 0) {
        perror("connect error");
        return -1;
    } 

    // send data
    write(sock, argv[3], strlen(argv[3]));

    char buf[1024 + 1] = {0};
    int n = read(sock, buf, 1024);
    if (n > 0)
        printf("%s\n", buf);
	return 0;
}

