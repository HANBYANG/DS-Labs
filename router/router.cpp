#include "rtbl.h"
#include "udp.h"
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>
#include <assert.h>
#include <map>

using namespace std;

mutex mtx;
map<string, RouterEntry>routerTable;
map<string, RouterEntry>buffer;
map<int, string>portTable;
map<string, RouterEntry>last;
Host localhost;

int printNum=0;

int fd;


/*
**** receive thread
**** update local routertable via update packages
*/
void* receive(void* ptr)
{
	while(1)
	{
		string info;
		int fromPort = udpReceive(fd, info);
		//printf("%s\n",info.c_str());
		string src = portTable[fromPort];
		Host srcHost;
		srcHost.name = src;
		map<string, RouterEntry> srcTable;

		
		unpackRouterInfo(info, srcTable, srcHost);
		mtx.lock();
		updateRouterTable(buffer,srcTable, routerTable, srcHost, localhost);
		mtx.unlock();

		//printTable(printNum,routerTable,localhost);
	}
}


/*
**** send thread
**** send update packages to neighbours every 5 sec
*/
int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf("usage: %s <prot_number> <file_name>\n", argv[0]);
		exit(-1);
	}
	
	int port = atoi(argv[1]);
	if(port <= 0)
	{
		printf("error: invalid <port_number>\n");
		exit(-1);
	}
	
	string filename = string(argv[2]);

	localhost.seqNum=0;

	initRouterTable(filename,buffer,routerTable,portTable,localhost);

	fd = udpBind(port);

	pthread_t child;

	pthread_create(&child, NULL, receive, NULL);


	while (1)
	{
		mtx.lock();
		int changed=initRouterTable(filename,buffer,routerTable,portTable,localhost);
		/*
		if (!last.empty())
		{
			bool flag=false;
			//if (last.size()!=routerTable.size()) flag=true;
			//else
			{
				for (auto iter=last.begin();iter!=last.end();iter++)
					if (!isEqualEntry(iter->second,routerTable[iter->first])) 
					{
						flag=true;
						break;
					}
			}
			if (flag)
			{
				localhost.seqNum+=2;
				routerTable[localhost.name].seqNum+=2;
			}
		}
		last=routerTable;
		*/
		
		if (changed==1) 
		{
			//localhost.seqNum+=2;
			//routerTable[localhost.name].seqNum+=2;
		}
		
		
		mtx.unlock();
		string info="";
		printTable(printNum,routerTable,localhost);
		printNum++;
		packRouterInfo(info,routerTable,localhost);
		for (auto iter=portTable.begin();iter!=portTable.end();iter++)
		{
			if (buffer[iter->second].cost < 10000-0.000001) 
			udpSend(fd,iter->first,info);
		}
		sleep(5);
	}

	close(fd);

	return 0;
}
