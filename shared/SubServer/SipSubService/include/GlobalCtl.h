#ifndef _GLOBALCTL
#define _GLOBALCTL
#include "Common.h"
#include "SipLocalConfig.h"
#include "ThreadPool.h"
#include "SipCore.h"
#include "SipDef.h"
class GlobalCtl;
#define GBOJ(obj) GlobalCtl::instance()->obj

static pj_status_t pjcall_thread_register(pj_thread_desc &desc)
{
    pj_thread_t *tread = 0;
    if (!pj_thread_is_registered())
    {
        return pj_thread_register(NULL, desc, &tread);
    }
    return PJ_SUCCESS;
}
class GlobalCtl
{
public:
    typedef struct _SupDomainInfo
    {
        _SupDomainInfo()
        {
            sipId = "";
            addrIp = "";
            sipPort = 0;
            protocal = 0;
            registered = 0;
            expires = 0;
            usr = "";
            pwd = "";
            isAuth = false;
            realm = "";
        }
        string sipId;
        string addrIp;
        int sipPort;
        int protocal;
        int registered;
        int expires;
        bool isAuth;
        string usr;
        string pwd;
        string realm;
    } SupDomainInfo;
    typedef list<SupDomainInfo> SUPDOMAININFOLIST;
    static void get_global_mutex()
    {
        pthread_mutex_lock(&globalLock);
    }

    static void free_global_mutex()
    {
        pthread_mutex_unlock(&globalLock);
    }
    static pthread_mutex_t globalLock;
    static bool _gStopPool;
private:
    /* data */
    GlobalCtl(/* args */)
    {
    }
    GlobalCtl(const GlobalCtl &data) = delete;
    const GlobalCtl &operator=(const GlobalCtl &data) = delete;
    ~GlobalCtl();
    static GlobalCtl *m_instance;
    SUPDOMAININFOLIST _supDomainInfoList;

public:
    static GlobalCtl *instance();
    bool init(void *param);
    SipLocalConfig *_gConfig;
    ThreadPool *gThPool = NULL;
    SipCore *gSipServer = NULL;

    SUPDOMAININFOLIST &getSupDomainInfoList()
    {
        return _supDomainInfoList;
    }
    pjsip_endpoint *get_pjsip_endpoint()
    {
        return gSipServer->get_pjsip_endpoint();
    }

    static DevTypeCode getSipDevInfo(string id);
};

#endif