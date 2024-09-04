#ifndef _SIPTASKBASE_H
#define _SIPTASKBASE_H
#include <pjlib-util.h>
#include <pjmedia.h>
#include <pjsip.h>
#include <pjsip_ua.h>
#include <pjsip/sip_auth.h>
#include <pjlib.h>
#include "Common.h"
class SipTaskBase
{
    public:
        SipTaskBase(){}
        virtual ~SipTaskBase()
        {
            LOG(INFO)<<"~SipTaskBase";
        }

        virtual void run(pjsip_rx_data *rdata) = 0;

        static tinyxml2::XMLElement* parseXmlData(pjsip_msg* msg,string& rootType,const string xmlkey,string& xmlvalue);
    protected:
        //从消息包里获取sipid
        string parseFromId(pjsip_msg* msg);
        string parseToId(pjsip_msg *msg);
        
};

#endif