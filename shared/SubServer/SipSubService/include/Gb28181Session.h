#ifndef _GB28181SESSION_H
#define _GB28181SESSION_H

#include "rtpsession.h"
#include "rtpsourcedata.h"
#include "rtptcptransmitter.h"
#include "rtptcpaddress.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtplibraryversion.h"
#include "rtcpsrpacket.h"
#include "Common.h"

#include "GlobalCtl.h"
#include "SipDef.h"

#ifdef __cplusplus
extern "C"{
#endif
#include "mpeg-ps.h"
#include "jthread.h"
#ifdef __cplusplus
}
#endif
#define JRTP_SET_SEND_BUFFER   ((30)*1024)
#define JRTP_SET_MULITTTL   (255)
#define PS_SEND_TIMESTAME   (3600)
using namespace jrtplib;


class Gb28181Session : public RTPSession
{
    public:
        Gb28181Session(const DeviceInfo& devInfo);
        Gb28181Session();
        ~Gb28181Session();
        //我们这里再实现个接口进行检测
        /* 我们每次调用接口的时候，我们先获取一下当前的时间，然后和最后接收到rtcp的RR包的时间戳进行一个判断 */
        int CheckAlive()
        {
            time_t cm_time = time(NULL);
            if(m_rtpRRTime == 0)
            {
                m_rtpRRTime = cm_time;
            }
            //大于 那么上级rr包无效
            if((cm_time - m_rtpRRTime) > 10)
            {
                return 1;
            }

            return 0;
        }
        int CreateRtpSession(int poto,string setup,string dstip,int dstport,int rtpPort);
        int RtpTcpInit(string dstip,int dstport,int localPort,string setup,int time);
		
    protected:
        enum
        {
            RtpPack_FrameContinue = 0,
            RtpPack_FrameCurFinsh,
            RtpPack_FrameNextStart,
        };
        /*
        接下来我们还需要完善下级对上级回复的rtcp的机制做检测，
        如果rtp使用的传输层协议为udp推流时，需要上级发送rtcp的RR包，也就是recv report,
        
        主要就是发送端要实时的了解接收端接收数据的情况，根据RR包报告的参数来做相应的调整，实现提高数据包的完整性，
        好  下面我们来实现检测的功能
        我们先需要重写RTPSession的虚函数
        */
        //这个接口只有对方发送了rtcp包才会被触发
        void OnRTCPCompoundPacket(RTCPCompoundPacket *pack,const RTPTime &receivetime, const RTPAddress *senderaddress);
        void OnNewSource(RTPSourceData *srcdat)
        {
			LOG(INFO)<<"OnNewSource";
			LOG(INFO)<<"srcdat->IsOwnSSRC():"<<srcdat->IsOwnSSRC();
            if(srcdat->IsOwnSSRC())
                return;
            
            uint32_t ip;
            uint16_t port;
			LOG(INFO)<<"00";
            if(srcdat->GetRTPDataAddress() != 0)
            {
				LOG(INFO)<<"11";
                const RTPIPv4Address* addr = (const RTPIPv4Address*)srcdat->GetRTPDataAddress();
                ip = addr->GetIP();
                port = addr->GetPort();
            }
            else if(srcdat->GetRTCPDataAddress() != 0)
            {
				LOG(INFO)<<"22";
                const RTPIPv4Address* addr = (const RTPIPv4Address*)srcdat->GetRTCPDataAddress();
                ip = addr->GetIP();
                port = addr->GetPort()-1;
            }
            else
			{
				LOG(INFO)<<"33";
				return;
			}
                
            
            RTPIPv4Address dest(ip,port);
            AddDestination(dest);
            struct in_addr inaddr;
            inaddr.s_addr = htonl(ip);
            LOG(INFO)<<"Adding destination "<<string(inet_ntoa(inaddr))<<":"<<port;
        }

