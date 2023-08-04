#include "AnyID_Boot_SysCfg.h"

u32 g_nSysState = 0;
u32 g_nDeviceNxtEraseAddr = 0;
u32 g_nDeviceNxtDownloadAddr = 0;
u32 g_nBootLimitTime = 0;

#define SYS_BOOT_VER_SIZE               50
const u8 SYS_BOOT_VERSION[SYS_BOOT_VER_SIZE] = "Boot V3.1_22032600 GD32F3xx";

void Sys_Delayms(u32 n)             //ϵͳ��ʱn����
{
    n *= 0x6000;
    n++;
    while(n--);
}

void Sys_CfgClock(void)
{
    ErrorStatus HSEStartUpStatus;

    RCC_DeInit();
    //Enable HSE
    RCC_HSEConfig(RCC_HSE_ON);

    //Wait till HSE is ready
    HSEStartUpStatus = RCC_WaitForHSEStartUp();

    if(HSEStartUpStatus == SUCCESS)
    {
        //HCLK = SYSCLK = 72.0M
        RCC_HCLKConfig(RCC_SYSCLK_Div1);

        //PCLK2 = HCLK = 72.0M
        RCC_PCLK2Config(RCC_HCLK_Div1);

        //PCLK1 = HCLK/2 = 33.9M
        RCC_PCLK1Config(RCC_HCLK_Div2);

        //ADCCLK = PCLK2/2
        RCC_ADCCLKConfig(RCC_PCLK2_Div8);

        // Select USBCLK source 72 / 1.5 = 48M
        RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);

        //Flash 2 wait state
        FLASH_SetLatency(FLASH_Latency_2);

        //Enable Prefetch Buffer
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

        //PLLCLK = 12.00MHz * 10 = 120 MHz
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_10);    //PLL���������

        //Enable PLL
        RCC_PLLCmd(ENABLE);

        //Wait till PLL is ready
        while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
        {
        }

        //Select PLL as system clock source
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

        //Wait till PLL is used as system clock source
        while(RCC_GetSYSCLKSource() != 0x08)
        {
        }
    }
}

void Sys_CfgPeriphClk(FunctionalState state)
{
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                          RCC_APB2Periph_GPIOB |
                          RCC_APB2Periph_GPIOC |
                          RCC_APB2Periph_GPIOD |
                          RCC_APB2Periph_AFIO, state);
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2 | 
                           RCC_APB1Periph_USART3 |RCC_APB1Periph_UART4|
                           RCC_APB1Periph_UART5  , state);

}

void Sys_CfgNVIC(void)
{
    //NVIC_InitTypeDef NVIC_InitStructure;
#ifdef  VECT_TAB_RAM
    //Set the Vector Table base location at 0x20000000
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else  //VECT_TAB_FLASH
    //Set the Vector Table base location at 0x08000000
#ifdef _ANYID_BOOT_STM32_
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x4000);
#else
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0000);
#endif
#endif

    //Configure the Priority Group to 2 bits
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}

const PORT_INF SYS_RUNLED_COM = {GPIOB, GPIO_Pin_1};
const PORT_INF SYS_DATALED_COM = {GPIOB, GPIO_Pin_3};
void Sys_CtrlIOInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = SYS_RUNLED_COM.Pin;
    GPIO_Init(SYS_RUNLED_COM.Port, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = SYS_DATALED_COM.Pin;
    GPIO_Init(SYS_DATALED_COM.Port, &GPIO_InitStructure);

}



void Sys_Init(void)
{
#if SYS_ENABLE_WDT
    WDG_InitIWDG();
#endif
    //
    Sys_CfgClock();

    Sys_CfgNVIC();
    Sys_CfgPeriphClk(ENABLE);

    //��ֹ�ж�
    Sys_DisableInt();
    Sys_CtrlIOInit();
    Sys_RunLedOn();

    FRam_InitInterface();
    Fram_ReadBootParamenter();
    
    Flash_InitInterface();
    Flash_Init();
    //Flash_ReadId();
    //Flash_Demo();               //--------------------����

    Gate_InitInterface(GATE_BAUDRARE);
    Gate_ConfigInt(ENABLE);
    Gate_EnableInt(ENABLE, DISABLE);
	
	//g_sFramBootParamenter.appState = FRAM_BOOT_APP_DATA_DOWD;
    
    if(g_sFramBootParamenter.appState != FRAM_BOOT_APP_OK)
    {
        EC20_Init();
        EC20_ConnectInit(&g_sEC20Connect, EC20_CNT_CMD_PWRON, &g_sEC20Params);
        a_SetState(g_sEC20Connect.state, EC20_CNT_OP_STAT_TX);
    }

    //SysTick ��ʼ�� 5ms
    STick_InitSysTick();
    Sys_RunLedOff();

    //ʹ���ж�
    Sys_EnableInt();
    

    
}

