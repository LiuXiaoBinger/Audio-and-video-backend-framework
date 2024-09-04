#ifdef WIN32
#pragma execution_character_set("utf-8")
#endif
#include <QMenu>
#include "gbclient.h"
#include "ECSocket.h"
#include "common.h"

WId screanPlayWnd;
GbClient::GbClient(QWidget *parent)
    : QWidget(parent)
{ 
    ui.setupUi(this);
	ui.treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	InitializeCriticalSection(&m_lockVdeoList);
	//获得控件句柄
	screanPlayWnd = ui.label->winId();
	m_threadFlag = true;
}

GbClient::~GbClient()
{}

void GbClient::init()
{
	int fd = ECSocket::createConn( servIp,port);
	if (fd < 0)
	{
		return;
	}
	char *buf = new char[1024]{0};
	int command = Command_Session_Register;
	memcpy(buf, &command, sizeof(int));
	int ret=ECSocket::sendData(fd, buf, sizeof(int));
	if (ret < 0)
	{
		cout << "send error" << ret<<endl;
	}

	//接收
	string revc = "";
	ECSocket::recvData(fd, revc);

	//创建一个空的 JSON 对象
	Json::Value resp = Json::Value(Json::objectValue);
	//json 解析器
	Json::CharReaderBuilder _readBuider;
	Json::CharReader*reader = _readBuider.newCharReader();

	string parse_error;
	if (!reader->parse(revc.c_str(), revc.c_str() + revc.length(), &resp, &parse_error))
	{
		cout << "json parse error:" << parse_error;
	}
	delete reader;
	delete buf;

	QList<QTreeWidgetItem*> rootList;
	for (int i = 0; i < resp["SubDomain"].size(); i++)
	{
		plamtId = resp["SubDomain"][i]["sipId"].asCString();
		QTreeWidgetItem* item = new QTreeWidgetItem;
		item->setText(0, tr(resp["SubDomain"][i]["sipId"].asString().c_str()));
		QVariant var(resp["SubDomain"][i]["sipId"].asString().c_str());
		item->setData(0, Qt::UserRole, var);
		rootList.append(item);
	}
	ui.treeWidget->insertTopLevelItems(0, rootList);
	//将树形全部展开
	ui.treeWidget->expandAll();
	this->show();
}



void GbClient::on_treeWidget_customContextMenuRequested(const QPoint& pos)
{
	QTreeWidgetItem* curItem = ui.treeWidget->itemAt(pos);
	if (!curItem)
		return;
	//获取键值
	QVariant var = curItem->data(0, Qt::UserRole);
	string id = var.toString().toStdString();
	string tmp = id.substr(10, 3);
	int type = atoi(tmp.c_str());
	//如果是中心id
	if (type == DevTypeCode::CenterServer_Code)
	{
		//当一个 QObject 对象被销毁时，它会自动销毁其所有子对象。
		//因此，如果您将 QMenu 对象的父对象设置为 this（通常是一个 QWidget 或 QMainWindow），那么当 this 对象被销毁时，QMenu 对象也会被自动销毁。
		QMenu *popMenu = new QMenu(this);
		QAction *catalog_get = popMenu->addAction(u8"获取设备目录");

		connect(catalog_get, &QAction::triggered, this,[=]()
		{
			getCatalog(id);
		});
		popMenu->popup(QCursor::pos());
	}
	if (type == DevTypeCode::Ipc_Code||type==DevTypeCode::Camera_Code||type==DevTypeCode::VGA_Code)
	{
		//当一个 QObject 对象被销毁时，它会自动销毁其所有子对象。
		//因此，如果您将 QMenu 对象的父对象设置为 this（通常是一个 QWidget 或 QMainWindow），那么当 this 对象被销毁时，QMenu 对象也会被自动销毁。
		QMenu *popMenu = new QMenu(this);
		QAction * realplay= popMenu->addAction(u8"预览");

		connect(realplay, &QAction::triggered, this, [=]()
		{
			openStream(id,3);
		});

		QAction* recordinfo = popMenu->addAction(u8"回放记录");
		connect(recordinfo, &QAction::triggered,
			[=]()
		{
			//getRecorInfo(id);
		}
		);
		popMenu->popup(QCursor::pos());
	}
}
//获取设备目录 id
void GbClient::getCatalog(std::string id)
{
	int fd = ECSocket::createConn(servIp, port);
	if (fd < 0)
	{
		return;
	}
	char *buf = new char[1024]{ 0 };
	int command = 2;
	memcpy(buf, &command, sizeof(int));
	int ret = ECSocket::sendData(fd, buf, sizeof(int));
	if (ret < 0)
	{
		cout << "send error" << ret << endl;
	}

	//接收
	string revc = "";
	ECSocket::recvData(fd, revc);

	//创建一个空的 JSON 对象
	Json::Value resp = Json::Value(Json::objectValue);
	//json 解析器
	Json::CharReaderBuilder _readBuider;
	Json::CharReader*reader = _readBuider.newCharReader();

	string parse_error;
	if (!reader->parse(revc.c_str(), revc.c_str() + revc.length(), &resp, &parse_error))
	{
		cout << "json parse error:" << parse_error;
	}
	delete reader;
	delete buf;
	loadJsonData(resp);

	//找到对应的根节点
	QTreeWidgetItem* rootItem = new QTreeWidgetItem;
	getRootItem(&rootItem);
	QList<QTreeWidgetItem*> rootList;
	rootList.append(rootItem);
	showCatalogTree(plamtId, &rootItem);
	ui.treeWidget->clear();
	ui.treeWidget->insertTopLevelItems(0, rootList);

	ui.treeWidget->expandAll();
}

