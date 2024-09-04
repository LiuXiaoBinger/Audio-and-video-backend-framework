#include "SipRegister.h"
#include "Common.h"
#include "SipMessage.h"
// 消息发送，对端返回数据触发的回调函数
static void client_cb(struct pjsip_regc_cbparam *param)
{
    LOG(INFO) << "code" << param->code;
    if (param->code == 200)
    {
        AutoMutexLock lock(&(GlobalCtl::globalLock));
        GlobalCtl::SupDomainInfo *subinfo = (GlobalCtl::SupDomainInfo *)param->token;
        subinfo->registered = true;
        //LOG(INFO) << "code 200";
    }
}

SipRegister::SipRegister()
{

    m_regTimer = new TaskTimer(3);
    
}

SipRegister::~SipRegister()
{
    if(m_regTimer)
    {
        delete m_regTimer;
        m_regTimer = NULL;
    }
}

void SipRegister::registerServiceStart()
{
    if(m_regTimer)
    {
        m_regTimer->setTimerFun(SipRegister::RegisterProc,(void*)this);
        m_regTimer->start();
    }
}

void SipRegister::RegisterProc(void *param)
{
    SipRegister* pthis = (SipRegister*)param;
    AutoMutexLock lock(&(GlobalCtl::globalLock));
    GlobalCtl::SUPDOMAININFOLIST::iterator iter = GlobalCtl::instance()->getSupDomainInfoList().begin();
    for (auto &item : GlobalCtl::instance()->getSupDomainInfoList())
    {
        if (!item.registered)
        {
            if (pthis->gbRegister(item) < 0)
            {
                LOG(ERROR) << "send SipMessage to Regist faild";
            }
        }
    }
}

int SipRegister::gbRegister(GlobalCtl::SupDomainInfo &node)
{
    SipMessage _msg;
    _msg.setFrom((char*)GBOJ(_gConfig)->sipId().c_str(),(char*)GBOJ(_gConfig)->sipIp().c_str());
    _msg.setTo((char*)GBOJ(_gConfig)->sipId().c_str(), (char*)GBOJ(_gConfig)->sipIp().c_str());
    
    if (node.protocal==0)
    {
        _msg.setUrl((char*)node.sipId.c_str(), (char*)node.addrIp.c_str(), node.sipPort, (char *)"tcp");
        
    }
    else
    {
        _msg.setUrl((char*)node.sipId.c_str(), (char*)node.addrIp.c_str(), node.sipPort, (char *)"udp");
        
    }
    _msg.setContact((char*)GBOJ(_gConfig)->sipId().c_str(), (char*)GBOJ(_gConfig)->sipIp().c_str(), GBOJ(_gConfig)->sipPort());
    // 转成psip类型
    pj_str_t from = pj_str(_msg.FromHeader());
    pj_str_t to = pj_str(_msg.ToHeader());
    pj_str_t request = pj_str(_msg.RequestUrl());
    pj_str_t contact = pj_str(_msg.Contact());
    pj_status_t _status = PJ_SUCCESS;
    do
    {
        // 创建用于处理 SIP 注册客户端（SIP Registration Client）
        pjsip_regc *_regc;
        // 创建注册客户端 设置回调函数
        _status = pjsip_regc_create(GBOJ(gSipServer)->get_pjsip_endpoint(), &node, client_cb, &_regc);
        if (_status != PJ_SUCCESS)
        {
            LOG(ERROR) << "pjsip_regc_create erorr";
            break;
        }
        // 初始化注册客户端
        _status = pjsip_regc_init(_regc, &request, &from, &to, 1, &contact, node.expires);
        if (_status != PJ_SUCCESS)
        {
            pjsip_regc_destroy(_regc);
            LOG(ERROR) << "pjsip_regc_init erorr";
            break;
        }
        //设置健权,不管是不是健权都要设置，上级返回401,pjsip会自动去健权
        if(node.isAuth)
        {
            pjsip_cred_info cred;
            pj_bzero(&cred,sizeof(pjsip_cred_info));
            cred.scheme = pj_str("digest");
            string realm=node.realm;
            cred.realm = pj_str((char*)realm.c_str());
            string usr = node.usr;
            cred.username = pj_str((char*)usr.c_str());
            cred.data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
            string pwd = node.pwd;
            cred.data = pj_str((char*)pwd.c_str());
            //LOG(ERROR)<<"pwd:"<<cred.data.ptr<<" realm:"<<cred.realm.ptr<<" usr:"<<cred.username.ptr;
            _status=pjsip_regc_set_credentials(_regc,1,&cred);
            if(PJ_SUCCESS != _status)
            {
                pjsip_regc_destroy(_regc);
                LOG(ERROR)<<"pjsip_regc_set_credentials error";
                break;
            }
        }
        // 生成注册请求
        pjsip_tx_data *_tx_data = NULL;
        _status = pjsip_regc_register(_regc, PJ_TRUE, &_tx_data);
        if (_status != PJ_SUCCESS)
        {
            pjsip_regc_destroy(_regc);
            LOG(ERROR) << "pjsip_regc_register erorr";
            break;
        }
        // 发送注册请求
        _status = pjsip_regc_send(_regc, _tx_data);
        if (_status != PJ_SUCCESS)
        {
            pjsip_regc_destroy(_regc);
            LOG(ERROR) << "pjsip_regc_send erorr";
            break;
        }
    } while (0);
    if(_status!=PJ_SUCCESS)
    {
        return -1;
    }
    return 0;
}