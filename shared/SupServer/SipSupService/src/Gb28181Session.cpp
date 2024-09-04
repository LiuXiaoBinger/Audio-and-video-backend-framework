#include "Gb28181Session.h"
#include "SipDef.h"
#include "mpeg4-avc.h"
#include "h264_stream.hpp"
#include "h265_stream.hpp"
#include "mpeg4-hevc.h"
#include "ECSocket.h"
// 定义个结构体
typedef struct
{
    uint16_t width;
    uint16_t height;
    float max_framerate;
} Picinfo;
// 这里定义个接口来实现从h264的编码中获取分辨率和帧率
int GetH264pic(const uint8_t *data, uint16_t size, Picinfo *info)
{

    // 解析分辨率
    if (data == NULL || size == 0)
    {
        return 1;
    }
    if (info == NULL)
    {
        return 1;
    }
    memset(info, 0, sizeof(Picinfo));

    int ret = 999;
    char s_buffer[4 * 1024];
    memset(s_buffer, 0, sizeof(s_buffer));
    struct mpeg4_avc_t avc;
    memset(&avc, 0, sizeof(mpeg4_avc_t));
    struct mpeg4_hevc_t hevc;
    memset(&hevc, 0, sizeof(mpeg4_hevc_t));

    ret = (int)mpeg4_annexbtomp4(&avc, data, size, s_buffer, sizeof(s_buffer));
    if (avc.nb_sps <= 0)
    {
        return 1;
    }

    h264_stream_t *h4 = h264_new();
    // 因为一帧数据里边也就是说一个idr帧数据里边只会有一个只会有一段sps的裸流，所以说它一直是存放在这个数组
    ret = h264_configure_parse(h4, avc.sps[0].data, avc.sps[0].bytes, H264_SPS);
    if (ret != 0)
    {
        h264_free(h4);
        return 1;
    }
    // 获取分辨率
    info->width = h4->info->width;
    info->height = h4->info->height;
    info->max_framerate = h4->info->max_framerate;
    h264_free(h4);

    return 0;
}
// 需要用codecid来判别裸流是音频还是视频数据
// stream 流的编号  用不上
// flags 视频关键帧的标识
static void ps_demux_callback(void *param, int stream, int codecid, int flags, int64_t pts, int64_t dts, const void *data, size_t bytes)
{
    LOG(INFO) << "data:" << bytes;
    PackProcStat *pProc = (PackProcStat *)param;
    // 然后需要在结构体里再添加几个成员

    // 数据流类型(音/视频)
    int media = -1;
    // 视频帧类型(I/P帧)
    int frameType = 1;

    // 我们先判别下当前的codecid是否有效
    if (codecid == 0) // unknown codec id
    {
        return;
    }

    // 首先先处理同一类型的流数据被拆成多个包的情况
    // 当前回调被第一次调用时，不会走这个逻辑
    if (pProc->sCodec == codecid && pProc->sPts == pts)
    {
        // 我们需要将当前的裸流保存到buffer，后续我们再处理buffer的数据
        if (pProc->slen < pProc->sNow + bytes + 1024)
        {
            pProc->slen = pProc->sNow + bytes + 1024;
            pProc->sBuf = (char *)realloc(pProc->sBuf, pProc->slen);
        }
        memcpy(pProc->sBuf + pProc->sNow, data, bytes);
        pProc->sNow += bytes;
        pProc->sKeyFrame = flags;
        return;
    }

    // 当前这个接口第一次调用时，会走这个逻辑
    // 后续这个逻辑是来处理当前传入的data为下一帧数据或者不同类型的数据，
    else
    {
        // 如果发送buffer的数据大于0，那么我们就进行发送，发送前还需要进行header的设定，一会我们再做
        if (pProc->sNow > 0)
        {
            do
            {
                if (pProc->sCodec == STREAM_VIDEO_H264 || pProc->sCodec == STREAM_VIDEO_H265)
                {
                    // 在这里可以先区分媒体流的类型，和当前视频帧是否为关键帧
                    media = 2;
                    // frameType = pProc->sKeyFrame > 1 ? FORMAT_VIDEO_I : FORMAT_VIDEO_P;
#if 1
                    if (pProc->sFp == NULL)
                    {
                        pProc->sFp = fopen("../../../conf/send.h264", "w+");
                    }

                    if (pProc->sFp != NULL)
                    {
                        fwrite(pProc->sBuf, 1, pProc->sNow, pProc->sFp);
                    }
#endif
                }
                else if (pProc->sCodec == STREAM_AUDIO_AAC || pProc->sCodec == STREAM_AUDIO_G711A || pProc->sCodec == STREAM_AUDIO_G711U)
                {
                    media = 1;
                }
                else
                {
                    // 	if(pProc->unknownCodecCnt == 0)
                    // 		LOG(INFO) << " unknown codec: " << pProc->sCodec;

                    // 	if(pProc->unknownCodecCnt ++ > 200)
                    // 		pProc->unknownCodecCnt = 0;

                    break;
                }

                ////unsigned long long microsecIn = pProc->sDts * 1000 / 90000;
                Gb28181Session *pGbSesson = (Gb28181Session *)pProc->session;
                int sendLen = pGbSesson->SendPacket(media, (char *)pProc->sBuf, pProc->sNow, pProc->sCodec);
            } while (0);
        }
        // LOG(INFO)<<"33333333";
        //  copy new data to send buffer
        memset(pProc->sBuf, 0, pProc->sNow);
        pProc->sNow = 0;
        pProc->sKeyFrame = 0;
        // 将当前数据copy到buffer中
        if (pProc->slen < pProc->sNow + bytes + 1024)
        {
            pProc->slen = pProc->sNow + bytes + 1024;
            pProc->sBuf = (char *)realloc(pProc->sBuf, pProc->slen);
        }
        memcpy(pProc->sBuf + pProc->sNow, data, bytes);
        pProc->sNow += bytes;
        pProc->sKeyFrame = flags;

        pProc->sPts = pts;

        pProc->sCodec = codecid;
    }

    return;
}
Gb28181Session::Gb28181Session(const DeviceInfo &devInfo) : Session(devInfo)
{
    m_proc = new PackProcStat();
    m_count = 0;
    m_rtpTcpFd = -1;
    m_listenFd = -1;
}

