#ifndef _SIPCORE
#define _SIPCORE
#include <pjlib-util.h>
#include <pjmedia.h>
#include <pjsip.h>
#include <pjsip_ua.h>
#include <pjsip/sip_auth.h>
#include <pjlib.h>
#include "SipTaskBase.h"
#include "tinyxml2.h"
typedef struct _threadParam
{
    _threadParam()
    {
        base = NULL;
        data = NULL;
    }
    ~_threadParam()
    {
        if(base)
        {
            delete base;
            base = NULL;
        }
        if(data)
        {
            pjsip_rx_data_free_cloned(data);
            base = NULL;
        }
    }
    SipTaskBase* base;
    pjsip_rx_data* data;
}threadParam;
class SipCore
{
public:
    SipCore();
    ~SipCore();

    // 初始化传输层接口 pjsip传输层支持udp tcp tls
    pj_status_t init_transport_layer(int sipPort);

    bool initSip(int sipPort);
    pjsip_endpoint*get_pjsip_endpoint(){
        return _endpoint;
    }
    pj_pool_t* GetPool(){return m_pool;}
    public:
        static void *dealTaskThread(void* arg);
private:
    pj_caching_pool _caching_pool;
    pjmedia_endpt* m_mediaEndpt;
    pjsip_endpoint *_endpoint;
    pj_pool_t* m_pool;
};

#endif