void Sys_LedTask(void)
{ 
    static u32 ledTimes = 0;
    if(a_CheckStateBit(g_nSysState, SYS_STAT_RUNLED))
    {
        a_ClearStateBit(g_nSysState, SYS_STAT_RUNLED);
        ledTimes ++;
        if(ledTimes & 0x01)
        {
            Sys_RunLedOff();
            Sys_DataLedOff();
        }
        else
        {
            Sys_RunLedOn();
            Sys_DataLedOn();
        }
    #if SYS_ENABLE_WDT
        WDG_FeedIWDog();
    #endif
    }
}

typedef  void (*pFunction)(void);
pFunction Jump_To_Application;
uint32_t JumpAddress;
void Sys_Jump(u32 address)
{
    u32 stackAddr = 0;
    Sys_DisableInt();
    stackAddr = *((vu32 *)address);
    //�鿴ջ��ַ�Ƿ���RAM������CCRAM��
    if((stackAddr & 0x2FFE0000) == 0x20000000)
    {
        JumpAddress = *(vu32 *)(address + 4);
        Jump_To_Application = (pFunction) JumpAddress;

        __set_MSP(*(vu32 *)address);
        Jump_To_Application();
    }
    else
    {
        a_SetState(g_nSysState, SYS_STAT_IDLE);
        g_nSysTick = 0;
    }
    //while(1)
    {
    #if SYS_ENABLE_WDT
        WDG_FeedIWDog();
    #endif
    }
    Sys_EnableInt();
}