Gb28181Session::~Gb28181Session()
{
    // 发送BYE数据包并离开会话 不用等待
    BYEDestroy(RTPTime(0, 0), 0, 0);
    if (m_rtpTcpFd != -1)
    {
        close(m_rtpTcpFd);
        m_rtpTcpFd = -1;
    }

    if (m_listenFd != -1)
    {
        close(m_listenFd);
        m_listenFd = -1;
    }

    GBOJ(_gConfig)->pushOneRandNum(rtp_loaclport);
}

int Gb28181Session::CreateRtpSession(string dstip, int dstport)
{
    LOG(INFO) << "CreateRtpSession";
    int ret = -1;
    RTPSessionParams sessParams;
        sessParams.SetOwnTimestampUnit(1.0 / 90000.0); // 采样率上级对下级INVITE
        sessParams.SetAcceptOwnPackets(true);
        sessParams.SetUsePollThread(true);
        sessParams.SetNeedThreadSafety(true);
        sessParams.SetMinimumRTCPTransmissionInterval(RTPTime(5, 0));

        RTPUDPv4TransmissionParams transparams;
    if (protocal == 0)
    {
        

        transparams.SetPortbase(rtp_loaclport);
        transparams.SetRTPReceiveBuffer(1024 * 1024);
        ret = Create(sessParams, &transparams);
        LOG(INFO) << "ret:" << ret;
        if (ret < 0)
        {
            LOG(ERROR) << "udp create fail";
        }
        else
        {
            LOG(INFO) << "udp create ok,bind:" << rtp_loaclport;
        }
    }
    if(protocal==1)
    {
        sessParams.SetMaximumPacketSize(65535);
        RTPTCPTransmissionParams transParams;
        ret = Create(sessParams, &transParams, RTPTransmitter::TCPProto);
        if(ret < 0)
        {
            LOG(ERROR) << "Rtp tcp error: " << RTPGetErrorString(ret);
            return -1;
        }

        //会话创建成功后，接下来需要创建tcp连接
        int sessFd = RtpTcpInit(dstip,dstport,5);
		LOG(INFO)<<"sessFd:"<<sessFd;
        if(sessFd < 0)
        {
            LOG(ERROR)<<"RtpTcpInit faild";
            return -1;
        }
        else
        {
            AddDestination(RTPTCPAddress(sessFd));
        }
    }

    return ret;
}

