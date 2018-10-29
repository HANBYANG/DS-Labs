#include <string>

using namespace std;

#define BUFFERSIZE 2048

int udpBind(int port);

int udpSend(int fd, int port, string info);

int udpReceive(int fd, string &info);