void Sys_GateTask(void)
{
    if(USART_GetFlagStatus(GATE_PORT, USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE))
    {
        USART_ClearFlag(GATE_PORT, USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE);
        
        Gate_EnableInt(DISABLE, DISABLE);
        memset(&g_sGateBootInfo, 0, sizeof(GATE_BOOTINFO));
        Gate_InitInterface(GATE_BAUDRARE);
        Gate_ConfigInt(ENABLE);
        Gate_EnableInt(ENABLE, DISABLE);
    }

    //���ﻹҪ�����ж��Ƿ���SM5002����
    if(a_CheckStateBit(g_nSysState, SYS_STAT_REPLACE_GATE_DATA))
    {
        if(Uart_IsRcvFrame(g_sGateRcvFrame))
        {
            Gate_ProcessBootFrame(&g_sGateRcvFrame);
            Uart_ResetFrame(g_sGateRcvFrame);
        }
        
        if(g_sGateBootInfo.state == GATE_OP_STAT_IDLE)
        {
            if(g_sGateBootInfo.slvIndex < g_sFramBootParamenter.gateNum)        //û�и������
            {
                Gate_ClearBootInfo();
				g_nBootLimitTime = g_nSysTick;
                g_sGateBootInfo.sectorNum = g_sFramBootParamenter.size - 1;     //���������յ�crcУ��֡�����֡�����ǲ���Ҫ���ص���Ƭ����
                Flash_ReadBuffer(FLASH_DATA_OPEN_ADDR  + GATE_BOOT_SECTOR_SIZE * (g_sGateBootInfo.sectorNum - 1), GATE_BOOT_SECTOR_SIZE,  g_sGateBootInfo.bin);       //�x����һ�M�������_ʼ����
                g_sGateBootInfo.cmd = UART_FRAME_CMD_RESET;
                g_sGateBootInfo.txFrame.len = Uart_ReqReset(g_sGateBootInfo.txFrame.buffer, g_sGateBootInfo.slvIndex + 1);
                g_sGateBootInfo.state = GATE_OP_STAT_TX;
            }
            else
            {
                 a_ClearStateBit(g_nSysState, SYS_STAT_REPLACE_GATE_DATA);
                 g_sFramBootParamenter.appState = FRAM_BOOT_APP_REPLACE_OVER;
                //�������---------
            }
        }
        else if(g_sGateBootInfo.state == GATE_OP_STAT_TX)
        {
            memset(&g_sGateRcvFrame, 0, sizeof(UART_RCVFRAME));
            Gate_WriteBuffer(g_sGateBootInfo.txFrame.buffer, g_sGateBootInfo.txFrame.len);
            g_sGateBootInfo.txFrame.len = 0;
            g_sGateBootInfo.tick = g_nSysTick;
            g_sGateBootInfo.state = GATE_OP_STAT_WAIT;
        }
        else if(g_sGateBootInfo.state == GATE_OP_STAT_WAIT)
        {
            if(g_sGateBootInfo.tick + GATE_BOOT_TO_TIM < g_nSysTick)
            {
                g_sGateBootInfo.state = GATE_OP_STAT_TO | GATE_OP_STAT_STEP;
            }
        }
        else if(g_sGateBootInfo.state & GATE_OP_STAT_STEP)
        {
            g_sGateBootInfo.state &= (~GATE_OP_STAT_STEP);
            if(g_sGateBootInfo.state == GATE_OP_STAT_TO)            //��ʱ���ԵĴ�����������������������������������������������������
            {
                //repeat���֮����������豸g_sGateBootInfo.slvIndex++��
                g_sGateBootInfo.state = GATE_OP_STAT_IDLE;
                
                if(g_sGateBootInfo.repat > GATE_UPDATA_OP_TICK)
                {
                    g_sGateBootInfo.slvIndex++;
                    g_sGateBootInfo.repat = 0;
                }
                else
                {
                    g_sGateBootInfo.repat ++;
                }
            }
            else
            {
                if(g_sGateBootInfo.cmd == UART_FRAME_CMD_DL)
                {
                    Flash_ReadBuffer(FLASH_DATA_OPEN_ADDR  + GATE_BOOT_SECTOR_SIZE * (g_sGateBootInfo.sectorIndex - 2), GATE_BOOT_SECTOR_SIZE,  g_sGateBootInfo.bin);;    
                }
                Gate_StepBoot(&g_sGateBootInfo, &g_sGateRcvFrame);
                g_sGateBootInfo.repat = 0;
                if(g_sGateBootInfo.txFrame.len > 0)
                {
                    g_sGateBootInfo.state = GATE_OP_STAT_TX;
                }
                else
                {
                    g_sGateBootInfo.state = GATE_OP_STAT_IDLE;
                }
                Sys_Delayms(10);
            }
            
        }
    }
}

