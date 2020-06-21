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
	struct sockaddr_in s_addr;
	struct sockaddr_in c_addr;
	int sock;
	socklen_t addr_len;
	int len;
	char buff[128];
	char buf_ip[128];
 
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(errno);
	} else
		printf("create socket.\n\r");
 
	memset(&s_addr, 0, sizeof(struct sockaddr_in));
	s_addr.sin_family = AF_INET;
 
	if (argv[2])
		s_addr.sin_port = htons(atoi(argv[2]));
	else
		s_addr.sin_port = htons(4444);
 
	if (argv[1])
		inet_pton(AF_INET, argv[1], &s_addr.sin_addr);
 
	if ((bind(sock, (struct sockaddr *) &s_addr, sizeof(s_addr))) == -1) {
		perror("bind error");
		exit(errno);
	} else
		printf("bind address to socket.\n\r");
 
	addr_len = sizeof(c_addr);
	while (1) {
		len = recvfrom(sock, buff, sizeof(buff) - 1, 0,
					   (struct sockaddr *) &c_addr, &addr_len);
		if (len < 0) {
			perror("recvfrom");
			exit(errno);
		}
 
		buff[len] = '\0';
		printf("receive from %s: buffer:%s\n\r",
				inet_ntop(AF_INET, &c_addr.sin_addr, buf_ip, sizeof(buf_ip)), 
				buff);
	}
	return 0;
}
