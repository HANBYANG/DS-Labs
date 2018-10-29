#include "udp.h"
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>


using namespace std;

#define LOCALHOST "127.0.0.1"

int udpSend(int fd, int port, string info)
{
	char *buf = new char[BUFFERSIZE];
	memset(buf, '\0', BUFFERSIZE);
	strcpy(buf, info.c_str());
	
	//build connection
	struct sockaddr_in siTo;
	memset((char *) &siTo, 0, sizeof(siTo));
	
	siTo.sin_family = AF_INET;
	siTo.sin_port = htons(port);
	siTo.sin_addr.s_addr = inet_addr(LOCALHOST);
		
	//send content
	int r = sendto(fd, buf, BUFFERSIZE - 1, 0, (struct sockaddr*)&siTo, sizeof(siTo));
	if(r == -1)
	{
		perror("send error\n");
		return -1;
	}
	return 0;
}

int udpReceive(int fd, string &info)
{
	char *buf = new char[BUFFERSIZE];
	memset(buf, '\0', BUFFERSIZE);
	
	//build connection
	struct sockaddr_in siFrom;
	memset((char *)&siFrom, 0, sizeof(siFrom));
	
	//receive content
	socklen_t len = sizeof(siFrom);
	int r = recvfrom(fd, buf, BUFFERSIZE - 1, 0, (struct sockaddr *)&siFrom, &len);
	
	if(r > 0)
	{
		int port = ntohs(siFrom.sin_port);
		info = string(buf);
		return port;
	}
	return -1;
	
}

int udpBind(int port)
{	
	//create socket
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd == -1)
	{
		perror("creat error\n");
		return -1;
	}
	//build connection
	struct sockaddr_in siMe;
	memset((char *) &siMe, 0, sizeof(siMe));
	
	siMe.sin_family = AF_INET;
	siMe.sin_port = htons(port);
	siMe.sin_addr.s_addr = inet_addr(LOCALHOST);
	
	int r = bind(fd, (struct sockaddr*)&siMe, sizeof(siMe));
	if(r == -1)
	{
		perror("bind error\n");
		close(fd);
		return -1;
	}
	return fd;
	
}