int Gb28181Session::RtpTcpInit(string dstip, int dstport, int time)
{
    int timeout = time*1000;
    if(setupType == "active")
    {
        m_rtpTcpFd = ECSocket::createConnByActive(dstip,dstport,rtp_loaclport,&timeout);
    }
    else if(setupType == "passive")
    {
        m_rtpTcpFd = ECSocket::createConnByPassive(&m_listenFd,rtp_loaclport,&timeout);
    }

    return m_rtpTcpFd;
}

int Gb28181Session::SendPacket(int media, char *data, int datalen, int codecId)
{
    // 先计算下推送包的整个长度，header部分+负载部分
    int len = sizeof(struct StreamHeader) + datalen;

    char *streamBuf = new char[len];
    memset(streamBuf, 0, len);

    // 先给header部分的参数进行赋值
    StreamHeader *header = (StreamHeader *)streamBuf;
    header->length = datalen;
    if (media == 2)
    {
        header->type = media;
        // 需要进行IDR帧的判断，并从SPS序列集中解出帧率和分别率
        // 判断下当前流的codeid是否属于h264编码
        if (codecId == STREAM_VIDEO_H264)
        {
            int nalType = 0, keyFrame = 0;
            int videoW = 0, videoH = 0;
            int videoFps = 25;
            // 先保存下当前流的编码格式，将int类型转为char，4-》1，其实就是存储了ascll码的值
            // header->format[0] = 1;
            // 然后再获取nalu类型，我们之前说过nalu的startcode为三个字节或者四个字节
            if (data[0] == 0x0 && data[1] == 0x0 && data[2] == 0x0 && data[3] == 0x01)
            {
                nalType = data[4] & 0x1F;
            }
            else if (data[0] == 0x0 && data[1] == 0x0 && data[2] == 0x01)
            {
                nalType = data[3] & 0x1F;
            }
            else
            {
                LOG(ERROR) << "Invalid h264 data, please check!";
                return -1;
            }

            // 如果说nalType为7，那么就代表这帧数据包含了SPS序列集，也就是IDR帧
            // 那么定义个变量标识下当前帧为关键帧
            if (nalType == 7)
            {
                keyFrame = 1;
            }
            else // 这里只需要将包含了SPS参数集的数据看作为关键帧，其他都为非关键帧
            {
                keyFrame = 0;
            }

            // 这里查询帧率和分辨率信息的频率不要太频繁，这里我们设置每50个关键帧查询一次
            if(keyFrame == 1){
                if (m_count == 0 || m_count > 50)
            {
                Picinfo info;
                if (GetH264pic((unsigned char *)data, datalen, &info) == 0)
                {
                    if (info.height != 0 && info.width != 0)
                    {
                        header->videoH = info.height;
                        header->videoW = info.width;

                        LOG(INFO) << "header->videoH:" << header->videoH << ",header->videoW:" << header->videoW;
                        m_count++;
                    }
                    // 帧率
                    if (info.max_framerate > 0)
                    {
                        header->format[0] = info.max_framerate;
                        LOG(INFO) << "info.max_framerate:" << info.max_framerate;
                    }
                    else if (info.max_framerate == 0)
                    {
                        header->format[0] = 25;
                    }
                    if (m_count > 50)
                    {
                        m_count = 0;
                    }
                }
                else
                {
                    LOG(ERROR) << "can't analysis video data !! data: " << (void *)data << ", dataLen: " << datalen;
                }
            }
            else
            {
                m_count++;
            }

            // 最后再保存下帧类型
            header->format[1] = keyFrame;
            // 将编码器类型也保存下
            header->format[2] = codecId;
        }
            }
            
        memcpy(streamBuf+sizeof(StreamHeader),data,datalen);
        
        if(bev != NULL)
        {
            bufferevent_write(bev,streamBuf,len);
            LOG(INFO)<<"send total data len:"<<len<<",payload data len:"<<datalen;
        }
    
    }
}

