
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
 
int main(int argc, char **argv)
{
	struct sockaddr_in6 s_addr;
	int sock;
	int addr_len;
	int len;
	char buff[128];
 
	if ((sock = socket(AF_INET6, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(errno);
	} else
		printf("create socket.\n\r");
 
	s_addr.sin6_family = AF_INET6;
	if (argv[2])
		s_addr.sin6_port = htons(atoi(argv[2]));
	else
		s_addr.sin6_port = htons(4444);
 
	if (argv[1])
		inet_pton(AF_INET6, argv[1], &s_addr.sin6_addr);
	else {
		printf("usage:./command ip port\n");
		exit(0);
	}
 
	addr_len = sizeof(s_addr);
	strcpy(buff, "hello i'm here");
	len = sendto(sock, buff, strlen(buff), 0,
				 (struct sockaddr *) &s_addr, addr_len);
	if (len < 0) {
		printf("\n\rsend error.\n\r");
		return 3;
	}
 
	printf("send success.\n\r");
	return 0;
}
