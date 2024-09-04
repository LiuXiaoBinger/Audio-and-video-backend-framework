#include "SipGbPlay.h"
#include "GlobalCtl.h"
#include "Common.h"
#include "SipDef.h"
#include "Gb28181Session.h"
#include "SipMessage.h"

SipGbPlay::MediaStreamInfo SipGbPlay::mediaInfoMap;
pthread_mutex_t SipGbPlay::streamLock = PTHREAD_MUTEX_INITIALIZER;
SipGbPlay::SipGbPlay()
{
}

SipGbPlay::~SipGbPlay()
{
}

void SipGbPlay::run(pjsip_rx_data *rdata)
{
    pjsip_msg *msg = rdata->msg_info.msg;
    if (msg->line.req.method.id == PJSIP_INVITE_METHOD)
    {
        dealWithInvite(rdata);
    }
    else if (msg->line.req.method.id == PJSIP_BYE_METHOD)
    {
        // 这里我们来实现下级处理上级的BYE请求
        dealWithBye(rdata);
    }
}
void SipGbPlay::dealWithInvite(pjsip_rx_data *rdata)
{
    string fromId = parseFromId(rdata->msg_info.msg);
    bool flag = false;
    int status_code = 200;
    string id;
    MediaInfo sdpInfo;
    SipPsCode *ps = NULL;
    do
    {
        {
            AutoMutexLock lock(&(GlobalCtl::globalLock));
            GlobalCtl::SUPDOMAININFOLIST::iterator iter = GlobalCtl::instance()->getSupDomainInfoList().begin();
            for (; iter != GlobalCtl::instance()->getSupDomainInfoList().end(); iter++)
            {
                if (iter->sipId == fromId && iter->registered)
                {
                    flag = true;
                    break;
                }
            }
        }

        if (!flag)
        {
            status_code = SIP_FORBIDDEN;
            break;
        }

        pjmedia_sdp_session *sdp = NULL;
        if (rdata->msg_info.msg->body)
        {
            pjsip_rdata_sdp_info *sdp_info = pjsip_rdata_get_sdp_info(rdata);
            sdp = sdp_info->sdp;
        }

        if (sdp && sdp->media_count == 0)
        {
            status_code = SIP_BADREQUEST;
            break;
        }

        string devId(sdp->origin.user.ptr, sdp->origin.user.slen);
        DevTypeCode type = GlobalCtl::getSipDevInfo(devId);
        if (type == Error_code)
        {
            status_code = SIP_FORBIDDEN;
            break;
        }
        id = devId;

        string tmp(sdp->name.ptr, sdp->name.slen);
        sdpInfo.sessionName = tmp;
        if (sdpInfo.sessionName == "PlayBack")
        {
            sdpInfo.startTime = sdp->time.start;
            sdpInfo.endTime = sdp->time.stop;
            
        }
        pjmedia_sdp_conn *c = sdp->conn;
        string dst_ip(c->addr.ptr, c->addr.slen);
        sdpInfo.dstRtpAddr = dst_ip;

        pjmedia_sdp_media *m = sdp->media[sdp->media_count - 1];
        int sdp_port = m->desc.port;
        sdpInfo.dstRtpPort = sdp_port;

        string protol(m->desc.transport.ptr, m->desc.transport.slen);
        sdpInfo.sdp_protol = protol;
        int poto = 0;
        if (sdpInfo.sdp_protol == "TCP/RTP/AVP")
        {
            poto = 1;
            pjmedia_sdp_attr *attr = pjmedia_sdp_attr_find2(m->attr_count, m->attr, "setup", NULL);
            string setup(attr->value.ptr, attr->value.slen);
            sdpInfo.setUp = setup;
        }

        sdpInfo.localRtpPort = GBOJ(_gConfig)->popOneRandNum();
        LOG(INFO) << "======sdp_port:" << sdp_port; // 对端链接的端口
        ps = new SipPsCode(dst_ip, sdp_port, sdpInfo.localRtpPort ,poto,sdpInfo.setUp,sdpInfo.startTime,sdpInfo.endTime );

        {
            // 需要在ps对象实例化后就插入到map中
            AutoMutexLock lck(&streamLock);
            mediaInfoMap.insert(pair<string, SipPsCode *>(devId, ps));
        }

    } while (0);

    resWithSdp(rdata, status_code, id, sdpInfo);

    sendPsRtpStream(&ps);
}
void SipGbPlay::dealWithBye(pjsip_rx_data *rdata)
{
    int code = SIP_SUCCESS;

    std::string devId = parseToId(rdata->msg_info.msg);
    LOG(INFO) << "======BYE  devId:" << devId;
    do
    {
        if (devId == "")
        {
            code = SIP_BADREQUEST;
            break;
        }
        AutoMutexLock lck(&streamLock);
        auto iter = mediaInfoMap.find(devId);
        if (iter != mediaInfoMap.end())
        {
            // 我们先判断下SipPsCode句柄是否不为空
            if (iter->second != NULL)
            {
                iter->second->stopFlag = true;
            }
            // delete iter->second;
            // 最后还需要从map中删除当前的键值对

            iter = mediaInfoMap.erase(iter);
        }
        else
        {
            code = SIP_FORBIDDEN;
        }
    } while (false);

    pj_status_t status = pjsip_endpt_respond(GBOJ(gSipServer)->get_pjsip_endpoint(), NULL, rdata, code, NULL, NULL, NULL, NULL);
    if (PJ_SUCCESS != status)
    {
        LOG(ERROR) << "create response failed";
        return;
    }
}
void SipGbPlay::sendPsRtpStream(SipPsCode **ps)
{
    int ret = (*ps)->initPsEncode();
    if (ret < 0)
    {
        LOG(ERROR) << "initPsEncode error:" << ret;
        return;
    }

    ret = recvFrame(&(*ps));
    if (ret < 0)
    {
        LOG(ERROR) << "recvFrame error";
    }
    return;
}
int SipGbPlay::recvFrame(SipPsCode **ps)
{
    // string out_video_path = "/home/ap/safm/ccbc/bin/forkCancer/out222222222.h264";
    // FILE* h264_fp = fopen(out_video_path.c_str(),"wb");
    // FILE* fp = fopen("/home/ap/safm/ccbc/bin/forkCancer/stream.file","rb");
    // if(!fp)
    // return -1;

    string out_video_path = "/mnt/hgfs/shared/conf/out.h264";
    FILE *h264_fp = fopen(out_video_path.c_str(), "wb");
    FILE *fp = fopen("/mnt/hgfs/shared/conf/stream.file", "rb");
    //在这里我们先对录像时间进行个转换，转成毫秒,因为数据的头部里的pts为毫秒级别
	int start = 0;
	int end = 0;
	bool backflag = false;  //定义个回放flag
	if((*ps)->m_sTime >= 0 && (*ps)->m_eTime > 0)
	{
		start = (*ps)->m_sTime * 1000;
		end = (*ps)->m_eTime * 1000;
		backflag = true;
	}
    if (!fp)
        return -1;
    /// 在这里我们先对录像时间进行个转换，转成毫秒,因为数据的头部里的pts为毫秒级别
    int ret = 0;
    unsigned char *buf = new unsigned char[sizeof(StreamHeader)];
    LOG(INFO) << "wite start";
    int filesize = 0;
    while (!feof(fp))
    {
        // 需要判断下结束的flag
            // 为true则发送rtp层的bye，并退出当前取流和推流的线程
            if ((*ps)->stopFlag)
            {
                
                delete *ps;
                *ps = NULL;
                break;
            }
        memset(buf, 0, sizeof(StreamHeader));
        // 一个字节一个字节读文件到buff
        int size = fread(buf, 1, sizeof(StreamHeader), fp);

        if (size < 0)
        {
            ret = -1;
        }
        StreamHeader* header = (StreamHeader*)buf;
		LOG(INFO)<<"header->pts:"<<header->pts;
		unsigned char* data = new unsigned char[header->length];
		fread(data,1,header->length,fp);

        if (header->type == 2)
        {
            if(backflag)
			{
				if(header->pts < start)
				{
					continue;  //协议头里的pts小于起始时间，那么我们就不推流
				}
				else if(header->pts > end)
				{
					break;   //当pts大于录像结束时间后我们断流
				}
			}
            /* 封装ps流 这里我们判断一下，如果就是说我们调用接口出错了，
            代表的是这一帧数据有可能不是一个完整帧，就是缺失了一些参数，缺失了一些编解码的数据，
            那么有可能导致它封装错误。因为现在在封装的时候，它需要用hr64的解码器，
            来去解析hr64的一个裸流获取一些参数。也就是说只有在当前真的H264的裸流的数据不完整的时候，
            才会导致它封装错误，那么封装错误的话我们就直接过掉。我们接着封装下一个，如果小于0的话，
            那么我们就继续continue */
            if ((*ps)->incomeVideoData(data, header->length, header->pts, header->keyFrame) < 0)
            {
                LOG(ERROR) << "wite falie:" << header->length << "";
                
                continue;
            }
            // 这里我们将去掉实部的裸流数据保存到本地文件中查看解析后的数据的完整性，以及是否可以正常播放
            size = fwrite(data, 1, header->length, h264_fp);
            filesize += size;
            LOG(INFO) << "wite size:" << size;
            
        }
        delete data;
        if (header->type != 2)
        {
            continue;
        }
        
        usleep(90000);
    }
    // LOG(ERROR)<<"wite finsh:"<<filesize/1024<<"k";
    delete buf;
    fclose(h264_fp);
    fclose(fp);
    sleep(1);
    LOG(ERROR) << "wite finsh:" << filesize / 1024 << "k";
    if ((*ps) != NULL)
    {
        delete *ps;
        *ps = NULL;
    }
    LOG(ERROR) << "wite finsh:" << filesize / 1024 << "k";
    return ret;
}