void Sys_BootTask(void)
{
    static u32 bootState =0;
    
    if(bootState != g_sFramBootParamenter.appState)                                                                                                     
    {
        if(g_sFramBootParamenter.appState == FRAM_BOOT_APP_OK  || g_sFramBootParamenter.appState == FRAM_BOOT_APP_NULL_REPLACE)
        {
            a_SetState(g_nSysState, SYS_STAT_IDLE);
        }
        else if(g_sFramBootParamenter.appState == FRAM_BOOT_APP_DATA_DOWD || g_sFramBootParamenter.appState == FRAM_BOOT_APP_FAIL)//-------У��ʧ�ܴ���
        {
            a_SetStateBit(g_nSysState, SYS_STAT_DOWNLOAD);
            g_sDeviceUpDataInfo.flag = DEVICE_UPDATA_FLAG_RQ;
        }
        else if(g_sFramBootParamenter.appState == FRAM_BOOT_APP_DATA_DOWN_OVER)
        {
				a_SetStateBit(g_nSysState, SYS_STAT_CHK_VERSION);
			//Device_At_Rsp(EC20_CNT_TIME_1S * 2, EC20_CNT_REPAT_NULL, DEVICE_HTTP_POST_INFO_VERSION);
        }
        else if(g_sFramBootParamenter.appState == FRAM_BOOT_APP_REPLACE)
        {
			a_SetStateBit(g_nSysState, SYS_STAT_CHK_VERSION);
            if(g_sFramBootParamenter.flag == DEVICE_TYPE_SM5001)
            {
                a_SetStateBit(g_nSysState, SYS_STAT_REPLACE_DATA);
                g_nDeviceNxtEraseAddr = SYS_APP_START_ADDR;
                FLASH_Unlock();
            }
            else if(g_sFramBootParamenter.flag == DEVICE_TYPE_SM5002)
            {
                a_SetStateBit(g_nSysState, SYS_STAT_REPLACE_GATE_DATA);
            }
            else
            {
                g_sFramBootParamenter.appState = FRAM_BOOT_APP_OK;     //���ܳ��򱣴�
            }

        }
        else if(g_sFramBootParamenter.appState == FRAM_BOOT_APP_REPLACE_OVER)
        {
            Device_ClearInfo(); 
			EC20_KeyLow();
			Sys_DataLedOff();
			Sys_Delayms(500);
            Sys_SoftReset();     
        }
        
        bootState = g_sFramBootParamenter.appState;
        Fram_WriteBootParamenter();
    }
    
    if(a_CheckStateBit(g_nSysState, SYS_STAT_IDLE))
    {
        if(g_nSysTick > 40)               
        {
            a_SetStateBit(g_nSysState, SYS_STAT_JMP);
        }
    }

    if(a_CheckStateBit(g_nSysState, SYS_STAT_JMP))
    {
       Sys_RunLedOff();
       a_ClearStateBit(g_nSysState, SYS_STAT_JMP);
       if(Sys_CheckVersion())
       {
          Sys_Jump(SYS_APP_START_ADDR);
       }
       else
       {
          g_sFramBootParamenter.appState = FRAM_BOOT_APP_FAIL;
       }
    }
    
    if(a_CheckStateBit(g_nSysState, SYS_STAT_CHK_VERSION))
    {
       a_ClearStateBit(g_nSysState, SYS_STAT_CHK_VERSION);
       
       if(Device_Chk_Version())                                            //����Ƿ���Ҫ����
       {
            if(g_sFramBootParamenter.flag == DEVICE_TYPE_SM5001 || g_sFramBootParamenter.flag == DEVICE_TYPE_SM5002 )
            {
              g_sFramBootParamenter.appState = FRAM_BOOT_APP_REPLACE;//�ֿس���洢
            }
       }
       else
       {
            g_sFramBootParamenter.appState = FRAM_BOOT_APP_NULL_REPLACE;
       }
    }
	
	if(g_nBootLimitTime  + SYS_LIMIT_TIME < g_nSysTick)	 
	{

			Device_ClearInfo(); 
			EC20_KeyLow();
			Sys_DataLedOff();
			Sys_Delayms(500);
            Sys_SoftReset(); 															//------��ʱ20�����ҳ����Ѳ��������������³�ʼ�����ظ���
	}
	
}

BOOL Sys_CheckVersion(void)
{
    BOOL b = FALSE;
    u8 *p = (u8 *)SYS_BOOT_VER_ADDR;
    u8 i = 0, c = 0;
        
    if(memcmp(p, SYS_VER_HEAD, SYS_VER_HEAD_SIZE) == 0) //�豸�ͺ���ȷ
    {
        for(i = SYS_VER_HEAD_SIZE; i < SYS_VERSION_SIZE; i++)
        {
            c = *p++;
            if((c < ' ' || c > 127) && (c != 0x00))
            {
                break;
            }
        }
        if(i == SYS_VERSION_SIZE)
        {
            b = TRUE;
        }
    }

    if(b == FALSE)
    {
        if(memcmp((u8 *)SYS_BOOT_HARDTYPE_ADDR, SYS_VER_HARD_TYPE, sizeof(SYS_VER_HARD_TYPE) - 1) == 0) //�豸Ӳ���ͺ���ȷҲ����
        {
            b = TRUE;
        }
    }
    
    return b;
}

