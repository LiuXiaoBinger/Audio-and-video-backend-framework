#ifndef _TCPKERNEL_H
#define _TCPKERNEL_H

#include <thread>
#include"fileserver.h"
#include "mysql.h"
#include <queue>
using namespace std;
//#include
//类成员函数指针 , 用于定义协议映射表
  class TcpKernel;
typedef void (TcpKernel::*PFUN)(int,char*,int nlen);
template<class K , class V>
struct MyMap
{
public:
    MyMap(){
        pthread_mutex_init(&m_lock , NULL);
    }
    map<K , V> m_map;
    //获取的结果 找不到 如果是对象 v  如果是指针 应该是NULL 规定该函数使用时, 调用是确保一定有
    V find( K k)
    {
        pthread_mutex_lock(&m_lock );
        V v = m_map[k];
        pthread_mutex_unlock(&m_lock );
        return v;
    }
    void insert( K k , V v)
    {
        pthread_mutex_lock(&m_lock );
        m_map[k] = v;
        pthread_mutex_unlock(&m_lock );
    }
    void erase(K k )
    {
        pthread_mutex_lock(&m_lock );
        m_map.erase(k);
        pthread_mutex_unlock(&m_lock );
    }
    bool IsExist( K k )
    {
        bool flag = false;
        pthread_mutex_lock(&m_lock );
        if( m_map.count(k) > 0 )
            flag = true;
        pthread_mutex_unlock(&m_lock );
        return flag;
    }
private:
    pthread_mutex_t m_lock;
};

template<class Data>
struct Mydataqueue
{
public:
    Mydataqueue(){
        pthread_mutex_init(&m_lock , NULL);
    }
    queue<Data> m_queue;
    //获取的结果 找不到 如果是对象 v  如果是指针 应该是NULL 规定该函数使用时, 调用是确保一定有
    Data front()
    {
        pthread_mutex_lock(&m_lock );
        Data data = m_queue.front();
        pthread_mutex_unlock(&m_lock );
        return data;
    }
    void push(Data data)
    {
        pthread_mutex_lock(&m_lock );
        m_queue.push(data);
        pthread_mutex_unlock(&m_lock );
    }
    void pop( )
    {
        pthread_mutex_lock(&m_lock );
        m_queue.pop();
        pthread_mutex_unlock(&m_lock );
    }
    bool IsExist()
    {
        bool flag = false;
        pthread_mutex_lock(&m_lock );
        if( m_queue.size() > 0 )
            flag = true;
        pthread_mutex_unlock(&m_lock );
        return flag;
    }
    int getsize(){
        pthread_mutex_lock(&m_lock );
        int size=m_queue.size();
        pthread_mutex_unlock(&m_lock );
        return size;
    }
private:
    pthread_mutex_t m_lock;
};
typedef struct downbreakpoint{
    int fd;
    int m_id;
    int switch_down;
    string dir;
    string filename;
    string filedir;
    string breakpoint;
    string md5;
}downbreakpoint;

typedef struct UserInfo
{
    UserInfo()
    {
        m_sockfd = 0;
        m_id = 0;
       // memset(m_userName, 0 , 1024);
        //memset(m_userfile, 0 , 512);
    }
    int  m_sockfd;
    int  m_id;
    string uersname;
    string filename[5];
    std::map<string,downbreakpoint*>downfile;

}UserInfo;
typedef struct fdpos{
    fdpos(){
        fd=0;
        pos=0;
        m_id=0;
    }
    int m_id;
    int fd;
    int pos;
    int filesize;
    string filename;
    string md5;
    string dir;
    string clientdir;
}fdpos;
//
typedef struct threaddownfile{
    threaddownfile(){
        memset(dir,0,512);
        memset(filename,0,512);
    }
    int usrid;
    int clienfd;
    int pos;
    char dir[512];
    char filename[512];
}threaddownfile;
class TcpKernel
{
public:
    //单例模式
    static TcpKernel* GetInstance();

    //开启服务
    int Open();

    //设置协议映射
    void setNetPackMap();
    //关闭核心服务
    void Close();
    //处理网络接收
    static void DealData(int clientfd, char*szbuf, int nlen);
    //事件循环
    void EventLoop();
    //发送数据
    void SendData( int clientfd, char*szbuf, int nlen );

    /************** 网络处理 *********************/
    //注册
    void dealRedisteRq(int clientfd, char*szbuf, int nlen);
    //登录
    void dealLonginRq(int clientfd, char*szbuf, int nlen);
    //处理下线请求
    void dealofflioneRq(int clientfd, char*szbuf, int nlen);

    //处理上传文件
    void dealupfilerq(int clientfd, char*szbuf, int nlen);
    void dealupfile(int clientfd, char*szbuf, int nlen);
    //文件上传完成
    void Finish_upfile(int clientfd, char*szbuf, int nlen);
    //处理上传文件属性
    //void dealofflioneRq(int clientfd, char*szbuf, int nlen);

    //获取用户文件，反馈给客户端
    void getUserfileLIst(int userid );

    //根据用户id查询用户信息
    void getlnfoByid(STRU_TCP_FRIEND_INFO* info, int id);
    //处理断点传序
    void delbreadpoint(int clientfd, char*szbuf, int nlen);

    //处理下载请求
    void deldownrq(int clientfd, char*szbuf, int nlen);
    //将要下载文件放到一个线程处理
    void deldownfile_rs(void* arg);
    //获取上传断点传需
    void getupbreadpoint(int);

    //获取下载断点传需
    void getdownbreadpoint(int);
    std::thread deldownfile(void* arg);//对handleEvent包装，然后可以使用C++11的多线程
    /*******************************************/
private:
    TcpKernel();
    ~TcpKernel();
    //数据库
    CMysql * m_sql;
    //网络;
    fileServer *m_tcp;
    //协议映射表
    PFUN m_netProtocalMap[100];
    map<int, UserInfo*>m_mapidToSocket;
    std::map<std::string,fdpos*>file;


    //map<int, UserInfo*>m_mapidToSocket;
    //int dfd;
    char*ptr;
//    Mydataqueue<databuff*>recvbuff;
//    Mydataqueue<databuff*>sendbuff;

    pthread_mutex_t m_filewrlock;
    pthread_mutex_t xiaxianlock;
};

#endif
