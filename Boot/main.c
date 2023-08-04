#include "AnyID_Boot_SysCfg.h"
#include "AnyID_Boot_Uart.h"

int main(void)
{
    Sys_Init();
    while(1)
    {
        Sys_LedTask();
        Sys_GateTask();
        Sys_BootTask();
        Sys_EC20Task();
        Sys_ServerTask();
        Sys_DownDataTask();
        Sys_ReplaceDeviceTask();

    }
}

                                                              
#ifdef  DEBUG
void assert_failed(u8* file, u32 line)
{
    while(1)
    {
    }
}
#endif