void Sys_EC20Task(void)
{
    if(a_CheckStateBit(g_nSysState, SYS_STAT_LTEDTU) || g_sFramBootParamenter.appState == FRAM_BOOT_APP_OK)
    {
        return; //ֻ�в���͸��ģʽ������Ҫ����ATָ�����
    }

    if(USART_GetFlagStatus(EC20_PORT, USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE))
    {
        USART_ClearFlag(EC20_PORT, USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE);
        EC20_InitInterface(EC20_BAUDRARE);
        EC20_ConfigInt();
        EC20_EnableInt(ENABLE, DISABLE);
    }

    if(Uart_IsRcvFrame(g_sEC20RcvFrame))
    {
        if(a_CheckStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_RX))
        {
            if(EC20_ConnectCheckRsp(&g_sEC20Connect, g_sEC20RcvFrame.buffer, g_sEC20RcvFrame.index))   //���У����Ӧ֡ʧ�ܣ��ͼ������գ�����λ���ջ�����
            {
                g_sEC20Connect.result = EC20_CNT_RESULT_OK;
                a_SetState(g_sEC20Connect.state, EC20_CNT_OP_STAT_STEP);
                
                EC20_ClearRxBuffer();
            }
            else
            {
                g_sEC20RcvFrame.state = UART_FLAG_RCV;                          //��������
                g_sEC20RcvFrame.idleTime = 0;
            }
        }
    }
    
    //����
    if(a_CheckStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_TX))      //����ATָ��
    {
        a_ClearStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_TX);

        if(g_sEC20Connect.index < g_sEC20Connect.num)
        {
            EC20_ClearRxBuffer();
            EC20_ConnectTxCmd(&g_sEC20Connect, g_nSysTick);
            a_SetState(g_sEC20Connect.state, EC20_CNT_OP_STAT_RX | EC20_CNT_OP_STAT_WAIT);
        }
    }

    //��ʱ
    if(a_CheckStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_WAIT))    //�жϳ�ʱ��ÿ��ָ��ĳ�ʱʱ�䶼��һ��
    {
        if(g_sEC20Connect.tick + g_sEC20Connect.to[g_sEC20Connect.index] < g_nSysTick)
        {
            g_sEC20Connect.repeat[g_sEC20Connect.index]--;              //��Щָ�������β���
            if(g_sEC20Connect.repeat[g_sEC20Connect.index] == 0)
            {
                a_ClearStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_WAIT);
                a_SetState(g_sEC20Connect.state, EC20_CNT_OP_STAT_STEP);
                g_sEC20Connect.result = EC20_CNT_RESULT_TO;             //��ǰ������ʱ��GPRSִ�����̴���
            }
            else
            {
                g_sEC20Connect.state = EC20_CNT_OP_STAT_TX;             //�ظ�����
            }
        }
    }

    if(a_CheckStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_STEP))    //��һ���߼�����
    {
        a_ClearStateBit(g_sEC20Connect.state, EC20_CNT_OP_STAT_STEP);

        EC20_ConnectStep(&g_sEC20Connect);                              //�������账��
        if(g_sEC20Connect.result == EC20_CNT_RESULT_OK)                 //��������ִ����ɣ����ҳɹ�����ʾ����DTUģʽ
        {
            g_sEC20Connect.state = EC20_CNT_OP_STAT_TX;
            if(g_sEC20Connect.index == g_sEC20Connect.num)              //����ǿ�������ִ����ɣ���ʾ����DTU�����߹ر�
            {
                a_ClearStateBit(g_nSysState, SYS_STAT_HTTP_TEST);
                if(g_sEC20Connect.cmd == EC20_CNT_CMD_PWRON)            
                {
                    g_sEC20Connect.state = EC20_CNT_OP_STAT_DTU;
                    a_SetStateBit(g_nSysState, SYS_STAT_LTEDTU);        //����HTTP�������ɹ�
                    g_sEC20RcvBuffer.state = EC20_CNT_OP_STAT_TEST;
                }
                else if(g_sEC20Connect.cmd == EC20_CNT_CMD_PWROFF) 
                {

                    EC20_KeyLow();//�ػ�ָ��ִ����ɣ��ٴ�ȷ�Ϲر�
                    g_sEC20Connect.state = EC20_CNT_OP_STAT_IDLE;
                }
            }
        }
        else
        {
            if(g_sEC20Connect.cmd == EC20_CNT_CMD_PWRON)                //����ִ��ʧ�ܣ�����رգ����ﲻ��Ҫ�����ˣ���Ϊ�´λ��ǻ�������
            {
                EC20_ConnectInit(&g_sEC20Connect, EC20_CNT_CMD_PWROFF, &g_sEC20Params);
                a_SetState(g_sEC20Connect.state, EC20_CNT_OP_STAT_TX);
            }
            else                                                        //����ػ�ʧ�ܣ���ֱ�ӹرյ�Դ
            {
                g_sEC20Connect.state = EC20_CNT_OP_STAT_IDLE;
                EC20_KeyLow();
            }
        }
    }
   
}