void GbClient::loadJsonData(Json::Value & resp)
{
	Json::Value catalog = resp["catalog"];
	for (int i = 0; i < catalog.size(); i++)
	{
		QString deviceid = QString::fromUtf8(catalog[i]["DeviceID"].asCString());
		QString name = QString::fromUtf8(catalog[i]["Name"].asCString());
		QString parent_id = QString::fromUtf8(catalog[i]["ParentID"].asCString());
		//是否包含子节点
		bool parental = (catalog[i]["Parental"].asString() == "1");
		TreeNode* node = new TreeNode(deviceid, parent_id, name, parental);
		nodes.push_back(node);
	}
}

void GbClient::getRootItem(QTreeWidgetItem ** rootItem)
{
	for (auto iter = nodes.begin(); iter != nodes.end();)
	{
		if (plamtId == (*iter)->deviceId)
		{
			(*rootItem)->setText(0, (*iter)->name);
			QVariant var((*iter)->deviceId);
			(*rootItem)->setData(0, Qt::UserRole, var);
			iter = nodes.erase(iter);
			break;
		}
		else
		{
			iter++;
		}
	}
}

void GbClient::showCatalogTree(QString deviceid, QTreeWidgetItem ** item)
{
	for (auto iter = nodes.begin(); iter != nodes.end();)
	{
		//将设备挂载到父级
		if ((*iter)->parentId == deviceid)
		{
			QTreeWidgetItem* child_item = new QTreeWidgetItem(*item);
			child_item->setText(0, (*iter)->name);
			QVariant var((*iter)->deviceId);
			child_item->setData(0, Qt::UserRole, var);
			(*item)->addChild(child_item);

			QString devid = (*iter)->deviceId;
			bool flag = (*iter)->parental;
			iter = nodes.erase(iter);
			//包含子节点，那就递归继续挂载到树
			if (flag)
			{
				showCatalogTree(devid, &child_item);
			}
			break;
		}
		else
		{
			iter++;
		}
	}
}

#include <fstream>
void GbClient::openStream(string id, int type)
{
	m_streamType = type;
	unsigned int recv_threadid;
	m_streamThread = (HANDLE)_beginthreadex(NULL, 0, &GbClient::recvStream, this, 0, &recv_threadid);
	unsigned int show_threadid;
	m_showThread = (HANDLE)_beginthreadex(NULL, 0, &GbClient::showVideo, this, 0, &show_threadid);
#if 0
	int fd = ECSocket::createConn(servIp, port);
	if (fd < 0)
		return;

	char* buf = new char[1024];
	memset(buf, 0, 1024);
	int command = type;
	memcpy(buf, &command, sizeof(int));
	int ret = ECSocket::sendData(fd, buf, 4);
	if (ret < 0)
		cout << "send error" << endl;

	std::ofstream h264_file("./data.h264", std::ios::binary);
	StreamHeader* header = NULL;
	int header_len = sizeof(StreamHeader);
	char* payload_buf = NULL;
	while (true)
	{
		int total = 0;
		char* streamBuf = new char[header_len];
		memset(streamBuf, 0, header_len);
		int len = recv(fd, streamBuf, header_len, 0);
		if (len > 0)
		{
			header = (StreamHeader*)streamBuf;
			payload_buf = new char[header->length];
			while (total < header->length)
			{
				len = recv(fd, payload_buf + total, header->length - total, 0);
				total += len;
			}
		}
		else if (len <= 0)
		{
			break;
		}
		h264_file.write(payload_buf, header->length);
		delete payload_buf;
	}
	closesocket(fd);
	h264_file.close();
#endif
	return;
}

