#include "GetRecordList.h"
#include "SipMessage.h"
#include "XmlParser.h"
GetRecordList::GetRecordList()
{
    RecordInfoGetPro(NULL);
}

GetRecordList::~GetRecordList()
{

}

void GetRecordList::RecordInfoGetPro(void* param)
{
    string devid = "11000000001310000059";
    string platformId = "11000000002000000001";

    //这里在组织message header 和request line
    SipMessage msg;
    XmlParser parse;
    AutoMutexLock lock(&(GlobalCtl::globalLock));
    GlobalCtl::SUBDOMAININFOLIST::iterator iter = GlobalCtl::instance()->getSubDomainInfoList().begin();
    for(;iter != GlobalCtl::instance()->getSubDomainInfoList().end(); iter++)
    {
        if(iter->sipId == platformId && iter->registered)
        {
            msg.setFrom((char*)GBOJ(_gConfig)->sipId().c_str(),(char*)GBOJ(_gConfig)->sipIp().c_str());
            //这里就默认的用模糊查询
            msg.setTo((char*)iter->sipId.c_str(),(char*)iter->addrIp.c_str());
            msg.setUrl((char*)iter->sipId.c_str(),(char*)iter->addrIp.c_str(),iter->sipPort);

            pj_str_t from = pj_str(msg.FromHeader());
            pj_str_t to = pj_str(msg.ToHeader());
            pj_str_t requestUrl = pj_str(msg.RequestUrl());

            string method = "MESSAGE";
            pjsip_method reqMethod = {PJSIP_OTHER_METHOD,{(char*)method.c_str(),method.length()}};
            pjsip_tx_data* tdata;
            pj_status_t status = pjsip_endpt_create_request(GBOJ(gSipServer)->get_pjsip_endpoint(),&reqMethod,&requestUrl,&from,&to,NULL,NULL,-1,NULL,&tdata);
            if(PJ_SUCCESS != status)
            {
                LOG(ERROR)<<"pjsip_endpt_create_request ERROR";
                return;
            }
            tinyxml2::XMLElement* rootNode = parse.AddRootNode((char*)"Query");
            parse.InsertSubNode(rootNode,(char*)"CmdType",(char*)"RecordInfo");
            int sn = random() % 1024;
            char tmpStr[32] = {0};
            sprintf(tmpStr,"%d",sn);
            parse.InsertSubNode(rootNode,(char*)"SN",tmpStr);
            parse.InsertSubNode(rootNode,(char*)"DeviceID",devid.c_str());
            string starttime = "2023-09-16T00:00:00";
            string endtime = "2023-09-16T23:59:00";
            string recordtype = "all";
            parse.InsertSubNode(rootNode, (char *)"StartTime", starttime.c_str());
            parse.InsertSubNode(rootNode, (char *)"EndTime", endtime.c_str());
            parse.InsertSubNode(rootNode, (char *)"Type", (char *)recordtype.c_str());
            parse.InsertSubNode(rootNode, (char *)"IndistinctQuery", "0");

            char* xmlbuf = new char[1024];
            memset(xmlbuf,0,1024);
            parse.getXmlData(xmlbuf);
            LOG(INFO)<<"xml:"<<xmlbuf;
            
            pj_str_t type = pj_str("Application");
            pj_str_t subtype = pj_str("MANSCDP+xml");
            pj_str_t xmldata = pj_str(xmlbuf);
            tdata->msg->body = pjsip_msg_body_create(tdata->pool,&type,&subtype,&xmldata);

            status = pjsip_endpt_send_request(GBOJ(gSipServer)->get_pjsip_endpoint(),tdata,-1,NULL,NULL);
            if(PJ_SUCCESS != status)
            {
                LOG(ERROR)<<"pjsip_endpt_send_request ERROR";
                return;
            }
        
        }
    }
    return;
}