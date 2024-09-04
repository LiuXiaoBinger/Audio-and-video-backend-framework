#pragma once
#include <QtCore>
#include <QtCore>
#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>
using namespace std;
extern std::string servIp;
extern int port;


//���ͱ���ö��
enum DevTypeCode
{
	Error_code = -1,
	Dvr_Code = 111,
	ViderServer_Code = 112,
	Encoder_Code = 113,
	Decoder_Code = 114,
	AlarmDev_Code = 117,
	NVR_Code = 118,

	Camera_Code = 131,
	Ipc_Code = 132,
	VGA_Code = 133,
	AlarmInput_Code = 134,
	AlarmOutput_Code = 135,

	CenterServer_Code = 200,
};
enum Command_Type
{
	Command_Session_Register = 1,
	Command_Session_Catalog,
	Command_Session_RealPlay,
	Command_Session_RecordInfo,
	Command_Session_RecordList,
	Command_Session_PlayBack,
	Command_Session_PlayStop
};
class TreeNode
{
public:
	TreeNode(QString id, QString parid, QString n, bool p)
		:deviceId(id)
		, parentId(parid)
		, name(n)
		, parental(p)
	{
	}
	QString deviceId;
	QString parentId;
	QString name;
	bool parental;

};

struct StreamHeader
{
	int type;   //ý������   1=��Ƶ  2=��Ƶ
	int length;   //��֡���ݵĳ���
	int videoH;   //��Ƶ�ֱ���-��
	int videoW;   //��Ƶ�ֱ���-��
	char format[4];
};