unsigned __stdcall GbClient::recvStream(void * param)
{
	GbClient* pthis = (GbClient*)param;
	int fd = ECSocket::createConn(servIp, port);
	if (fd < 0)
		return -1;

	char* buf = new char[1024];
	memset(buf, 0, 1024);
	int command = pthis->m_streamType;
	memcpy(buf, &command, sizeof(int));
	int ret = ECSocket::sendData(fd, buf, 4);
	if (ret < 0)
		cout << "send error" << endl;

	std::ofstream h264_file("./data.h264", std::ios::binary);
	StreamHeader* header = NULL;
	int header_len = sizeof(StreamHeader);
	char* payload_buf = NULL;
	frameData* data = new frameData();
	while (true)
	{
		int total = 0;
		char* streamBuf = new char[header_len];
		memset(streamBuf, 0, header_len);
		int len = recv(fd, streamBuf, header_len, 0);
		if (len > 0)
		{
			header = (StreamHeader*)streamBuf;
			payload_buf = new char[header->length];
			while (total < header->length)
			{
				len = recv(fd, payload_buf + total, header->length - total, 0);
				total += len;
			}
		}
		else if (len <= 0)
		{
			break;
		}

		if (payload_buf != NULL && header->length != 0)
		{
			data->dataBuf = payload_buf;
			data->dataLen = header->length;
			EnterCriticalSection(&pthis->m_lockVdeoList);
			pthis->m_videoList.push_back(data);
			LeaveCriticalSection(&pthis->m_lockVdeoList);
		}
		delete streamBuf;
		streamBuf = NULL;
		//h264_file.write(payload_buf, header->length);
		//delete payload_buf;
	}
	delete data;
	free(payload_buf);
	payload_buf = NULL;
	closesocket(fd);
	//h264_file.close();
	_endthreadex(0);
	pthis->m_threadFlag = false;
	return 0;
}