        void OnRemoveSource(RTPSourceData *srcdat)
        {
            if(srcdat->IsOwnSSRC())
                return;

            if(srcdat->ReceivedBYE())
                return;
            
            uint32_t ip;
            uint16_t port;
            if(srcdat->GetRTPDataAddress() != 0)
            {
                const RTPIPv4Address* addr = (const RTPIPv4Address*)srcdat->GetRTPDataAddress();
                ip = addr->GetIP();
                port = addr->GetPort();
            }
            else if(srcdat->GetRTCPDataAddress() != 0)
            {
                const RTPIPv4Address* addr = (const RTPIPv4Address*)srcdat->GetRTCPDataAddress();
                ip = addr->GetIP();
                port = addr->GetPort()-1;
            }
            else
                return;
            
            RTPIPv4Address dest(ip,port);
            DeleteDestination(dest);
			
			struct in_addr inaddr;
			inaddr.s_addr = htonl(ip);
			LOG(INFO) << __FUNCTION__ << " Deleting destination " << std::string(inet_ntoa(inaddr)) << ":" << port;
        }

        void OnBYEPacket(RTPSourceData *srcdat)
        {
            if(srcdat->IsOwnSSRC())
                return;
            
            uint32_t ip;
            uint16_t port;
            if(srcdat->GetRTPDataAddress() != 0)
            {
                const RTPIPv4Address* addr = (const RTPIPv4Address*)srcdat->GetRTPDataAddress();
                ip = addr->GetIP();
                port = addr->GetPort();
            }
            else if(srcdat->GetRTCPDataAddress() != 0)
            {
                const RTPIPv4Address* addr = (const RTPIPv4Address*)srcdat->GetRTCPDataAddress();
                ip = addr->GetIP();
                port = addr->GetPort()-1;
            }
            else
                return;
            
            RTPIPv4Address dest(ip,port);
            DeleteDestination(dest);
			
			struct in_addr inaddr;
			inaddr.s_addr = htonl(ip);
			
			LOG(INFO) <<__FUNCTION__<< " Deleting destination " << std::string(inet_ntoa(inaddr)) << ":" << port;
        }

    private:
        int m_rtpRRTime;
		int m_count;
        int m_rtpTcpFd;
        int m_listenFd;
};

class SipPsCode
{
    public:
        SipPsCode(string dstip,int dstport,int rtpPort,int poto,string setup,int s,int e)
        {
            m_dstip = dstip;
            m_dstport = dstport;
               m_avStreamIndex = -1;
            m_auStreamIndex = -1;
			m_localRtpPort = rtpPort;
			stopFlag = false;
			m_sTime = s;
			m_eTime = e;
            m_poto = poto;
            m_connType = setup;
        }

        ~SipPsCode()
        {
            if(m_muxer)
            {
                ps_muxer_destroy(m_muxer);
            }

            if(m_gbRtpHandle)
            {
                delete m_gbRtpHandle;
                m_gbRtpHandle = NULL;
            }
            
			GBOJ(_gConfig)->pushOneRandNum(m_localRtpPort);
			stopFlag = false;
        }

        int initPsEncode();
        int gbRtpInit();

        static void* Alloc(void* param, size_t bytes);
        static void Free(void* param, void* packet);
        static int onPsPacket(void* param, int stream, void* packet, size_t bytes);
        //封装视频ps包
        int incomeVideoData(unsigned char* avdata,int len,int pts,int isIframe);
        //封装音频ps包
        int incomeAudioData(unsigned char* audata,int len,int pts);
        //发送ps包
        int sendPackData(void* packet, size_t bytes);

        bool stopFlag;
		int m_sTime;
		int m_eTime;
        int m_poto = 0;
        string m_connType;
    private:
        string m_dstip;
        int m_dstport;
        Gb28181Session* m_gbRtpHandle;
        ps_muxer_t* m_muxer;
        int m_avStreamIndex;//视频索引
        int m_auStreamIndex;//音频索引索引
		int m_localRtpPort;


};
#endif