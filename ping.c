#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>

int raw_sock;
int seq;
char* dst_ip;

void sig_alarm(int sig) {
   printf("%s %d timeout\n", dst_ip, seq);
}

/*
 * in_cksum --
 *	Checksum routine for Internet Protocol family headers (C Version)
 */
u_short in_cksum(u_short *addr, int len) {
	int nleft = len;
	u_short *w = addr;
	int sum = 0;
	u_short answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}

void tv_sub(struct timeval* out, struct timeval* in) {
    if ((out->tv_usec -= in->tv_usec) < 0) {
        --out->tv_sec;
        out->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}

int main(int argc, char* argv[]) {
    dst_ip = argv[1];
    // create a ipv4 raw socket
    raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (raw_sock < 0) {
        perror("socket error");
        exit(0);
    }
    
    // catch SIGALARM for timeout
    signal(SIGALRM, sig_alarm);

    // set dst addr
    struct sockaddr_in dst;
    bzero(&dst, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = inet_addr(argv[1]);

    // fill an icmp message buf
    char buf[1024];
    char* hello = "hello";
    int len = 8 + sizeof(struct timeval) + strlen(hello); // 8 icmp header, 16 timestamp, "hello"
    struct icmp *icmp = (struct icmp*)buf;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = getpid() & 0xFFFF;
    icmp->icmp_seq = htons(++seq);
    // set timestamp to icmp optional data
    gettimeofday((struct timeval*)icmp->icmp_data, NULL);
    strcpy((char*)icmp->icmp_data + sizeof(struct timeval), hello);
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = in_cksum((u_short*)icmp, len);
  
    // send icmp echo request 
    int ret = sendto(raw_sock, buf, len, 0, (struct sockaddr*)&dst, sizeof(dst));
    if (ret < 0)
        perror("sendto error");

    // wait for icmp echo response
    char recvbuf[1024];
    int addrlen = sizeof(dst);
    // set recv timeout
    struct timeval rcvtimeo;
    rcvtimeo.tv_sec = 5;
    rcvtimeo.tv_usec = 0;
    setsockopt(raw_sock, SOL_SOCKET, SO_RCVTIMEO, &rcvtimeo, sizeof(rcvtimeo));
    int n = recvfrom(raw_sock, recvbuf, sizeof(recvbuf), 0,
                (struct sockaddr*)&dst, &addrlen);
    if (n < 0) {
        if (errno == EAGAIN)
            printf("%s request timeout\n", dst_ip);
        else
            perror("recvfrom error");
        exit(0);
    }
    
    struct ip* ip = (struct ip*)recvbuf;
    int ip_hdl = ip->ip_hl * 4;
    if (ip->ip_p != IPPROTO_ICMP) {
        printf("not icmp\n");
        exit(0);
    }

    int icmp_size = n - ip_hdl;
    icmp = (struct icmp*)(recvbuf + ip_hdl);
    if (icmp->icmp_type == ICMP_ECHOREPLY) {
        if (icmp->icmp_id != getpid() & 0xFFFF) {
            printf("icmp id wrong\n");
            exit(0);
        }
        struct timeval* send_tv = (struct timeval*)icmp->icmp_data;
        struct timeval cur_tv;
        gettimeofday(&cur_tv, NULL);
        tv_sub(&cur_tv, send_tv);
        float rtt = cur_tv.tv_sec * 1000.0f + cur_tv.tv_usec / 1000.0f;
        char msg[64];
        strncpy(msg, (char*)icmp->icmp_data + sizeof(struct timeval),
            icmp_size - sizeof(struct timeval) - 8); 
        printf("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms, msg=%s\n",
            icmp_size, inet_ntoa(ip->ip_src), ntohs(icmp->icmp_seq),
            ip->ip_ttl, rtt, msg);
    }

    return 0;
}