void Sys_ServerTask(void)
{
    if(!a_CheckStateBit(g_nSysState, SYS_STAT_LTEDTU))  //ֻ��͸���ˣ�����Ҫ���������
    {
        return;
    }
    //���ڴ�����:���³�ʼ��


    if(USART_GetFlagStatus(EC20_PORT, USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE))
    {
        USART_ClearFlag(EC20_PORT, USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE);
        EC20_InitInterface(EC20_BAUDRARE);
        EC20_ConfigInt();
        EC20_EnableInt(ENABLE, DISABLE);
    }
    
     if(Uart_IsRcvFrame(g_sEC20RcvFrame))
    {
        
       if(a_CheckStateBit(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_RX))
        {
            if(Device_CommunCheckRsp(&g_sDeviceServerTxBuf, g_sEC20RcvFrame.buffer))   
            {
                g_sDeviceServerTxBuf.result = EC20_CNT_RESULT_OK;
                a_SetState(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_STEP);
            }
            else
            {
                g_sDeviceServerTxBuf.result = EC20_CNT_RESULT_FAIL;
                g_sEC20RcvFrame.state = UART_FLAG_RCV;                          //��������
                g_sEC20RcvFrame.idleTime = 0;
            }
            g_sDeviceServerTxBuf.index++;   
        }
        if(EC20_ConnectCheckClose(g_sEC20RcvFrame.buffer))
        {
            EC20_ConnectInit(&g_sEC20Connect, EC20_CNT_CMD_PWROFF, &g_sEC20Params);
            a_SetState(g_sEC20Connect.state, EC20_CNT_OP_STAT_TX);
            a_ClearStateBit(g_nSysState, SYS_STAT_LTEDTU);
        }
        EC20_ClearRxBuffer();
    }
     
    if(a_CheckStateBit(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_TX))      
    {
        a_ClearStateBit(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_TX);

        if(g_sDeviceServerTxBuf.index < g_sDeviceServerTxBuf.num)
        {
            EC20_ClearRxBuffer();
            Device_CommunTxCmd(&g_sDeviceServerTxBuf, g_nSysTick);
			g_sDeviceUpDataInfo.linkTick ++;
            a_SetState(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_RX | DEVICE_SERVER_TXSTAT_WAIT);
        }
    }
    
    if(a_CheckStateBit(g_sDeviceServerTxBuf.state, DEVICE_SERVER_TXSTAT_WAIT))    
     {
        if(g_sDeviceServerTxBuf.tick + g_sDeviceServerTxBuf.to[g_sDeviceServerTxBuf.index] < g_nSysTick)
        {
            g_sDeviceServerTxBuf.repeat[g_sDeviceServerTxBuf.index - 1]--;              
            if(g_sDeviceServerTxBuf.repeat[g_sDeviceServerTxBuf.index - 1] == 0)
            {
                a_ClearStateBit(g_sDeviceServerTxBuf.state, DEVICE_SERVER_TXSTAT_WAIT);
                a_SetState(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_STEP);
                g_sDeviceServerTxBuf.result = EC20_CNT_RESULT_TO;             
            }
            else
            {
                g_sDeviceServerTxBuf.state = EC20_CNT_OP_STAT_TX;             
            }
        }
    }
   
  
     if(a_CheckStateBit(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_STEP))    //��һ���߼�����
    {
        a_ClearStateBit(g_sDeviceServerTxBuf.state, EC20_CNT_OP_STAT_STEP);
        Device_CommunStep(&g_sDeviceServerTxBuf); 
        if(g_sDeviceUpDataInfo.flag == DEVICE_UPDATA_FLAG_DOWNING)
        {
            if(g_sDeviceServerTxBuf.result == EC20_CNT_RESULT_OK)
            {
                if(Device_WeiteData(&g_sDeviceUpDataInfo))
                {
                    if(g_sDeviceUpDataInfo.step >= g_sDeviceUpDataInfo.num)
                    {
                        g_sDeviceUpDataInfo.flag = DEVICE_UPDATA_FLAG_OVER;
                    }
                    else
                    {
                        g_sDeviceUpDataInfo.flag = DEVICE_UPDATA_FLAG_DOWN;
                        g_sDeviceUpDataInfo.step ++;
                    }
                }
                else
                {
                    g_sDeviceUpDataInfo.flag = DEVICE_UPDATA_FLAG_FAIL;
                    
                    //������;ʧ�ܡ���
                }
                
            }
        }

        if(g_sDeviceServerTxBuf.index >= g_sDeviceServerTxBuf.num)
        {
            g_sDeviceServerTxBuf.num = 0;   
            g_sDeviceServerTxBuf.index = 0;

        }
    }
	
	
	if(g_sDeviceUpDataInfo.linkTick > 20)
	{
		g_sEC20Connect.state = EC20_CNT_OP_STAT_IDLE;
		EC20_KeyLow();
		Sys_Delayms(500);
		Sys_SoftReset();

	}
}

