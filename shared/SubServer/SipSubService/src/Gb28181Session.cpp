#include "Gb28181Session.h"
#include "ECSocket.h"
using namespace EC;
Gb28181Session::Gb28181Session(const DeviceInfo &devInfo)
{
}

Gb28181Session::Gb28181Session()
{
}

Gb28181Session::~Gb28181Session()
{
    if (m_rtpTcpFd != -1)
    {
        DeleteDestination(RTPTCPAddress(m_rtpTcpFd)); // rtp-tcp时需要删除连接
        close(m_rtpTcpFd);
        m_rtpTcpFd = -1;
    }

    if (m_listenFd != -1)
    {
        close(m_listenFd);
        m_listenFd = -1;
    }
    // 发送BYE数据包并离开会话 不用等待
    BYEDestroy(RTPTime(0, 0), 0, 0);
}

int Gb28181Session::CreateRtpSession(int poto, string setup, string dstip, int dstport, int rtpPort)
{
    LOG(INFO) << "CreateRtpSession";
    uint32_t destip = inet_addr(dstip.c_str());
    destip = ntohl(destip);
    RTPSessionParams sessParams;
    sessParams.SetOwnTimestampUnit(1.0 / 90000.0); // 采样率上级对下级INVITE
    sessParams.SetAcceptOwnPackets(true);
    sessParams.SetUsePollThread(true);
    sessParams.SetNeedThreadSafety(true);
    sessParams.SetMinimumRTCPTransmissionInterval(RTPTime(5, 0));
    int ret = -1;
    if (poto == 0)
    {
        RTPUDPv4TransmissionParams transparams;

        transparams.SetPortbase(rtpPort);
        // 设置一下rtp发送的缓冲区的大小
        transparams.SetRTPSendBuffer(JRTP_SET_SEND_BUFFER);
        // 然后跳数就设置25，就这个数据包它是经过一个路由器它就会减一，如果说这个数据包就是减为0了之后还没有到达对端的话，那么这个包就会被丢弃，所以说我们这个值设置稍微大一点
        transparams.SetMulticastTTL(JRTP_SET_MULITTTL);
        ret = Create(sessParams, &transparams);
        LOG(INFO) << "ret:" << ret;
        if (ret < 0)
        {
            LOG(ERROR) << "udp create fail";
        }
        else
        {
            RTPIPv4Address dest(destip, dstport);
            AddDestination(dest);
            // 这里将ip和prot填入源表上级的OnNewSource就会触发
            LOG(INFO) << "udp create ok,bind:" << rtpPort;
        }
    }
    else
    {
        sessParams.SetMaximumPacketSize(65535);
        RTPTCPTransmissionParams transParams;
        ret = Create(sessParams, &transParams, RTPTransmitter::TCPProto);
        if (ret < 0)
        {
            LOG(ERROR) << "Rtp tcp error: " << RTPGetErrorString(ret);
            return -1;
        }

        // 会话创建成功后，接下来我们需要创建tcp连接
        int sessFd = RtpTcpInit(dstip, dstport, rtpPort, setup, 5);
        if (sessFd < 0)
        {
            LOG(ERROR) << "RtpTcpInit faild";
            return -1;
        }
        else
        {
            AddDestination(RTPTCPAddress(sessFd));
        }
    }
    return ret;
}

int Gb28181Session::RtpTcpInit(string dstip, int dstport, int localPort, string setup, int time)
{
    LOG(INFO) << "setup:" << setup;
    int timeout = time * 1000;
    // 在这里我们就需要判断SDP中的setup字段值来调用不同的接口
    if (setup == "active")
    {
        m_rtpTcpFd = ECSocket::createConnByPassive(&m_listenFd, localPort, &timeout);
    }
    else if (setup == "passive")
    {
        m_rtpTcpFd = ECSocket::createConnByActive(dstip, dstport, localPort, &timeout);
    }
    LOG(INFO) << "m_rtpTcpFd:" << m_rtpTcpFd << ",m_listenFd:" << m_listenFd;
    return m_rtpTcpFd;
}

void Gb28181Session::OnRTCPCompoundPacket(RTCPCompoundPacket *pack, const RTPTime &receivetime, const RTPAddress *senderaddress)
{
    RTCPPacket *rtcpPack;
    pack->GotoFirstPacket();
    while ((rtcpPack = pack->GetNextPacket()) != 0)
    {
        if (rtcpPack->IsKnownFormat())
        {
            switch (rtcpPack->GetPacketType())
            {
            case RTCPPacket::RR:
            {
                // 这里我们每次收到RR包，我们就更新下时间戳，我们用时间来检测RR包的机制
                time_t now_time = time(NULL);
                m_rtpRRTime = now_time;
                LOG(INFO) << "=====m_rtpRRTime:" << m_rtpRRTime;
                break;
            }
            }
        }
    }
}

