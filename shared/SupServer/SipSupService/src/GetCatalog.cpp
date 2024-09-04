#include "GetCatalog.h"
#include "GlobalCtl.h"
#include "SipMessage.h"
#include "XmlParser.h"
GetCatalog::GetCatalog(struct bufferevent* bev)
    :ThreadTask(bev)
{
    //DirectoryGetPro(NULL);
}

GetCatalog::~GetCatalog()
{
}

void GetCatalog::run()
{
    DirectoryGetPro(NULL);
    GBOJ(gThPool)->waitInfo();
    GlobalCtl::get_global_mutex();
    int bodyLen = GlobalCtl::gCatalogPayload.length();
    int len = sizeof(int)+bodyLen;
    char* buf = new char[len];
    memcpy(buf,&bodyLen,sizeof(int));
    memcpy(buf+sizeof(int),GlobalCtl::gCatalogPayload.c_str(),bodyLen);
    bufferevent_write(m_bev,buf,len);
    GlobalCtl::free_global_mutex();
    delete buf;

    return;
}


void GetCatalog::DirectoryGetPro(void *param)
{
     SipMessage msg;
    XmlParser parse;
    AutoMutexLock lock(&(GlobalCtl::globalLock));
    GlobalCtl::SUBDOMAININFOLIST::iterator iter = GlobalCtl::instance()->getSubDomainInfoList().begin();
    for (auto &item : GlobalCtl::instance()->getSubDomainInfoList())
    {
        if (item.registered)
        {
            //设置请求from to
            msg.setFrom((char*)GBOJ(_gConfig)->sipId().c_str(),(char*)GBOJ(_gConfig)->sipIp().c_str());
            msg.setTo((char*)GBOJ(_gConfig)->sipId().c_str(),(char*)item.addrIp.c_str());
            msg.setUrl((char*)GBOJ(_gConfig)->sipId().c_str(),(char*)item.addrIp.c_str(),item.sipPort);

            pj_str_t from = pj_str(msg.FromHeader());
            pj_str_t to = pj_str(msg.ToHeader());
            pj_str_t requestUrl = pj_str(msg.RequestUrl());

            string method = "MESSAGE";
            pjsip_method reqMethod = {PJSIP_OTHER_METHOD,{(char*)method.c_str(),method.length()}};
            pjsip_tx_data* tdata;
            pj_status_t status = pjsip_endpt_create_request
            (GBOJ(gSipServer)->get_pjsip_endpoint(),
            &reqMethod,&requestUrl,&from,&to,NULL,NULL,-1,NULL,&tdata);
             if(PJ_SUCCESS != status)
            {
                LOG(ERROR)<<"pjsip_endpt_create_request ERROR";
                return;
            }
            //组织xml部分body
            tinyxml2::XMLElement* rootNode = parse.AddRootNode((char*)"Query");
            parse.InsertSubNode(rootNode,(char*)"CmdType",(char*)"Catalog");
            int sn = random() % 1024;
            char tmpStr[32] = {0};
            sprintf(tmpStr,"%d",sn);
            parse.InsertSubNode(rootNode,(char*)"SN",tmpStr);
            parse.InsertSubNode(rootNode,(char*)"DeviceID",iter->sipId.c_str());
            char* xmlbuf = new char[1024];
            memset(xmlbuf,0,1024);
            parse.getXmlData(xmlbuf);
            LOG(INFO)<<"xml:"<<xmlbuf;
            
            pj_str_t type = pj_str("Application");
            pj_str_t subtype = pj_str("MANSCDP+xml");
            pj_str_t xmldata = pj_str(xmlbuf);
            tdata->msg->body = pjsip_msg_body_create(tdata->pool,&type,&subtype,&xmldata);
            //我们发送目录请求，不需要关心是否返回200 还是 403 只需要关注是否返回目录数据
            status = pjsip_endpt_send_request(GBOJ(gSipServer)->get_pjsip_endpoint(),tdata,-1,NULL,NULL);
            if(PJ_SUCCESS != status)
            {
                LOG(ERROR)<<"pjsip_endpt_send_request ERROR";
                return;
            }
        }
    }
}
