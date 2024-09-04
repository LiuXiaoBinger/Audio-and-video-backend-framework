#include "SipHeartBeat.h"
#include "SipMessage.h"
#include "XmlParser.h"
int SipHeartBeat::m_snIndex=0;
static void response_callback(void *token, pjsip_event *e)
{
    pjsip_transaction* tsx = e->body.tsx_state.tsx;
    GlobalCtl::SupDomainInfo* node = (GlobalCtl::SupDomainInfo*)token;
    if(tsx->status_code != 200)
    {
        node->registered = false;
    }
    return;
}
SipHeartBeat::SipHeartBeat()
{
    m_heartTimer=new TaskTimer(10);
}

SipHeartBeat::~SipHeartBeat()
{
    if(m_heartTimer){
        delete m_heartTimer;
        m_heartTimer=nullptr;
    }
}

void SipHeartBeat::gbHeartBeatServiceStart()
{
    if(m_heartTimer)
    {
        m_heartTimer->setTimerFun(HeartBeatProc,this);
        m_heartTimer->start();
    }
}

void SipHeartBeat::HeartBeatProc(void *param)
{
    SipHeartBeat* pthis = (SipHeartBeat*)param;
    AutoMutexLock lock(&(GlobalCtl::globalLock));
    GlobalCtl::SUPDOMAININFOLIST::iterator iter = GlobalCtl::instance()->getSupDomainInfoList().begin();
    for(;iter != GlobalCtl::instance()->getSupDomainInfoList().end(); iter++)
    {
        if(iter->registered)
        {
            if(pthis->gbHeartBeat(*iter)<0)
            {
                LOG(ERROR)<<"keep alive error for:"<<iter->sipId;
            }
        }
    }
}

int SipHeartBeat::gbHeartBeat(GlobalCtl::SupDomainInfo &node)
{
    SipMessage msg;
    msg.setFrom((char*)GBOJ(_gConfig)->sipId().c_str(),(char*)GBOJ(_gConfig)->sipIp().c_str());
    msg.setTo((char*)node.sipId.c_str(),(char*)node.addrIp.c_str());
    msg.setUrl((char*)node.sipId.c_str(),(char*)node.addrIp.c_str(),node.sipPort);

    pj_str_t from = pj_str(msg.FromHeader());
    pj_str_t to = pj_str(msg.ToHeader());
    pj_str_t line = pj_str(msg.RequestUrl());
    string method = "MESSAGE";
    pjsip_method reqMethod = {PJSIP_OTHER_METHOD,{(char*)method.c_str(),method.length()}};
    pjsip_tx_data* tdata;
    pj_status_t status = pjsip_endpt_create_request(GBOJ(gSipServer)->get_pjsip_endpoint(),&reqMethod,&line,&from,&to,NULL,NULL,-1,NULL,&tdata);
    if(PJ_SUCCESS != status)
    {
        LOG(ERROR)<<"pjsip_endpt_create_request ERROR";
        return -1;
    }

    char strIndex[16] = {0};
    sprintf(strIndex,"%d",m_snIndex++);
    #if 0
    string keepalive = "<?xml version=\"1.0\"?>\r\n";
    keepalive += "<Notify>\r\n";
    keepalive += "<CmdType>keepalive</CmdType>\r\n";
    keepalive += "<SN>";
    keepalive += strIndex;
    keepalive += "</SN>\r\n";
    keepalive += "<DeviceID>";
    keepalive += GBOJ(_gConfig)->sipId();
    keepalive += "</DeviceID>\r\n";
    keepalive += "<Status>OK</Status>\r\n";
    keepalive += "</Notify>\r\n";
    #endif
    #if 1
    XmlParser parse;
    tinyxml2::XMLElement* rootNode = parse.AddRootNode((char*)"Notify");
    parse.InsertSubNode(rootNode,(char*)"CmdType",(char*)"keepalive");
    parse.InsertSubNode(rootNode,(char*)"SN",strIndex);
    parse.InsertSubNode(rootNode,(char*)"DeviceID",GBOJ(_gConfig)->sipId().c_str());
    parse.InsertSubNode(rootNode,(char*)"Status",(char*)"OK");
    char* xmlbuf = new char[1024];
    memset(xmlbuf,0,1024);
    parse.getXmlData(xmlbuf);
    #endif
    //LOG(INFO)<<"xmlbuf:"<<xmlbuf;

    pj_str_t type = pj_str("Application");
    pj_str_t subtype = pj_str("MANSCDP+xml");
    pj_str_t xmldata = pj_str(xmlbuf);
    tdata->msg->body = pjsip_msg_body_create(tdata->pool,&type,&subtype,&xmldata);

    status = pjsip_endpt_send_request(GBOJ(gSipServer)->get_pjsip_endpoint(),tdata,-1,&node,&response_callback);
    if(PJ_SUCCESS != status)
    {
        LOG(ERROR)<<"pjsip_endpt_send_request ERROR";
        return -1;
    }
    return 0;
}
