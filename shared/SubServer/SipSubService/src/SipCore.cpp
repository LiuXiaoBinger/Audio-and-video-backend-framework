#include "SipCore.h"
#include "Common.h"
#include "SipDef.h"
#include "GlobalCtl.h"
#include "SipTaskBase.h"
#include "SipDirectory.h"
#include "SipGbPlay.h"
#include "SipRecordList.h"
//论寻处理endpoind 事物
static int pollingEvent(void* arg)
{
    LOG(INFO)<<"polling event thread start success";
    pjsip_endpoint* ept = (pjsip_endpoint*)arg;
    while(!(GlobalCtl::_gStopPool))
    {
        pj_time_val timeout = {0,500};
        pj_status_t status = pjsip_endpt_handle_events(ept,&timeout);
        if(PJ_SUCCESS != status)
        {
            LOG(ERROR)<<"polling events failed,code:"<<status;
            return -1;
        }
    }

    return 0;

}
pj_bool_t onRxRequest(pjsip_rx_data *rdata)
{
    LOG(INFO)<<"request msg coming....";
    threadParam* param = new threadParam();
    pjsip_rx_data_clone(rdata,0,&param->data);
    pjsip_msg* msg = rdata->msg_info.msg;
    if( msg->line.req.method.id ==PJSIP_OTHER_METHOD)
    {
     
        string rootType = "",cmdType = "CmdType",cmdValue;
        tinyxml2::XMLElement* root = SipTaskBase::parseXmlData(msg,rootType,cmdType,cmdValue);
        LOG(INFO)<<"rootType:"<<rootType;
        LOG(INFO)<<"cmdValue:"<<cmdValue;
        if(rootType == SIP_QUERY )
        {
            if(cmdValue == SIP_CATALOG)
            {
                param->base = new SipDirectory(root);
            }
            else if(cmdValue == SIP_RECORDINFO)
            {
                param->base = new SipRecordList();
            }
            
        }
    }
    else if(msg->line.req.method.id == PJSIP_INVITE_METHOD
            ||msg->line.req.method.id == PJSIP_BYE_METHOD)
    {
        param->base = new SipGbPlay();
    }

    //
    if(param->base==NULL)
    {
        return PJ_SUCCESS;
    }
     pthread_t pid;
    int ret = EC::ECThread::createThread(SipCore::dealTaskThread,param,pid);
    if(ret != 0)
    {
        LOG(ERROR)<<"create task thread error";
        if(param)
        {
            delete param;
            param = NULL;
        }
        return PJ_FALSE;
    }
    return PJ_SUCCESS;
}
static pjsip_module recv_mod=
{

    NULL, NULL,                      // prev, next
    {"mod-recv",8},                    // 模块名称
    -1,                              // 模块ID
    PJSIP_MOD_PRIORITY_APPLICATION,  // 模块优先级
    NULL,                            // 用户数据
    NULL,                            // load 回调
    NULL,                            // start 回调
    NULL,                            // stop 回调
    //NULL,                            // unload 回调
    &onRxRequest,                  // on_rx_request 回调
    NULL,                            // on_rx_response 回调
    NULL,                            // on_tx_request 回调
    NULL,                            // on_tx_response 回调
    NULL                            // on_tsx_state 回调
};
SipCore::SipCore():_endpoint(NULL)
{
}

SipCore::~SipCore()
{
     //pjmedia_endpt_destroy(m_mediaEndpt);
    pjsip_endpt_destroy(_endpoint);
    pj_caching_pool_destroy(&_caching_pool);
     pj_shutdown();
    GlobalCtl::_gStopPool = true;
}

pj_status_t SipCore::init_transport_layer(int sipPort)
{
    pj_status_t status;
    pj_sockaddr_in addr;
    pj_bzero(&addr,sizeof(pj_sockaddr_in));
    addr.sin_family=pj_AF_INET();
    addr.sin_addr.s_addr=0;//表示使用本地ip
    addr.sin_port=pj_htons((pj_uint16_t)sipPort);
    do{
        //udp 
        status=pjsip_udp_transport_start(_endpoint,&addr,NULL,1,NULL);
        if (PJ_SUCCESS != status)
        {
            LOG(ERROR) << "init pjlib faild ,code:" << status;
        }
        status=pjsip_tcp_transport_start(_endpoint,&addr,1,NULL);
        if(PJ_SUCCESS != status)
        {
            LOG(ERROR)<<"start tcp server faild,code:"<<status;
            break;
        }

        LOG(INFO)<<"sip tcp:"<<sipPort<<" udp:"<<sipPort<<" running";
    }while(0);

    return status;
}

