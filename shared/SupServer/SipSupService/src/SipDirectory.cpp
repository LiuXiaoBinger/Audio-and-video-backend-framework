#include "SipDirectory.h"
#include "SipDef.h"
#include "GlobalCtl.h"
Json::Value SipDirectory::m_jsonIn;
int SipDirectory::m_jsonIndex = 0;
SipDirectory::SipDirectory(tinyxml2::XMLElement* root)
    :SipTaskBase()
{
    m_pRootElement = root;
}

SipDirectory::~SipDirectory()
{

}

pj_status_t SipDirectory::run(pjsip_rx_data *rdata)
{
    int status_code = SIP_SUCCESS;
    //解析message-body-xml数据
    SaveDir(status_code);
    //响应
    pj_status_t status = pjsip_endpt_respond(GBOJ(gSipServer)->get_pjsip_endpoint(),NULL,rdata,status_code,NULL,NULL,NULL,NULL);
    if(PJ_SUCCESS != status)
    {
        LOG(ERROR)<<"pjsip_endpt_respond error";
    }

    return status;
}

void SipDirectory::SaveDir(int& status_code)
{
    tinyxml2::XMLElement* pRootElement = m_pRootElement;
    if(!pRootElement)
    {
        status_code = SIP_BADREQUEST;
        return;
    }

    string strCenterDeviceID,strSumNum,strSn,strDeviceID,strName,strManufacturer,
    strModel,strOwner,strCivilCode,strParental,strParentID,strSafetyWay,
    strRegisterWay,strSecrecy,strStatus;
    tinyxml2::XMLElement* pElement = pRootElement->FirstChildElement("DeviceID");
    if(pElement && pElement->GetText())
        strCenterDeviceID = pElement->GetText();

    if(!GlobalCtl::checkIsVaild(strCenterDeviceID))
    {
        status_code = SIP_BADREQUEST;
        return;
    }
    
    pElement = pRootElement->FirstChildElement("SumNum");
    if(pElement && pElement->GetText())
        strSumNum = pElement->GetText();

    pElement = pRootElement->FirstChildElement("SN");
    if(pElement && pElement->GetText())
        strSn = pElement->GetText();


    pElement = pRootElement->FirstChildElement("DeviceList");
    if(pElement)
    {
        tinyxml2::XMLElement* pItem = pElement->FirstChildElement("item");
        while(pItem)
        {
            tinyxml2::XMLElement* pChild = pItem->FirstChildElement("DeviceID");
            if(pChild && pChild->GetText())
            {
                strDeviceID = pChild->GetText();
                if(strDeviceID.length() == 20)
                {
                    DevTypeCode type = GlobalCtl::getSipDevInfo(strDeviceID);
                    if(type == Camera_Code || type == Ipc_Code)
                    {
                        GlobalCtl::gRcvIpc = true;
                        
                        LOG(INFO)<<"get ipc device";
                    }
                }
            }
                
            pChild = pItem->FirstChildElement("Name");
            if(pChild && pChild->GetText())
                strName = pChild->GetText();
            
            pChild = pItem->FirstChildElement("Manufacturer");
            if(pChild && pChild->GetText())
                strManufacturer = pChild->GetText();
            
            pChild = pItem->FirstChildElement("Model");
            if(pChild && pChild->GetText())
                strModel = pChild->GetText();

            pChild = pItem->FirstChildElement("Owner");
            if(pChild && pChild->GetText())
                strOwner = pChild->GetText();

            pChild = pItem->FirstChildElement("CivilCode");
            if(pChild && pChild->GetText())
                strCivilCode = pChild->GetText();

            pChild = pItem->FirstChildElement("Parental");
            if(pChild && pChild->GetText())
                strParental = pChild->GetText();

            pChild = pItem->FirstChildElement("ParentID");
            if(pChild && pChild->GetText())
                strParentID = pChild->GetText();

            pChild = pItem->FirstChildElement("SafetyWay");
            if(pChild && pChild->GetText())
                strSafetyWay = pChild->GetText();

            pChild = pItem->FirstChildElement("RegisterWay");
            if(pChild && pChild->GetText())
                strRegisterWay = pChild->GetText();
            
            pChild = pItem->FirstChildElement("Secrecy");
            if(pChild && pChild->GetText())
                strSecrecy = pChild->GetText();
            
            pChild = pItem->FirstChildElement("Status");
            if(pChild && pChild->GetText())
                strStatus = pChild->GetText();

            GlobalCtl::get_global_mutex();
            m_jsonIn["catalog"][m_jsonIndex]["DeviceID"] = strDeviceID;
            m_jsonIn["catalog"][m_jsonIndex]["Name"] = strName;
            m_jsonIn["catalog"][m_jsonIndex]["Parental"] = strParental;
            m_jsonIn["catalog"][m_jsonIndex]["ParentID"] = strParentID;
            m_jsonIndex++;
            GlobalCtl::free_global_mutex();
            pItem = pItem->NextSiblingElement();
        }

    }
    int sumNum = atoi(strSumNum.c_str());
    if(m_jsonIn["catalog"].size() == sumNum)
    {
        GlobalCtl::get_global_mutex();
        GlobalCtl::gCatalogPayload = JsonParse(m_jsonIn).toString();
        GlobalCtl::free_global_mutex();
        m_jsonIn.clear();
        m_jsonIndex = 0;
        GBOJ(gThPool)->postInfo();
    }
}