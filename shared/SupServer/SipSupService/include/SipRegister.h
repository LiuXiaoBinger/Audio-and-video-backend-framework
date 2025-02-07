#ifndef _SIPREGISTER_H
#define _SIPREGISTER_H
#include "SipTaskBase.h"
#include "TaskTimer.h"

class SipRegister : public SipTaskBase
{
    public:
        SipRegister();
        ~SipRegister();

        virtual pj_status_t run(pjsip_rx_data *rdata);
        pj_status_t RegisterRequestMessage(pjsip_rx_data *rdata);
        pj_status_t dealWithAuthorRegister(pjsip_rx_data *rdata);
        pj_status_t dealWithRegister(pjsip_rx_data *rdata);
        void registerServiceStart();
        static void RegisterCheckProc(void* param);
    private:
        TaskTimer* m_regTimer;

};

#endif