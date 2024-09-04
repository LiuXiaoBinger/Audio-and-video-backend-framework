#ifndef _SIPLOCALCONFIG_H
#define _SIPLOCALCONFIG_H
#include "ConfReader.h"
#include "Common.h"
#include <list>
#include <algorithm>
#include <queue>


class SipLocalConfig
{
    public:
        SipLocalConfig();
        ~SipLocalConfig();

        int ReadConf();
		void initRandPort();
        int popOneRandNum();
        int pushOneRandNum(int num);
		
        inline string localIp(){return m_localIp;}
        inline int localPort(){return m_localPort;}
        inline int sipPort(){return m_sipPort;}
        inline string sipId(){return m_sipId;}
        inline string sipIp(){return m_sipIp;}
        inline string realm(){return m_sipRealm;}
        inline string usr(){return m_usr;}
        inline string pwd(){return m_pwd;}
        struct SupNodeInfo
        {
            string id;
            string ip;
            int port;
            int poto;
            int expires;
            string usr;
            string pwd;
            int auth;
            string realm;
        };
        list<SupNodeInfo> upNodeInfoList;
    private:
        ConfReader m_conf;
        string m_localIp;
        int m_localPort;
        string m_sipId;
        string m_sipIp;
        int m_sipPort;
        string m_usr;
        string m_pwd;
        string m_sipRealm;
        string m_SupNodeIp;
        int m_SupNodePort;
        int m_SupNodePoto;
        int m_SupNodeAuth;
		int m_rtpPortBegin;
        int m_rtpPortEnd;
        std::queue<int> m_RandNum;
        pthread_mutex_t m_rtpPortLock;
};
#endif