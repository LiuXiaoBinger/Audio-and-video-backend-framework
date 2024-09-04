#include "SipLocalConfig.h"

#define CONFIGFILE_PATH "../../../conf/SipSubService.conf"
#define LOCAL_SECTION "localserver"
#define SIP_SECTION "sipserver"

static const string keyLocalIp = "local_ip";
static const string keyLocalPort = "local_port";
static const string keySipId = "sip_id";
static const string keySipIp = "sip_ip";
static const string keySipPort = "sip_port";
static const std::string keyRtpPortBegin   = "rtp_port_begin";
static const std::string keyRtpPortEnd   = "rtp_port_end";

static const string keySupNodeNum = "supnode_num";
static const string keySupNodeId = "sip_supnode_id";
static const string keySupNodeIp = "sip_supnode_ip";
static const string keySupNodePort = "sip_supnode_port";
static const string keySupNodePoto = "sip_supnode_poto";
static const string keySupNodeExpires = "sip_supnode_expires";
static const string keySupNodeAuth = "sip_supnode_auth";
static const string keySupNodeUsr = "sip_supnode_usr";
static const string keySupNodePwd = "sip_supnode_pwd";
static const string keySupNodeRealm = "sip_supnode_realm";

SipLocalConfig::SipLocalConfig()
:m_conf(CONFIGFILE_PATH)
{
    m_localIp = "";
    m_localPort = 0;
    m_sipId = "";
    m_sipIp = "";
    m_sipPort = 0;
    m_sipRealm = "";
    m_SupNodeIp = "";
    m_SupNodePort = 0;
    m_SupNodePoto = 0;
    m_SupNodeAuth = 0;
}
SipLocalConfig::~SipLocalConfig()
{

}

int SipLocalConfig::ReadConf()
{
    int ret = 0;
    m_conf.setSection(LOCAL_SECTION);
    m_localIp = m_conf.readStr(keyLocalIp);
    if(m_localIp.empty())
    {
        ret = -1;
        LOG(ERROR)<<"localIp is wrong";
        return ret;
    }
    m_localPort = m_conf.readInt(keyLocalPort);
    if(m_localPort <= 0)
    {
        ret = -1;
        LOG(ERROR)<<"localPort is wrong";
        return ret;
    }

    m_conf.setSection(SIP_SECTION);
    m_sipId = m_conf.readStr(keySipId);
    if(m_sipId.empty())
    {
        ret = -1;
        LOG(ERROR)<<"sipId is wrong";
        return ret;
    }

    m_sipIp = m_conf.readStr(keySipIp);
    if(m_sipIp.empty())
    {
        ret = -1;
        LOG(ERROR)<<"sipIp is wrong";
        return ret;
    }
    m_sipPort = m_conf.readInt(keySipPort);
    if(m_sipPort <= 0)
    {
        ret = -1;
        LOG(ERROR)<<"sipPort is wrong";
        return ret;
    }
    LOG(INFO)<<"localip:"<<m_localIp<<",localport:"<<m_localPort<<",sipid:"<<m_sipId<<",sipip:"<<m_sipIp\
    <<",sipport:"<<m_sipPort;
	
	m_rtpPortBegin = m_conf.readInt(keyRtpPortBegin);
    if(m_rtpPortBegin<=0)
    {
       ret =-1;
       LOG(ERROR)<<"rtpPortBegin is NULL";
       return ret;
    }

    m_rtpPortEnd = m_conf.readInt(keyRtpPortEnd);
    if(m_rtpPortEnd<=0)
    {
       ret =-1;
       LOG(ERROR)<<"rtpPortEnd is NULL";
       return ret;
    }
	initRandPort();
	
    int num = m_conf.readInt(keySupNodeNum);
    LOG(INFO)<<"num:"<<num;
    SupNodeInfo info;
    for(int i = 1;i<num+1;++i)
    {
        string id = keySupNodeId + to_string(i);
        string ip = keySupNodeIp + to_string(i);
        string port = keySupNodePort + to_string(i);
        string poto = keySupNodePoto + to_string(i);
        string auth = keySupNodeAuth + to_string(i);
        string expires = keySupNodeExpires + to_string(i);
        string usr = keySupNodeUsr+to_string(i);
        string pwd = keySupNodePwd+to_string(i);
        string realm = keySupNodeRealm + to_string(i);
        
        info.id = m_conf.readStr(id);
        info.ip = m_conf.readStr(ip);
        info.port = m_conf.readInt(port);
        info.poto = m_conf.readInt(poto);
        LOG(INFO)<<"info.poto:"<<info.poto;
        info.expires = m_conf.readInt(expires);
        info.usr = m_conf.readStr(usr);
        info.pwd = m_conf.readStr(pwd);
        info.auth = m_conf.readInt(auth);
        info.realm = m_conf.readStr(realm);
        upNodeInfoList.push_back(info);

        // LOG(INFO)<<"subnodeip:"<<m_supNodeIp<<",subnodeport:"<<m_supNodePort\
        // <<",subnodepoto:"<<m_supNodePoto<<",subnodeauth:"<<m_supNodeAuth;
    }
    LOG(INFO)<<"upNodeInfoList.SIZE:"<<upNodeInfoList.size();


    return ret;
}

void SipLocalConfig::initRandPort()
{
    
    while(m_rtpPortBegin % 2)  
    {
        m_rtpPortBegin++;
    }


    while(m_rtpPortEnd % 2 == 0)
    {
        m_rtpPortEnd--;
    }

    
    AutoMutexLock lck(&m_rtpPortLock);
    for(int i=m_rtpPortBegin;i< (m_rtpPortEnd + 1);i++)
    {
        m_RandNum.push(i);
    }
    LOG(INFO)<<"rand size:"<<m_RandNum.size();
}


int SipLocalConfig::popOneRandNum()
{
    int rtpPort = 0;
    AutoMutexLock lck(&m_rtpPortLock);
    if(m_RandNum.size()>0)
    {
        //取完一个就扔出队列，每次取两个号
        rtpPort = m_RandNum.front();
        m_RandNum.pop();
        if(rtpPort % 2) //这里我们再做下判断
        {
            //如果第一个是奇数，那么重新取
            rtpPort = m_RandNum.front();
            m_RandNum.pop();
        }
        //取第二个号
        m_RandNum.pop();
    }
    return rtpPort;
}

//回收rtp port
int SipLocalConfig::pushOneRandNum(int num)
{
    if(num < m_rtpPortBegin || num > m_rtpPortEnd)
    {
        return -1;
    }
    AutoMutexLock lck(&m_rtpPortLock);
    m_RandNum.push(num);
    m_RandNum.push(num+1);
	LOG(INFO)<<"push rtp:"<<num<<",rtcp:"<<num+1;
	return 0;
}