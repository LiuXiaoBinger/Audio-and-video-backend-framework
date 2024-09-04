#include "ECSocket.h"
#include "ECEventPoll.h"

using namespace EC;

int ECSocket::createConnByPassive(int* lsockfd,int localPort,int* timeout)
{
    LOG(INFO)<<"start tcpserver... ";
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1 == sockfd)
    {
        LOG(ERROR)<<"socket create error";
        return -1;
    }
    /*
    在这里我们需要对创建的fd进行复用选项的设定，
    因为当这个fd在后需关闭时会有一个TIME_WAIT的状态，这个是tcp连接关闭过程中的一个状态，
    这个状态持续的时间和操作系统有关,一般为2-4分钟，在这期间端口一直属于被占用的状态，
    所以我们需要设定SO_REUSEADDR参数，让fd在关闭连接后可以达到立刻被重新绑定使用的效果
    */
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char*)&sockfd,sizeof(sockfd));
   
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(localPort);
    if( bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        LOG(ERROR) << "socket bind error..please set ulimit..";
        return -1;
    }
    
    if( listen(sockfd, 20) == -1)
    {
        LOG(ERROR) << "socket listen error..please set ulimit..";
        close(sockfd);
        return -1;
    }

    sockaddr_in clientAddr;  
    socklen_t addrLen = sizeof(clientAddr);  
    
    //这里我们先判断下timeout如果为空那我们就直接调用accept
    //因为fd默认为阻塞模式，那么accept函数会一直等到有client连接进来才会返回
    if(timeout == NULL)
    {   
        int connfd = accept(sockfd, (struct sockaddr *)&clientAddr, &addrLen);
        if(connfd != -1)
        {
            //将监听fd传出
            *lsockfd = sockfd;
            return connfd;
        }
        else
        {
            close(sockfd);
            return -1;
        }
    }

    //如果设定了超时时间，那么我们就使用io复用的机制来处理
    EventPoll eventPoll;
    eventPoll.init(2);
    eventPoll.addEvent(sockfd,EC_POLLIN);

    int ret = -1;
    vector<PollEventType> events;
    //这里我们就需要轮询的来返回就绪的事件
    
    while(true)
    {
        StatusType status;
        status = eventPoll.poll(events,timeout);
        LOG(INFO)<<"ENENT SIZE::"<<events.size();
        if(status== ST_OK)
        {
            for(int i = 0;i<events.size();i++)
            {
                if(events[i].sockfd == sockfd) 
                {
                    int connfd = accept(sockfd, (struct sockaddr *)&clientAddr, &addrLen);
                    if(connfd != -1)
                    {
                        LOG(INFO)<<"ENENT TYPE::"<<events[i].outEvents;
                        *lsockfd = sockfd;
                        return connfd;
                    }
                    else
                    {
                        close(sockfd);
                        return -1;
                    }
                }

            }
        }
        /* else if(ret == 0)
        {
            LOG(INFO)<<"TIME OUT";
            break;
        } */
        else if(status==ST_SYSERROR)
        {
            LOG(INFO)<<"poll ERROR";
            break;
        }
    }
    eventPoll.removeEvent(sockfd);
    close(sockfd);
    return ret;

}

int ECSocket::createConnByActive(string dstip,int dstport,int localPort,int* timeout)
{
    LOG(INFO)<<"tcpclient connect...";

    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1 == sockfd)
    {
        LOG(ERROR)<<"socket create error";
        return -1;
    }
    //设置socket复用，允许立即重用，忽略返回值
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char*)&sockfd,sizeof(sockfd));
     setNonblocking(sockfd);
    struct sockaddr_in client_addr;
    memset(&client_addr,0,sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port = htons(localPort);
    if( bind(sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr)) == -1)
    {
        LOG(ERROR) << "socket bind error..please set ulimit..";
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(dstip.c_str());
    server_addr.sin_port = htons(dstport);

    int status=-1;
    if (timeout == NULL)  
    {  
        //发送SYN
        status = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));  
        if (status < 0)
        {
            LOG(ERROR) << "connect error,errno:" << errno;
            close(sockfd);
            return status;
        }
        setlocking(sockfd);
        return sockfd;  
    }  

    //如果我们指定了超时时间，那么我们就将连接的fd交由我们封装的多路io复用函数进行管理
    status = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));  
    if (status == 0)
        {
            setlocking(sockfd);
            LOG(ERROR) << "connect to,server successfully:" ;
            return sockfd;  
            
        }
        if(status==-1)
        {
            if(errno==EINTR)
            {
                LOG(ERROR) << "connect interruptted by signal ,try again:" << errno;
            }
            else if(errno== EINPROGRESS)//链接正在尝试中
            {

            }
            else
            {
                LOG(ERROR) << "connect error:" << errno;
                close(sockfd);
                return status;
            }
            
        }

    EventPoll eventPoll;
    eventPoll.init(2);
    eventPoll.addEvent(sockfd,EC_POLLOUT);
    int ret = -1;
	vector<PollEventType> events;
    while(true)
    {
        StatusType status = eventPoll.poll(events,timeout);
        if(status == ST_OK)
        {
            for(int i = 0;i<events.size();i++)
            {
                if(events[i].sockfd == sockfd) 
                {
                    setlocking(sockfd);
                    LOG(ERROR) << "connect to,server successfully:" ;
                    return sockfd;
                }
                else
                {
                    return -1;
                }

            }
        }
       /*  else if(status == ST_TIMEOUT)
        {
            LOG(INFO)<<"TIME OUT";
            break;
        } */
        else if(status == ST_SYSERROR)
        {
            LOG(INFO)<<"poll ERROR";
            break;
        }
    }
    eventPoll.removeEvent(sockfd);
    close(sockfd);
    return -1;
}

int EC::ECSocket::setNonblocking(int sockfd)
{
    int opts=fcntl(sockfd,F_GETFL);
    if(opts<0)return -1;
    opts=opts|O_NONBLOCK;
    if(fcntl(sockfd,F_SETFL,opts)<0)
    {
        return -1;
    }
    return 0;
}

int EC::ECSocket::setlocking(int sockfd)
{
    int opts=fcntl(sockfd,F_GETFL);
    if(opts<0)return -1;
    opts=opts&~O_NONBLOCK;
    if(fcntl(sockfd,F_SETFL,opts)<0)
    {
        return -1;
    }
    return 0;
}
