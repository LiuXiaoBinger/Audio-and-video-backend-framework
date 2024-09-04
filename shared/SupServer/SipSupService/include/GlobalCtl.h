#ifndef _GLOBALCTL
#define _GLOBALCTL
#include "Common.h"
#include "SipLocalConfig.h"
#include "ThreadPool.h"
#include "SipCore.h"
#include "SipDef.h"
#include <time.h>
#include <random>
#include <sstream>
#include <unistd.h>
#include <sys/time.h>
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
class Session
{
public:
    Session(const DeviceInfo &devInfo)
    {
        devid = devInfo.devid;
        platformId = devInfo.playformId;
        streamName = devInfo.streamName;
        setupType = devInfo.setupType;
        protocal = devInfo.protocal;
        startTime = devInfo.startTime;
        endTime = devInfo.endTime;
        gettimeofday(&m_curTime, NULL);
        rtp_loaclport = 0;
        bev = devInfo.bev;
    }

    virtual ~Session()
    {
    }

public:
    string devid;
    string platformId;
    string streamName;
    string setupType;
    int protocal;
    int startTime;
    int endTime;
    timeval m_curTime;
    int rtp_loaclport;
    struct bufferevent* bev;
};
class GlobalCtl
{
public:
    typedef struct _SubDomainInfo
    {
        _SubDomainInfo()
        {
            sipId = "";
            addrIp = "";
            sipPort = 0;
            protocal = 0;
            registered = false;
            expires = 0;
            lastRegTime = 0;
            auth = false;
        }

        bool operator==(string id)
        {
            return (this->sipId == id);
        }
        string sipId;
        string addrIp;
        int sipPort;
        int protocal;
        bool registered;    // 是否是登录状态
        int expires;        // 注册有效时间
        time_t lastRegTime; // 注册的时间
        bool auth;          // 是否是健权
    } SubDomainInfo;
    typedef list<SubDomainInfo> SUBDOMAININFOLIST;
    static void get_global_mutex()
    {
        pthread_mutex_lock(&globalLock);
    }

    static void free_global_mutex()
    {
        pthread_mutex_unlock(&globalLock);
    }

private:
    /* data */
    GlobalCtl(/* args */)
    {
    }
    GlobalCtl(const GlobalCtl &data) = delete;
    const GlobalCtl &operator=(const GlobalCtl &data) = delete;
    ~GlobalCtl();
    static GlobalCtl *m_instance;

public:
    typedef list<Session *> ListSession;
    static ListSession glistSession;
    static pthread_mutex_t gStreamLock;

    static pthread_mutex_t globalLock;
    static bool _gStopPool;
    static bool gRcvIpc;
    static GlobalCtl *instance();
    bool init(void *param);
    SipLocalConfig *_gConfig;
    ThreadPool *gThPool = NULL;
    SipCore *gSipServer = NULL;
    static SUBDOMAININFOLIST _subDomainInfoList;
    SUBDOMAININFOLIST &getSubDomainInfoList()
    {
        return _subDomainInfoList;
    }
    pjsip_endpoint *get_pjsip_endpoint()
    {
        return gSipServer->get_pjsip_endpoint();
    }
    static bool checkIsExist(string id);
    static bool checkIsVaild(string id);
    static void setExpires(string id, int expires);
    static void setRegister(string id, bool registered);
    static void setLastRegTime(string id, time_t t);
    static bool getAuth(string id);
    // static DevTypeCode getSipDevInfo(string id);

    static string randomNum(int length);
    static DevTypeCode getSipDevInfo(string id);

    static string gCatalogPayload;
};

#endif