bool SipCore::initSip(int sipPort)
{
     pj_status_t status;
    // 0-关闭  6-详细
    pj_log_set_level(0);
    do
    {
        // pjlib初始化
        status = pj_init();
        if (PJ_SUCCESS != status)
        {
            LOG(ERROR) << "init pjlib faild ,code:" << status;
        }
        // 初始化算法库
        status = pjlib_util_init();
        if (PJ_SUCCESS != status)
        {
            LOG(ERROR) << "init pjlib_util faild ,code:" << status;
        }
        // 创建pj的内存池
        pj_caching_pool_init(&_caching_pool, NULL, SIP_STACK_SIZE);

        // 创建pjsip_endpoint对象,一个进程只有一此对象，其他模块对象都是由pjsip_endpoint对象管理和创建
        pjsip_endpt_create(&_caching_pool.factory, NULL, &_endpoint);
        if (PJ_SUCCESS != status)
        {
            LOG(ERROR) << "create sip_endpt faild ,code:" << status;
        }
        // 初始化 事物层模块
        status = pjsip_tsx_layer_init_module(_endpoint);
        if (PJ_SUCCESS != status)
        {
            LOG(ERROR) << "init tsx_layer faild ,code:" << status;
        }
        // 初始化 pjsip_ua_init_module 是 PJSIP 库中的一个函数，用于初始化用户代理（User Agent，简称 UA）模块
        status = pjsip_ua_init_module(_endpoint, NULL);
        if (PJ_SUCCESS != status)
        {
            LOG(ERROR) << "init UA module faild ,code:" << status;
        }
        // 初始化 传输层
        status = init_transport_layer(sipPort);
        if (PJ_SUCCESS != status)
        {
            LOG(ERROR) << "init transport layer faild ,code:" << status;
        }
        // 初始化应用层 pjsip_endpt_register_module 是 PJSIP 库中的一个函数，用于注册一个 SIP 模块到 SIP 端点（endpoint）
        status = pjsip_endpt_register_module(_endpoint, &recv_mod);
        if (PJ_SUCCESS != status)
        {
            LOG(ERROR) << "register recv module faild,code:" << status;
            break;
        }
        status = pjsip_100rel_init_module(_endpoint);
        if(PJ_SUCCESS != status)
        {
            LOG(ERROR)<<"100rel module init faild,code:"<<status;
            break;
        }

        pjsip_inv_callback inv_cb;
        pj_bzero(&inv_cb,sizeof(inv_cb));
        inv_cb.on_state_changed = &SipGbPlay::OnStateChanged;
        inv_cb.on_new_session = &SipGbPlay::OnNewSession;
        inv_cb.on_media_update = &SipGbPlay::OnMediaUpdate;
        inv_cb.on_send_ack = &SipGbPlay::OnSendAck;
        status = pjsip_inv_usage_init(_endpoint,&inv_cb);
        if(PJ_SUCCESS != status)
        {
            LOG(ERROR)<<"register invite module faild,code:"<<status;
            break;
        }
        // 为endpoint创建内存
        m_pool = pjsip_endpt_create_pool(_endpoint, NULL, SIP_ALLOC_POOL_1M, SIP_ALLOC_POOL_1M);
        if (NULL == m_pool)
        {
            LOG(ERROR) << "create pool faild";
            break;
        }
        // 开启一个线程论寻处理endpoint的事件
        pj_thread_t *eventThread = NULL;
        status = pj_thread_create(m_pool, NULL, &pollingEvent, _endpoint, 0, 0, &eventThread);
        if (PJ_SUCCESS != status)
        {
            LOG(ERROR) << "create pjsip thread faild,code:" << status;
            break;
        }
    } while (0);
    return false;
}

void* SipCore::dealTaskThread(void* arg)
{
    threadParam* param = (threadParam*)arg;
    if(!param || param->base == NULL)
    {
        return NULL;
    }
    pj_thread_desc desc;
    pjcall_thread_register(desc);
    param->base->run(param->data);
    delete param;
    param = NULL;
}
