#ifndef _TCPKERNEL_H
#define _TCPKERNEL_H


#include"block_epoll_net.h"
#include "Mysql.h"


//类成员函数指针 , 用于定义协议映射表
class TcpKernel;
typedef void (TcpKernel::*PFUN)(int,char*,int nlen);


class TcpKernel
{
public:
    //单例模式
    static TcpKernel* GetInstance();

    //开启核心服务
    int Open(int port);

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
    //获取登录用户好友列表的信息，需要包括自己
    void getUserLIst(int id);
    //根据用户id查询用户信息
    void getlnfoByid(STRU_TCP_FRIEND_INFO* info, int id);
    //处理聊天请求
    void dealChatRq(int clientfd, char*szbuf, int nlen);
    //处理添加好友请求
    void dealAddFriendRq(int clientfd, char*szbuf, int nlen);
    //处理添加好友回复
    void dealAddFriendRs(int clientfd, char*szbuf, int nlen);
    //处理下线请求
    void dealofflioneRq(int clientfd, char*szbuf, int nlen);
    //处理音频帧
    void AudioFrameRq(int clientfd, char *szbuf, int nlen);

    //处理视频帧
    void VideoFrameRq(int clientfd, char *szbuf, int nlen);
    //音频注册
    void AudioRegister(int clientfd, char *szbuf, int nlen);
    //视频注册
    void VideoRegister(int clientfd, char *szbuf, int nlen);

    /*******************************************/
private:
    TcpKernel();
    ~TcpKernel();
    //数据库
    CMysql * m_sql;
    //网络
    Block_Epoll_Net * m_tcp;
    //协议映射表
    PFUN m_netProtocalMap[_DEF_PROTOCAL_COUNT];
    map<int, UserInfo*>m_mapidToSocket;
};

#endif
