#include <map>
#include <string>

using namespace std;

struct RouterEntry
{
	RouterEntry(string dst, string nextHop, double cost, int seqNum);
	RouterEntry();
	string dst;       //destination
	string nextHop;   // next hop to the dst
	double cost;      // total length to the dst
	int seqNum;       // sequence number
};

struct Host
{
	string name;     //name of the host
	int seqNum;      //sequence number
	int degree;      //number of neighbours
};

bool isEqualEntry(const RouterEntry& e1, const RouterEntry& e2);

int initRouterTable(string fileName, map<string, RouterEntry>& lastTable, map<string, RouterEntry>& routerTable,map<int, string>& portTable, Host& host);

int packRouterInfo(string& info, map<string, RouterEntry>& routerTable, Host& host);

int unpackRouterInfo(string info, map<string, RouterEntry>& routerTable, Host& host);

int updateRouterTable(map<string,RouterEntry>& rawTable,map<string,RouterEntry>& newTable, map<string, RouterEntry>& currTable, Host src, Host& host);

int printTable(int number, map<string, RouterEntry>& routerTable, Host host);