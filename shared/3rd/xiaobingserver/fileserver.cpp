#include "fileserver.h"
using namespace std ;
#include<netinet/tcp.h>
fileServer::fileServer()
{

}

//初始化 采用回调的方式, 解决数据接收处理
bool fileServer::InitNet(void (*recv_callback)( int , char* , int )){
    m_listenEv=new myevent(this);
    m_recv_callback = recv_callback;
    //    my_Threadpool=new thread_pool;
    //    if(my_Threadpool->Pool_create(200,10,5000)==false){
    //        //创建线程池失败
    //        cout<<"创建线程池失败"<<endl;
    //    }
    InitThreadPool();
    //creat a tcp socket
    listensocket = socket(AF_INET, SOCK_STREAM, 0);
    if ( listensocket  == -1 ){
        perror("create socket error");
        return false;
    }
    int flag=1;

    int err = 0;
    //set REUSERADDR
    err = setsockopt(listensocket, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
    if ( err == -1 ){
        perror("setsockopt error");
        return false;
    }
    //监听套接字listensocket 采用 LT 非阻塞模式
    //set NONBLOCK
    setNonBlockFd( listensocket );

    struct sockaddr_in local_addr;
    bzero( &local_addr , sizeof(sockaddr_in) );
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(_DEF_TCP_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    //bind addr
    err = bind(listensocket, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
    if( err == -1 ) {
        perror("bind error");
        close(listensocket);
        return false;
    }
    //监听
    if (listen(listensocket, 128) == -1 ){
        perror("listen error");
        close(listensocket);
        return false;
    }

    //create epoll
    epoll_fd = epoll_create( MAX_EVENTS );

    m_listenEv->eventset(listensocket ,epoll_fd );
    //将监听套接字 添加到epoll中 , 模式LT 非阻塞
    m_listenEv->eventadd( EPOLLIN);
    printf("ceeee\n");

    return true;
}
bool fileServer::InitThreadPool()
{
    m_pool = NULL;
    my_Threadpool = new thread_pool;

    //创建拥有10个线程的线程池 最大线程数200 环形队列最大值50000
    if((m_pool = (my_Threadpool->Pool_create(200,10,50000))) == NULL)
    {
        perror("Create Thread_Pool Failed:");
        exit(-1);
    }

    return true;
}
//epoll事件循环
void fileServer::EventLoop(){

    int i;
    while(1){
        int fd =epoll_wait(epoll_fd,events,MAX_EVENTS+1,-1);
        if(fd<0){
            cout<<"epoll_wait：失败"<<endl;
            return ;
        }
        for(i=0;i<fd;i++){
            myevent*ev=(myevent*)events[i].data.ptr;
            int fds = ev->fd;
            if ( (events[i].events & EPOLLIN) ) {
                if( fds == listensocket ){

                    accept_clien();
                    cout<<"accept_event"<<endl;
                }
                else
                {
                    printf("EPOLLIN\n");
                    recv_event( ev );
                }
                //printf("kk\n");
            }
            if ((events[i].events & EPOLLOUT) ) {
                epollout_event( ev );
            }
        }
    }

}

//epoll事件处理
void fileServer::accept_clien(){
    struct sockaddr_in caddr;
    socklen_t len = sizeof(caddr);
    int clientfd ;
    if ((clientfd = accept(listensocket, (struct sockaddr *)&caddr, &len)) == -1) {
        if (errno != EAGAIN && errno != EINTR) {
        }
        printf("%s: accept, %s\n", __func__, strerror(errno));
        return;
    }
    //    //设置非阻塞
    setNonBlockFd( clientfd );

    //设置接收缓冲区大小
    setRecvBuffSize( clientfd );
    //设置发送缓冲区大小
    setRecvBuffSize( clientfd );
    //设置 无延迟nagel
    setNoDelay( clientfd );

    myevent * clientEv = new myevent(this);
    clientEv->eventset( clientfd , epoll_fd );
    // 使用EPOLLONESHOT epoll监听不会重复检测 避免多线程并发 使得同一个套接字接收是排队的
    clientEv->eventadd(  EPOLLIN|EPOLLET|EPOLLONESHOT );

    m_mapSockfdToEvent[clientfd] = clientEv;

    printf("new connect [%s:%d][time:%ld] \n",
           inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), time(NULL) );
    return;
}
//接收: 事件到来recv_event --> 接收数据 recv_task -> 处理 Buffer_Deal
void fileServer::recv_event(myevent *ev){
    printf("recv_event\n");
    //接收事件采用线程池 多线程处理, 由于使用EPOLLONESHOT , 避免同一套机字的线程并发问题
    my_Threadpool->Producer_add(m_pool ,recv_task , (void*) ev );
}

void fileServer::epollout_event(myevent *ev){

}

void* fileServer::recv_task(void* arg)
{
    cout<<"recv_task"<<endl;
    //    //如何读取请求?不要阻塞循环读取,导致客户端与线程池中的线程绑定，该线程持续被占用Epoll +非阻塞读取.socket
    //    //recv调用一次并不一定会读取足够的数据,所以需要循环读取+非阻塞
    myevent* ev = (myevent*)arg;
    //利用全局指针 方便操作
    fileServer * pthis = ev->pNet;
    pthread_mutex_lock(&ev->m_lock );
    if(ev->recvBuff.buff.size()>ev->recvBuff.pos){
        vector <char>mark(ev->recvBuff.buff.begin()+ev->recvBuff.pos,ev->recvBuff.buff.end());
        ev->recvBuff.buff.swap(mark);
        ev->recvBuff.pos=0;

    }

    else if(ev->recvBuff.buff.size()==ev->recvBuff.pos){
        ev->recvBuff.pos=0;
        vector <char>mark;
        ev->recvBuff.buff.swap(mark);
    }

    char markbuff[17*1024];
    int num=0;
    int nRelReadNum = 0;
    bzero(markbuff,17*1024);



    while((nRelReadNum = recv(ev->fd,&markbuff,sizeof(markbuff),MSG_DONTWAIT)) !=0){

        if(nRelReadNum==0){
            cout<<"aaaaaaaaaaaaaaaaaa"<<nRelReadNum<<endl;
            ev->eventdel();
            close(ev->fd);
            pthread_mutex_unlock(&ev->m_lock );

            //回收event结构
            pthis->m_mapSockfdToEvent.erase( ev->fd );
            delete ev;

            return NULL;
        }

        if(nRelReadNum<0){
            if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                break;
            //cout<<"sdasd"<<endl;

            cout<<"aaaaaaaaaaaaaaaaaa"<<nRelReadNum<<endl;
            //pthis->ckenel->delbreadpoint(ev->usrid);
            abnormal_Offline rq;
            rq.id=ev->usrid;
            pthis->m_recv_callback(ev->fd,(char*)&rq,sizeof(rq));
            sleep(2);
            ev->eventdel();
            close(ev->fd);
            pthread_mutex_unlock(&ev->m_lock );

            //回收event结构
            pthis->m_mapSockfdToEvent.erase( ev->fd );
            delete ev;


            return NULL;

        }
        num+=nRelReadNum;
        if(num>ev->recvBuff.buff.capacity()){
            ev->recvBuff.buff.reserve( num);
        }
        for(int i=0;i<nRelReadNum;i++)
            ev->recvBuff.buff.push_back(markbuff[i]);
        //cout<<"aaaaaaaaaaaaaaaaaa"<<nRelReadNum<<endl;
        bzero(markbuff,17*1024);
    }

    DataBuffer * buffer = new DataBuffer(ev->pNet , ev->fd , "dfsd" , 5 ,ev);
    pthis->my_Threadpool->Producer_add(pthis->m_pool,Buffer_Deal , (void*) buffer );
    //cout<<"sdasd"<<endl;
    if(ev!=NULL)
        ev->eventadd(ev->events);
    pthread_mutex_unlock(&ev->m_lock );
    {
    //    //接收和处理分离
    //    int nRelReadNum = 0;
    //    int nPackSize = 0;
    //    char *pSzBuf = NULL;


    //    while(nRelReadNum = recv(ev->fd,&nPackSize,sizeof(nPackSize),MSG_DONTWAIT) ){

    //        if(nRelReadNum = 0){
    //            ev->eventdel();
    //            close(ev->fd);
    //            //回收event结构
    //            pthis->m_mapSockfdToEvent.erase( ev->fd );
    //            delete ev;
    //            ev=NULL;
    //            return NULL;
    //                    break;
    //        }

    //        if(nRelReadNum<0){
    //            break;
    //        }
    //        //cout<<"packsize:"<<nPackSize<<endl;
    //        pSzBuf = new char[nPackSize];
    //        int nOffSet = 0;
    //        nRelReadNum = 0;
    //        // 接收包的数据
    //        while(nPackSize){
    //             nRelReadNum = recv(ev->fd,pSzBuf+nOffSet,nPackSize,0);
    //             if(nRelReadNum ==0)
    //                        break;
    //            if(nRelReadNum>0){
    //             nOffSet += nRelReadNum;
    //             cout<<nRelReadNum<<endl;
    //             nPackSize -= nRelReadNum;
    //            }
    //            cout<<nRelReadNum<<endl;
    //        }
    //        //接收和处理分离 跑线程池里其他线程处理 , 避免处理影响接收
    //        DataBuffer * buffer = new DataBuffer(ev->pNet , ev->fd , pSzBuf , nOffSet );
    //        pthis->my_Threadpool->Producer_add(pthis->m_pool,Buffer_Deal , (void*) buffer );

    //        //pthis->m_recv_callback(ev->fd,pSzBuf,nOffSet);
    //       // delete[]pSzBuf;
    //        printf("fenpei\n");
    //    }

    //    // 这次接收完 要重新注册事件  此时 EPOLL MODE -> EPOLLIN|EPOLLET|EPOLLONESHOT 没有修改, 使用重复值
    //    if(ev!=NULL)
    //    ev->eventadd(ev->events);

}
}
//数据包处理
void * fileServer::Buffer_Deal( void * arg )
{
    cout<<"Buffer_Deal::"<<endl;
    DataBuffer * buffer = (DataBuffer *)arg;
    if( !buffer ) return NULL;

    //    buffer->pNet->m_recv_callback(buffer->sockfd,buffer->buf,buffer->nlen);
    //    if(buffer->buf != NULL)
    //    {
    //        delete [] buffer->buf;
    //        buffer->buf = NULL;
    //    }
    //    delete buffer;
    //    return 0;
    cout<<"hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"<<endl;
    char *pSzBuf = NULL;
    if(buffer->pNet->m_mapSockfdToEvent.count( buffer->ev->fd )){
    pthread_mutex_lock(&buffer->ev->m_lock );
    }else{
        return NULL;
    }
    int i=0;

    while(buffer->ev){
        char* buf = &*buffer->ev->recvBuff.buff.begin()+buffer->ev->recvBuff.pos;
        //if(buffer->ev->recvBuff.buff.size()-buffer->ev->recvBuff.pos<4)break;
        int nPackSize=*(int*)buf;
        perror("yyyy");
        cout<<"ggggggggggggggggggggggggggggggggggggggggg"<<endl;
              // cout<<"buffer->ev->recvBuff.buff.size()::"<<buffer->ev->recvBuff.buff.size()<<endl;
               //cout<<"(int)(buffer->ev->recvBuff.buff.size()-(buffer->ev->recvBuff.pos+4))::"<<(int)(buffer->ev->recvBuff.buff.size()-(buffer->ev->recvBuff.pos+4))<<endl;
        if(buffer->ev->recvBuff.buff.size()!=buffer->ev->recvBuff.pos&&(int)(buffer->ev->recvBuff.buff.size()-(buffer->ev->recvBuff.pos+4))>=nPackSize){
            cout<<"packsize::"<<nPackSize<<endl;
            pSzBuf = new char[nPackSize];
            memcpy(pSzBuf,buf+4,nPackSize);
            buffer->pNet->m_recv_callback(buffer->sockfd,pSzBuf,nPackSize);
            cout<<++i<<endl;
            delete []pSzBuf;
            buffer->ev->recvBuff.pos+=(4+nPackSize);
            //cout<<"buffer->ev->recvBuff.pos::"<<buffer->ev->recvBuff.pos<<endl;
        }
        else
            break;

    }
    if(buffer->pNet->m_mapSockfdToEvent.count( buffer->ev->fd ))
    pthread_mutex_unlock(&buffer->ev->m_lock );
    delete buffer;
    return 0;
}

//发送数据
int fileServer::SendData(int fd, char* szbuf , int nlen ){
    myevent* ev = m_mapSockfdToEvent[fd];
    pthread_mutex_lock(&ev->m_lock1);
    int nPackSize = nlen + 4;
    vector<char> vecbuf( nPackSize , 0);
    //vecbuf.resize( nPackSize );
    char* buf = &* vecbuf.begin();
    char* tmp = buf;
    *(int*)tmp = nlen;//按四个字节int写入
    tmp += sizeof(int );
    memcpy( tmp , szbuf , nlen );
    //std::vector<char>buff;
    //buff.push_back();
    for(int i=0;i<nPackSize;i++){
        m_mapSockfdToEvent[fd]->sendBuff.buff.push_back(vecbuf[i]);
    }
    //ev->sendBuff.buff.insert(ev->sendBuff.buff.end(),vecbuf.begin(),vecbuf.end());
    pthread_mutex_unlock(&ev->m_lock1);
    //sdas.erase();
//    fd_set set;
//    FD_ZERO(&set);
//    FD_SET(fd,&set);
    //delete[]szbuf;
    DataBuffer * buffer = new DataBuffer(ev->pNet , ev->fd , "dfsd" , 5 ,ev);
    my_Threadpool->Producer_add(m_pool,this->senddata , (void*) buffer );

    return 0;
}
void* fileServer::senddata(void*arg){
    //cout<<"Buffer_Deal::"<<endl;
    DataBuffer * buffer = (DataBuffer *)arg;
    if( !buffer ) return NULL;
    int nPackSize=0;
    int res;
    pthread_mutex_lock(&buffer->ev->m_lock1);
    //cout<<buffer->ev->sendBuff.buff.size()<<endl;
    while(1){
        if(buffer->ev->sendBuff.pos==buffer->ev->sendBuff.buff.size()){
            break;
        }
        char* buf = &*buffer->ev->sendBuff.buff.begin()+buffer->ev->sendBuff.pos;
        nPackSize=buffer->ev->sendBuff.buff.size()-buffer->ev->sendBuff.pos;
        res= send( buffer->ev->fd,buf, nPackSize ,MSG_NOSIGNAL);
        //        if(res==0){
        //           fdpack.erase(fd);
        //        }
        if(res<0){
            if(errno == EAGAIN)break;
            if(errno == EINTR || errno == EWOULDBLOCK)
                continue;
        }
        if(res>0)
        buffer->ev->sendBuff.pos+=res;
        //cout<<buffer->ev->sendBuff.pos<<endl;
        //cout<<"发送成功"<<*(int*)buf<<res<<endl;

    }
    //防止缓冲器过大
    if(buffer->ev->sendBuff.buff.size()>buffer->ev->sendBuff.pos){
        vector <char>mark(buffer->ev->sendBuff.buff.begin()+buffer->ev->sendBuff.pos,buffer->ev->sendBuff.buff.end());
        buffer->ev->sendBuff.buff.swap(mark);
        buffer->ev->sendBuff.pos=0;

    }
    else if(buffer->ev->sendBuff.buff.size()==buffer->ev->sendBuff.pos){
        buffer->ev->sendBuff.pos=0;
        vector <char>mark;
        buffer->ev->sendBuff.buff.swap(mark);
    }
    pthread_mutex_unlock(&buffer->ev->m_lock1);
}

//将客户端socket设置成非阻塞io
void fileServer::setNonBlockFd(int fd)
{
    int flags = 0;
    flags = fcntl(fd, F_GETFL, 0);
    int ret = fcntl(fd, F_SETFL, flags|O_NONBLOCK);
    if( ret == -1)
        perror("setNonBlockFd fail:");
}
//接受缓冲器扩容
void fileServer::setRecvBuffSize( int fd)
{
    //接收缓冲区
    int nRecvBuf = 256*1024;//设置为 256 K
    setsockopt(fd,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
}
//发送缓冲区扩容
void fileServer::setSendBuffSize( int fd)
{
    //发送缓冲区
    int nSendBuf=128*1024;//设置为 128 K
    setsockopt(fd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
}
//关闭掉默认的nagol算法，减少网络拥堵，但是客户端服务器交互更频繁
void fileServer::setNoDelay( int fd)
{
    //nodelay
    int value = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY ,(char *)&value, sizeof(int));
}