void *SipPsCode::Alloc(void *param, size_t bytes)
{
    return malloc(bytes);
}

void SipPsCode::Free(void *param, void *packet)
{
    return free(packet);
}

int SipPsCode::onPsPacket(void *param, int stream, void *packet, size_t bytes)
{
    LOG(INFO) << bytes << " packet demutex";
    SipPsCode *self = (SipPsCode *)param;
    self->sendPackData(packet, bytes);
}
int SipPsCode::incomeVideoData(unsigned char *avdata, int len, int pts, int isIframe)
{
    if (m_avStreamIndex == -1)
    {
        // 想这个封装器添加一些流的信息 STREAM_VIDEO_H264流的编解码描述符 返回添加流的索引值类似一个句柄
        m_avStreamIndex = ps_muxer_add_stream(m_muxer, STREAM_VIDEO_H264, NULL, 0);
    }
    if (m_gbRtpHandle->CheckAlive())
    {
        LOG(ERROR) << "[OnRecv] The upper service connection is disconnected and send ps packet is stopped";
        this->stopFlag = true;
        return -1;
    }
    // 关键帧：ps h | ps sys h| ps sys map|pes h|h264 raw data
    // 非关键帧： ps h | pes h|h264 raw data
    // 音频：pes h | aac raw data
    int ret = ps_muxer_input(m_muxer, m_avStreamIndex, isIframe, pts, pts, avdata, len); // isIframe表示是i真还是非i帧
    if (ret < 0)
    {
        LOG(INFO) << "error to push av frame:" << ret;
    }

    return ret;
}
int SipPsCode::incomeAudioData(unsigned char *audata, int len, int pts)
{
    if (m_auStreamIndex == -1)
    {
        m_auStreamIndex = ps_muxer_add_stream(m_muxer, STREAM_AUDIO_AAC, NULL, 0);
    }
    if (m_gbRtpHandle->CheckAlive())
    {
        LOG(ERROR) << "[OnRecv] The upper service connection is disconnected and send ps packet is stopped";
        this->stopFlag = true;
        return -1;
    }
    // 关键帧：ps h | ps sys h| ps sys map|pes h|h264 raw data
    // 非关键帧： ps h | pes h|h264 raw data
    // 音频：pes h | aac raw data
    int ret = ps_muxer_input(m_muxer, m_auStreamIndex, 0, pts, pts, audata, len);
    if (ret < 0)
    {
        LOG(INFO) << "error to push au frame:" << ret;
    }

    return ret;
}
int SipPsCode::sendPackData(void *packet, size_t bytes)
{
    // 这里需要根据网络1500字节来拆分ps包
    int ps_buff_len = 1300;
    int size = 0;
    int status = 0;
    while (size < bytes)
    {
        int packlen = (bytes - size) >= ps_buff_len ? ps_buff_len : (bytes - size);
        /*  我们来先判断一下，如果说当前进来的数据，
         Python的长度是小于个1300字节的，那么我们就直接整包发送，
         这个逻辑其实就是给音频包来制定的 */
        if (bytes < ps_buff_len)
        {
            status = m_gbRtpHandle->SendPacket(packet, bytes, 96, true, PS_SEND_TIMESTAME);
            if (status < 0)
            {
                LOG(ERROR) << RTPGetErrorString(status);
            }
        }
        else
        {
            if ((bytes - size) > ps_buff_len)
            {
                status = m_gbRtpHandle->SendPacket((packet + size), packlen, 96, false, 0);
                if (status < 0)
                {
                    LOG(ERROR) << RTPGetErrorString(status);
                }
            }
            else
            {
                status = m_gbRtpHandle->SendPacket((packet + size), packlen, 96, true, PS_SEND_TIMESTAME);
                if (status < 0)
                {
                    LOG(ERROR) << RTPGetErrorString(status);
                }
            }
        }
        size += packlen;
    }
    return status;
}
// 初始化ps封装器
int SipPsCode::initPsEncode()
{
    // 初始化回调，释放资源，开辟空间,其中对于数据包的就是packet的内存分配释放以及处理的回调函数
    ps_muxer_func_t func;
    func.alloc = Alloc;
    func.free = Free;
    func.write = onPsPacket;

    m_muxer = ps_muxer_create(&func, this);
    if (nullptr == m_muxer)
    {
        LOG(ERROR) << "ps_muxer_create failed";
        return -1;
    }
    sleep(1);
    if (gbRtpInit() < 0)
    {
        LOG(ERROR) << "rtp init error";
        return -1;
    }
    return 0;
}

int SipPsCode::gbRtpInit()
{
    m_gbRtpHandle = new Gb28181Session();
    return m_gbRtpHandle->CreateRtpSession(this->m_poto, this->m_connType, this->m_dstip, this->m_dstport, this->m_localRtpPort);
}
