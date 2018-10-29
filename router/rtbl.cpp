#include "rtbl.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <math.h>

using namespace std;

RouterEntry::RouterEntry():dst(""),nextHop(""),cost(0),seqNum(0){};

RouterEntry::RouterEntry(string dst, string nextHop, double cost, int seqNum)
         :dst(dst),nextHop(nextHop),cost(cost),seqNum(seqNum){};


/*
***  compare two RouterEntry
***  if equal return TRUE else return FALSE
*/
bool isEqualEntry(const RouterEntry& e1, const RouterEntry& e2)
{
	return e1.dst==e2.dst && e1.nextHop==e2.nextHop && ((e1.cost<0 && e2.cost<0) || fabs(e1.cost-e2.cost)<0.000001);
}


/*
***  print info of one single entry 
***  For Debug
*/
int printEntry(RouterEntry& entry)
{
	printf("dst:%s nextHop:%s cost:%lf\n",entry.dst.c_str(),entry.nextHop.c_str(),entry.cost);
}

/*
***  read local router information and update the local router table
     return 1 if there're changes in router information
     else return 0
*/
int initRouterTable
(string fileName, map<string, RouterEntry>& lastTable,map<string, RouterEntry>& routerTable, map<int, string>& portTable, Host& host)
{
	ifstream ist(fileName.c_str());
	if (!ist) return -1;
	//map<string,RouterEntry> oldTable=routerTable;
	bool flag=false;
	if (routerTable.empty()) flag=true;
	bool changed=false;
	string hostName;
	int degree;
	ist>>degree>>hostName;
	host.name=hostName;
	host.degree=degree;
	string neighbour;
	double cost;
	int portNum;

	while (ist>>neighbour>>cost>>portNum)
	{
		//printf("%s %lf %d\n",neighbour.c_str(),cost,portNum);
		if (flag)
		{
			lastTable[neighbour]=routerTable[neighbour]=RouterEntry(neighbour,neighbour,cost<0?10000.0:cost,0);
			lastTable[hostName]=routerTable[hostName]=RouterEntry(hostName,hostName,0.0,0);
			//printEntry(lastTable[neighbour]);
		}
		else
		{
			RouterEntry oldEntry=routerTable[neighbour];
			RouterEntry lastEntry=lastTable[neighbour];
			RouterEntry newEntry=RouterEntry(neighbour,neighbour,cost,oldEntry.seqNum);
			if (newEntry.cost<0) newEntry.cost=10000.0;
			if (fabs(lastEntry.cost-newEntry.cost)>0.000001)
			{
				changed=true;
				/*
				if (newEntry.cost * lastEntry.cost < 0)
				{
					newEntry.seqNum++;
				}
				*/
				for (auto iter=routerTable.begin();iter!=routerTable.end();iter++)
				{
					if (iter->second.nextHop==neighbour)
					{
						iter->second.cost+=newEntry.cost-lastEntry.cost;
						iter->second.seqNum+=2;
					}
				}
				//routerTable[neighbour]=newEntry;
			}
			lastTable[neighbour]=newEntry;
			//printEntry(newEntry);
		}
		portTable[portNum]=neighbour;
	}
	lastTable[hostName].nextHop=hostName;
	lastTable[hostName].cost=0.0;
	//printf("\n");
	assert(lastTable.size()==degree+1);
	/*
	if (changed) 
	{
		routerTable.clear();
		for (auto it=lastTable.begin();it!=lastTable.end();it++)
			routerTable[it->first]=it->second;
		//host.seqNum+=2;
	}
	*/
	//if (flag) routerTable[hostName]=RouterEntry(hostName,hostName,0,host.seqNum);
	//else routerTable[hostName].seqNum=host.seqNum;
	return changed;
}

/*
***  pack the information of router table into string,
***  which helps in sending update packages
*/
int packRouterInfo(string& info, map<string, RouterEntry>& routerTable, Host& host)
{
	ostringstream osst;
	osst<<host.name<<endl<<host.degree<<endl<<routerTable[host.name].seqNum<<endl;
	for(map<string, RouterEntry>::iterator it=routerTable.begin();it != routerTable.end(); it ++)
	{
		osst<<(it->second).dst<<' '<<(it->second).nextHop<<' '<<(it->second).cost<<' '<<(it->second).seqNum<<'\n';
	}
	info = osst.str();
	return 0;
}


