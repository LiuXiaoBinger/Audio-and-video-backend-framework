#include "SipMessage.h"

SipMessage::SipMessage()
{
    memset(_fromHeader,0,sizeof(_fromHeader));
    memset(_toHeader,0,sizeof(_toHeader));
    memset(_requestUrl,0,sizeof(_requestUrl));
    memset(_contact,0,sizeof(_contact));
}

SipMessage::~SipMessage()
{
    
}

void SipMessage::setFrom(char* fromUsr,char* fromIp)
{
    sprintf(_fromHeader,"<sip:%s@%s>",fromUsr,fromIp);
}

void SipMessage::setTo(char* toUsr,char* toIp)
{
    sprintf(_toHeader,"<sip:%s@%s>",toUsr,toIp);
}

void SipMessage::setUrl(char* sipId,char* url_ip,int url_port,char* url_proto)
{
    sprintf(_requestUrl,"sip:%s@%s:%d;transport=%s",sipId,url_ip,url_port,url_proto);
}

void SipMessage::setContact(char* sipId,char* natIp,int natPort)
{   
    sprintf(_contact,"sip:%s@%s:%d",sipId,natIp,natPort);
}
