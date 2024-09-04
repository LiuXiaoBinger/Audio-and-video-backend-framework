#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <stdio.h>
using namespace std;
class ECSocket
{
public:
	static int createConn(string dstip, int dstport);
	static int sendData(int fd, const char* buf, int buflen, int* timeout = NULL);
	static int recvData(int fd, string& payload, int* timeout = NULL);

private :
	ECSocket() {

	}
	~ECSocket() {

	}
};

