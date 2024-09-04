#include "ckenel.h"
//计算数组下标的宏替换
#define NetPackFunMap(a)  TcpKernel::GetInstance()->m_netProtocalMap[a-_DEF_PROTOCAL_BASE-1]

TcpKernel::TcpKernel()
{
    //dfd=0;
}

//设置协议映射
void TcpKernel::setNetPackMap(){
    //初始化
    memset(m_netProtocalMap, 0, 100);
    //绑定协议头和处理函数
    NetPackFunMap(_DEF_PACK_TCP_UPFILE_RQ) =&TcpKernel::dealupfile;
    NetPackFunMap(_DEF_PACK_TCP_REGISTER_RQ) =&TcpKernel::dealRedisteRq;
    NetPackFunMap(_DEF_PACK_TCP_LOGIN_RQ) = &TcpKernel::dealLonginRq;
    NetPackFunMap(_DEF_PACK_UP_FILEAttributes) = &TcpKernel::dealupfilerq;

    NetPackFunMap(_DEF_PACK_Finish_UPFILE) = &TcpKernel::Finish_upfile;

    NetPackFunMap(_DEF_PACK_DOWN_FILERQ) =&TcpKernel::deldownrq;
    NetPackFunMap(_DEF_PACK_abnormal_Offline) =&TcpKernel::delbreadpoint;
    NetPackFunMap(_DEF_PACK_TCP_OFFLINE_RQ) = &TcpKernel::dealofflioneRq;
    //    NetPackFunMap(_DEF_PACK_UP_FILEAttributes) = &TcpKernel::dealupfilerq;
}

//单例模式
TcpKernel* TcpKernel::GetInstance(){
    static TcpKernel kernel;
    return &kernel;
}
TcpKernel::~TcpKernel()
{

}
//开启核心服务
int TcpKernel::Open(){
    printf( "net init fail:\n" );
    setNetPackMap();
    m_sql = new CMysql;
    // 数据库  没有的话需要创建 不然报错
    if(  !m_sql->ConnectMysql("localhost","root","lyf776688","FileIM")  )
    {
        printf("Conncet Mysql Failed...\n");
        return FALSE;
    }
    else
    {
        printf("MySql Connect Success...\n");
    }
    //初始网络
    m_tcp = new fileServer();
    bool res = m_tcp->InitNet( &TcpKernel::DealData ) ;
    if( !res )
        printf( "net init fail:\n" );

    return TRUE;
}


//关闭核心服务
void TcpKernel::Close(){
    m_sql->DisConnect();
}
//处理网络接收
void TcpKernel::DealData(int clientfd, char*szbuf, int nlen){
    PackType type = *(PackType*)szbuf;
    cout << type << endl;
    //TcpKernel::GetInstance()->dealupfile( clientfd , szbuf , nlen);
    //delete[]szbuf;
    if( (type >= _DEF_PROTOCAL_BASE+1) && ( type < _DEF_PROTOCAL_BASE + 100) )
    {
        PFUN pf = NetPackFunMap( type);
        if( pf )
        {
            (TcpKernel::GetInstance()->*pf)( clientfd , szbuf , nlen);
        }
    }
    //delete[]szbuf;
    return;
}
//事件循环
void TcpKernel::EventLoop(){
    printf( "net init fail:\n" );
    m_tcp->EventLoop();
}
//发送数据
void TcpKernel::SendData( int clientfd, char*szbuf, int nlen ){

    m_tcp->SendData(clientfd,szbuf,nlen);
}

