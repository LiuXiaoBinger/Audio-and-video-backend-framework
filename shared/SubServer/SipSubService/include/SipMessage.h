#ifndef _SIPMESSAGE_H
#define _SIPMESSAGE_H
#include "Common.h"
#include "GlobalCtl.h"
class SipMessage
{
    public:
        SipMessage();
        ~SipMessage();
    
    public:
        void setFrom(char* fromUsr,char* fromIp);
        void setTo(char* toUsr,char* toIp);
        void setUrl(char* sipId,char* url_ip,int url_port,char* url_proto = (char*)"udp");
        void setContact(char* sipId,char* natIp,int natPort);

        inline char* FromHeader(){return _fromHeader;}
        inline char* ToHeader(){return _toHeader;}
        inline char* RequestUrl(){return _requestUrl;}
        inline char* Contact(){return _contact;}

    private:
        char _fromHeader[128];
        char _toHeader[128];
        char _requestUrl[128];
        char _contact[128];
};
#endif