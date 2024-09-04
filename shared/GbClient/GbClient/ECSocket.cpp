#include "ECSocket.h"

int ECSocket::createConn(string dstip, int dstport)
{
	//我们需要先定义一个结构体，这个结构是用于保存windows初始化数据，里边包括了版本号还有支持的网络特别字的选项等等
	WSADATA data;
	WSAStartup(0x202, &data);

	SOCKET fd = -1;
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		cout << "create socket error" << fd << endl;
		return fd;
	}
	//创建IPv4的结构体
	struct  sockaddr_in _addr;
	memset(&_addr, 0, sizeof(sockaddr_in));
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = inet_addr(dstip.c_str());
	_addr.sin_port = htons(dstport);

	//链接
	if (connect(fd, (struct sockaddr*)&_addr, sizeof(sockaddr_in)))
	{
		closesocket(fd);
		return -1;
	}
	return fd;

}

int ECSocket::sendData(int fd, const char * buf, int buflen, int * timeout)
{
	if (fd < 0)
	{
		return -1;
	}

	if (timeout != NULL && *timeout != 0)
	{
		setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)& timeout, sizeof(timeout));
	}
	int ret = send(fd, buf, buflen, 0);
	if (ret <= 0)
	{
		return -1;
	}
	return ret;
}

int ECSocket::recvData(int fd, string & payload, int * timeout)
{
	if (fd <= 0)
		return -1;

	if (timeout != NULL && *timeout != 0)
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

	char buf[1024];
	memset(buf, 0, 1024);
	int ret = recv(fd, buf, 4, 0);
	int bodylen = *(int*)buf;
	int len = 0;
	while (len < bodylen)
	{
		memset(buf, 0, 1024);
		int recv_len = recv(fd, buf + len, bodylen - len, 0);
		if (recv_len < 0)
			break;

		len += recv_len;
		payload += buf;
	}
	closesocket(fd);
	return len;
}