void Gb28181Session::OnPollThreadStep()
{
    // 调用这个接口开始对接收到的数据进行访问，就这个函数的作用是通知就是HP内部库开始准备访问接收到的数据
    BeginDataAccess();
    // 首先我们先检查是否有传入的数据包
    // 我们现在要定位到第一个具有可用数据的源
    if (GotoFirstSourceWithData())
    {
        // 需要通过循环遍历每个源，然后在每个圆上处理所有的数据包
        /*   如果说有多个圆的话，那么我们需要循环遍历每个源来进行处理，
          就是不管你这个对应的圆上有没有数据，但是我们要先去遍历每个圆，
          如果有数据的话我们就取来用，没有数据的话我们就再进行下一个圆 */
        do
        {
            RTPSourceData *srcdat = NULL;
            RTPPacket *pack = nullptr;
            srcdat = GetCurrentSourceInfo();
            // 循环读进当前源所有数据包
            while ((pack = GetNextPacket()) != NULL)
            {
                ProcessRTPPacket(*srcdat, *pack);
                // 处理完删除数据包
                DeletePacket(pack);
            }

            /* 我们要通过解析将FTP的负载的部分给解出来，
            解出来之后因为一帧数据被拆成了多个rtp包来进行推送的，
            那么我们也是接收了多个rtp包，然后我们将多个rtp包需要进行一个完整帧的组包，
            只有组成完整帧数据之后，我们才能调用ps的解封装的接口来进行ps数据的一个解封装，
            解出H24的裸流 */
        } while (GotoNextSourceWithData());
    }
    EndDataAccess();
}

void Gb28181Session::ProcessRTPPacket(RTPSourceData &srcdat, RTPPacket &pack)
{
    int payloadType = pack.GetPayloadType(); // 负载包类型ps，h264...
    int payloadLen = pack.GetPayloadLength();
    int marker = pack.HasMarker();                 // Mark是判断帧边界的
    int curSeq = pack.GetExtendedSequenceNumber(); // 当前的一个序列号
    int timestamp = pack.GetTimestamp();           // 时间戳
    unsigned char *payloadData = pack.GetPayloadData();

    // 在这里更新下下级最后有rtp包推送的时间
    gettimeofday(&m_curTime, NULL);
    // 那么就在接收rtp包的时机给这个session进行赋值
    // 这里需要先判断下
    if (m_proc && m_proc->session == NULL)
    {
        m_proc->session = (void *)this;
    }
    if (payloadType != 96 && payloadType != 98)
    {
        LOG(ERROR) << "rtp unknown payload type:" << payloadType;
        return;
    }
    if (payloadType == 96)
    {
        OnRTPPacketProcPs(marker, curSeq, timestamp, payloadData, payloadLen);
    }
    if (payloadType == 98)
    {
    }
}

