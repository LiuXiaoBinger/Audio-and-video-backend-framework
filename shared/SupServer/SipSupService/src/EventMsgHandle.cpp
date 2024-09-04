#include "EventMsgHandle.h"
#include "SipDef.h"
#include "ThreadPool.h"
#include "GetPlamtInfo.h"
#include "GetCatalog.h"
#include "GlobalCtl.h"
#include "OpenStream.h"
void parseReadEvent(struct bufferevent *bev, void *ctx)
{
    LOG(INFO)<<"parseReadEvent";
    char buf[1024] = {0};
    int len = bufferevent_read(bev,buf,4);
    ThreadTask* task = NULL;
     if(len==4)
    {
        /* memset(buf,0,1024);
        int datalen = bufferevent_read(bev,buf,4);
        if(datalen!=4)return; */
        int command = *(int*)buf;
        
        //判断
        LOG(INFO)<<"command:"<<command;
        switch(command)
        {
             case Command_Session_Register:
            {
                task = new GetPlamtInfo(bev);
                break;
            }
            case Command_Session_Catalog:
            {
               /*  memset(buf,0,1024);
                int datalen = bufferevent_read(bev,buf,len-4);
                if(datalen!=(len-4))return;
                string sipid(buf);
                LOG(INFO)<<"sipid:"<<sipid; */
                task = new GetCatalog(bev);
                break;
            }
            case Command_Session_RealPlay:
            {
                task = new OpenStream(bev,&command);
                break;
            }
            case Command_Session_RecordInfo:
            {
                task = new OpenStream(bev,&command);
                break;
            }
            default:
                return;
        }
        if(task != NULL)
            GBOJ(gThPool)->postTask(task);
            
    } 

}
void event_callback(struct bufferevent *bev, short events, void *ctx)
{
    if(events & BEV_EVENT_EOF)
    {
        LOG(ERROR)<<"connection closed";
        bufferevent_free(bev);
    }
}

void onAccept(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *arg)
{
    LOG(INFO)<<"accept client ip:"<<inet_ntoa(((struct sockaddr_in*)address)->sin_addr)<<" port:"<<((struct sockaddr_in*)address)->sin_port<<" fd:"<<fd;
    evutil_make_socket_nonblocking(fd);
    int on = 1;
    setsockopt(fd,SOL_SOCKET,SO_KEEPALIVE,&on,sizeof(on));
    int keepIdle = 60;
    int keepInterval = 20;
    int keepCount = 3;
    setsockopt(fd,SOL_SOCKET,TCP_KEEPIDLE,(void*)&keepIdle,sizeof(keepIdle));
    setsockopt(fd,SOL_SOCKET,TCP_KEEPINTVL,(void*)&keepInterval,sizeof(keepInterval));
    setsockopt(fd,SOL_SOCKET,TCP_KEEPCNT,(void*)&keepCount,sizeof(keepCount));

    event_base* base = (event_base*)arg;
    struct bufferevent* new_buff_event = bufferevent_socket_new(base,fd,BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
    bufferevent_setcb(new_buff_event,parseReadEvent,NULL,event_callback,NULL);
    bufferevent_enable(new_buff_event,EV_READ | EV_PERSIST);
    bufferevent_setwatermark(new_buff_event,EV_WRITE,0,0);    
}
EventMsgHandle::EventMsgHandle(const std::string &servIp, const int &servPort):m_serIp(servIp),m_serPort(servPort)
{
}

EventMsgHandle::~EventMsgHandle()
{
}

int EventMsgHandle::init()
{
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(m_serPort);
    servaddr.sin_addr.s_addr = inet_addr(m_serIp.c_str());

    int ret = evthread_use_pthreads();

    event_base* base = event_base_new();

    struct evconnlistener* listener = evconnlistener_new_bind(base,onAccept,base,
    LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE,1000,
    (struct sockaddr*)&servaddr,sizeof(struct sockaddr_in));
    if(!listener)
    {
        LOG(ERROR)<<"evconnlistener_new bing ERROR";
        return -1;
    }

    event_base_dispatch(base);

    return 0;

}