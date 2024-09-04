#include <iostream>

#include <signal.h>
#include <pjlib-util.h>
#include <pjmedia.h>
#include <pjsip.h>
#include <pjsip_ua.h>
#include <pjsip/sip_auth.h>
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

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/thread.h>
#include "Common.h"
#include "SipLocalConfig.h"
#include "GlobalCtl.h"
#include "ECThread.h"
#include "SipRegister.h"
#include "GetCatalog.h"
#include "OpenStream.h"
#include "GetRecordList.h"
#include "EventMsgHandle.h"
class SetGlogLevel
{
public:
	SetGlogLevel(const int type)
	{

		// 将日志重定向到指定文件中
		google::InitGoogleLogging(LOG_FILE_NAME);
		// 设置输出到控制台的Log等级
		FLAGS_stderrthreshold = type;
		FLAGS_colorlogtostderr = true;
		FLAGS_logbufsecs = 0;
		FLAGS_log_dir = LOG_DIR;
		FLAGS_max_log_size = 4;
		google::SetLogDestination(google::GLOG_WARNING, "");
		google::SetLogDestination(google::GLOG_ERROR, "");
		signal(SIGPIPE, SIG_IGN);
	}
	~SetGlogLevel()
	{
		google::ShutdownGoogleLogging();
	}
};

void* func(void *data){
	std::cout<<"__func__"<<std::endl;
}
int main()
{
	srand(time(0));
	signal(SIGINT, SIG_IGN);
	SetGlogLevel glog(0);
	SipLocalConfig *_config = new SipLocalConfig();
	int ret = _config->ReadConf();
	if (ret == -1)
	{
		LOG(ERROR) << "read config has error";
		return ret;
	}

	bool re = GlobalCtl::instance()->init((void *)_config);
	if (re == false)
	{
		LOG(ERROR) << "init error";
		return -1;
	}
	LOG(INFO) << GBOJ(_gConfig)->localIp();
	pthread_t id;

	 ret = EC::ECThread::createThread(func,nullptr,id) ;
	if (ret!=0)
	{
		ret=-1;
		LOG(ERROR)<<"creat thread fild";
		return ret;
	}
	LOG(INFO)<<"creat thread id"<<id;
	LOG(INFO)<<"main thread id"<<pthread_self();
	EventMsgHandle* pMsgHandle = new EventMsgHandle(GBOJ(_gConfig)->localIp(),GBOJ(_gConfig)->localPort());
	if(pMsgHandle != NULL)
	{
		if(pMsgHandle->init() != 0)
		{
			LOG(ERROR)<<"EventMsgHandle init error";
			return -1;
		}
		delete pMsgHandle;
		pMsgHandle = NULL;
	}

	SipRegister* reg=new SipRegister();
	reg->registerServiceStart();

	/* sleep(3);
	GetCatalog _getcatalog;
	_getcatalog.DirectoryGetPro((void*)""); */
	
/* 	sleep(3);
	OpenStream _OpenStream;
	_OpenStream.StreamServiceStart();
	sleep(4);
	GetRecordList aa; */
	
	//OpenStream::StreamStop("11000000002000000001","11000000001310000059");
	while (1)
	{
		sleep(30);
	}
	return 1;
}
