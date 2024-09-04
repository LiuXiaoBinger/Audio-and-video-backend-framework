#ifndef _OPENSTREAM_H
#define _OPENSTREAM_H
#include "TaskTimer.h"
#include "Common.h"
#include "ThreadPool.h"
//rtp负载类型定义
static string rtpmap_ps = "96 PS/90000";
class OpenStream :public ThreadTask
{
    public:
        OpenStream(struct bufferevent* bev,void* arg);
        ~OpenStream();

        //void StreamServiceStart();
        static void CheckSession(void* param);
        virtual void run();
         void StreamGetProc(void* param);
		
		 static void StreamStop(string platformId, string devId);
    private:
        TaskTimer* m_pStreamTimer;
        TaskTimer* m_pCheckSessionTimer;
};

#endif