void SipGbPlay::resWithSdp(pjsip_rx_data *rdata, int status_code, string devid, MediaInfo sdpInfo)
{
    pjsip_tx_data *tdata;
    pjsip_endpt_create_response(GBOJ(gSipServer)->get_pjsip_endpoint(), rdata, status_code, NULL, &tdata);
    pj_str_t type = {"Application", 11};
    pj_str_t sdptype = {"SDP", 3};
    if (status_code != SIP_SUCCESS)
    {
        tdata->msg->body = pjsip_msg_body_create(tdata->pool, &type, &sdptype, &(pjsip_rdata_get_sdp_info(rdata)->body));
    }
    else
    {
        stringstream ss;
        ss << "v=" << "0" << "\r\n";
        ss << "o=" << devid << " 0 0 IN IP4 " << GBOJ(_gConfig)->sipIp() << "\r\n";
        ss << "s=" << "Play" << "\r\n";
        ss << "c=" << "IN IP4 " << GBOJ(_gConfig)->sipIp() << "\r\n";
        ss << "t=" << "0 0" << "\r\n";
        ss << "m=video " << sdpInfo.localRtpPort << " " << sdpInfo.sdp_protol << " 96" << "\r\n";
        ss << "a=rtpmap:96 PS/90000" << "\r\n";
        ss << "a=sendonly" << "\r\n";
        if (sdpInfo.setUp != "")
        {
            if (sdpInfo.setUp == "passive")
            {
                ss << "a=setup:" << "active" << "\r\n";
            }
            else if (sdpInfo.setUp == "active")
            {
                ss << "a=setup:" << "passive" << "\r\n";
            }
        }

        string sResp = ss.str();
        pj_str_t sdpData = pj_str((char *)sResp.c_str());
        tdata->msg->body = pjsip_msg_body_create(tdata->pool, &type, &sdptype, &sdpData);
    }
    // pjsip底层会检测字段，，如果不对就算返回200ok 对端也会判断回复失败
    SipMessage msg;
    msg.setContact((char *)GBOJ(_gConfig)->sipId().c_str(), (char *)GBOJ(_gConfig)->sipIp().c_str(), GBOJ(_gConfig)->sipPort());
    const pj_str_t contactHeader = pj_str("Contact");
    const pj_str_t param = pj_str(msg.Contact());
    pjsip_generic_string_hdr *customHeader = pjsip_generic_string_hdr_create(tdata->pool, &contactHeader, &param);
    pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr *)customHeader);
    // 获取发送端目的信息
    pjsip_response_addr res_addr;
    pj_status_t status = pjsip_get_response_addr(tdata->pool, rdata, &res_addr);
    if (PJ_SUCCESS != status)
    {
        pjsip_tx_data_dec_ref(tdata);
        return;
    }
    status = pjsip_endpt_send_response(GBOJ(gSipServer)->get_pjsip_endpoint(), &res_addr, tdata, NULL, NULL);
    if (PJ_SUCCESS != status)
    {
        pjsip_tx_data_dec_ref(tdata);
        return;
    }

    return;
}

void SipGbPlay::OnStateChanged(pjsip_inv_session *inv, pjsip_event *e)
{
}
void SipGbPlay::OnNewSession(pjsip_inv_session *inv, pjsip_event *e)
{
}
void SipGbPlay::OnMediaUpdate(pjsip_inv_session *inv_ses, pj_status_t status)
{
}
void SipGbPlay::OnSendAck(pjsip_inv_session *inv, pjsip_rx_data *rdata)
{
}