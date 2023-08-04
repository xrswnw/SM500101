/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c
  * @author  MCD Application Team
  * @version V3.3.0
  * @date    04/16/2010
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"


/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
    g_nSysTick ++;
  //����ʱ�ӵ���������5ms����     

    if((g_nSysTick % 21) == 0)
    {
        a_SetStateBit(g_nSysState, SYS_STAT_RUNLED | SYS_STAT_SENSOR_CHK | SYS_STAT_TEMP_CHK | SYS_STAT_MQTT_HEART | SYS_STAT_AD | SYS_STAT_VOICE_CHK |  SYS_STAT_WATER_CTR | SYS_STAT_TEST_TIM | SYS_STAT_GATE_STAT_CHK);  
        a_SetStateBit(g_sElectInfo.state, ELECT_STAT_TX);
    }
    
    
    Uart_IncIdleTime(STICK_TIME_MS, g_sUartRcvFrame);
    Uart_IncIdleTime(STICK_TIME_MS, g_sW232RcvFrame);
  
}

void SOUND_IRQHandler(void)
{
    if(SOUND_PORT->SR & USART_FLAG_RXNE)
    {
        u8 byte =0;
        
        USART_ClearFlag(SOUND_PORT,USART_FLAG_RXNE);
        byte =  Sound_ReadByte();
        Sound_ReceiveFrame(byte,&g_sSoundInfo);
    }
    else
    {
        Sound_ReadByte();
    }
    SOUND_PORT->SR &= (~0x3FF);
}


 
void RTC_IRQHandler(void)
{

    if(RTC_GetITStatus(RTC_IT_OW)  == RESET)
    {
        RTC_ClearFlag(RTC_IT_OW);     //����RTC������Ҫ����
        //Rtc_UpdateAlarmTime(RTC_ALARM_TIME);
        RTC_SetCounter(0);
        g_sRtcTime.counter ++;
        // Wait for RTC registers synchronization
        RTC_WaitForSynchro();
        // Wait until last write operation on RTC registers has finished
        RTC_WaitForLastTask();
    }
}




u16 g_nElectSr = 0;
u16 g_nElectDr = 0;
void ELECT_IRQHandler(void)
{
    if(ELECT_PORT->SR & USART_FLAG_IDLE)
    {    
        Elect_DisableRxDma();
        g_sElectInfo.rxBuf.len = Elect_GetRxLen(); 
        g_sElectInfo.state = ELECT_STAT_RCV;    //�����������
    }    
    
    g_nElectSr = ELECT_PORT->SR;  //ͨ����ȡSR��DR����жϱ�־
    g_nElectDr = ELECT_PORT->DR;    
}

void ELECT_RxDMAIRQHandler(void)
{
    Elect_DisableRxDma(); //���ջ���������һ�㲻��������������������У��Ϳ���ϵͳ�й���
    g_sElectInfo.rxBuf.len = Elect_GetRxLen(); 
    g_sElectInfo.state = ELECT_STAT_RCV;    //�����������
}

void ELECT_TxDMAIRQHandler(void)
{
    Elect_DisableTxDma();                    //DMA��ɺ����һ���ֽڿ���û�з��ͳ�ȥ����Ҫ����ѭ�������жϴ���
    g_sElectInfo.state = ELECT_STAT_TX_IDLE;
}



u16 g_nWaterSr = 0;
u16 g_nWaterDr = 0;
void Water_IRQHandler(void)
{
    if(WATER_PORT->SR & USART_FLAG_IDLE)
    {    
        Water_DisableRxDma();
        g_sWaterInfo.rxBuf.len = Water_GetRxLen(); 
        g_sWaterInfo.state = WATER_STAT_RCV;    //�����������
    }    
    
    g_nWaterSr = WATER_PORT->SR;  //ͨ����ȡSR��DR����жϱ�־
    g_nWaterDr = WATER_PORT->DR;    
}

void WATER_RxDMAIRQHandler(void)
{
    Water_DisableRxDma(); //���ջ���������һ�㲻��������������������У��Ϳ���ϵͳ�й���
    g_sWaterInfo.rxBuf.len = Water_GetRxLen(); 
    g_sWaterInfo.state = WATER_STAT_RCV;    //�����������
}

void WATER_TxDMAIRQHandler(void)
{
    Water_DisableTxDma();//DMA��ɺ����һ���ֽڿ���û�з��ͳ�ȥ����Ҫ����ѭ�������жϴ���
    Water_EnableRxDma();
    //g_sWaterInfo.state = WATER_STAT_TX_IDLE;
}


 u8 g_nW232RxByte = 0;


void W232_IRQHandler(void)
{
    if(USART_GetITStatus(W232_PORT, USART_IT_RXNE) != RESET)        //reg
    {
        g_nW232RxByte = W232_ReadByte();

        g_sW232RcvFrame.buffer[g_sW232RcvFrame.index++] = g_nW232RxByte;
        if(g_sW232RcvFrame.index < UART_BUFFER_MAX_LEN)
        {
            g_sW232RcvFrame.state |= UART_FLAG_RCV;
        }
        else
        {
           g_sW232RcvFrame.state = UART_STAT_END; 
        }
        g_sW232RcvFrame.idleTime = 0;
    }
    else
    {
        W232_ReadByte();
    }
    W232_PORT->SR &= (~0x3FF);
}

/*
u16 g_nLteSr = 0;
u16 g_nLteDr = 0;
void W232_IRQHandler(void)
{
    if(W232_PORT->SR & USART_IT_RXNE)
    {    
        g_nLteDr = W232_ReadByte();
        g_sW232RcvFrame.buffer[g_sW232RcvFrame.index++] =  g_nLteDr;
        if(g_sW232RcvFrame.index  >= UART_BUFFER_MAX_LEN)
        {
           g_sW232RcvFrame.state |=  UART_FLAG_RCV;     //�����������
        }
        else
        {
            g_sW232RcvFrame.state = UART_STAT_END;
        }
        g_sW232RcvFrame.idleTime = 0;
    }

    g_nLteSr = W232_PORT->SR;                                       //ͨ����ȡSR��DR����жϱ�־
    g_nLteDr = W232_PORT->DR;  
}

          */



void ADC1_2_IRQHandler(void)
{

}

void Gate_RxDMAIRQHandler(void)
{
    Gate_DisableRxDma();                                    //���ջ���������һ�㲻��������������������У��Ϳ���ϵͳ�й���
    g_sGateOpInfo.rxFrame.len = Gate_GetRxLen(); 
    g_sGateOpInfo.rxFrame.state = UART_STAT_RX_OVR;         //�����������
}

void Gate_TxDMAIRQHandler(void)
{
    Gate_DisableTxDma();                                    //DMA��ɺ����һ���ֽڿ���û�з��ͳ�ȥ����Ҫ����ѭ�������жϴ���
    Gate_EnableRxDma();                                     //ʹ�ܽ���
}

u16 g_nGateSr = 0;
u16 g_nGateDr = 0;
void Gate_IRQHandler(void)
{
    if(GATE_PORT->SR & GATE_SR_IDLE)
    {    
        Gate_DisableRxDma();
        g_sGateOpInfo.rxFrame.len = Gate_GetRxLen(); 
        g_sGateOpInfo.rxFrame.state = UART_STAT_RX_END;     //�����������
    }

    g_nGateSr = GATE_PORT->SR;                              //ͨ����ȡSR��DR����жϱ�־
    g_nGateDr = GATE_PORT->DR;    
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */


/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