unsigned __stdcall GbClient::showVideo(void * param)
{
	GbClient* pthis = (GbClient*)param;
	//这个接口是用于对所有的格式，还有编解码器的注册，但是这个接口是在iPad4.0后的版本已经不用了,这里的话调用会兼容到老的版本
	av_register_all();
	//初始化所有的可用的子系统
	if (SDL_Init(SDL_INIT_EVERYTHING))
	{
		fprintf(stderr, "can not init sdl -%s\n", SDL_GetError());
		exit(1);
	}
	//再创建 sdl的窗口，和qt控件关联
	SDL_Window* screan = SDL_CreateWindowFrom((void*)screanPlayWnd);
	if (!screan)
	{
		fprintf(stderr, "can not create sdl window -%s\n", SDL_GetError());
		exit(1);
	}
	//创建sdl渲染器
	/*参数是指定渲染驱动器，比如说使用open GL或者是direct3D，
		或者是软件渲染，再或者是硬件加速渲染器，
		一般情况下我们这里就是默认使用 - 1
		，然后再根据后边这个参数来选择第一个支持的渲染驱动器。
		后面这个是可以指定使用软件渲染器或者是硬件加速渲染器，
		我们这边就默认改成0，就是表示是无特殊选项。渲染器的行为标志*/
	SDL_Renderer* renderer = SDL_CreateRenderer(screan, -1, 0);
	if (!renderer)
	{
		fprintf(stderr, "can not create renderer -%s\n", SDL_GetError());
		exit(1);
	}
	//初始化解码器相关的，也就是使用FM派克的接口，可以从服务端获取解码参数
	AVCodec* videoCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (videoCodec == NULL)
	{
		return -1;
	}
	//创建我们的解码器的上下文
	AVCodecContext* pCodecCtx = avcodec_alloc_context3(videoCodec);
	/*时间基数的分子 = 1. 然后时间基数的分母 = 25, 
		那么这就是也是表示是帧率，也就是说我们初始化解码器上下文时间基数也就是帧率为25，
		也就是40毫秒一帧，用1÷25*/
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;
	//因为我们这是安防领域，所以我们需要尽快处理每一帧视频数据，然后确保它的实时性，那么这里我们需要设置为每包一个视频证数据
	pCodecCtx->frame_number = 1;
	/*codec_type 表示编解码器的类型。
		设置为 AVMEDIA_TYPE_VIDEO 表示这是一个视频编解码器。*/
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->bit_rate = 0;
	pCodecCtx->width = 1280;
	pCodecCtx->height = 720;
	//打开视频解码器
	if (avcodec_open2(pCodecCtx, videoCodec, NULL) < 0)
		return -1;
	frameData* frData = NULL;
	AVFrame* pFrame = av_frame_alloc();
	int frameFinished = 2;
	
	while (true)
	{
		if (!pthis->m_videoList.empty())
		{
			frData = pthis->m_videoList.front();
			break;
		}
		else
		{
			Sleep(5);
		}
	}
	AVPacket packet1 = { 0 };
	packet1.data = (uint8_t*)frData->dataBuf;
	packet1.size = frData->dataLen;
	avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet1);
	if (frameFinished)
	{
		cout << "decode success" << endl;
	}
	/*
	1280*720
	Y 1280*720字节
	U V 1280/2*720/2  1280*720/4  V   U
	*/
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING,
		pCodecCtx->width, pCodecCtx->height);
	if (!texture)
	{
		exit(1);
	}

	SwsContext* sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
		pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height,
		AV_PIX_FMT_YUV420P, SWS_BICUBLIN, NULL, NULL, NULL
	);

	size_t yPlanSz = pCodecCtx->width * pCodecCtx->height;
	size_t uPlanSz = pCodecCtx->width * pCodecCtx->height / 4;
	size_t vPlanSz = pCodecCtx->width * pCodecCtx->height / 4;

	Uint8* yPlane = (Uint8*)malloc(yPlanSz);
	Uint8* uPlane = (Uint8*)malloc(uPlanSz);
	Uint8* vPlane = (Uint8*)malloc(vPlanSz);
	if (!yPlane || !uPlane || !vPlane)
		return -1;

	while (true)
	{
		EnterCriticalSection(&pthis->m_lockVdeoList);
		if (!pthis->m_videoList.empty())
		{
			frData = pthis->m_videoList.front();
			pthis->m_videoList.pop_front();
			LeaveCriticalSection(&pthis->m_lockVdeoList);
		}
		else
		{
			LeaveCriticalSection(&pthis->m_lockVdeoList);
			if (!pthis->m_threadFlag)
			{
				av_frame_free(&pFrame);
				SDL_DestroyRenderer(renderer);
				SDL_DestroyWindow(screan);
				SDL_Quit();
				break;

			}
			continue;
		}
		AVPacket packet = { 0 };
		packet.data = (uint8_t*)frData->dataBuf;
		packet.size = frData->dataLen;
		avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
		if (frameFinished)
		{
			AVPicture pict;
			pict.data[0] = yPlane;
			pict.data[1] = uPlane;
			pict.data[2] = vPlane;
			pict.linesize[0] = pCodecCtx->width;
			pict.linesize[1] = pCodecCtx->width / 2;
			pict.linesize[2] = pCodecCtx->width / 2;

			sws_scale(sws_ctx, (uint8_t const* const*)pFrame->data, pFrame->linesize, 0,
				pCodecCtx->height, pict.data, pict.linesize);

			SDL_UpdateYUVTexture(texture, NULL, yPlane, pCodecCtx->width, uPlane, pCodecCtx->width / 2, vPlane, pCodecCtx->width / 2);

			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, NULL, NULL);
			SDL_RenderPresent(renderer);
		}
		av_free_packet(&packet);
		SDL_Event event;
		SDL_PollEvent(&event);
		switch (event.type)
		{
		case SDL_QUIT:
			SDL_DestroyTexture(texture);
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(screan);
			SDL_Quit();
			exit(0);
			break;
		default:
			break;
		}

		
	}
	av_free_packet(&packet1);
	av_frame_free(&pFrame);
	free(yPlane);
	free(uPlane);
	free(vPlane);
	avcodec_close(pCodecCtx);

	return 0;
}
