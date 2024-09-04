#ifndef _SIPDIRECTORY_H
#define _SIPDIRECTORY_H
#include "SipTaskBase.h"

class SipDirectory :public SipTaskBase
{
    public:
        SipDirectory(tinyxml2::XMLElement* root);
        ~SipDirectory();
        SipDirectory &operator =(const SipDirectory & DIR)=delete;
        SipDirectory(const SipDirectory & DIR)=delete;

        virtual void run(pjsip_rx_data *rdata);
        //解析目录
        void resDir(pjsip_rx_data *rdata,int &sn);

        void directoryQuery(Json::Value& jsonOut);
        void sendSipDirMsg(pjsip_rx_data *rdata, char *sendData);

        void constructMANSCDPXml(Json::Value listdata, int *begin, int itemCount, char *sendData, int sn);

    private:
        tinyxml2::XMLElement* m_pRootElement;

};


#endif