void Gb28181Session::OnRTPPacketProcPs(int mark, int curSeq, int timestamp, unsigned char *payloadData, int payloadLen)
{
    // LOG(INFO)<<"mark:"<<mark;
    int FrameStat = mark;

    if (m_proc->rSeq == -1)
        m_proc->rSeq = curSeq;

    if (m_proc->rTimeStamp == 0)
        m_proc->rTimeStamp = timestamp;

    if (curSeq - m_proc->rSeq > 1)
    {
        m_proc->rState = 2;
        LOG(ERROR) << "rtp drop pack diff:" << curSeq - m_proc->rSeq;
    }

    if (FrameStat == 0)
    {
        // LOG(INFO)<<"m_proc->rTimeStamp:"<<m_proc->rTimeStamp"<< timestamp:"<<timestamp;

        if (timestamp != m_proc->rTimeStamp)
        {
            FrameStat = RtpPack_FrameNextStart;
        }
    }

    m_proc->rSeq = curSeq;
    m_proc->rTimeStamp = timestamp;

    if (m_proc->rState == 1)
    {
        if (FrameStat == RtpPack_FrameContinue || FrameStat == RtpPack_FrameCurFinsh)
        {
            if (m_proc->rlen < payloadLen + m_proc->rNow)
            {
                m_proc->rlen = payloadLen + m_proc->rNow + 1024;
                m_proc->rBuf = (char *)realloc(m_proc->rBuf, m_proc->rlen);
            }
            memcpy(m_proc->rBuf + m_proc->rNow, payloadData, payloadLen);
            m_proc->rNow += payloadLen;
        }
    }
    else if (m_proc->rState == 2)
    {
        m_proc->rState = 1;

        memset(m_proc->rBuf, 0, m_proc->rNow);
        m_proc->rNow = 0;
        m_proc->rSeq = -1;
        m_proc->rTimeStamp = 0;

        if (FrameStat == RtpPack_FrameNextStart)
        {
            if (m_proc->rlen < payloadLen + m_proc->rNow)
            {
                m_proc->rlen = payloadLen + m_proc->rNow + 1024;
                m_proc->rBuf = (char *)realloc(m_proc->rBuf, m_proc->rlen);
            }
            memcpy(m_proc->rBuf + m_proc->rNow, payloadData, payloadLen);
            m_proc->rNow += payloadLen;
        }
        return;
    }
    // LOG(INFO)<<"FrameStat:"<<FrameStat;
    if (FrameStat)
    {
#if 0
        if(m_proc->psFp == NULL)
        {
            m_proc->psFp = fopen("../../../conf/data.ps","wb");
        }
        if(m_proc->psFp)
        {
            fwrite(m_proc->rBuf,1,m_proc->rNow,m_proc->psFp);
        }
#endif
        if (m_proc->unpackHnd == NULL)
        {
            LOG(INFO) << "ps_demuxer_create";
            // 这里需要定义一个回调接口，用来接收处理的解封装后的音视频数据
            m_proc->unpackHnd = (void *)ps_demuxer_create((ps_demuxer_onpacket)ps_demux_callback, (void *)m_proc);
        }

        if (m_proc->unpackHnd)
        {
            LOG(INFO) << "PS SIZE:" << m_proc->rNow;
            int offset = 0;

            while (offset < m_proc->rNow)
            {
                // 这里需将缓存的数据传入到ps解封装接口中，
                int ret = ps_demuxer_input((struct ps_demuxer_t *)m_proc->unpackHnd, (const uint8_t *)m_proc->rBuf + offset, m_proc->rNow - offset);
                if (ret == 0)
                {
                    LOG(ERROR) << "wrong payload data !!!!! can't demux the PS data";
                }
                offset += ret;
            }
        }
        // ps demutex
        memset(m_proc->rBuf, 0, m_proc->rNow);
        m_proc->rNow = 0;

        if (FrameStat == RtpPack_FrameNextStart)
        {
            if (m_proc->rlen < payloadLen + m_proc->rNow)
            {
                m_proc->rlen = payloadLen + m_proc->rNow + 1024;
                m_proc->rBuf = (char *)realloc(m_proc->rBuf, m_proc->rlen);
            }
            memcpy(m_proc->rBuf + m_proc->rNow, payloadData, payloadLen);
            m_proc->rNow += payloadLen;
        }
    }

    return;
}
