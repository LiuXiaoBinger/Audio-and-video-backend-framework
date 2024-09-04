#include "GlobalCtl.h"

 GlobalCtl *GlobalCtl::m_instance = nullptr;
 GlobalCtl::SUBDOMAININFOLIST GlobalCtl::_subDomainInfoList;
 pthread_mutex_t GlobalCtl::globalLock = PTHREAD_MUTEX_INITIALIZER;
 bool GlobalCtl::_gStopPool=false;
 bool GlobalCtl::gRcvIpc = false;

 GlobalCtl::ListSession GlobalCtl::glistSession;
pthread_mutex_t GlobalCtl::gStreamLock = PTHREAD_MUTEX_INITIALIZER;
string GlobalCtl::gCatalogPayload;
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
    SubDomainInfo info;
    auto iter = _gConfig->ubNodeInfoList.begin();
    for(;iter != _gConfig->ubNodeInfoList.end();++iter)
    {
        info.sipId = iter->id;
        info.addrIp = iter->ip;
        info.sipPort = iter->port;
        info.protocal = iter->poto;
        info.auth = iter->auth;
        _subDomainInfoList.push_back(info);
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

bool GlobalCtl::checkIsExist(string id)
{
    AutoMutexLock lck(&globalLock);
    SUBDOMAININFOLIST::iterator it;
    it = std::find(_subDomainInfoList.begin(),_subDomainInfoList.end(),id);
    if(it != _subDomainInfoList.end())
    {
        return true;
    }
    return false;

}

bool GlobalCtl::checkIsVaild(string id)
{
    AutoMutexLock lck(&globalLock);
    SUBDOMAININFOLIST::iterator it;
    it = std::find(_subDomainInfoList.begin(),_subDomainInfoList.end(),id);
    if(it != _subDomainInfoList.end() && it->registered)
    {
        return true;
    }
    return false;
}

void GlobalCtl::setExpires(string id,int expires)
{
    AutoMutexLock lck(&globalLock);
    SUBDOMAININFOLIST::iterator it;
    it = std::find(_subDomainInfoList.begin(),_subDomainInfoList.end(),id);
    if(it != _subDomainInfoList.end())
    {
        it->expires = expires;
    }
}

void GlobalCtl::setRegister(string id,bool registered)
{
    AutoMutexLock lck(&globalLock);
    SUBDOMAININFOLIST::iterator it;
    it = std::find(_subDomainInfoList.begin(),_subDomainInfoList.end(),id);
    if(it != _subDomainInfoList.end())
    {
        it->registered = registered;
    }
}

void GlobalCtl::setLastRegTime(string id,time_t t)
{
    AutoMutexLock lck(&globalLock);
    SUBDOMAININFOLIST::iterator it;
    it = std::find(_subDomainInfoList.begin(),_subDomainInfoList.end(),id);
    if(it != _subDomainInfoList.end())
    {
        it->lastRegTime = t;
    }
}

bool GlobalCtl::getAuth(string id)
{
    AutoMutexLock lck(&globalLock);
    SUBDOMAININFOLIST::iterator it;
    it = std::find(_subDomainInfoList.begin(),_subDomainInfoList.end(),id);
    if(it != _subDomainInfoList.end())
    {
        return it->auth;
    }
}

string GlobalCtl::randomNum(int length)
{
    #if 0
    //随机数种子
    random_device rd;
    //随机数生成器
    mt19937 gen(rd());
    //分布器类模板  设定一个0-15的均匀整数分布的区间范围
    uniform_int_distribution<> dis(0,15);
    stringstream ss;
    for(int i =0;i<length;++i)
    {
        //每次使用随机数生成器在指定的分布范围内获取一个随机数
        int value = dis(gen);
        ss<< std::hex << value;
    }
    #endif
    stringstream ss;
    for(int i =0;i<length;++i)
    {   
        int value = random()%15;
        ss<< std::hex << value;
    }
    
    return ss.str();
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