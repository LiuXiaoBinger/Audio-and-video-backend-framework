#include "SipRegister.h"
#include "GlobalCtl.h"
#include "SipDef.h"
#include <sys/sysinfo.h>

static pj_status_t auth_cred_callback(pj_pool_t *pool,
                                      const pj_str_t *realm,
                                      const pj_str_t *acc_name,
                                      pjsip_cred_info *cred_info)
{
    string usr = GBOJ(_gConfig)->usr();
    if (string((char *)acc_name->ptr) != string(usr))
    {
        LOG(ERROR) << "usr name wrong";
        return PJ_FALSE;
    }
    string pwd = (char *)GBOJ(_gConfig)->pwd().c_str();
    // LOG(ERROR)<<"pwd:"<<pwd<<" usr:"<<usr;
    cred_info->realm = *realm;
    cred_info->username = pj_str((char *)usr.c_str());
    cred_info->data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    cred_info->data = pj_str((char *)pwd.c_str());
    // LOG(ERROR)<<"pwd:"<<cred_info->data.ptr<<" realm:"<<cred_info->realm.ptr<<" usr:"<<cred_info->username.ptr;
    return PJ_SUCCESS;
}
SipRegister::SipRegister() : m_regTimer(nullptr)
{
}

SipRegister::~SipRegister()
{
    if (m_regTimer)
    {
        delete m_regTimer;
        m_regTimer = nullptr;
    }
}

pj_status_t SipRegister::run(pjsip_rx_data *rdata)
{
    LOG(INFO) << "SipRegister::run";
    // 处理接收到的消息
    return RegisterRequestMessage(rdata);
}

pj_status_t SipRegister::RegisterRequestMessage(pjsip_rx_data *rdata)
{
    pjsip_msg *msg = rdata->msg_info.msg;
    // 判断是否注册是健权
    if (GlobalCtl::getAuth(parseFromId(msg)))
    {
        return dealWithAuthorRegister(rdata);
    }
    else
    {
        return dealWithRegister(rdata);
    }
    // return dealWithRegister(rdata);
}

pj_status_t SipRegister::dealWithAuthorRegister(pjsip_rx_data *rdata)
{

    pjsip_msg *msg = rdata->msg_info.msg;
    // 获取sipid
    std::string fromId = parseFromId(msg);
    int expiresValue = 0;
    pjsip_hdr hdr_list;
    pj_list_init(&hdr_list);
    int status_code = 401;
    pj_status_t status;
    bool registered = false;
    // 寻找是否有健权头
    if (pjsip_msg_find_hdr(msg, PJSIP_H_AUTHORIZATION, NULL) == NULL)
    {
        pjsip_www_authenticate_hdr *hdr = pjsip_www_authenticate_hdr_create(rdata->tp_info.pool);
        hdr->scheme = pj_str("digest");
        // nonce
        //string nonce = GlobalCtl::randomNum(32);
        char *nonce = (char *)malloc(GlobalCtl::randomNum(32).length() + 1);
        memset(nonce, 0, GBOJ(_gConfig)->realm().length() + 1);
        strcpy(nonce, GlobalCtl::randomNum(32).c_str());
        LOG(INFO) << "nonce:" << nonce;
        hdr->challenge.digest.nonce = pj_str(nonce);
        // realm
        string realmstr = (char *)GBOJ(_gConfig)->realm().c_str();
        hdr->challenge.digest.realm = pj_str((char*)realmstr.c_str());
        //LOG(ERROR) << "realm:" << (char *)GBOJ(_gConfig)->realm().c_str();
        // opaque
        //string opaque = GlobalCtl::randomNum(32);
        char *opaque = (char *)malloc(GlobalCtl::randomNum(32).length() + 1);
        memset(opaque, 0, GBOJ(_gConfig)->realm().length() + 1);
        strcpy(opaque, GlobalCtl::randomNum(32).c_str());
        LOG(INFO) << "opaque:" << opaque;
        hdr->challenge.digest.opaque = pj_str(opaque);
        // 加密方式
        hdr->challenge.digest.algorithm = pj_str("MD5");

        pj_list_push_back(&hdr_list, hdr);

        status = pjsip_endpt_respond(GBOJ(gSipServer)->get_pjsip_endpoint(), NULL, rdata, status_code, NULL, &hdr_list, NULL, NULL);
       free(opaque);
       free(nonce);
    }
    else
    {
        // 描述服务器身份信息的结构体
        pjsip_auth_srv auth_srv;

        char *dest = (char *)malloc(GBOJ(_gConfig)->realm().length() + 1);
        memset(dest, 0, GBOJ(_gConfig)->realm().length() + 1);
        strcpy(dest, GBOJ(_gConfig)->realm().c_str());
        pj_str_t realm = pj_str(dest);
        status = pjsip_auth_srv_init(rdata->tp_info.pool, &auth_srv, &realm, &auth_cred_callback, 0);
        if (PJ_SUCCESS != status)
        {
            LOG(ERROR) << "pjsip_auth_srv_init failed";
            status_code = 401;
        }
        pjsip_auth_srv_verify(&auth_srv, rdata, &status_code);
        LOG(INFO) << "status_code:" << status_code;
        free(dest);

        if (SIP_SUCCESS == status_code)
        {
            pjsip_expires_hdr *expires = (pjsip_expires_hdr *)pjsip_msg_find_hdr(msg, PJSIP_H_EXPIRES, NULL);
            expiresValue = expires->ivalue;
            GlobalCtl::setExpires(fromId, expiresValue);

            // data字段hdr部分组织
            time_t t;
            t = time(0);
            char bufT[32] = {0};
            strftime(bufT, sizeof(bufT), "%y-%m-%d%H:%M:%S", localtime(&t));
            pj_str_t value_time = pj_str(bufT);
            pj_str_t key = pj_str("Date");
            pjsip_date_hdr *date_hrd = (pjsip_date_hdr *)pjsip_date_hdr_create(rdata->tp_info.pool, &key, &value_time);
            pj_list_push_back(&hdr_list, date_hrd);
            registered = true;
        }
        status = pjsip_endpt_respond(GBOJ(gSipServer)->get_pjsip_endpoint(), NULL, rdata, status_code, NULL, &hdr_list, NULL, NULL);
    }
    
    
    if (PJ_SUCCESS != status)
    {
        LOG(ERROR) << "pjsip_endpt_respond failed";
        return status;
    }
     if(registered)
    {
        if(expiresValue > 0)
        {
            time_t regTime = 0;
            struct sysinfo info;
            memset(&info,0,sizeof(info));
            int ret = sysinfo(&info);
            if(ret == 0)
            {
                regTime = info.uptime;
            }
            else
            {
                regTime = time(NULL);
            }
            GlobalCtl::setRegister(fromId,true);
            GlobalCtl::setLastRegTime(fromId,regTime);
        }
        else if(expiresValue == 0)
        {
            GlobalCtl::setRegister(fromId,false);
            GlobalCtl::setLastRegTime(fromId,0);
        }
    }
}

