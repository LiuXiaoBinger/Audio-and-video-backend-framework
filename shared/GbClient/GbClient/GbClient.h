#pragma once

#include <Winsock2.h>
#include <process.h>
#include <QtWidgets/QWidget>
#include "ui_gbclient.h"
#include "common.h"
#include "json/json.h"
#include "sdl/SDL.h"
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"lib_json.lib")
#pragma comment(lib,"SDL2.lib")
typedef struct _Frame
{
	char* dataBuf;
	int dataLen;
}frameData;
class GbClient : public QWidget
{
    Q_OBJECT

public:
    GbClient(QWidget *parent = nullptr);
    ~GbClient();
public:
	void init();
	//获取设备目录
	void getCatalog(std::string id);
	void loadJsonData(Json::Value&resp);
	void getRootItem(QTreeWidgetItem** rootItem);
	void showCatalogTree(QString deviceid, QTreeWidgetItem** item);
	void openStream(string id, int type);
	static unsigned __stdcall recvStream(void* param);
	static unsigned __stdcall showVideo(void* param);
private slots:
	void on_treeWidget_customContextMenuRequested(const QPoint& pos);
private:
    Ui::GbClientClass ui;

	QString plamtId;
	vector<TreeNode*> nodes;
	std::list<frameData*> m_videoList;
	CRITICAL_SECTION m_lockVdeoList;
	HANDLE m_streamThread;
	HANDLE m_showThread;
	int m_streamType;
	bool m_threadFlag;
};
