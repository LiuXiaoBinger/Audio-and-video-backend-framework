#include "OpenStream.h"
#include "SipDef.h"
#include "SipMessage.h"
#include "GlobalCtl.h"
#include "Gb28181Session.h"
OpenStream::OpenStream(struct bufferevent* bev,void* arg)
    :ThreadTask(bev,arg)
{
   // m_pStreamTimer = new TaskTimer(600);
    // m_pCheckSessionTimer = new TaskTimer(5);
}

OpenStream::~OpenStream()
{
    #if 0
    LOG(INFO) << "~OpenStream";
    if (m_pStreamTimer)
    {
        delete m_pStreamTimer;
        m_pStreamTimer = NULL;
    }

    if (m_pCheckSessionTimer)
    {
        delete m_pCheckSessionTimer;
        m_pCheckSessionTimer = NULL;
    }
   #endif
}
 #if 0
void OpenStream::StreamServiceStart()
{
    if (m_pStreamTimer /* && m_pCheckSessionTimer */)
    {
        m_pStreamTimer->setTimerFun(OpenStream::StreamGetProc, this);
        m_pStreamTimer->start();

        /*  m_pCheckSessionTimer->setTimerFun(OpenStream::CheckSession,this);
         m_pCheckSessionTimer->start(); */
    }
}

 #endif
 void OpenStream::CheckSession(void *param)
{
    AutoMutexLock lck(&(GlobalCtl::gStreamLock));
    GlobalCtl::ListSession::iterator iter = GlobalCtl::glistSession.begin();
    while (iter != GlobalCtl::glistSession.end())
    {
        timeval curttime;
        gettimeofday(&curttime, NULL);
        Session *pSession = *iter;
        if (pSession != NULL && curttime.tv_sec - pSession->m_curTime.tv_sec >= 10)
        {
            /* 先发送sip层bye
            在发送rtp层bye */
            StreamStop(pSession->platformId, pSession->devid);
            Gb28181Session *pGb28181Session = dynamic_cast<Gb28181Session *>(pSession);
            // pGb28181Session->Destroy();  //rtp session destroy
            LOG(ERROR) << "pGb28181Session->Destroy()";
            delete pGb28181Session;
            iter = GlobalCtl::glistSession.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}
// 上级请求开流组织sdp发送给下级
void OpenStream::StreamGetProc(void *param)
{

    if (!GlobalCtl::gRcvIpc)
        return;
    LOG(INFO) << "start stream";
    DeviceInfo info;
    info.devid = "11000000001310000059";
    info.playformId = "11000000002000000001";
    if(streamType == 3)
    {
        info.streamName = "Play";
        info.startTime = 0;
        info.endTime = 0;
        info.protocal = 0;
    }
    else if(streamType == 5)
    {
        info.streamName = "PlayBack";
        info.startTime = 5;
        info.endTime = 10;
        info.protocal = 0;
    }
    info.bev = m_bev;
    //info.setupType ="passive";

#if 1
    {
        // 不支持同一设备及类型重复开流
        AutoMutexLock lck(&(GlobalCtl::gStreamLock));
        GlobalCtl::ListSession::iterator iter = GlobalCtl::glistSession.begin();
        while (iter != GlobalCtl::glistSession.end())
        {
            if ((*iter)->devid == info.devid && (*iter)->streamName == info.streamName)
            {
                LOG(INFO) << "不支持同一设备及类型重复开流:";
                return;
            }
        }
    }
#endif
    int rtp_port = GBOJ(_gConfig)->popOneRandNum();
    LOG(INFO) << "rtp_port:" << rtp_port;
    // request INVTE
    SipMessage msg;
    {
        AutoMutexLock lock(&(GlobalCtl::globalLock));
        GlobalCtl::SUBDOMAININFOLIST::iterator iter = GlobalCtl::instance()->getSubDomainInfoList().begin();
        for (; iter != GlobalCtl::instance()->getSubDomainInfoList().end(); iter++)
        {
            if (iter->sipId == info.playformId)
            {
                msg.setFrom((char *)GBOJ(_gConfig)->sipId().c_str(), (char *)GBOJ(_gConfig)->sipIp().c_str());
                msg.setTo((char *)iter->sipId.c_str(), (char *)iter->addrIp.c_str());
                msg.setUrl((char *)iter->sipId.c_str(), (char *)iter->addrIp.c_str(), iter->sipPort);
                msg.setContact((char *)GBOJ(_gConfig)->sipId().c_str(), (char *)GBOJ(_gConfig)->sipIp().c_str(), GBOJ(_gConfig)->sipPort());
            }
        }
    }

    pj_str_t from = pj_str(msg.FromHeader());
    pj_str_t to = pj_str(msg.ToHeader());
    pj_str_t contact = pj_str(msg.Contact());
    pj_str_t requestUrl = pj_str(msg.RequestUrl());

    // 创建dialog
    pjsip_dialog *dlg;
    pj_status_t status = pjsip_dlg_create_uac(pjsip_ua_instance(), &from, &contact, &to, &requestUrl, &dlg);
    if (PJ_SUCCESS != status)
    {
        LOG(ERROR) << "pjsip_dlg_create_uac ERROR";
        return;
    }

    // 组织sdp part
    pjmedia_sdp_session *sdp = NULL;

    sdp = (pjmedia_sdp_session *)pj_pool_zalloc(GBOJ(gSipServer)->GetPool(), sizeof(pjmedia_sdp_session));
    // Session Description Protocol Version (v): 0
    sdp->origin.version = 0;
    // Owner/Creator, Session Id (o): 33010602001310019325 0 0 IN IP4 10.64.49.44
    sdp->origin.user = pj_str((char *)info.devid.c_str());
    sdp->origin.id = 0;
    sdp->origin.net_type = pj_str("IN");
    sdp->origin.addr_type = pj_str("IP4");
    sdp->origin.addr = pj_str((char *)GBOJ(_gConfig)->sipIp().c_str());
    // Session Name (s): Play
    sdp->name = pj_str((char *)info.streamName.c_str());
    // Connection Information (c): IN IP4 10.64.49.218 //收流端ip信息
    sdp->conn = (pjmedia_sdp_conn *)pj_pool_zalloc(GBOJ(gSipServer)->GetPool(), sizeof(pjmedia_sdp_conn));
    sdp->conn->net_type = pj_str("IN");
    sdp->conn->addr_type = pj_str("IP4");
    sdp->conn->addr = pj_str((char *)GBOJ(_gConfig)->sipIp().c_str());
    // Time Description, active time (t): 0 0
    sdp->time.start = info.startTime;
    sdp->time.stop = info.endTime;
    // Media Description, name and address (m): video 5514 RTP/AVP 96 收留端端口
    sdp->media_count = 1;
    pjmedia_sdp_media *m = (pjmedia_sdp_media *)pj_pool_zalloc(GBOJ(gSipServer)->GetPool(), sizeof(pjmedia_sdp_media));
    sdp->media[0] = m;
    m->desc.media = pj_str("video");
    m->desc.port = rtp_port;
    m->desc.port_count = 1;
    if (info.protocal)
    {
        m->desc.transport = pj_str("TCP/RTP/AVP");
    }
    else
    {
        m->desc.transport = pj_str("RTP/AVP");
    }
    m->desc.fmt_count = 1;
    m->desc.fmt[0] = pj_str("96");
    /*
    Media Attribute (a): rtpmap:96 PS/90000
        Media Attribute (a): recvonly
        Media Attribute (a): setup:active
    */
    m->attr_count = 0;
    pjmedia_sdp_attr *attr = (pjmedia_sdp_attr *)pj_pool_zalloc(GBOJ(gSipServer)->GetPool(), sizeof(pjmedia_sdp_attr));
    attr->name = pj_str("rtpmap");
    attr->value = pj_str((char *)rtpmap_ps.c_str());
    m->attr[m->attr_count++] = attr;

    attr = (pjmedia_sdp_attr *)pj_pool_zalloc(GBOJ(gSipServer)->GetPool(), sizeof(pjmedia_sdp_attr));
    attr->name = pj_str("recvonly");
    m->attr[m->attr_count++] = attr;
    if (info.protocal)
    {
        attr = (pjmedia_sdp_attr *)pj_pool_zalloc(GBOJ(gSipServer)->GetPool(), sizeof(pjmedia_sdp_attr));
        attr->name = pj_str("setup");
        attr->value = pj_str((char *)info.setupType.c_str());
        m->attr[m->attr_count++] = attr;
    }

    pjsip_inv_session *inv;
    status = pjsip_inv_create_uac(dlg, sdp, 0, &inv);
    if (PJ_SUCCESS != status)
    {
        pjsip_dlg_terminate(dlg);
        LOG(ERROR) << "pjsip_inv_create_uac ERROR";
        return;
    }
    // 那么在这里就可以使用Gb28181Session类来实例化一个session类
    Session *pSession = new Gb28181Session(info);
    pSession->rtp_loaclport = rtp_port;
    inv->mod_data[0] = (void *)pSession;
    pjsip_tx_data *tdata;
    status = pjsip_inv_invite(inv, &tdata);
    if (PJ_SUCCESS != status)
    {
        pjsip_dlg_terminate(dlg);
        LOG(ERROR) << "pjsip_inv_invite ERROR";
        return;
    }
    pj_str_t subjectName = pj_str("Subject");
    char subjectBuf[128] = {0};
    sprintf(subjectBuf, "%s:0,%s:0", info.devid.c_str(), GBOJ(_gConfig)->sipId().c_str());
    pj_str_t subjectValue = pj_str(subjectBuf);
    pjsip_generic_string_hdr *hdr = pjsip_generic_string_hdr_create(GBOJ(gSipServer)->GetPool(), &subjectName, &subjectValue);
    pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr *)hdr);

    status = pjsip_inv_send_msg(inv, tdata);
    if (PJ_SUCCESS != status)
    {
        pjsip_dlg_terminate(dlg);
        LOG(ERROR) << "pjsip_inv_send_msg ERROR";
        return;
    }
    AutoMutexLock lck(&(GlobalCtl::gStreamLock));
    GlobalCtl::glistSession.push_back(pSession);
    // GlobalCtl::gRcvIpc = false;
    /*   sleep(3);
      StreamStop("11000000002000000001","11000000001310000059"); */
}

