#include "GlobalCtl.h"
#include "SipDef.h"
 GlobalCtl *GlobalCtl::m_instance = nullptr;
 pthread_mutex_t GlobalCtl::globalLock = PTHREAD_MUTEX_INITIALIZER;
 bool GlobalCtl::_gStopPool=false;
 GlobalCtl::~GlobalCtl()
 {
 }
 GlobalCtl *GlobalCtl::instance()
 {
     if (!m_instance)
     {
         m_instance = new GlobalCtl();
     }
     return m_instance;
}

bool GlobalCtl::init(void *param)
{
    _gConfig=(SipLocalConfig*)param;
    if(!_gConfig)
    {
        return false;

    }
    //初始化上级服务器列表信息
    SupDomainInfo info;
    auto iter = _gConfig->upNodeInfoList.begin();
    for(;iter != _gConfig->upNodeInfoList.end();++iter)
    {
        info.sipId = iter->id;
        info.addrIp = iter->ip;
        info.sipPort = iter->port;
        info.protocal = iter->poto;
        info.expires = iter->expires;
        if(iter->auth)
        {
            info.isAuth = (iter->auth = 1)?true:false;
            info.usr = iter->usr;
            info.pwd = iter->pwd;
            info.realm = iter->realm;
        }
        _supDomainInfoList.push_back(info);
    }
     if(!gThPool)
    {
        gThPool =  new ThreadPool();
        gThPool->createThreadPool(10);
    }
    if(!gSipServer)
    {
        gSipServer = new SipCore();
    }
    gSipServer->initSip(_gConfig->sipPort());
    return true;
}


DevTypeCode GlobalCtl::getSipDevInfo(string id)
{
    DevTypeCode code_type = Error_code;
    string tmp = id.substr(10,3);
    int type = atoi(tmp.c_str());
    
    switch(type)
    {
        case Camera_Code:
            code_type = Camera_Code;
            break;
        case Ipc_Code:
            code_type = Ipc_Code;
            break;   
        default:
            code_type = Error_code;
            break;
    }

    return code_type;
}