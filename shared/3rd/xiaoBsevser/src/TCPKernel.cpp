#include<TCPKernel.h>
#include "packdef.h"
#include<stdio.h>
#include<sys/time.h>

using namespace std;

//计算数组下标的宏替换
#define NetPackFunMap(a)  TcpKernel::GetInstance()->m_netProtocalMap[a-_DEF_PROTOCAL_BASE-1]
//设置网络协议映射
void TcpKernel::setNetPackMap()
{
    //初始化
    memset(m_netProtocalMap, 0, _DEF_PROTOCAL_COUNT);
    //绑定协议头和处理函数
    NetPackFunMap(_DEF_PACK_TCP_REGISTER_RQ) =&TcpKernel::dealRedisteRq;
    NetPackFunMap(_DEF_PACK_TCP_LOGIN_RQ) = &TcpKernel::dealLonginRq;
    NetPackFunMap(_DEF_PACK_TCP_CHAT_RQ) = &TcpKernel::dealChatRq;
    NetPackFunMap(_DEF_TCP_voice_RS) = &TcpKernel::dealChatRq;
    NetPackFunMap(_DEF_TCP_voice_RQ) = &TcpKernel::dealChatRq;
    NetPackFunMap(_DEF_PACK_TCP_ADDFRIEND_RQ) = &TcpKernel::dealAddFriendRq;
    NetPackFunMap(_DEF_PACK_TCP_ADDFRIEND_RS) = &TcpKernel::dealAddFriendRs;
    NetPackFunMap(_DEF_PACK_TCP_OFFLINE_RQ) = &TcpKernel::dealofflioneRq;
    NetPackFunMap(_DEF_TCP_AUDIO_CHAT) = &TcpKernel::AudioFrameRq;
    NetPackFunMap(EXIT_VIDEO_CALL_RQ) = &TcpKernel::dealChatRq;
    NetPackFunMap(_DEF_TCP_VIDEO_CHAT) = &TcpKernel::VideoFrameRq;
    NetPackFunMap(_DEF_TCP_VIDEO_RQ) = &TcpKernel::dealChatRq;
    NetPackFunMap(_DEF_TCP_VIDEO_RS) = &TcpKernel::dealChatRq;

    NetPackFunMap(DEF_PACK_VIDEO_REGISTER) = &TcpKernel::VideoRegister;
    NetPackFunMap(DEF_PACK_AUDIO_REGISTER) = &TcpKernel::AudioRegister;


}


TcpKernel::TcpKernel()
{

}

TcpKernel::~TcpKernel()
{

}

TcpKernel *TcpKernel::GetInstance()
{
    static TcpKernel kernel;
    return &kernel;
}

int TcpKernel::Open( int port)
{
    //initRand();
    setNetPackMap();
    m_sql = new CMysql;
    // 数据库 使用127.0.0.1 地址  用户名root 密码colin123 数据库 wechat 没有的话需要创建 不然报错
    if(  !m_sql->ConnectMysql("localhost","root","lyf776688","Xbim")  )
    {
        printf("Conncet Mysql Failed...\n");
        return FALSE;
    }
    else
    {
        printf("MySql Connect Success...\n");
    }
    //初始网络
    m_tcp = new Block_Epoll_Net;
    bool res = m_tcp->InitNet( port , &TcpKernel::DealData ) ;
    if( !res )
        err_str( "net init fail:" ,-1);

    return TRUE;
}

void TcpKernel::Close()
{
    m_sql->DisConnect();

}



void TcpKernel::EventLoop()
{
    printf("event loop\n");
    m_tcp->EventLoop();
}

void TcpKernel::SendData(int clientfd, char *szbuf, int nlen)
{
    m_tcp->SendData(clientfd , szbuf ,nlen );
}
void TcpKernel::DealData(int clientfd,char *szbuf,int nlen)
{
    PackType type = *(PackType*)szbuf;
    cout << type << endl;
    if( (type >= _DEF_PROTOCAL_BASE) && ( type < _DEF_PROTOCAL_BASE + DEF_PACK_COUNT) )
    {
        PFUN pf = NetPackFunMap( type );
        if( pf )
        {
            (TcpKernel::GetInstance()->*pf)( clientfd , szbuf , nlen);
        }
    }

    return;
}


