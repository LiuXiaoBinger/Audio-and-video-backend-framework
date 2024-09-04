#ifndef PACK_H
#define PACK_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include "err_str.h"
#include <malloc.h>
#include <sys/mman.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<string.h>
#include<sys/stat.h>
//service sshd start
//mysql -u root -p
#include<iostream>
#include<map>
#include<list>
#include<string.h>
//#include<QByteArray>
//service sshd start
//mysql -u root -p

#define TRUE true
#define FALSE false
#include<string.h>
#define _DEF_UDP_PORT (12345)
#define _DEF_RECV_BUF_SIZE (4096)
#define _DEF_NAME_SIZE (60)
#define _DEF_CONTENT_SIZE (1024)
#define _DEF_TCP_PORT (8080)
#define _DEF_TCP_SERVER_IP ("192.168.3.239")
#define _DEF_PROTOCOL_COUNT (10)
//协议
#define _DEF_PROTOCAL_BASE (1000)
//注册
#define _DEF_PACK_TCP_REGISTER_RQ   (_DEF_PROTOCAL_BASE+ 1)
#define _DEF_PACK_TCP_REGISTER_RS   (_DEF_PROTOCAL_BASE + 2)
//登录
#define _DEF_PACK_TCP_LOGIN_RQ      (_DEF_PROTOCAL_BASE +3)
#define _DEF_PACK_TCP_LOGIN_RS      (_DEF_PROTOCAL_BASE +4)
//文件上传请求

#define _DEF_PACK_TCP_UPFILE_RQ      (_DEF_PROTOCAL_BASE +6)
//下线
#define _DEF_PACK_TCP_OFFLINE_RQ    (_DEF_PROTOCAL_BASE+ 10)

//获取好友信息
#define _DEF_PACK_TCP_FRIEND_INFO   (_DEF_PROTOCAL_BASE + 5)
#define _DEF_PACK_UP_FILEAttributes (_DEF_PROTOCAL_BASE + 7)
//文件传输成功
#define _DEF_PACK_UP_FILESUESS (_DEF_PROTOCAL_BASE + 8)
//请求结构//注册请求结果
#define register_success (0)
#define user_is_exist    (1)
#define parameter_error   (3)
//登录请求结果
#define login_success  (0)
#define user_not_exist (1)
#define password_error (2)
//用户状态
#define status_online  (0)
#define status_offline (1)


//协议结构
typedef int PackType;
// 注册请求; 协议头、昵称、手机号、密码
typedef struct STRU_TCP_REGISTER_RQ{
    STRU_TCP_REGISTER_RQ():type(_DEF_PACK_TCP_REGISTER_RQ){
        memset(name,0,_DEF_NAME_SIZE);
        memset(tel,0,_DEF_NAME_SIZE);
        memset(password,0,_DEF_NAME_SIZE);
    }
    PackType type;
    char name[_DEF_NAME_SIZE];
    char tel[_DEF_NAME_SIZE];
    char password[_DEF_NAME_SIZE];
} STRU_TCP_REGISTER_RQ;
// 注册回复:协议头、注册结果
typedef struct STRU_TGP_REGISTER_RS {
    STRU_TGP_REGISTER_RS() :type(_DEF_PACK_TCP_REGISTER_RS), result(register_success) {
    }
    PackType type;
    int result;
}STRU_TGP_REGISTER_RS;

//登录请求:协议头、手机号、密码
typedef struct STRU_TCP_LOGIN_RQ{
    STRU_TCP_LOGIN_RQ() :type(_DEF_PACK_TCP_LOGIN_RQ){
        memset(tel,0,_DEF_NAME_SIZE);
        memset(password,0,_DEF_NAME_SIZE);
    }
    PackType type;
    char tel[_DEF_NAME_SIZE];
    char password[_DEF_NAME_SIZE];
} STRU_TCP_LOGIN_RQ;
//登录回复:协议头、登录结果、用户id
typedef struct STRU_TCP_LOGIN_RS
{
    STRU_TCP_LOGIN_RS() :type(_DEF_PACK_TCP_LOGIN_RS), result(login_success), userld(0) {
    }
    PackType type;
    int result;
    int userld;
} STRU_TCP_LOGIN_RS;
//获取好友信息:协议头、用户id、昵称、签名、头像id、用户状态
typedef struct STRU_TCP_FRIEND_INFO
{
    STRU_TCP_FRIEND_INFO() :type(_DEF_PACK_TCP_FRIEND_INFO), userld(0), iconld(0){
        memset(name,0,_DEF_NAME_SIZE);
        memset(feeling,0,_DEF_NAME_SIZE);
    }
    PackType type;
    int userld;
    int iconld;
    int state;
    char name[_DEF_NAME_SIZE];
    char feeling[_DEF_NAME_SIZE];
} STRU_TCP_FRIEND_INFO;
//下线请求:协议头、用户id
typedef struct STRU_TCP_OFFLINE_RQ {
    STRU_TCP_OFFLINE_RQ() :type(_DEF_PACK_TCP_OFFLINE_RQ), userld(0) {
    }
    PackType type;
    int userld;
} STRU_TCP_OFFLINE_RQ;
//下载请求包
typedef struct UP_FILERQ{
    UP_FILERQ() :type(_DEF_PACK_TCP_UPFILE_RQ), userld(0),filelen(0),filesize(0),pos(0){
        memset(filename,0,512);
        memset(file,0,16*1024);
    }
    PackType type;
    int userld;
    int filelen;
    int filesize;
    int pos;
    char filename[512];
    char file[16*1024];

}UP_FILERQ;
//文件属性包
typedef struct UP_FILEAttributes{
    UP_FILEAttributes() :type(_DEF_PACK_UP_FILEAttributes), userld(0),datasize(0){
        memset(md5,0,60);
        memset(dir,0,512);
        memset(szFileID,0,512);
        memset(filename,0,512);
    }
    PackType type;
    int userld;
    int datasize;
    char md5[60];
    char dir[512];
    char filename[512];
    char szFileID[512];
}UP_FILEAttributes;