/************** 网络处理 *********************/
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
        cout<<"失败，参数错误"<<endl;
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
    string dir("./netdiscwork/");
    dir+=rp->name;
    mkdir(dir.c_str(),0777);
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
        cout<<"登录shiabai"<<endl;
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
        // cout<<password.c_str()<<"  "<<rp->password<<endl;
        if (0 == strcmp(password.c_str(), rp->password)) {

            //密码一致，返回登录成功
            rs.result = login_success;
            rs.userld = userid;
            //保存id和socket的映射关系
            UserInfo * pInfo = new UserInfo;
            pInfo->m_id = userid;
            pInfo->m_sockfd = clientfd;
            m_mapidToSocket[userid] = pInfo ;

            //---------------------------------------------
            STRU_TCP_FRIEND_INFO userlnfo;
            getlnfoByid(&userlnfo, userid);
            //取出自己的客户端socket
            if (m_mapidToSocket.find(userid) == m_mapidToSocket.end()) {
                return;
            }
            pInfo->uersname=userlnfo.name;
            int userSock = m_mapidToSocket[userid]->m_sockfd;
            cout << password.c_str() << endl;
            //服务器事件集合保存用户id，方便网络意外断开处理
            m_tcp->m_mapSockfdToEvent[clientfd]->usrid=userid;
            //获取用户文件，反馈给客户端
            getUserfileLIst(userid);
            getdownbreadpoint( userid );
            SendData(clientfd, (char*)&rs, sizeof(rs));
            //-----------------------------------------------
            //发送自己的信息给客户端
            //SendData(userSock, (char*)&userlnfo, sizeof(userlnfo));
            cout<<"密码一致"<<endl;
        }
        else {
            //密码不一致
            rs.result = password_error;
            cout<<"密码不一致"<<endl;
            SendData(clientfd, (char*)&rs, sizeof(rs));
        }
    }
    else {
        //如果这个用户不存在，就返回用户不存在
        rs.result = user_not_exist;
        SendData(clientfd, (char*)&rs, sizeof(rs));
        cout<<"用户不存在"<<endl;
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
//获取上传断点传需
void TcpKernel::getupbreadpoint(int userid ){

}

//获取下载断点传需
void TcpKernel::getdownbreadpoint(int userid ){
    list<string>resultList;
    char sql[1024] = "";
    sprintf(sql, "select clienfiledir,breakpointdir,dir ,filename from downbreakpointfile where usrid= %d;", userid);
    if (!m_sql->SelectMysql(sql, 4, resultList)) {
        cout << "查询数据库失败" << endl;
    }
    while(resultList.size()>0){
        FUWU_DOWNBREAKPOINT rq;
        rq.userld=userid;
        memcpy(rq.clienfiledir,resultList.front().c_str(),resultList.front().size());
        resultList.pop_front();
        memcpy(rq.breakpointfile,resultList.front().c_str(),resultList.front().size());
        resultList.pop_front();
        memcpy(rq.filedir,resultList.front().c_str(),resultList.front().size());
        resultList.pop_front();
        memcpy(rq.filename,resultList.front().c_str(),resultList.front().size());
        resultList.pop_front();
        SendData(m_mapidToSocket[userid]->m_sockfd, (char*)&rq, sizeof(rq));
    }
}

//获取用户文件，反馈给客户端,mysql
void TcpKernel::getUserfileLIst(int userid ){
    //SHOW_FILE


    list<string>resultList;
    char sql[1024] = "";
    sprintf(sql, "select filename from file_management where usrid= %d;", userid);
    if (!m_sql->SelectMysql(sql, 1, resultList)) {
        cout << "查询数据库失败" << endl;
    }
    while(resultList.size()>0){
        SHOW_FILE rs;
        rs.userld=userid;
        memcpy(rs.filename,resultList.front().c_str(),resultList.front().size());
        cout<<rs.filename<<"   "<<resultList.front()<<endl;
        resultList.pop_front();
        SendData(m_mapidToSocket[userid]->m_sockfd, (char*)&rs, sizeof(rs));
    }
}
//处理下线请求
void TcpKernel::dealofflioneRq(int clientfd, char*szbuf, int nlen){
    cout << "dealOfflioneRq" << endl;
    // 1、拆包
    STRU_TCP_OFFLINE_RQ* rq = (STRU_TCP_OFFLINE_RQ*)szbuf;
    //2、取出用户id
    int userld = rq->userld;
    abnormal_Offline rqf;
    rqf.id=userld;
    delbreadpoint(0, (char*)&rqf, 0);

}
//处理断点传序
void TcpKernel::delbreadpoint(int clientfd, char*szbuf, int nlen){
    cout << "delbreadpoint" << endl;
    abnormal_Offline*buf=(abnormal_Offline*)szbuf;
    int usrid=buf->id;
    pthread_mutex_lock(&xiaxianlock);
    if(m_mapidToSocket.find(usrid)!=m_mapidToSocket.end()){
        UserInfo* ans=m_mapidToSocket[usrid];
        for(int i=0;i<5;++i){
            if(ans->filename[i].size()>0){
                fdpos*ptr=file[ans->filename[i]];
                ptr->clientdir;
                char sql[1024] ="";
                //注册信息存入数据库
                sprintf(sql, "insert into upbreakpointfile(usrid,dir,filename ,clienfiledir ,MD5 ,flag ,Pos ,filesize)values(%d,'%s','%s','%s','%s',%d,%d,%d);",
                        ptr->m_id,ptr->dir.c_str(),ptr->filename.c_str(),ptr->clientdir.c_str(),ptr->md5.c_str(),1,ptr->pos,ptr->filesize);
                if (!m_sql->UpdataMysql(sql)){
                    cout << "查询数据库失败" << endl;
                }
                close(ptr->fd);
                delete ptr;
                file.erase(ans->filename[i]);
                ans->filename[i].clear();
            }
        }
        if(ans->downfile.size()>0){
            for(auto ptr=ans->downfile.begin();ptr!=ans->downfile.end();ptr++){
                ptr->second->switch_down=0;
                sleep(1);
                char sql[1024] = "";
                //注册信息存入数据库

                int id=ptr->second->m_id;
                const char*dir=ptr->second->dir.c_str();
                const char* breakpoint=ptr->second->breakpoint.c_str();
                const char* clienfiledir=ptr->second->filedir.c_str();
                //const char* filename=ptr->second->filedir.c_str();
                sprintf(sql, "insert into downbreakpointfile(usrid,dir,breakpointdir ,clienfiledir,filename,Pos)values(%d,'%s','%s','%s','%s',%d);",
                        id, dir, breakpoint, clienfiledir,ptr->second->filename.c_str(), 0);
                if (!m_sql->UpdataMysql(sql)) {
                    cout << "查询数据库失败" << endl;
                }

                close(ptr->second->fd);
                cout<<ptr->second->breakpoint<<endl;
            }

            }

        for (auto ite = ans->downfile.begin (); ite != ans->downfile.end();){
            downbreakpoint*chat = ite->second;
            if (chat){

                delete chat;
                chat = NULL;
            }
            ite = ans->downfile.erase (ite) ;
            //ans->downfile.clear();
            perror("dsdasdasdas");
        }

        delete ans;
    }
    if (m_mapidToSocket.count(usrid) > 0) {
        m_mapidToSocket.erase(usrid);
    }
    pthread_mutex_unlock(&xiaxianlock);
    cout<<" ccccccccccccccccccccccccccccccccccc"<<endl;
}
//处理上传文件
void TcpKernel::dealupfilerq(int clientfd, char*szbuf, int nlen){
    UP_FILEAttributes* rq = (UP_FILEAttributes*)szbuf;
    UPFILE_SEUSS rs;
    list<string>resultList;
    char sql[1024] = "";
    sprintf(sql, "select filename from file_management where filename= '%s'and usrid=% d;", rq->filename,rq->userld);

    if (!m_sql->SelectMysql(sql, 2, resultList)) {
        cout << "查询数据库失败" << endl;
    }

    if (resultList.size() > 0) {
        rs.userld=rq->userld;
        rs.result=0;
        SendData(clientfd, (char*)&rs, sizeof(rs));
        cout<<rq->filename<<"xiangtong"<<endl;
        resultList.clear();
        return ;
    }
    resultList.clear();
    cout << "sql:" << sql << endl;
    sprintf(sql, "select MD5 from file_management where MD5= '%s';", rq->md5);
    cout << "sql:" << sql << endl;
    if (!m_sql->SelectMysql(sql, 1, resultList)) {
        cout <<"查询数据库失败"<< endl;
    }

    if (resultList.size() > 0) {
        resultList.clear();
        //查询数据库同步给用户
        sprintf(sql, "select usrid ,flag,dir from file_management where MD5= '%s'and usrid=% d;", rq->md5,rq->userld);
        m_sql->SelectMysql(sql, 3, resultList);
        //判断传输文件是否是自己
        if(/*atoi(resultList.front().c_str())!=rq->userld*/resultList.size()<=0){
            resultList.clear();
            //在查一次表
            sprintf(sql, "select usrid ,flag,dir from file_management where MD5= '%s';", rq->md5);
            cout << "sql:" << sql << endl;
            if (!m_sql->SelectMysql(sql, 3, resultList)) {
                cout << "查询数据库失败" << endl;
            }
            resultList.pop_front();
            cout << "------" << endl;
            //将数据库中的数据取出赋值给新的客户
            int flag=atoi(resultList.front().c_str());
            resultList.pop_front();
            sprintf(sql, "insert into file_management(usrid, filename ,MD5 ,flag,dir) values('%d','%s','%s',%d,'%s');",
                    rq->userld,rq->filename,rq->md5,flag+1,resultList.front().c_str());
            if (!m_sql->UpdataMysql(sql)) {
                cout << "查询数据库失败" << endl;
            }
        }
        rs.userld=rq->userld;
        rs.result=1;
        SendData(clientfd, (char*)&rs, sizeof(rs));
        cout<<"MD5"<<endl;

        return;
    }
    START_UPFILE rss;
    rss.userld=rq->userld;
    //rss.result=0;
    memcpy(rss.dir,rq->dir,512);
    memcpy(rss.filename,rq->filename,strlen(rq->filename)+1);

    cout<<rss.dir<<endl;
    fdpos*fileans=new fdpos;
    fileans->filename=rq->filename;
    fileans->filesize=rq->datasize;
    fileans->clientdir=rq->dir;
    fileans->dir="./netdiscwork/";
    UserInfo*ans=m_mapidToSocket[rq->userld];

    fileans->dir+=ans->uersname;
    fileans->dir+='/';
    fileans->dir+=rq->filename;
    cout<<fileans->dir.c_str()<<endl;
    int dfd=open(fileans->dir.c_str(),O_RDWR|O_CREAT,0664);
    ftruncate(dfd,rq->datasize);

    fileans->fd=dfd;
    fileans->pos=0;
    fileans->md5=rq->md5;

    file[rq->filename]=fileans;


    cout<<rq->filename<<endl;
    for(int i=0;i<5;i++){
        if(ans->filename[i].empty()){
            ans->filename[i]=rq->filename;
            break;
        }
    }
    //ans->filename=rq->filename;
    //ans->file[rq->filename]=fileans;
    SendData(clientfd, (char*)&rss, sizeof(rss));
    cout<<"fasongkais"<<endl;
    // m_mapidToSocket[rq->userld]->file[rq->filename];

    //resultList.pop_front();
}

void TcpKernel::dealupfile(int clientfd, char*szbuf, int nlen){
    // printf("dealupfile\n");
    UP_FILERQ*filebuff=(UP_FILERQ*)szbuf;
    ////    string filemu="../";//m_mapidToSocket[filebuff->userld]->m_userfile;
    ////    string kkk=filebuff->filename;
    //    //filemu+=kkk;
    //    cout<<filebuff->pos<<endl;
    //    if(dfd==0){
    //            dfd=open(filebuff->filename,O_RDWR|O_CREAT,0664);
    //            ftruncate(dfd,filebuff->filesize);
    //    }
    //    lseek(dfd,filebuff->pos,SEEK_SET);
    //     ptr =(char*) mmap(NULL,filebuff->filelen,PROT_READ|PROT_WRITE,MAP_SHARED,dfd,0);
    //       memcpy(ptr,filebuff->file,filebuff->filelen);
    //        //int filesize=lseek(fd,0,SEEK_END);
    //       munmap((void * )ptr ,filebuff->filelen);
    //   // printf("%d\n",lendd);

    //    //close(dfd);

    pthread_mutex_lock(&m_filewrlock );


    int dfd=file[filebuff->filename]->fd;
    //cout<<filebuff->pos<<endl;
    //int dfd=open(filebuff->filename,O_RDWR|O_CREAT,0664);

    lseek(dfd,filebuff->pos,SEEK_SET);
    write(dfd,filebuff->file,filebuff->filelen);
    file[filebuff->filename]->pos=filebuff->pos;
    //close(dfd);
    pthread_mutex_unlock(&m_filewrlock );
    // cout<<filebuff->filename<<endl;
}
//文件上传完成
void TcpKernel::Finish_upfile(int clientfd, char*szbuf, int nlen){
    cout<<" 文件上传完成 "<<endl;
    Finish_UPFILE*filebuff=(Finish_UPFILE*)szbuf;
    close(file[filebuff->filename]->fd);
    for(int i=0;i<5;i++){
        if(m_mapidToSocket[filebuff->userld]->filename[i]==string(filebuff->filename)){
            m_mapidToSocket[filebuff->userld]->filename[i].clear();
            break;
        }
    }
    char sql[1024] = "";
    //注册信息存入数据库
    sprintf(sql, "insert into file_management(usrid, filename ,MD5 ,flag,dir) values(%d,'%s','%s',%d,'%s');",
            filebuff->userld,filebuff->filename,file[filebuff->filename]->md5.c_str(),1,file[filebuff->filename]->dir.c_str());
    if (!m_sql->UpdataMysql(sql)) {
        cout << "查询数据库失败" << endl;
    }
    file.erase(filebuff->filename);

}
//处理下载请求
void TcpKernel::deldownrq(int clientfd, char*szbuf, int nlen){
    cout<<" deldownrq "<<endl;
    DOWN_FILERQ *rq=(DOWN_FILERQ*)szbuf;
    list<string>resultList;
    char sql[1024] = "";
    sprintf(sql, "select dir from file_management where filename= '%s'and usrid=%d;", rq->filename,rq->userld);
    if (!m_sql->SelectMysql(sql, 1, resultList)) {
        cout << "查询数据库失败" << endl;
    }

    UserInfo* ans=m_mapidToSocket[rq->userld];

    //将下载的目录和断点串需包存起来，方便存到数据库
    downbreakpoint*ptr=new downbreakpoint;
    //ans->downfile.insert(pair<string,downbreakpoint*>(rq->filename,ptr));
    cout<<rq->filename<<endl;
    if(ans){
        cout << "kkkkkkkkkkkkkkkkk" << endl;
    }
    string mark(rq->filename);
    ans->downfile[mark]=ptr;

    perror("ddddddddddddd");

    ptr->breakpoint=rq->breakpointfile;
    ptr->filedir=rq->filedir;
    ptr->m_id=rq->userld;

    ptr->dir=resultList.front().c_str();

    ptr->switch_down=1;
    ptr->filename=rq->filename;
    cout<<" ffffffffffffffffffffff "<<endl;
    threaddownfile*df_filedir=new threaddownfile;
  //  cout << "fffffffffffffffffffffffffff" << endl;
    df_filedir->clienfd=m_mapidToSocket[rq->userld]->m_sockfd;
    df_filedir->pos=rq->pos;
    df_filedir->usrid=rq->userld;
    memcpy(df_filedir->filename,rq->filename,strlen(rq->filename)+1);
    memcpy(df_filedir->dir,resultList.front().c_str(),resultList.front().size());
    deldownfile((void*)df_filedir).detach();

}
std::thread TcpKernel::deldownfile(void *arg)
{
    return std::thread(&TcpKernel::deldownfile_rs,this,arg);
}
//将要下载文件放到一个线程处理
void TcpKernel::deldownfile_rs(void* arg){
    cout<<" deldownfile_rs "<<endl;
    threaddownfile*df_filedir=(threaddownfile*)arg;
    TcpKernel*pthis=TcpKernel::GetInstance();
    int fd=open(df_filedir->dir,O_RDWR);
    int filesize=lseek(fd,0,SEEK_END);
    lseek(fd,df_filedir->pos,SEEK_SET);
    UserInfo* ans=m_mapidToSocket[df_filedir->usrid];
    ans->downfile[df_filedir->filename]->fd=fd;
    int npos=df_filedir->pos;
    int i=0;
    while(1){
        if(ans->downfile[df_filedir->filename]->switch_down==0){
            cout<<"shibai"<<endl;
            return;
        }
        DOWN_FILERS rs;
        rs.filesize=filesize;
        memcpy(rs.filename,df_filedir->filename,strlen(df_filedir->filename)+1);
        int num=read(fd,rs.file,16*1024);
        rs.pos=npos;
        rs.filelen=num;
        rs.num=++i;
        //cout<<npos<<rs.file<<endl;
        SendData(df_filedir->clienfd, (char*)&rs, sizeof(rs));
        npos+=num;
        if(npos==filesize){
            cout<<"npos::::::"<<npos<<endl;
            break;

        }
    }
    cout<<"下载回复包"<<i<<endl;
    close(fd);
    sleep(3);
    ans->downfile.erase(df_filedir->filename);
    DOWN_FILEFinish rq;
    char lll[512]="dfdfdfdfdfdfdfdfdfdfdfdfdfdfdfdfdfdfdfdfdfdfdfddfdfdfdfdfdfdfdfdfdf";
    memcpy(rq.filename,df_filedir->filename,strlen(df_filedir->filename)+1);
    memcpy(rq.buff,lll,strlen(lll)+1);
    SendData(df_filedir->clienfd, (char*)&rq, sizeof(rq));
    delete df_filedir;
    char sql[1024] = "";
    //注册信息存入数据库
    sprintf(sql, "delete from downbreakpointfile where usrid = 2;");

    if (!m_sql->UpdataMysql(sql)) {
        cout << "查询数据库失败" << endl;
    }

}


