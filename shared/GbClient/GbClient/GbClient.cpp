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
	//��ÿؼ����
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

	//����
	string revc = "";
	ECSocket::recvData(fd, revc);

	//����һ���յ� JSON ����
	Json::Value resp = Json::Value(Json::objectValue);
	//json ������
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
	//������ȫ��չ��
	ui.treeWidget->expandAll();
	this->show();
}



void GbClient::on_treeWidget_customContextMenuRequested(const QPoint& pos)
{
	QTreeWidgetItem* curItem = ui.treeWidget->itemAt(pos);
	if (!curItem)
		return;
	//��ȡ��ֵ
	QVariant var = curItem->data(0, Qt::UserRole);
	string id = var.toString().toStdString();
	string tmp = id.substr(10, 3);
	int type = atoi(tmp.c_str());
	//���������id
	if (type == DevTypeCode::CenterServer_Code)
	{
		//��һ�� QObject ��������ʱ�������Զ������������Ӷ���
		//��ˣ�������� QMenu ����ĸ���������Ϊ this��ͨ����һ�� QWidget �� QMainWindow������ô�� this ��������ʱ��QMenu ����Ҳ�ᱻ�Զ����١�
		QMenu *popMenu = new QMenu(this);
		QAction *catalog_get = popMenu->addAction(u8"��ȡ�豸Ŀ¼");

		connect(catalog_get, &QAction::triggered, this,[=]()
		{
			getCatalog(id);
		});
		popMenu->popup(QCursor::pos());
	}
	if (type == DevTypeCode::Ipc_Code||type==DevTypeCode::Camera_Code||type==DevTypeCode::VGA_Code)
	{
		//��һ�� QObject ��������ʱ�������Զ������������Ӷ���
		//��ˣ�������� QMenu ����ĸ���������Ϊ this��ͨ����һ�� QWidget �� QMainWindow������ô�� this ��������ʱ��QMenu ����Ҳ�ᱻ�Զ����١�
		QMenu *popMenu = new QMenu(this);
		QAction * realplay= popMenu->addAction(u8"Ԥ��");

		connect(realplay, &QAction::triggered, this, [=]()
		{
			openStream(id,3);
		});

		QAction* recordinfo = popMenu->addAction(u8"�طż�¼");
		connect(recordinfo, &QAction::triggered,
			[=]()
		{
			//getRecorInfo(id);
		}
		);
		popMenu->popup(QCursor::pos());
	}
}
//��ȡ�豸Ŀ¼ id
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

	//����
	string revc = "";
	ECSocket::recvData(fd, revc);

	//����һ���յ� JSON ����
	Json::Value resp = Json::Value(Json::objectValue);
	//json ������
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

	//�ҵ���Ӧ�ĸ��ڵ�
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
		//�Ƿ�����ӽڵ�
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
		//���豸���ص�����
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
			//�����ӽڵ㣬�Ǿ͵ݹ�������ص���
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
	//����ӿ������ڶ����еĸ�ʽ�����б��������ע�ᣬ��������ӿ�����iPad4.0��İ汾�Ѿ�������,����Ļ����û���ݵ��ϵİ汾
	av_register_all();
	//��ʼ�����еĿ��õ���ϵͳ
	if (SDL_Init(SDL_INIT_EVERYTHING))
	{
		fprintf(stderr, "can not init sdl -%s\n", SDL_GetError());
		exit(1);
	}
	//�ٴ��� sdl�Ĵ��ڣ���qt�ؼ�����
	SDL_Window* screan = SDL_CreateWindowFrom((void*)screanPlayWnd);
	if (!screan)
	{
		fprintf(stderr, "can not create sdl window -%s\n", SDL_GetError());
		exit(1);
	}
	//����sdl��Ⱦ��
	/*������ָ����Ⱦ������������˵ʹ��open GL������direct3D��
		�����������Ⱦ���ٻ�����Ӳ��������Ⱦ����
		һ������������������Ĭ��ʹ�� - 1
		��Ȼ���ٸ��ݺ�����������ѡ���һ��֧�ֵ���Ⱦ��������
		��������ǿ���ָ��ʹ�������Ⱦ��������Ӳ��������Ⱦ����
		������߾�Ĭ�ϸĳ�0�����Ǳ�ʾ��������ѡ���Ⱦ������Ϊ��־*/
	SDL_Renderer* renderer = SDL_CreateRenderer(screan, -1, 0);
	if (!renderer)
	{
		fprintf(stderr, "can not create renderer -%s\n", SDL_GetError());
		exit(1);
	}
	//��ʼ����������صģ�Ҳ����ʹ��FM�ɿ˵Ľӿڣ����Դӷ���˻�ȡ�������
	AVCodec* videoCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (videoCodec == NULL)
	{
		return -1;
	}
	//�������ǵĽ�������������
	AVCodecContext* pCodecCtx = avcodec_alloc_context3(videoCodec);
	/*ʱ������ķ��� = 1. Ȼ��ʱ������ķ�ĸ = 25, 
		��ô�����Ҳ�Ǳ�ʾ��֡�ʣ�Ҳ����˵���ǳ�ʼ��������������ʱ�����Ҳ����֡��Ϊ25��
		Ҳ����40����һ֡����1��25*/
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;
	//��Ϊ�������ǰ�����������������Ҫ���촦��ÿһ֡��Ƶ���ݣ�Ȼ��ȷ������ʵʱ�ԣ���ô����������Ҫ����Ϊÿ��һ����Ƶ֤����
	pCodecCtx->frame_number = 1;
	/*codec_type ��ʾ������������͡�
		����Ϊ AVMEDIA_TYPE_VIDEO ��ʾ����һ����Ƶ���������*/
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->bit_rate = 0;
	pCodecCtx->width = 1280;
	pCodecCtx->height = 720;
	//����Ƶ������
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
	Y 1280*720�ֽ�
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