void Sys_DownDataTask()
{   
    static u8 upTime = 0, upTick = 0, infromTick = 0, infromTime = 0; 
    if(a_CheckStateBit(g_nSysState, SYS_STAT_LTEDTU) && a_CheckStateBit(g_nSysState, SYS_STAT_DOWNLOAD)) 
    {
        if(a_CheckStateBit(g_nSysState, SYS_STAT_UPDATA))
        {
            if(upTime >= DEVICE_UPDATA_CHK_TIME)
            {
                upTime = 0;
                if(g_sDeviceUpDataInfo.flag == DEVICE_UPDATA_FLAG_RQ)
                {
                    upTick++;
                    Device_At_Rsp(EC20_CNT_TIME_1S, EC20_CNT_REPAT_NULL, DEVICE_HTTP_GET_REQUEST_CKECK);
                }
                else if(g_sDeviceUpDataInfo.flag == DEVICE_UPDATA_FLAG_DOWN || g_sDeviceUpDataInfo.flag == DEVICE_UPDATA_FLAG_DOWNING )
                {   
                    Sys_DataLedOn();
                    g_sDeviceUpDataInfo.flag = DEVICE_UPDATA_FLAG_DOWNING;
					g_nBootLimitTime = g_nSysTick;
                    Device_At_Rsp(EC20_CNT_TIME_1S * 2, EC20_CNT_REPAT_NULL, DEVICE_HTTP_GET_REQUEST_DOWNLOAD);
                }
                else if(g_sDeviceUpDataInfo.flag == DEVICE_UPDATA_FLAG_FAIL)
                {   
                    upTick ++;     //����һֱʧ������BOOT��
					g_nBootLimitTime = g_nSysTick;
                    g_sFramBootParamenter.appState = FRAM_BOOT_APP_FAIL;
                }
                else if(g_sDeviceUpDataInfo.flag == DEVICE_UPDATA_FLAG_OVER)
                {   
                    Sys_DataLedOff();
					g_nBootLimitTime = g_nSysTick;
                    a_ClearStateBit(g_nSysState, SYS_STAT_DOWNLOAD)  ;
                    g_sFramBootParamenter.appState = FRAM_BOOT_APP_DATA_DOWN_OVER;
                }
                else
                {
                   g_sDeviceUpDataInfo.flag = DEVICE_UPDATA_FLAG_RQ;
                }
            
            }
            else
            {
                upTime ++;
            }
            
            if(upTick >= DEVICE_UPDATA_CHK_TIME)
            {
                upTick = 0;
                g_sFramBootParamenter.appState = FRAM_BOOT_APP_NULL_REPLACE;
            }
            a_ClearStateBit(g_nSysState, SYS_STAT_UPDATA);
        }
    }
    
    
     if(a_CheckStateBit(g_nSysState, SYS_STAT_LTEDTU) && a_CheckStateBit(g_nSysState, SYS_STAT_INFORM_INFO)) 
    {
			if(g_sDeviceUpDataInfo.tid[1] != 0 && g_sDeviceUpDataInfo.tid[0] != 0 && g_sDeviceUpDataInfo.tid[2] != 0 && g_sDeviceUpDataInfo.tid[3] != 0)
			{
				        if(a_CheckStateBit(g_nSysState, SYS_STAT_UPDATA))
				{
					if(infromTick >= DEVICE_UPDATA_CHK_TIME)
					{
						infromTick = 0;
						if(g_sDeviceUpDataInfo.flag == DEVICE_UPDATA_FLAG_OVER || g_sDeviceUpDataInfo.flag == DEVICE_UPDATA_FLAG_NULL )
						{
							infromTime ++;
							Device_At_Rsp(EC20_CNT_TIME_1S, EC20_CNT_REPAT_NULL, DEVICE_HTTP_POST_INFO_VERSION);
						}
						else if(g_sDeviceUpDataInfo.flag == DEVICE_UPDATA_INFORM_OK)
						{
							g_sFramBootParamenter.appState = FRAM_BOOT_APP_OK;
						}
					}
					else
					{
						infromTick ++;
					}
					
					if(infromTime >= DEVICE_UPDATA_CHK_TIME)
					{
						infromTime = 0;
						g_sFramBootParamenter.appState = FRAM_BOOT_APP_OK;
					}
					a_ClearStateBit(g_nSysState, SYS_STAT_UPDATA);
				}
			}
			else
			{
				g_sFramBootParamenter.appState = FRAM_BOOT_APP_DATA_DOWD;
			}

    }

}