//注册
void TcpKernel::dealRedisteRq(int clientfd, char*szbuf, int nlen){
        cout << "dealRedisteRq" << endl;
        cout << nlen << endl;
        //拆包
        STRU_TCP_REGISTER_RQ* rp = (STRU_TCP_REGISTER_RQ*)szbuf;
        STRU_TGP_REGISTER_RS rs;
        //校验数据合法性
        if (strlen(rp->name) > 10 || strlen(rp->tel) != 11 || strlen(rp->password) > 15 || strlen(rp->name) == 0
            || strlen(rp->password) == 0) {
            //给客户端回复注册结果————失败，参数错误
            rs.result = packet_error;
            SendData(clientfd, (char*)&rs, sizeof(rs));
            return;
        }

        //注册用户是否存在----昵称唯一
        list<string>resultList;
        char sql[1024] = "";
        sprintf(sql, "select id from t_user where name='%s';", rp->name);
        //cout << "sql" << sql << endl;
        if (!m_sql->SelectMysql(sql, 1, resultList)) {
            cout << "查询数据库失败" << endl;
        }
        //判断插叙结果
        if (resultList.size()>0) {
            //昵称存在
            rs.result = user_is_exist;
        }
        else {
            //昵称不纯在
            //注册用户是否存在---手机号唯一
            sprintf(sql, "select tel from t_user where tel= '%s';", rp->tel);
            //cout << "sql" << sql << endl;
            if (!m_sql->SelectMysql(sql, 1, resultList)) {
                cout << "查询数据库失败" << endl;
            }
            //查询结果
            if (resultList.size() > 0) {
                //电话号码已经存在
                rs.result = user_is_exist;
            }
            else {
                //5、注册信息存入数据库
                sprintf(sql, "insert into t_user(name, tel ,password ,icon ,feeling) values('%s','%s','%s',%d,'%s');",
                        rp->name,rp->tel,rp->password,1,"rty");
                cout <<  rp->name<< rp->tel << rp->password<<endl;
                cout << "sql" << sql << endl;
                if (!m_sql->UpdataMysql(sql)) {
                    cout << "查询数据库失败" << endl;
                }
                rs.result = register_success;
            }
        }
        SendData(clientfd, (char*)&rs, sizeof(rs));
}
//登录
void TcpKernel::dealLonginRq(int clientfd, char*szbuf, int nlen){
    cout << "dealLonginRq" << endl;
        //测试代码
        /*STRU_TCP_LOGIN_RS rs;
        m_pServer->SendData(clientfd, (char*)&rs, sizeof(rs));*/
        STRU_TCP_LOGIN_RQ* rp = (STRU_TCP_LOGIN_RQ*)szbuf;
        STRU_TCP_LOGIN_RS rs;
        //校验
        //校验数据合法性
        if ( strlen(rp->tel) != 11 || strlen(rp->password) > 15
            || strlen(rp->password) == 0) {
            //给客户端回复注册结果————失败，参数错误
            rs.result = packet_error;
            SendData(clientfd, (char*)&rs, sizeof(rs));
            return;
        }
        //根据tel查询是否有这个用户
        list<string>resultList;
        char sql[1024] = "";
        sprintf(sql, "select id, password from t_user where tel= '%s';", rp->tel);
        cout << "sql" << sql << endl;
        if (!m_sql->SelectMysql(sql, 2, resultList)) {
            cout << "查询数据库失败" << endl;
        }
        int userid = 0;
        string password = "";
        if (resultList.size() > 0) {
            //如果存在这个用户，取出密码和用户输入的密码进行比较
            userid = atoi(resultList.front().c_str());
            resultList.pop_front();

            password = resultList.front();
            resultList.pop_front();

            if (0 == strcmp(password.c_str(), rp->password)) {

                //密码一致，返回登录成功
                rs.result = login_success;
                rs.userld = userid;
                //保存id和socket的映射关系
                UserInfo * pInfo = new UserInfo;
                            pInfo->m_id = userid;

                            pInfo->m_sockfd = clientfd;
                m_mapidToSocket[userid] = pInfo ;
                cout << password.c_str() << endl;
                SendData(clientfd, (char*)&rs, sizeof(rs));
                //获取好友的信息，包括自己
                getUserLIst(userid);

            }
            else {
                //密码不一致
                rs.result = password_error;

                SendData(clientfd, (char*)&rs, sizeof(rs));
            }
        }
        else {
            //如果这个用户不存在，就返回用户不存在
            rs.result = user_not_exist;
            SendData(clientfd, (char*)&rs, sizeof(rs));
        }
}
//获取登录用户好友列表的信息，需要包括自己
void TcpKernel::getUserLIst(int id){
    //查询自己的信息

        STRU_TCP_FRIEND_INFO userlnfo;
        getlnfoByid(&userlnfo, id);
        //取出自己的客户端socket
        if (m_mapidToSocket.find(id) == m_mapidToSocket.end()) {
            return;
        }
        int userSock = m_mapidToSocket[id]->m_sockfd;

        //发送自己的信息给客户端
        SendData(userSock, (char*)&userlnfo, sizeof(userlnfo));

        //查询好友id
        list<string>resultList;
        char sql[1024] = "";
        sprintf(sql, "select idB from t_friend where idA= '%d';", id);
        cout << "sql:" << sql << endl;
        if (!m_sql->SelectMysql(sql, 1, resultList)) {
            cout << "查询数据库失败" << endl;
        }
         cout <<"getUserLIst:"<< endl;
        int friendld = 0;
        STRU_TCP_FRIEND_INFO friendInfo;
        int friendSock = 0;
        // 5、遍历查询结果
        while (resultList.size() > 0) {
            // 6、取出好友id
            friendld = atoi(resultList.front().c_str());
            resultList.pop_front();
            //7、根据好友id查询好友信息
            getlnfoByid(&friendInfo, friendld);
            // 8、把好友信息发送给客户端
            SendData(userSock, (char*)&friendInfo, sizeof(friendInfo));
            // 9、如果好友在线，需要通知好友自己已上线
            if (m_mapidToSocket.find(friendld) == m_mapidToSocket.end()) {
                continue;
            }
            friendSock = m_mapidToSocket[friendld]->m_sockfd;
            SendData(friendSock, (char*)&userlnfo, sizeof(userlnfo));
        }

}
//根据用户id查询用户信息
void TcpKernel::getlnfoByid(STRU_TCP_FRIEND_INFO* info, int id){
    cout << "getlnfoByid" << endl;
        //保存用户id
        info->userld = id;
        //根据id查询用户信息
        list<string>resultList;
        char sql[1024] = "";
        sprintf(sql, "select name,icon feeling from t_user where id= '%d';", id);
        cout << "sql" << sql << endl;
        if (!m_sql->SelectMysql(sql, 3, resultList)) {
            cout << "查询数据库失败" << endl;
        }
        if (3 == resultList.size()) {
            strcpy(info->name, resultList.front().c_str());
            resultList.pop_front();
            info->iconld = atoi(resultList.front().c_str());
            resultList.pop_front();
            strcpy(info->feeling, resultList.front().c_str());
            resultList.pop_front();
        }
        //判断用户是否在线
        if (m_mapidToSocket.find(id) != m_mapidToSocket.end()) {
            //在线
            info->state=1;
        }
        else {
            //不在线
            info->state = 0;
        }
}
//处理聊天请求
void TcpKernel::dealChatRq(int clientfd, char*szbuf, int nlen){
        cout << "dealChatRq" << endl;
            //1、拆包
        char* tmp = szbuf;
        //跳过type
        tmp += sizeof(int);
        //读取 userid
        int uid = *(int*) tmp;
        //跳过 userid
        tmp += sizeof(int);
        //获取 roomid
        int friendld =  *(int*) tmp;  //按照int取数据

        //2、查看对端是否在线
        if (m_mapidToSocket.find(friendld) == m_mapidToSocket.end()) {
            //3、如果不在线，就直接回复用户不在线
             cout << "转发shibai" << endl;
            STRU_TCP_CHAT_RS rs;
            rs.friendld = friendld;
            rs.result = user_offline;
            rs.userld = uid;
            SendData(clientfd, (char*)&rs, sizeof(rs));
        }
        else {
            //4、在线就转发聊天请求
            cout << "转发成功" << endl;
            //4.1、去除好友的socket
            SendData(m_mapidToSocket[friendld]->m_sockfd, szbuf, nlen);
        }
}
//处理添加好友请求
void TcpKernel::dealAddFriendRq(int clientfd, char*szbuf, int nlen){
    cout << "dealAddFriendRq" << endl;
        //1、拆包
        STRU_TCP_ADD_FRIEND_RQ* rq = (STRU_TCP_ADD_FRIEND_RQ*)szbuf;
        //2、根据昵称查询用户id
        list<string>resultList;
        char sql[1024] = "";
        sprintf(sql, "select id from t_user where name= '%s';", rq->friendName);
        cout << "sql" << sql << endl;
        if (!m_sql->SelectMysql(sql, 1, resultList)) {
            cout << "查询数据库失败" << endl;
        }
        STRU_TCP_ADD_FRIEND_RS rs;
        //3.判断这个用户是否存在
        if (0 == resultList.size()) {
            //3.1、如果用户不存在，给客户端返回结果
            rs.friendld = 0;
            rs.userld = rq->userld;
            strcpy(rs.friendName, rq->friendName);
            rs.result = no_this_user;

            SendData(clientfd, (char*)&rs, sizeof(rs));
        }
        else {
            //3.2、如果用户存在，取出好友id
            int friendld = atoi(resultList.front().c_str());
            resultList.pop_front();

            //3.3、判断好友是否在线
            if (m_mapidToSocket.count(friendld) > 0) {
                //3.3.1、如果好友在线，取出好友的socket，转发添加好友请求
                SendData(m_mapidToSocket[friendld]->m_sockfd, szbuf, nlen);
            }
            else {
                //3.3.2、如果好友不在线，回复用户好友不在线
                rs.friendld = friendld;
                rs.userld = rq->userld;
                strcpy(rs.friendName, rq->friendName);
                rs.result = user_offline;

                SendData(clientfd, (char*)&rs, sizeof(rs));

            }
        }
}
//处理添加好友回复
void TcpKernel::dealAddFriendRs(int clientfd, char*szbuf, int nlen){
    cout << "dealAddFriendRs" << endl;
        //1、拆包
        STRU_TCP_ADD_FRIEND_RS* rs = (STRU_TCP_ADD_FRIEND_RS*)szbuf;

        //2、判断好友是否同意
        if (add_success == rs->result) {
            //3.如果好友同意添加请求，把好友关系写入数据库

            char sqlBuf[1024] = "";
            sprintf(sqlBuf, "insert into t_friend values(%d,%d);", rs->friendld, rs->userld);
            cout << "sql" << sqlBuf << endl;
            if (!m_sql->UpdataMysql(sqlBuf)) {
                cout << "查询数据库失败" << endl;
            }
            sprintf(sqlBuf, "insert into t_friend values(%d,%d);", rs->userld, rs->friendld);
            cout << "sql" << sqlBuf << endl;
            if (!m_sql->UpdataMysql(sqlBuf)) {
                cout << "查询数据库失败" << endl;
            }
            //4、更新好友列表
            getUserLIst(rs->friendld);
            //5、不管好友是否同意添加请求，都需要转发回复包给客户端
            if (m_mapidToSocket.count(rs->userld) > 0) {
                SendData(m_mapidToSocket[rs->userld]->m_sockfd, szbuf, nlen);
            }
        }
}
//处理下线请求
void TcpKernel::dealofflioneRq(int clientfd, char*szbuf, int nlen){
    cout << "dealOfflioneRq" << endl;
        // 1、拆包
        STRU_TCP_OFFLINE_RQ* rq = (STRU_TCP_OFFLINE_RQ*)szbuf;
        //2、取出用户id
        int userld = rq->userld;
        // 3、根据用户id，查询用户的好友列表
        list<string> resultList;
        char sql[1024] = "";
        sprintf(sql,"select idB from t_friend where idA = '%d';", userld);
        cout << "sql: " << sql << endl;
        if (!m_sql->SelectMysql(sql, 1, resultList)) {
            cout << "查询数据库失败" << endl;
        }

        int friendld = 0;
        int friendSock = -1;
        //4、遍历查询结果
        while (resultList.size() > 0) {
            //5、获取每个好友的id
            friendld = atoi(resultList.front().c_str());
            resultList.pop_front();
            // 6、根据好友id获取到每个好友的socket
            if (m_mapidToSocket.count(friendld)>0){
                friendSock = m_mapidToSocket[friendld]->m_sockfd;
                //7、给每个好友转发下线通知
                SendData(friendSock,szbuf, nlen);
               }
        }
        // 8、把用户的socket从map中删除
        if (m_mapidToSocket.count(userld) > 0) {
            m_mapidToSocket.erase(userld);
        }
}
//处理音频帧
void TcpKernel::AudioFrameRq(int clientfd, char *szbuf, int nlen)
{
     //   //拆包
        char* tmp = szbuf;
        //跳过type
        tmp += sizeof(int);
        //读取 userid
        int uid = *(int*) tmp;
        //跳过 userid
        tmp += sizeof(int);
        //获取 roomid
        int friendld =  *(int*) tmp;  //按照int取数据
        //2、查看对端是否在线
        if (m_mapidToSocket.find(friendld) == m_mapidToSocket.end()) {
            //3、如果不在线，就直接回复用户不在线
            cout << "转发shibai" << endl;
            STRU_TCP_CHAT_RS rs;
            rs.friendld = friendld;
            rs.result = user_offline;
            rs.userld = uid;
            SendData(clientfd, (char*)&rs, sizeof(rs));
        }
        else {
            //4、在线就转发聊天请求
            cout << "转发成功" << endl;
            //4.1、去除好友的socket
            SendData(m_mapidToSocket[friendld]->m_audiofd, szbuf, nlen);
        }
}
//处理视频帧
void TcpKernel::VideoFrameRq(int clientfd, char *szbuf, int nlen)
{
 //   printf("clientfd:%d VideoFrameRq\n", clientfd);

    //拆包
    char* tmp = szbuf;
    //跳过type
    tmp += sizeof(int);
    //读取 userid
    int uid = *(int*) tmp;
    //跳过 userid
    tmp += sizeof(int);
    //获取 roomid
    int friendld =  *(int*) tmp;  //按照int取数据

    //2、查看对端是否在线
    if (m_mapidToSocket.find(friendld) == m_mapidToSocket.end()) {
        //3、如果不在线，就直接回复用户不在线
        cout << "转发shibai" << endl;
        STRU_TCP_CHAT_RS rs;
        rs.friendld = friendld;
        rs.result = user_offline;
        rs.userld = uid;
        SendData(clientfd, (char*)&rs, sizeof(rs));
    }
    else {
        //4、在线就转发聊天请求
        cout << "转发成功" <<m_mapidToSocket[friendld]->m_videofd<< endl;
        //4.1、去除好友的socket
        SendData(m_mapidToSocket[friendld]->m_videofd, szbuf, nlen);
    }
}

//音频注册
void TcpKernel::AudioRegister(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d AudioRegister\n", clientfd);
    //拆包
    STRU_AUDIO_REGISTER *rq = (STRU_AUDIO_REGISTER *)szbuf;
    int userid = rq->m_userid;
      cout<<userid<<endl;
    //根据 userid  找到节点 更新 fd
    if( m_mapidToSocket.count(userid) )
    {
        UserInfo *  info = m_mapidToSocket[userid];
        info->m_audiofd = clientfd;
    }
}
//视频注册
void TcpKernel::VideoRegister(int clientfd, char *szbuf, int nlen)
{
    printf("clientfd:%d VideoRegister\n", clientfd);
    //拆包
    STRU_VIDEO_REGISTER *rq = (STRU_VIDEO_REGISTER *)szbuf;
    int userid = rq->m_userid;
    cout<<userid<<endl;
    //根据 userid  找到节点 更新 fd
    if( m_mapidToSocket.count(userid) )
    {
        UserInfo *  info = m_mapidToSocket[userid];
        info->m_videofd = clientfd;
    }
}
