#include "GetPlamtInfo.h"
#include "GlobalCtl.h"
#include "EventMsgHandle.h"
GetPlamtInfo::GetPlamtInfo(struct bufferevent* bev)
    :ThreadTask(bev)
{

}
void GetPlamtInfo::run()
{
    Json::Value jsonIn;
    int index = 0;
    AutoMutexLock lock(&(GlobalCtl::globalLock));
    GlobalCtl::SUBDOMAININFOLIST::iterator iter = GlobalCtl::instance()->getSubDomainInfoList().begin();
    for(;iter != GlobalCtl::instance()->getSubDomainInfoList().end(); iter++)
    {
        if(iter->registered)
        {
            jsonIn["SubDomain"][index]["sipId"] = iter->sipId;
            index++;
        }
    }
    
    Json::FastWriter fase_writer;
    string payLoadIn = fase_writer.write(jsonIn);
    LOG(INFO)<<"payLoadIn:"<<payLoadIn;
    int bodyLen = payLoadIn.length();
    int len = sizeof(int)+bodyLen;
    char* buf = new char[len];
    memcpy(buf,&bodyLen,sizeof(int));
    memcpy(buf+sizeof(int),payLoadIn.c_str(),bodyLen);
    bufferevent_write(m_bev,buf,len);
    delete buf;
}