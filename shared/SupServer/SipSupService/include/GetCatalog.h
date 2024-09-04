#ifndef _GETCATALOG_H
#define _GETCATALOG_H
#include "SipTaskBase.h"
#include "ThreadPool.h"
//请求目录
class GetCatalog : public ThreadTask
{
    public:
        GetCatalog(struct bufferevent* bev);
        ~GetCatalog();
        virtual void run();

        void DirectoryGetPro(void* param);
};

#endif 