//断点传续包
typedef struct LoadPacket
{
    int fd;
    int loadsize;
} loadPacket;


//文件上传成功
typedef struct UPFILE_SEUSS
{
    UPFILE_SEUSS() :type(_DEF_PACK_UP_FILESUESS), userld(0),result(0){

    }
    PackType type;
    int userld;
    int result;
} UPFILE_SEUSS;
//数据库没有文件标识，开始上传文件
#define _DEF_PACK_START_UPFILE      (_DEF_PROTOCAL_BASE + 9)
//数据库没有文件标识，开始上传文件
typedef struct START_UPFILE
{
    START_UPFILE() :type(_DEF_PACK_START_UPFILE), userld(0),result(0){

        memset(dir,0,512);

        memset(filename,0,512);
    }
    PackType type;
    int userld;
    int result;
    char dir[512];
    char filename[512];
} START_UPFILE;


//结束文件上传完成文件上传
#define _DEF_PACK_Finish_UPFILE    (_DEF_PROTOCAL_BASE + 11)
typedef struct Finish_UPFILE
{
    Finish_UPFILE() :type(_DEF_PACK_Finish_UPFILE), userld(0){
        memset(filename,0,512);
    }
    PackType type;
    int userld;
    char filename[512];
} Finish_UPFILE;
//服务器返回用户文件显示到界面上
#define _DEF_PACK_SHOW_FILE    (_DEF_PROTOCAL_BASE + 12)
typedef struct SHOW_FILE
{
    SHOW_FILE() :type(_DEF_PACK_SHOW_FILE), userld(0){
        memset(filename,0,512);
    }
    PackType type;
    int userld;
    int filesize;
    char filename[512];
} SHOW_FILE;

//下载文件请求
//下载文件请求
#define _DEF_PACK_DOWN_FILERQ    (_DEF_PROTOCAL_BASE + 13)
typedef struct DOWN_FILERQ
{
    DOWN_FILERQ () :type(_DEF_PACK_DOWN_FILERQ), userld(0){
        memset(filename,0,512);
    }
    PackType type;
    int userld;
    int pos;
    char filename[512];
    char filedir[512];
    char breakpointfile[512];
} DOWN_FILERQ ;
//下载文件回复
#define _DEF_PACK_DOWN_FILERS    (_DEF_PROTOCAL_BASE + 14)
typedef struct DOWN_FILERS{
    DOWN_FILERS() :type(_DEF_PACK_DOWN_FILERS), userld(0),filelen(0),filesize(0),pos(0){
        memset(filename,0,512);
        memset(file,0,16*1024);
    }
    PackType type;
    int userld;
    int filelen;
    int filesize;
    int pos;

    char filename[512];
    char file[16*1024];
    int num;
}DOWN_FILERS;

//下载文件完成清理文件FILE*
#define _DEF_PACK_DOWN_FILEFinish    (_DEF_PROTOCAL_BASE + 15)
typedef struct DOWN_FILEFinish{
    DOWN_FILEFinish():type(_DEF_PACK_DOWN_FILEFinish), userld(0){
memset(buff,0,512);
    }
    PackType type;
    int userld;
    char filename[512];
    char buff[512];
}DOWN_FILEFinish;
//异常下线pack
#define _DEF_PACK_abnormal_Offline    (_DEF_PROTOCAL_BASE + 51)
struct abnormal_Offline{
    abnormal_Offline():type(_DEF_PACK_abnormal_Offline){

    }

    PackType type;
    int id;

};
//服务器传来的下载断点传续
#define _DEF_PACK_FUWU_DOWNBREAKPOINT    (_DEF_PROTOCAL_BASE + 16)
typedef struct FUWU_DOWNBREAKPOINT{
    FUWU_DOWNBREAKPOINT():type(_DEF_PACK_FUWU_DOWNBREAKPOINT), userld(0){
        memset(filename,0,512);
        memset(breakpointfile,0,512);
        memset(filedir,0,512);
        memset(clienfiledir,0,512);
    }
    PackType type;
    int userld;
    char filename[512];
    char breakpointfile[512];
    char filedir[512];
    char clienfiledir[512];
}FUWU_DOWNBREAKPOINT;
#endif // PACK_H