pj_status_t SipRegister::dealWithRegister(pjsip_rx_data *rdata)
{
    pjsip_msg *msg = rdata->msg_info.msg;
    int status_code = 200;
    string fromId = parseFromId(msg);
    LOG(INFO) << "fromId:" << fromId;
    pj_int32_t expiresValue = 0;
    if (!(GlobalCtl::checkIsExist(fromId)))
    {
        status_code = SIP_FORBIDDEN;
    }
    else
    {
        pjsip_expires_hdr *expires = (pjsip_expires_hdr *)pjsip_msg_find_hdr(msg, PJSIP_H_EXPIRES, NULL);
        expiresValue = expires->ivalue;
        GlobalCtl::setExpires(fromId, expiresValue);
    }
    // 发送数据
    pjsip_tx_data *txdata;
    // 创建txdata数据结构
    pj_status_t status = pjsip_endpt_create_response(GBOJ(gSipServer)->get_pjsip_endpoint(), rdata, status_code, NULL, &txdata);
    if (PJ_SUCCESS != status)
    {
        LOG(ERROR) << "create response failed";
        return status;
    }
    time_t t;
    t = time(0);
    char bufT[32] = {0};
    strftime(bufT, sizeof(bufT), "%y-%m-%d%H:%M:%S", localtime(&t));
    // LOG(INFO)<<"TIME::"<<bufT;
    pj_str_t timeStr = pj_str(bufT);
    pj_str_t key = pj_str("Date");
    // 创建数据头Date，并分配到内存
    pjsip_date_hdr *data_head = pjsip_date_hdr_create(txdata->pool, &key, &timeStr);
    // 将Date加入到要发送的结构体的消息中
    pjsip_msg_add_hdr(txdata->msg, (pjsip_hdr *)data_head);

    // 获取响应地址
    pjsip_response_addr res_addr;
    status = pjsip_get_response_addr(txdata->pool, rdata, &res_addr);
    if (PJ_SUCCESS != status)
    {
        pjsip_tx_data_dec_ref(txdata);
        LOG(ERROR) << "get response addr failed";
        return status;
    }
    // 发送响应
    status = pjsip_endpt_send_response(GBOJ(gSipServer)->get_pjsip_endpoint(), &res_addr, txdata, NULL, NULL);
    if (PJ_SUCCESS != status)
    {
        pjsip_tx_data_dec_ref(txdata);
        LOG(ERROR) << "send_response msg failed";
        return status;
    }

    if (status_code == 200)
    {
        // 判断是注册还是登录
        if (expiresValue > 0)
        {
            // 获取当前系统时间
            time_t currtime = 0;
            struct sysinfo _info;
            memset(&_info, 0, sizeof(_info));
            int ret = sysinfo(&_info);
            if (ret == 0)
            {
                currtime = _info.uptime;
            }
            if (ret == -1)
            {
                currtime = time(NULL);
            }
            GlobalCtl::setRegister(fromId, true);
            GlobalCtl::setLastRegTime(fromId, currtime);
        }
        else if (expiresValue == 0)
        {
            GlobalCtl::setRegister(fromId, false);
            GlobalCtl::setLastRegTime(fromId, 0);
        }
    }
    return status;
}

void SipRegister::registerServiceStart()
{
    // 每60秒检测下级服务是否注册失效
    m_regTimer = new TaskTimer(60);
    m_regTimer->setTimerFun(RegisterCheckProc, this);
    m_regTimer->start();
}

void SipRegister::RegisterCheckProc(void *param)
{
    // 获取当前系统时间
    time_t currtime = 0;
    struct sysinfo _info;
    memset(&_info, 0, sizeof(_info));
    int ret = sysinfo(&_info);
    if (ret == 0)
    {
        currtime = _info.uptime;
    }
    if (ret == -1)
    {
        currtime = time(NULL);
    }
    AutoMutexLock lock(&(GlobalCtl::globalLock));

    GlobalCtl::SUBDOMAININFOLIST::iterator iter = GlobalCtl::instance()->getSubDomainInfoList().begin();
    for (; iter != GlobalCtl::instance()->getSubDomainInfoList().end(); iter++)
    {
        if (iter->registered)
        {
            LOG(INFO) << "currTime::" << currtime << "lastRegTime" << iter->lastRegTime;
            if (currtime - iter->lastRegTime >= iter->expires)
            {
                iter->registered = false;
                LOG(INFO) << "registor is expired";
            }
        }
    }
}
