#ifndef FILESERVER_H
#define FILESERVER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include<iostream>
#include<vector>
#include<list>
#include<map>
#include"packdef.h"
#define MAX_EVENTS 4096
#include"Thread_pool.h"
#include<queue>
//#include"ckenel.h"
class fileServer;
struct myevent;
 //class TcpKernel;
//数据缓存
struct DataBuffer
{
    DataBuffer( fileServer* _pNet,  int _sock , char* _buf , int _nlen ,myevent *ev):pNet(_pNet),sockfd(_sock),buf(_buf),nlen(_nlen),ev(ev){

    }
    myevent *ev;
    fileServer* pNet;
    int sockfd;
    char* buf;
    int nlen;

};
//接受缓冲区
struct recvbuff{
    recvbuff():pos(0){

    }

    int pos;
    std::vector<char>buff;
};
//接受缓冲区
struct sendbuff{
    sendbuff():pos(0){
    }
    int pos;
    std::vector<char>buff;
};

//事件结构
struct myevent {

    int fd; //cfd listenfd
    int epoll_fd; //epoll_create 句柄
    int events; //EPOLLIN EPLLOUT
    int status;/* status:1表示在监听事件中，0表示不在 */
    int usrid;
    fileServer* pNet;
    recvbuff recvBuff;
    sendbuff sendBuff;
    pthread_mutex_t m_lock;

    pthread_mutex_t m_lock1;
    myevent(fileServer* _pNet)
    {
        this->pNet = _pNet;
        pthread_mutex_init(&m_lock , NULL);
        pthread_mutex_init(&m_lock1 , NULL);
    }
    void eventset(int fd ,int efd/*epoll_create返回的句柄*/)
    {
        this->fd = fd;
        this->events = 0;
        this->status = 0;
        epoll_fd = efd;
    }
    //上监听树
    void eventadd( int events)
    {
        struct epoll_event epv = {0, {0}};
        int op;
        epv.data.ptr = this;
        epv.events = this->events = events;
        if (this->status == 1) {
            op = EPOLL_CTL_MOD;
        }
        else {
            op = EPOLL_CTL_ADD;
            this->status = 1;
        }
        int ret = epoll_ctl(epoll_fd, op, this->fd, &epv);
        if (ret < 0)
            printf("event add failed [fd=%d], events[%d]\n", this->fd, events);

        return;
    }
    //下监听树
    void eventdel()
    {
        struct epoll_event epv = {0, {0}};
        if (this->status != 1)
            return;
        epv.data.ptr = this;
        this->status = 0;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, this->fd, &epv);
        return;
    }
};
struct databuff{

    int clientfd;
    char*data;
    int len;
};
class fileServer
{
public:
    fileServer();
public:

    //初始化 采用回调的方式, 解决数据接收处理
    bool InitNet(void (*recv_callback)( int , char* , int ));
    //epoll事件循环
    void EventLoop();
    //发送数据
    int SendData(int fd, char* szbuf , int nlen );
    //
    static void *senddata(void*arg);

    //socket 设置
    static void setNonBlockFd( int fd );
    static void setRecvBuffSize(int fd);
    static void setSendBuffSize(int fd);
    static void setNoDelay(int fd);
    //线程函数 处理数据包,调用kenel类里面的处理函数
    static void *Buffer_Deal(void *arg);
    //线程函数 接收数据
    static void *recv_task(void *arg);
    //接收处理回调函数，目的事件来了去调用kennel处理函数
    void (*m_recv_callback)( int , char* , int );
    //void send
    //epoll事件处理
    void accept_clien();
    void recv_event(myevent *ev); //接收: 事件到来recv_event --> 接收数据 recv_task -> 处理 Buffer_Deal
    void epollout_event(myevent *ev);
    bool InitThreadPool();
    //监听套接字对应的是事件
    myevent * m_listenEv;
    //监听套接字
    int listensocket;
    //epoll描述赋
    int epoll_fd;

    //每一个套接字 对应一个事件结构
    std::map< int , myevent*> m_mapSockfdToEvent;
    /* 事件循环使用 */
    struct epoll_event events[MAX_EVENTS+1];
    //线程池相关
    thread_pool *my_Threadpool;
    pool_t *m_pool;

     std::map<int,std::queue<databuff*> >fdpack;

};


#endif // FILESERVER_H
