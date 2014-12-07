#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    char* server_ip = argv[1];
    short server_port = atoi(argv[2]);
    char* cmd = argv[3];
    int cmd_len = strlen(cmd);

    // create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // connect to cmd server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_aton(server_ip, &server_addr.sin_addr); 
    int res = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (res < 0) {
        perror("connect error");
        return -1;
    } 

    // send command length
    write(sock, &cmd_len, sizeof(cmd_len));

    /********************************************************
     * This will create partial read at server.
     ******************************************************** 
    write(sock, &cmd_len, sizeof(cmd_len) - 1);
    sleep(10);
    write(sock, (char*)&cmd_len + 3, 1);
    */

    // send command string
    write(sock, cmd, cmd_len);

    char buf[4096 + 1] = {0};
    int n = read(sock, buf, 4096);
    if (n > 0)
        printf("%s", buf);

	return 0;
}

