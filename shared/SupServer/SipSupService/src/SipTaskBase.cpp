#include "SipTaskBase.h"

string SipTaskBase::parseFromId(pjsip_msg *msg)
{
    pjsip_from_hdr* from = (pjsip_from_hdr*)pjsip_msg_find_hdr(msg,PJSIP_H_FROM,NULL);
    if(NULL == from)
    {
        return "";
    }
    char buf[1024] = {0};
    string fromId = "";
    int size = from->vptr->print_on(from,buf,1024);
    if(size > 0)
    {
        fromId = buf;
        fromId = fromId.substr(11,20);
    }
    return fromId;
}

tinyxml2::XMLElement* SipTaskBase::parseXmlData(pjsip_msg* msg,string& rootType,const string xmlkey,string& xmlvalue)
{
       //创建解析xml对象
    tinyxml2::XMLDocument* pxmlDoc = NULL;
    pxmlDoc = new tinyxml2::XMLDocument();
    if(pxmlDoc)
    {
        pxmlDoc->Parse((char*)msg->body->data);
    }
    //获取根节点
    tinyxml2::XMLElement* pRootElement = pxmlDoc->RootElement();
    //获取根节点的值
    rootType = pRootElement->Value();
     //获取子节点
    tinyxml2::XMLElement* cmdElement = pRootElement->FirstChildElement((char*)xmlkey.c_str());
    if(cmdElement && cmdElement->GetText())
    {
        xmlvalue = cmdElement->GetText();
    }

    return pRootElement;
}


        

        

       
