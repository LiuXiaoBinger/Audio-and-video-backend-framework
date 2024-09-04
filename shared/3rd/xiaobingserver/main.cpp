#include <iostream>
#include"thread_pool.h"
#include"ckenel.h"
using namespace std;
#include <signal.h>
int main()
{
    sigset_t nset,oset ;
    sigemptyset(&nset);
    sigaddset(&nset,SIGPIPE);
    sigprocmask(SIG_SETMASK,&nset,&oset);
    chdir("../");
    TcpKernel * pKernel =  TcpKernel::GetInstance();

    //开启服务
    pKernel->Open();
    // 事件循环 : 循环监听事件
    pKernel->EventLoop();

    pKernel->Close();
    return 0;
}