void OpenStream::run()
{
    StreamGetProc(NULL);
}



void OpenStream::StreamStop(string platformId, string devId)
{
    pj_thread_desc desc;
    pjcall_thread_register(desc);
    SipMessage msg;
    {
        AutoMutexLock lck(&(GlobalCtl::globalLock));
        GlobalCtl::SUBDOMAININFOLIST::iterator iter = GlobalCtl::instance()->getSubDomainInfoList().begin();
        for (; iter != GlobalCtl::instance()->getSubDomainInfoList().end(); iter++)
        {
            if (iter->sipId == platformId)
            {
                // header part
                msg.setFrom((char *)GBOJ(_gConfig)->sipId().c_str(), (char *)GBOJ(_gConfig)->sipIp().c_str());
                msg.setTo((char *)devId.c_str(), (char *)iter->addrIp.c_str());
                msg.setUrl((char *)devId.c_str(), (char *)iter->addrIp.c_str(), iter->sipPort);
                msg.setContact((char *)devId.c_str(), (char *)iter->addrIp.c_str(), iter->sipPort);
            }
        }
    }
    pjsip_tx_data *tdata;
    pj_str_t from = pj_str(msg.FromHeader());
    pj_str_t to = pj_str(msg.ToHeader());
    pj_str_t requestUrl = pj_str(msg.RequestUrl());
    pj_str_t contact = pj_str(msg.Contact());
    std::string method = "BYE";
    pjsip_method reqMethod = {PJSIP_OTHER_METHOD, {(char *)method.c_str(), method.length()}};
    pj_status_t status = pjsip_endpt_create_request(GBOJ(gSipServer)->get_pjsip_endpoint(), &reqMethod, &requestUrl, &from, &to, &contact, NULL, -1, NULL, &tdata);
    if (status != PJ_SUCCESS)
    {
        LOG(ERROR) << "request catalog error";
        return;
    }

    // 发送信令，将当前用户保存到token中，作为回调的参数
    status = pjsip_endpt_send_request(GBOJ(gSipServer)->get_pjsip_endpoint(), tdata, -1, NULL, NULL);
    if (status != PJ_SUCCESS)
    {
        LOG(ERROR) << "send request error";
    }
    return;
}