/*
***  contrary to packRouterInfo(), build routerTable from package string
*/
int unpackRouterInfo(string info, map<string, RouterEntry>& tempTable, Host& src)
{
	istringstream isst(info);
	if(!(isst>>src.name>>src.degree>>src.seqNum)) return -1;
	tempTable.clear();
	string dst;
	string nextHop;
	double cost;
	int seqNum;
	while (isst)
	{
		isst>>dst>>nextHop>>cost>>seqNum;
		tempTable[dst]=RouterEntry(dst,nextHop,cost,seqNum);
	}
	return 0;

}

/*
***  update local router table via update packages
***  use information with larger seqNum
***  choose the shorter path when seqNums equal
*/
int updateRouterTable(map<string,RouterEntry>& rawTable,map<string,RouterEntry>& newTable, map<string, RouterEntry>& currTable, Host src, Host& host)
{
	bool flag=false;
	for (map<string, RouterEntry>::iterator iter=newTable.begin();iter!=newTable.end();iter++)
	{
		RouterEntry newEntry = iter->second;
		if (currTable.find(iter->first)==currTable.end())
		{
			double newCost=newEntry.cost<0?-1:newEntry.cost+rawTable[src.name].cost;
			currTable[iter->first]=
				RouterEntry(newEntry.dst,src.name,newCost,newEntry.seqNum);
			//flag=true;
			continue;
		}
		RouterEntry oldEntry = currTable[iter->first];
		/*
		if (newEntry.dst==src.name)
		{
			currTable[iter->first].seqNum=max(oldEntry.seqNum,newEntry.seqNum);
			continue;
		}
		*/
		if (oldEntry.seqNum==newEntry.seqNum)
		{
			double newCost=newEntry.cost<0?10001:newEntry.cost+rawTable[src.name].cost;
			double oldCost=oldEntry.cost<0?10001:oldEntry.cost;
			if (oldCost-newCost > 0.000001)
			{
				currTable[iter->first]=
					RouterEntry(newEntry.dst,src.name,newEntry.cost+rawTable[src.name].cost,newEntry.seqNum);
			}
			//printf("SRC %s NEW %lf OLD %lf\n",src.name.c_str(),newCost,oldCost);
		}
		else if (oldEntry.seqNum < newEntry.seqNum)
		{
			if (newEntry.dst==host.name) currTable[iter->first].seqNum=newEntry.seqNum+2;
			else
			{
				double newCost=newEntry.cost<0?-1:newEntry.cost+rawTable[src.name].cost;
				//if (iter->first==host.name) host.seqNum=newEntry.seqNum;
				currTable[iter->first]=
						RouterEntry(newEntry.dst,src.name,newCost,newEntry.seqNum);
			}
		}
		/*
		if (!isEqualEntry(oldEntry,currTable[iter->first])) 
		{
			printEntry(oldEntry);
			printEntry(currTable[iter->first]);
			flag=true;
		}
		*/
	}
	/*
	if (flag)
	{
		host.seqNum+=2;
		currTable[host.name].seqNum+=2;
	}
	*/
	
	return flag;
}


/*
*** print the router table in format
*/
int printTable(int number, map<string, RouterEntry>& routerTable, Host host)
{
	printf("## print-out number %d\n",number);
	for (map<string, RouterEntry>::iterator iter=routerTable.begin();iter!=routerTable.end();iter++)
	{
		RouterEntry entry=iter->second;
		double cost=entry.cost;
		if (cost>10000-0.000001) cost=10000.0;
		printf("shortest path to node %s (seq# %d): the next hop is %s and the cost is %lf, %s -> %s: %lf\n",
			entry.dst.c_str(), entry.seqNum, entry.nextHop.c_str(), cost, host.name.c_str(), entry.dst.c_str(), cost);
	}
	return 0;
}