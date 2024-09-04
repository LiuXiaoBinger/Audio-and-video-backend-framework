#ifndef _SIPREGISTER_H
#define _SIPREGISTER_H
#include "GlobalCtl.h"
#include "TaskTimer.h"
class SipRegister
{
    public:
        SipRegister();
        ~SipRegister();
        void registerServiceStart();
        static void RegisterProc(void* param);
        int gbRegister(GlobalCtl::SupDomainInfo& node);
    private:
        TaskTimer* m_regTimer;
};

#endif