void Sys_ReplaceDeviceTask()
{
    static u8 flashTime = 0, sector = 0; 
    u32 addr = 0;
    if(a_CheckStateBit(g_nSysState, SYS_STAT_REPLACE_DATA))  //�̼�������ɣ������������
    {
        if(g_sFramBootParamenter.flag == DEVICE_TYPE_SM5001)
        {
            if(a_CheckStateBit(g_nSysState, SYS_STAT_WR_RE_FLASH))
            {
               a_ClearStateBit(g_nSysState, SYS_STAT_WR_RE_FLASH);
               
               if(flashTime == 2)
               {
                    flashTime = 0;
                    if(g_sDeviceUpDataInfo.state == DEVICE_UPDATA_ERASE_FLASH)
                    {
                        addr = SYS_APP_START_ADDR + (sector << 10);
                        if(addr >= SYS_APP_START_ADDR)
                        {
                            if(g_nDeviceNxtEraseAddr == addr)               //������ַ�����������ģ������������δ����
                            {
                                Sys_DataLedOn();  
                                g_nDeviceNxtEraseAddr = addr + (1 << 10);   //ÿ������1K
                                if(addr <= SYS_APP_START_ADDR + (g_sFramBootParamenter.size + 1) *  (1 << 10))
                                {
                                    if(Uart_EraseFlash(addr))
                                    {
                                        sector++;
										g_nBootLimitTime = g_nSysTick;
										
                                    }
                                    else
                                    {
                                        addr = 0;
                                        sector = 0;
                                        g_nDeviceNxtEraseAddr = 0;
                                        
                                        g_sFramBootParamenter.appState = FRAM_BOOT_APP_FAIL;
                                    }
                                    Sys_DataLedOff();  
                                }
                                else
                                {
                                    addr = 0;
                                    sector = 0;
                                    g_nDeviceNxtEraseAddr = 0;
                                    g_sDeviceUpDataInfo.state = DEVICE_UPDATA_WRITE_FLASH;
                                }
                            }
                        }
                    }
                    else if(g_sDeviceUpDataInfo.state == DEVICE_UPDATA_WRITE_FLASH)
                    {
                        addr = SYS_APP_START_ADDR + sector * FLASH_UPDATA_LEN;
                        if(addr >= SYS_APP_START_ADDR)
                        {
                            if(sector <= g_sFramBootParamenter.size )
                            {
                                Sys_DataLedOn();
                                if(Flash_ReadBuffer(FLASH_DATA_OPEN_ADDR + sector * FLASH_UPDATA_LEN, FLASH_UPDATA_LEN, g_nFlashUpData))
                                {
                                    if(BL_WriteImagePage(addr, g_nFlashUpData, FLASH_UPDATA_LEN))
                                    {
										  sector ++;
										  g_nBootLimitTime = g_nSysTick;
                                    }
                                    else
                                    {
                                        sector = 0;
                                        g_sFramBootParamenter.appState = FRAM_BOOT_APP_FAIL;
                                        //д��ʧ��
                                    }
                                }
                                Sys_DataLedOff();
                            }
                            else
                            {
                                FLASH_Lock();
                                if((g_nFlashUpData[3] << 8 | g_nFlashUpData[2] << 0) == a_GetCrc((u8 *)SYS_APP_START_ADDR, (g_nFlashUpData[1] << 8 | g_nFlashUpData[0] << 0)))
                                {
                                    g_sFramBootParamenter.appState = FRAM_BOOT_APP_REPLACE_OVER;
                                }
                                else
                                {
                                    g_sFramBootParamenter.appState = FRAM_BOOT_APP_FAIL;       //crcУ��ʧ�ܴ���������
                                }
                                a_ClearStateBit(g_nSysState, SYS_STAT_REPLACE_DATA);
                                
                            }
                        }
                        
                    }
                }
                else
                {
                    flashTime++;
                }
                a_ClearStateBit(g_nSysState, SYS_STAT_WR_RE_FLASH);
            }                                  
        }
    }

}

