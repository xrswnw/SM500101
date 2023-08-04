#include "AnyID_SM5001_Device.h"

const u8 DEVICE_VERSION[DEVICE_VERSION_SIZE]@0x08005000 = "SM5001 23080401 GD322302";


READER_RSPFRAME g_sDeviceRspFrame = {0};
DEVICE_PARAMS g_sDeviceParams = {0};                             		
DEVICE_SENVER_TXBUFFER g_sDeviceServerTxBuf = {0};
DEVICE_IMPRSP_INFO g_sDeviceImpRspInfo = {0};
DEVICE_TEST_INFO g_sDeviceTestInfo = {0};
BOOL g_nBatOpenFlag = TRUE;
u32 g_nBratIngTick = 0;


void Device_Init()
{
    Device_ReadDeviceParamenter();
    Device_ReadMqttKey();
    Device_VoiceCtrFrame(SOUND_CNT_TIME_1S * 4 , SOUND_REPAT_NULL, SOUND_VOICE_CTR_STRENGH, g_sDeviceParams.voiceSth);
    g_sSoundInfo.test = SOUND_VOICE_TEST_FLAG;
    W232_ConnectInit(&g_sW232Connect, W232_CNT_CMD_PWRON, &g_sDeviceParams.serverParams);
    a_SetState(g_sW232Connect.state, W232_CNT_OP_STAT_TX);
    Elect_Init(g_sDeviceParams.electMode);
}



void Reader_Delayms(u32 n)
{
    //72MHZ
    n *= 0x6000;
    n++;
    while(n--);
}


void Device_ReadDeviceParamenter(void)                             		     //OK
{
     BOOL b = FALSE, bBackup = FALSE;
     
    b = FRam_ReadBuffer(FRAME_INFO_ADDR, sizeof(DEVICE_PARAMS), (u8 *)(&g_sDeviceParams));
    if(b)
    {
        u32 crc1 = 0, crc2 = 0;

        crc1 = a_GetCrc((u8 *)(&g_sDeviceParams), (sizeof(DEVICE_PARAMS)) - DEVICE_CRC32_LEN);
        crc2 = g_sDeviceParams.crc;

        //�������Ƿ���ȷ���������ȷ��ʹ��Ĭ�ϲ�������
        if(crc1 != crc2)
        {
            b = FALSE;
        }
    }  
    bBackup = FRam_ReadBuffer(FRAME_INFO_BACKUP_ADDR, sizeof(DEVICE_PARAMS), (u8 *)(&g_sDeviceParams));
    if(bBackup)
    {
        u32 crc1 = 0, crc2 = 0;

        crc1 = a_GetCrc((u8 *)(&g_sDeviceParams), (sizeof(DEVICE_PARAMS)) - DEVICE_CRC32_LEN);
        crc2 = g_sDeviceParams.crc;

        //�������Ƿ���ȷ���������ȷ��ʹ��Ĭ�ϲ�������                        .
        if(crc1 != crc2)
        {
            bBackup = FALSE;
        }
    }
    if((b == FALSE && bBackup == FALSE) || Device_ParmenterChkCode(&g_sDeviceParams))
    {
        memset(&g_sDeviceParams, 0, sizeof(g_sDeviceParams));
        
        g_sDeviceParams.addr = DEVICE_NORMAL_ADDR;
        g_sDeviceParams.voiceSth = SOUND_VOCIE_NORMAL;
        g_sDeviceParams.electMode = ELECT_MODE_645;
        g_sDeviceParams.gateTick = DEVICE_HEART_TIME;
        g_sDeviceParams.gateTxTick =  GATE_OP_TX_TIM;             
        g_sDeviceParams.gateNum = GATE_SLAVER_NUM;
        g_sDeviceParams.gateParams.ledLowVolLev = DEVICE_LED_LOWVOL_DFT;
        g_sDeviceParams.gateParams.alarmTmpr = DEVICE_TMPR_ALARM_DFT;
        g_sDeviceParams.gateParams.chagParams.fulVolLev = CHAG_VOL_LEV_FUL;
        g_sDeviceParams.gateParams.chagParams.lowVolLev = CHAG_VOL_LEV_LOW;
        g_sDeviceParams.gateParams.chagParams.higVolLev = CHAG_VOL_LEV_HIG;
        g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX0].vol = CHAG_VOL_STEP1;
        g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX0].cur = CHAG_CUR_STEP1;
        g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX1].vol = CHAG_VOL_STEP2;
        g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX1].cur = CHAG_CUR_STEP2;
        g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX2].vol = CHAG_VOL_STEP3;
        g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX2].cur = CHAG_CUR_STEP3;

        Device_WriteDeviceParamenter();
    }
    else if(b == TRUE && bBackup == FALSE)
    {
        FRam_ReadBuffer(FRAME_INFO_ADDR, sizeof(DEVICE_PARAMS), (u8 *)(&g_sDeviceParams));
        FRam_WriteBuffer(FRAME_INFO_BACKUP_ADDR, sizeof(DEVICE_PARAMS), (u8 *)(&g_sDeviceParams));
    }
    else if(b == FALSE && bBackup == TRUE)
    {
        FRam_ReadBuffer(FRAME_INFO_BACKUP_ADDR, sizeof(DEVICE_PARAMS), (u8 *)(&g_sDeviceParams));
        FRam_WriteBuffer(FRAME_INFO_ADDR, sizeof(DEVICE_PARAMS), (u8 *)(&g_sDeviceParams));
    }
    
    Fram_ReadBootParamenter();
	if((g_sFramBootParamenter.currentVerSion[0] == 0 && g_sFramBootParamenter.currentVerSion[1] == 0))
	{
		memcpy(g_sFramBootParamenter.currentVerSion, "SM5001_00000000_FFFF", FRAM_VERSION_SIZE);//��ȡ����ʧ�ܣ��ָ�Ĭ��
		Fram_WriteBootParamenter();
			
	}
	if((g_sFramBootParamenter.appState != FRAM_BOOT_APP_OK) ||
       (g_sFramBootParamenter.gateNum != g_sDeviceParams.gateNum) )
    {
		g_sFramBootParamenter.gateNum = g_sDeviceParams.gateNum;
        g_sFramBootParamenter.appState = FRAM_BOOT_APP_OK;
        Fram_WriteBootParamenter();
    }

	

}

BOOL Device_ParmenterChkCode(DEVICE_PARAMS *pPaementInfo)
{
    BOOL bOk = FALSE;
    
    if((pPaementInfo->addr != DEVICE_NORMAL_ADDR) 
       || (pPaementInfo->gateNum == 0) 
       || ((pPaementInfo->gateTick) > DEVICE_NORMAL_HEART_TIME)  
       || (pPaementInfo->voiceSth > SOUND_VOCIE_MAX) 
       || (pPaementInfo->gateParams.alarmTmpr > DEVICE_NORMAL_TEMPR_MAST) 
       || (pPaementInfo->gateParams.chagParams.fulVolLev < DEVICE_LED_LOWVOL_DFT))
        {
            bOk = TRUE;
        }
    
    
    return bOk;
}

BOOL Device_ChkVersion()
{
    BOOL tf = FALSE;
    
    if(memcmp(g_sFramBootParamenter.aimVerSion, DEVICE_VERSION, FRAM_VERSION_SIZE) || g_sFramBootParamenter.appState != FRAM_BOOT_APP_OK)
    {
        tf = TRUE;
    }

    return tf;
}

void Device_ReadMqttKey()                             		     //OK
{
     BOOL b = FALSE, bBackup = FALSE;
    b = FRam_ReadBuffer(FRAME_MQTT_KEY_ADDR, sizeof(MQTT_FRAM_KEY), (u8 *)(&g_sMqttKey));
    if(b)
    {
        u32 crc1 = 0, crc2 = 0;

        crc1 = a_GetCrc((u8 *)(&g_sMqttKey), (sizeof(MQTT_FRAM_KEY)) - DEVICE_CRC32_LEN);
        crc2 = g_sMqttKey.crc;

        //�������Ƿ���ȷ���������ȷ��ʹ��Ĭ�ϲ�������
        if(crc1 != crc2)
        {
            b = FALSE;
        }
    }  
    bBackup = FRam_ReadBuffer(FRAME_MQTT_KEY_BACKUP_ADDR, sizeof(MQTT_FRAM_KEY), (u8 *)(&g_sMqttKey));
    if(bBackup)
    {
        u32 crc1 = 0, crc2 = 0;

        crc1 = a_GetCrc((u8 *)(&g_sMqttKey), (sizeof(MQTT_FRAM_KEY)) - DEVICE_CRC32_LEN);
        crc2 = g_sMqttKey.crc;

        //�������Ƿ���ȷ���������ȷ��ʹ��Ĭ�ϲ�������
        if(crc1 != crc2)
        {
            bBackup = FALSE;
        }
    }
    if(b == FALSE && bBackup == FALSE)
    {
        memset(&g_sMqttKey, 0, sizeof(g_sMqttKey));
        memcpy(g_sMqttKey.keyBuffer, TESTTOKEN, W232_TOKEN_LEN);
        g_sMqttKey.len = W232_TOKEN_LEN;
        
        Device_WriteMqttKey();
    }
    else if(b == TRUE && bBackup == FALSE)
    {
        FRam_ReadBuffer(FRAME_MQTT_KEY_ADDR, sizeof(MQTT_FRAM_KEY), (u8 *)(&g_sMqttKey));
        FRam_WriteBuffer(FRAME_MQTT_KEY_BACKUP_ADDR, sizeof(MQTT_FRAM_KEY), (u8 *)(&g_sMqttKey));
    }
    else if(b == FALSE && bBackup == TRUE)
    {
        FRam_ReadBuffer(FRAME_MQTT_KEY_BACKUP_ADDR, sizeof(MQTT_FRAM_KEY), (u8 *)(&g_sMqttKey));
        FRam_WriteBuffer(FRAME_MQTT_KEY_ADDR, sizeof(MQTT_FRAM_KEY), (u8 *)(&g_sMqttKey));
    }
}


BOOL Device_WriteDeviceParamenter(void)
{
    BOOL b = FALSE;

    g_sDeviceParams.crc = 0;
    g_sDeviceParams.crc = a_GetCrc((u8 *)(&g_sDeviceParams), (sizeof(DEVICE_PARAMS)) - DEVICE_CRC32_LEN);

    b = FRam_WriteBuffer(FRAME_INFO_ADDR, sizeof(DEVICE_PARAMS), (u8 *)(&g_sDeviceParams));
    b = FRam_WriteBuffer(FRAME_INFO_BACKUP_ADDR, sizeof(DEVICE_PARAMS), (u8 *)(&g_sDeviceParams));
    
    return b;
}


BOOL Device_WriteMqttKey()
{
    BOOL b = FALSE;

    g_sMqttKey.crc = 0;
    g_sMqttKey.crc = a_GetCrc((u8 *)(&g_sMqttKey), (sizeof(MQTT_FRAM_KEY)) - DEVICE_CRC32_LEN);

    b = FRam_WriteBuffer(FRAME_MQTT_KEY_ADDR, sizeof(MQTT_FRAM_KEY), (u8 *)(&g_sMqttKey));
    b = FRam_WriteBuffer(FRAME_MQTT_KEY_BACKUP_ADDR, sizeof(MQTT_FRAM_KEY), (u8 *)(&g_sMqttKey));
    
    return b;
}



u16 Device_ResponseFrame(u8 *pParam, u8 len, READER_RSPFRAME *pOpResult)
{
    u16 pos = 0;
    u16 crc = 0;
    
    pOpResult->buffer[pos++] = UART_FRAME_RESPONSE_FLAG; 

    if(len > UART_FRAME_PARAM_MAX_LEN)
    {
        pOpResult->buffer[pos++] = (len >> 0) & 0xFF;
        pOpResult->buffer[pos++] = (len >> 8) & 0xFF;

        memcpy(pOpResult->buffer + pos, pParam, len);
        pos += len;
    }
    else
    {
        if(pOpResult->flag == DEVICE_RSPFRAME_FLAG_OK)
        {
            memcpy(pOpResult->buffer + pos, pParam, len);
        }
        else
        {
            memset(pOpResult->buffer + pos, 0, len);
        }
        pos += len;
        pOpResult->buffer[pos++] = pOpResult->err;
        pOpResult->buffer[pos++] = UART_FRAME_PARAM_RFU;
        pOpResult->buffer[pos++] = UART_FRAME_PARAM_RFU;
        
    }

    crc = a_GetCrc(pOpResult->buffer, pos); //��LEN��ʼ����crc
    pOpResult->buffer[pos++] = crc & 0xFF;
    pOpResult->buffer[pos++] = (crc >> 8) & 0xFF;

    pOpResult->len = pos;

    return pos;
}

u16 Device_ResponseRtBat(u8 *pBuffer)
{
    u16 pos = 0;
    u16 crc = 0;
    
    pBuffer[pos++] = UART_FRAME_RESPONSE_FLAG; 
    pBuffer[pos++] = g_sGateOpInfo.rtnBat.step;
    pBuffer[pos++] = g_sGateOpInfo.rtnBat.flag;
    pBuffer[pos++] = UART_FRAME_PARAM_RFU;
    pBuffer[pos++] = UART_FRAME_PARAM_RFU;
    crc = a_GetCrc(pBuffer, pos); //��LEN��ʼ����crc
    pBuffer[pos++] = crc & 0xFF;
    pBuffer[pos++] = (crc >> 8) & 0xFF;

    return pos;
}

u16 Device_ResponseBrBat(u8 *pBuffer)
{
    u16 pos = 0;
    u16 crc = 0;
    
    pBuffer[pos++] = UART_FRAME_RESPONSE_FLAG; 
    pBuffer[pos++] = g_sGateOpInfo.brwBat.step;
    pBuffer[pos++] = g_sGateOpInfo.brwBat.flag;
    pBuffer[pos++] = UART_FRAME_PARAM_RFU;
    pBuffer[pos++] = UART_FRAME_PARAM_RFU;
    crc = a_GetCrc(pBuffer, pos); //��LEN��ʼ����crc
    pBuffer[pos++] = crc & 0xFF;
    pBuffer[pos++] = (crc >> 8) & 0xFF;

    return pos;
}

u16 Device_ResponseInfoChg(u8 *pBuffer, u8 add, u8 gateState, u8 batState, u8 chagState, u32 mainState)
{
    u16 pos = 0;
    u16 crc = 0;
    
    pBuffer[pos++] = UART_FRAME_RESPONSE_FLAG;
    if(add == DEVICE_SM5001_ID)
    {
        pBuffer[pos++] = (mainState >> 24 ) & 0xFF; 
        pBuffer[pos++] = (mainState >> 16 ) & 0xFF;
        pBuffer[pos++] = (mainState >> 8 ) & 0xFF;
        pBuffer[pos++] = (mainState >> 0 ) & 0xFF;
    
    }
    else
    {
        pBuffer[pos++] = UART_FRAME_PARAM_RFU; 
        pBuffer[pos++] = gateState;
        pBuffer[pos++] = batState;
        pBuffer[pos++] = chagState;
    }
    pBuffer[pos++] = UART_FRAME_PARAM_RFU;
    pBuffer[pos++] = UART_FRAME_PARAM_RFU;
    crc = a_GetCrc(pBuffer, pos); //��LEN��ʼ����crc
    pBuffer[pos++] = crc & 0xFF;
    pBuffer[pos++] = (crc >> 8) & 0xFF;
    
    return pos;
}

u16 Device_ResponseBat(READER_RSPFRAME *pOpResult, u8 mode)
{
    u16 pos = 0;
    u16 crc = 0;
    
    pOpResult->buffer[pos++] = UART_FRAME_RESPONSE_FLAG; 
    if(mode == GATE_FRAME_CMD_BRWBAT)
    {
        pOpResult->buffer[pos++] = g_sGateOpInfo.brwBat.step;
        pOpResult->buffer[pos++] = g_sGateOpInfo.brwBat.flag;
    }
    else if(mode == GATE_FRAME_CMD_RTNBAT)
    {
        pOpResult->buffer[pos++] = g_sGateOpInfo.rtnBat.step;
        pOpResult->buffer[pos++] = g_sGateOpInfo.rtnBat.flag;
    }
    pOpResult->buffer[pos++] = UART_FRAME_PARAM_RFU;
    pOpResult->buffer[pos++] = UART_FRAME_PARAM_RFU;
    crc = a_GetCrc(pOpResult->buffer, pos); //��LEN��ʼ����crc
    pOpResult->buffer[pos++] = crc & 0xFF;
    pOpResult->buffer[pos++] = (crc >> 8) & 0xFF;

    pOpResult->len = pos;

    return pos;
}

u16 Device_GateResponse(READER_RSPFRAME *pOpResult, u8 *pParam, u8 len)
{
    u16 pos = 0;
    u16 crc = 0;
    
    pOpResult->buffer[pos++] = UART_FRAME_RESPONSE_FLAG;   
    memcpy(pOpResult->buffer + pos, pParam, len);
    pos += len;
    pOpResult->buffer[pos++] = UART_FRAME_PARAM_RFU;
    pOpResult->buffer[pos++] = UART_FRAME_PARAM_RFU;
    crc = a_GetCrc(pOpResult->buffer, pos); //��LEN��ʼ����crc
    pOpResult->buffer[pos++] = crc & 0xFF;
    pOpResult->buffer[pos++] = (crc >> 8) & 0xFF;

    pOpResult->len = pos;

    return pos;
}


u16 Device_ResponseCfg( READER_RSPFRAME *pOpResult)
{
    u16 pos = 0;
    u16 crc = 0;
    
    pOpResult->buffer[pos++] = UART_FRAME_RESPONSE_FLAG; 
    pOpResult->buffer[pos++] = g_sDeviceParams.gateTick;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateTxTick;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateNum;
    pOpResult->buffer[pos++] = g_sDeviceParams.rfu1;
    pOpResult->buffer[pos++] = g_sDeviceParams.rfu2 >> 8;
    pOpResult->buffer[pos++] = g_sDeviceParams.rfu2 >> 0;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.ledLowVolLev >> 8;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.ledLowVolLev >> 0;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.alarmTmpr >> 8;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.alarmTmpr >> 0;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.fulVolLev >> 8;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.fulVolLev >> 0;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.lowVolLev >> 8;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.lowVolLev >> 0;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.higVolLev >> 8;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.higVolLev >> 0;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX0].vol >> 8;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX0].vol >> 0;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX0].cur  >> 8;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX0].cur  >> 0;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX1].vol >> 8;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX1].vol >> 0;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX1].cur >> 8;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX1].cur >> 0;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX2].vol >> 8;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX2].vol >> 0;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX2].cur >> 8;
    pOpResult->buffer[pos++] = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX2].cur >> 0;
    pOpResult->buffer[pos++] = pOpResult->err;
    pOpResult->buffer[pos++] = UART_FRAME_PARAM_RFU;
    pOpResult->buffer[pos++] = UART_FRAME_PARAM_RFU;
    crc = a_GetCrc(pOpResult->buffer, pos); //��LEN��ʼ����crc
    pOpResult->buffer[pos++] = crc & 0xFF;
    pOpResult->buffer[pos++] = (crc >> 8) & 0xFF;

    pOpResult->len = pos;

    return pos;
}



//�汾��Ϣ�ϱ�
u16 Device_ResponseVersion( READER_RSPFRAME *pOpResult)
{
    u16 pos = 0;
    u16 crc = 0;
    
    pOpResult->buffer[pos++] = UART_FRAME_RESPONSE_FLAG; 
    
    memcpy(pOpResult->buffer + pos, g_sFramBootParamenter.currentVerSion, FRAM_VERSION_SIZE);
    pos += FRAM_VERSION_SIZE;
    pOpResult->buffer[pos++] = pOpResult->err;
    pOpResult->buffer[pos++] = UART_FRAME_PARAM_RFU;
    pOpResult->buffer[pos++] = UART_FRAME_PARAM_RFU;
    crc = a_GetCrc(pOpResult->buffer, pos); //��LEN��ʼ����crc
    pOpResult->buffer[pos++] = crc & 0xFF;
    pOpResult->buffer[pos++] = (crc >> 8) & 0xFF;

    pOpResult->len = pos;

    return pos;
}



u16 Device_ResponseGateFrame(u8 add, u8 mode, READER_RSPFRAME *pOpResult)
{
    u16 pos = 0;
    u16 crc = 0;
    u8 index = 0;
    pOpResult->buffer[pos++] = UART_FRAME_RESPONSE_FLAG;
	
	
    if(mode == DEVICE_GET_GATEINFO_MODE_ONCE)
    {
          if(add <=(GATE_SLAVER_NUM << 1))
          {
             pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.tmpr;
             pOpResult->buffer[pos++] = (g_aGateSlvInfo[add].sensorInfo.sensorState.fan << 3) | 
                             		    (g_aGateSlvInfo[add].sensorInfo.sensorState.smoke << 2) | 
                             		    (g_aGateSlvInfo[add].sensorInfo.sensorState.rfid << 1) | 
                             		    (g_aGateSlvInfo[add].sensorInfo.sensorState.door << 0);
            
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.state >> 0;

            memcpy(pOpResult->buffer + pos, g_aGateSlvInfo[add].sensorInfo.batInfo.sn, BAT_SN_LEN);
            pos +=  BAT_SN_LEN ;
			
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.volValue >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.volValue >> 0;
			pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.itemNum >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.itemNum >> 0;
			pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.volLev >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.volLev >> 0;
			
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.remainCap >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.remainCap >> 0;

            
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.sohValue >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.sohValue >> 0;
            
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.chagCur >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.chagCur >> 0;
            
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.envTmpr >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.envTmpr >> 0;
            
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.itemMinTmpr >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.itemMinTmpr >> 0;
            
			pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.mosTmpr >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.mosTmpr >> 0;
            
			pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.itemMaxTmpr >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.itemMaxTmpr >> 0;

            
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.err.alarm0 >> 0;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.err.alarm1 >> 0;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.err.alarm2 >> 0;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.err.alarm3 >> 0;  
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.err.alarm4 >> 0;
			pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.err.alarm5 >> 0;
			pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.err.alarm6 >> 0;
             

            
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.state >> 0;
            memcpy(pOpResult->buffer + pos, g_aGateSlvInfo[add].sensorInfo.chagInfo.vendorName,CHAG_VENDOR_NAME_LEN);
            pos += CHAG_VENDOR_NAME_LEN;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.softVer >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.softVer >> 0;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.hardVer >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.hardVer >> 0;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.type >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.type >> 0;            
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.pwr >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.pwr >> 0;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.maxVol >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.maxVol >> 0;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.maxCur >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.maxCur >> 0;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.chagVol >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.chagVol >> 0;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.chagCur >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.chagCur >> 0;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.errStatusInfo >> 8;
            pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.chagInfo.errStatusInfo >> 0;
          }
    
    }
    else
    {
        for(index = 0 ;index < (GATE_SLAVER_NUM << 1); index ++)
        {
         	if(g_aGateSlvInfo[index].state == GATE_STAT_OK)
          	{
				  pOpResult->buffer[pos++] = index + 1;
				  pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.tmpr >> 0;
				  pOpResult->buffer[pos++] = (g_aGateSlvInfo[index].sensorInfo.sensorState.fan << 3) |
										   (g_aGateSlvInfo[index].sensorInfo.sensorState.smoke << 2) | 
											(g_aGateSlvInfo[index].sensorInfo.sensorState.rfid << 1) | 
											  (g_aGateSlvInfo[index].sensorInfo.sensorState.door << 0);
				  pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.batInfo.state >> 0;
				  memcpy(pOpResult->buffer + pos, g_aGateSlvInfo[index].sensorInfo.batInfo.sn, BAT_SN_LEN);
				  pos +=  BAT_SN_LEN ;

				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.volValue >> 8;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.volValue >> 0;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.itemNum >> 8;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.itemNum >> 0;
				  
				  pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.batInfo.status.volLev >> 8;
				  pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.batInfo.status.volLev >> 0;
					
				  pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.batInfo.status.remainCap >> 8;
				  pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.batInfo.status.remainCap >> 0;

					
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.sohValue >> 8;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.sohValue >> 0;
				  
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.chagCur >> 8;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.chagCur >> 0;
					
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.envTmpr >> 8;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.envTmpr >> 0;
					
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.itemMinTmpr >> 8;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.itemMinTmpr >> 0;
					
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.mosTmpr >> 8;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.mosTmpr >> 0;
					
				  
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.itemMaxTmpr >> 8;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[add].sensorInfo.batInfo.status.itemMaxTmpr >> 0;
				  pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.state >> 0;
				  pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.pwr >> 8;
				  pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.pwr >> 0;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.maxVol >> 8;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.maxVol >> 0;
				 // pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.maxCur >> 8;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.maxCur >> 0;
				 // pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.chagVol >> 8;
				 // pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.chagVol >> 0;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.chagCur >> 8;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.chagCur >> 0;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.errStatusInfo >> 8;
				  //pOpResult->buffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.errStatusInfo >> 0;
			  }
			  else
			  {
					pOpResult->buffer[pos++] = index + 1;
					memset(pOpResult->buffer + pos,0xFF, DEVICE_GATE_STAT_INFO + BAT_SN_LEN);
					pos += (DEVICE_GATE_STAT_INFO + BAT_SN_LEN);
			  }
        }
    }
      
    pOpResult->buffer[pos++] = UART_FRAME_PARAM_RFU;
    pOpResult->buffer[pos++] = UART_FRAME_PARAM_RFU;
    crc = a_GetCrc(pOpResult->buffer, pos); //��LEN��ʼ����crc
    pOpResult->buffer[pos++] = crc & 0xFF;
    pOpResult->buffer[pos++] = (crc >> 8) & 0xFF;

    pOpResult->len = pos;

     return pos;
}


void Device_GateRsvSlvInfo(u8 *pParams, u16 paramsLen, GATE_OPINFO *pOpInfo)
{
    u16 pos = 0;
    u8 num = 0, i = 0;
    u8 infoIdx = 0;
    GATE_SENSORINFO *pInfo = NULL, *pPreInfo = NULL;
    
    num = pParams[pos++];
    for(i = 0; i < num; i++)
    {
        pInfo = (GATE_SENSORINFO *)(pParams + pos);
        infoIdx = (pOpInfo->slvIndex << 1) + pInfo->addr;
        pPreInfo = &g_aGateSlvInfo[infoIdx].sensorInfo;
        if(g_aGateSlvInfo[infoIdx].state != GATE_STAT_OK)
        {
            g_aGateSlvInfo[infoIdx].state = GATE_STAT_OK;
            g_aGateSlvInfo[infoIdx].bTxInfo = TRUE;
        }
        else 
        {
            if((memcmp(&pInfo->sensorState, &pPreInfo->sensorState, 1) != 0) ||
               (pInfo->batInfo.state != pPreInfo->batInfo.state) || 
               (pInfo->chagInfo.state != pPreInfo->chagInfo.state))     //״̬�����仯
            {
                g_aGateSlvInfo[infoIdx].bTxInfo = TRUE;//
            }
            else
            {
                if(g_aGateSlvInfo[infoIdx].txTick + g_sDeviceParams.gateTxTick < g_nSysTick)
                {
                    g_aGateSlvInfo[infoIdx].bTxInfo = TRUE;
                }
            }
        }
        memcpy(pPreInfo, pInfo, sizeof(GATE_SENSORINFO));

        pos += sizeof(GATE_SENSORINFO);
    }
    g_sGateOpInfo.comErr[g_sGateOpInfo.slvIndex] = 0;
}



BOOL Device_GateProceRspFrame(u8 *pFrame, GATE_OPINFO *pOpInfo, u32 tick)
{
    BOOL b = FALSE;
    u8 *pParams = NULL;
    u16 paramsLen = 0;
    u16 addr = 0;
     
    u16 batAddr = 0;
    
    
    batAddr  = ((g_sDeviceImpRspInfo.add -1)>> 1) + 1;
    addr = *((u16 *)(pFrame + UART_FRAME_POS_SRCADDR));
    if((addr == pOpInfo->slvIndex + 1 && pFrame[UART_RFRAME_POS_CMD] == pOpInfo->cmd) || (batAddr == addr && pFrame[UART_RFRAME_POS_CMD] == GATE_FRAME_CMD_RTNBAT)
       || (batAddr == addr && pFrame[UART_RFRAME_POS_CMD] == GATE_FRAME_CMD_BRWBAT))    //�軹���ָ���Ƿ����֣�
    {
        b = TRUE;
        if(pFrame[UART_FRAME_POS_LEN] == 0)
        {
            pParams = pFrame + UART_RFRAME_POS_PAR + 2;
            paramsLen = *((u16 *)(pFrame + UART_RFRAME_POS_PAR));
        }
        else
        {
            pParams = pFrame + UART_RFRAME_POS_PAR;
            paramsLen = pFrame[UART_FRAME_POS_LEN] + 3 - UART_RFRAME_MIN_LEN;
        }
        
        if(pFrame[UART_RFRAME_POS_CMD] == GATE_FRAME_CMD_RTNBAT)                        //���������ʱ����
        {
          pOpInfo->cmd = GATE_FRAME_CMD_RTNBAT   ;
        }
        else if(pFrame[UART_RFRAME_POS_CMD] == GATE_FRAME_CMD_BRWBAT)
        {
          pOpInfo->cmd =  GATE_FRAME_CMD_BRWBAT;
        }
        
        switch(pOpInfo->cmd)
        {

            case GATE_FRAME_CMD_GET_ININFO:
                
                Device_GateRsvSlvInfo(pParams, paramsLen, pOpInfo);
            break;
            case GATE_FRAME_CMD_SET_OUTINFO:
              
              a_ClearStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_GATE);
              if(!a_CheckStateBit(g_sDeviceTestInfo.flag, GATE_FLAG_DOOR_TEST))      //�ſ����������ϱ�
              {
                  Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_CMD);
              }
            break;
            case GATE_FRAME_CMD_CHARGE:
              
                a_ClearStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_GATE);
                pOpInfo->slvCmd.cmd = 0;            //������ɣ���ղ���
                Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_CMD);
            break;
            case GATE_FRAME_CMD_RTNBAT:
            if(paramsLen == 2)
            {
				if(a_CheckStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_REBAT))
				{  
					pOpInfo->rtnBat.step = pParams[0];     
					pOpInfo->rtnBat.flag = pParams[1]; 
					a_ClearStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_REBAT);
						   
					if(g_nBatOpenFlag)
					{
						g_nBatOpenFlag = FALSE;
						pOpInfo->batOpState = GATE_OP_BAT_STAT_OPEN;
					}
					g_sDeviceRspFrame.len = Device_ResponseBat(&g_sDeviceRspFrame, GATE_FRAME_CMD_RTNBAT);
					Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_CMD);
				}
				else
				{
					g_nBratIngTick =   g_nSysTick;
					if(pParams[0] == 0x00)
					{
						b = TRUE;       //����Ҫ�ϱ�����

						pOpInfo->rtnBat.tick = g_nSysTick;
						g_nBatOpenFlag = TRUE;
						pOpInfo->rtnBat.step  = pParams[0];     
						pOpInfo->rtnBat.flag = pParams[1]; 
						pOpInfo->batOpState = GATE_OP_BAT_STAT_OVER;
						g_sDeviceImpRspInfo.rtuLen = Device_ResponseRtBat(g_sDeviceImpRspInfo.rtuBuffer);
						Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_REBAT);
								 
						Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_REBAT);
						Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_REBAT);      //--��ֹ���ݶ�ʧ���ϱ�����
						pOpInfo->slvCmd.cmd = 0;            //������ɣ���ղ���
					
					}
					else
					{
						if((pOpInfo->rtnBat.step != pParams[0]) || (pOpInfo->rtnBat.flag != pParams[1]))
						{
							pOpInfo->rtnBat.tick = g_nSysTick;
							pOpInfo->rtnBat.step  = pParams[0];     
							pOpInfo->rtnBat.flag = pParams[1];
							if(g_nBatOpenFlag)
							{
								g_nBatOpenFlag = FALSE;
								pOpInfo->batOpState = GATE_OP_BAT_STAT_OPEN;
							}
							g_sDeviceImpRspInfo.rtuLen = Device_ResponseRtBat(g_sDeviceImpRspInfo.rtuBuffer);
							Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_REBAT);
						}
						b = FALSE;
						pOpInfo->tick = g_nSysTick;
					}
				
				}
				g_sGateOpInfo.batInfoRepat = 0 ;
				Device_CtrBatVolce(g_sDeviceImpRspInfo.add - 1, DEVICE_BAT_RTN, pOpInfo->rtnBat.step, pOpInfo->rtnBat.flag)  ;      
            }
            else
            {
                b = FALSE;
                pOpInfo->tick = g_nSysTick;
            }
            break;
            case GATE_FRAME_CMD_GET_VERSON:
                
            break;
            case GATE_FRAME_CMD_BRWBAT:
            if(paramsLen == 2)
            {
                    if(a_CheckStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_BWBAT))
                    {  
                         pOpInfo->brwBat.step  = pParams[0];     
                         pOpInfo->brwBat.flag = pParams[1];
                         a_ClearStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_BWBAT);
                         if(g_nBatOpenFlag)
                         {
                          	g_nBatOpenFlag = FALSE;
                          	pOpInfo->batOpState = GATE_OP_BAT_STAT_OPEN;
                         }
                         g_sDeviceRspFrame.len = Device_ResponseBat(&g_sDeviceRspFrame, GATE_FRAME_CMD_BRWBAT);
                         Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_CMD);
                    }
                    else
                    {
						g_nBratIngTick = g_nSysTick;
						if(pParams[0] == 0x00)
						{
							b = TRUE;       //����Ҫ�ϱ�����
							pOpInfo->slvCmd.cmd = 0;            //������ɣ���ղ���
							g_nBatOpenFlag = TRUE;
							pOpInfo->brwBat.step  = pParams[0];     
							pOpInfo->brwBat.flag = pParams[1];
							pOpInfo->brwBat.tick = g_nSysTick;
							pOpInfo->batOpState = GATE_OP_BAT_STAT_OVER;
							g_sDeviceImpRspInfo.brwLen = Device_ResponseBrBat(g_sDeviceImpRspInfo.brwBuffer);
							Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_BWBAT);
								   
							Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_BWBAT);
							Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_BWBAT);//--
							 
						  }
						  else
						  {
							   if((pOpInfo->brwBat.step != pParams[0]) || (pOpInfo->brwBat.flag != pParams[1]))
							   {
									pOpInfo->brwBat.tick = g_nSysTick;
									pOpInfo->brwBat.step  = pParams[0];     
									pOpInfo->brwBat.flag = pParams[1];
									if(g_nBatOpenFlag)
									{
										g_nBatOpenFlag = FALSE;
										pOpInfo->batOpState = GATE_OP_BAT_STAT_OPEN;
									}
									g_sDeviceImpRspInfo.brwLen = Device_ResponseBrBat(g_sDeviceImpRspInfo.brwBuffer);
									Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_BWBAT);
							   }
							   b = FALSE;
							   pOpInfo->tick = g_nSysTick;

						  }
						  
                    }
                    g_sGateOpInfo.batInfoRepat = 0;
                    Device_CtrBatVolce(g_sDeviceImpRspInfo.add - 1, DEVICE_BAT_BRW, pOpInfo->brwBat.step, pOpInfo->brwBat.flag);
            }
            else
            {
                b = FALSE;
                pOpInfo->tick = g_nSysTick;
            }

            break;
            case GATE_FRAME_CMD_SET_PARAMS:   //  10
              if(paramsLen > 0)
              {
                    //�汾��Ϣ��һ��ֿؿ��������֣��汾��Ϣһ��
                    memcpy(g_aGateSlvInfo[2 * (addr - 1)].softWare, pParams, GATE_VERSION_LEN);
                    memcpy(g_aGateSlvInfo[2 * (addr - 1) + 1].softWare, pParams, GATE_VERSION_LEN);
                    memcpy(g_aGateSlvInfo[2 * (addr - 1)].hardWare, pParams + GATE_VERSION_LEN + 1 , GATE_VERSION_LEN);
                    memcpy(g_aGateSlvInfo[2 * (addr - 1) + 1].hardWare, pParams + GATE_VERSION_LEN + 1, GATE_VERSION_LEN);

              } 
            break;
            case GATE_FRAME_CMD_GET_PARAMS:
            break;
          case GATE_FRAME_CMD_PLANE_BAT:
                
                a_ClearStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_GATE);
                g_sDeviceRspFrame.len = Device_GateResponse(&g_sDeviceRspFrame, pParams, 1);
                Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_CMD);
          break;
        }
    }
    return b;
}

void  Device_GateBatTwice(u8 cmd, u8 addr)
{

    g_sGateOpInfo.slvIndex = (g_sDeviceImpRspInfo.add -1)>> 1;
    g_sGateOpInfo.state = GATE_OP_STAT_WAIT;
    g_sGateOpInfo.mode = GATE_MODE_CMD;
    g_sGateOpInfo.cmd = cmd;
    g_sGateOpInfo.slvCmd.params[0] = (addr )% 2;
    g_sGateOpInfo.slvCmd.paramsLen = 1;
}

u16 Device_WaterProceRspFrame(u8 *pFrame, WATER_INFO *pOpInfo, u8 len)
{
    u8 cmd = 0;
    u8 paramsLen = 0;

    cmd = *(pFrame + UART_FRAME_POS_CMD + 1);
    pOpInfo->txBuf.cmd = cmd;
    pOpInfo->txBuf.flag = UART_FRAME_FLAG_OK;
    pOpInfo->txBuf.err = UART_FRAME_RSP_NOERR;
    pOpInfo->txBuf.len = 0;
    paramsLen = len - UART_FRAME_MIN_LEN;
    switch(cmd)
    {
        case WATER_CMD_GET_UID:
            if(paramsLen > 0)
            {
				if(memcmp(pOpInfo->smapleUid, pFrame + 20, WATER_UID_LEN))         //���� 
                {
                    memcpy(pOpInfo->smapleUid, pFrame + 20, WATER_UID_LEN);
					pOpInfo->smapleTick = 0;
                }
				else
				{
					pOpInfo->smapleTick++;
				}
				
				if(pOpInfo->smapleTick >= WATER_SAMPLE_TIME)
				{
					if(memcmp(pOpInfo->uid, pOpInfo->smapleUid, WATER_UID_LEN))
					{
						memcpy(pOpInfo->uid, pOpInfo->smapleUid, WATER_UID_LEN);
					}
					pOpInfo->smapleTick = 0;
				}
				
                g_sWaterInfo.txBuf.repat[WATER_STAT_MODE_RFID] = 0;
                g_sWaterInfo.txBuf.result[WATER_STAT_MODE_RFID] = TRUE;
            }
            break;
          case DEVICE_CMD_MQTT_GET_IMEI:
            if(paramsLen > 0)
            {
                //Water_WriteStr((char *)g_nImsiStr);
            }
            break; 
            
    }
    if(pOpInfo->txBuf.len == 0)
    {
        pOpInfo->txBuf.flag = UART_FRAME_FLAG_FAIL;
        pOpInfo->txBuf.err = UART_FRAME_RSP_CRCERR;

    }
    return pOpInfo->txBuf.len;
}





u16 Reader_ProcessUartFrame(u8 *pFrame, u8 add, u16 len, u32 tick)
{ 
    u8 cmd = 0;
    BOOL bRfOperation = FALSE;
    u16 paramsLen = 0;
    g_sDeviceRspFrame.len = 0;
    
    memset(&g_sDeviceRspFrame, 0, sizeof(READER_RSPFRAME));
    cmd = g_sW232RcvBuffer.cmd;
    if(cmd == UART_FRAME_RESPONSE_FLAG)
    {
        cmd = *(pFrame + UART_FRAME_POS_MQTT_CMD + 1);
    }

    g_sDeviceRspFrame.cmd = cmd;
    g_sDeviceRspFrame.flag = UART_FRAME_FLAG_OK;
    g_sDeviceRspFrame.err = UART_FRAME_RSP_NOERR;
   

    paramsLen = len - UART_FRAME_MIN_MQTT_LEN;

        switch(cmd)
        {
            case DEVICE_CMD_RESET:
                if(paramsLen == 0)
                {
                    if(add == DEVICE_SM5001_ID)
                    {
                         g_sDeviceRspFrame.flag = DEVICE_RESPONSE_FLAG_RESET;
                    }
                    else
                    {
                         g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                    }
                    g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                }
                break;
            case DEVICE_CMD_VERSION_UPDATA:
                if(paramsLen == FRAM_VERSION_SIZE + 2)
                {
                    if(add == DEVICE_SM5001_ID)
                    {
                         memcpy(g_sFramBootParamenter.aimVerSion, pFrame, FRAM_VERSION_SIZE);
						 //chk_SM5001_XXXXXXXX_1000/0100;

						 if(!memcmp(pFrame, "SM5001_", 7) && (!memcmp(pFrame + 15, "_1000", 5) || !memcmp(pFrame + 15, "_0100", 5)))     //��ֻʶ�����زֿظ��°�
						 {
							 if(memcmp(g_sFramBootParamenter.aimVerSion, g_sFramBootParamenter.currentVerSion, FRAM_VERSION_SIZE))
							 {
                                     g_sDeviceRspFrame.flag = DEVICE_RESPONSE_FLAG_UPDATA;//DEVICE_RESPONSE_FLAG_UPDATA;
                                     Fram_WriteBootParamenter();
							 }
							 else
							 {
							 	g_sDeviceRspFrame.err = READER_RESPONSE_ERR_FREAM;
							 }
						 }
						 else
						 {
						 	g_sDeviceRspFrame.err = READER_RESPONSE_ERR_FREAM;
						 }

                    }
                    else
                    {
                         g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                    }
                    g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                }
                break;
            case DEVICE_CMD_GET_VERSION:
                if(paramsLen == 20)
                {
                    g_sDeviceRspFrame.len = Device_ResponseFrame((u8 *)DEVICE_VERSION, DEVICE_VERSION_SIZE, &g_sDeviceRspFrame);
                }
                break;
            case DEVICE_CMD_GET_CPUID:
                if(paramsLen == 0)
                {
                    g_sDeviceRspFrame.len = Device_ResponseFrame((u8 *)STM32_CPUID_ADDR, STM32_CPUID_LEN, &g_sDeviceRspFrame);
                }
                break;
            case DEVICE_CMD_GATE_VOICE_CTR:
                if(paramsLen == 5)
                {
                  if(add == DEVICE_SM5001_ID)
                  {
                         if(*(pFrame + 0) <= SOUND_VOCIE_MAX)
                         {
                            Device_VoiceCtrFrame(SOUND_CNT_TIME_1S * 2 , SOUND_REPAT_NULL, SOUND_VOICE_CTR_STRENGH, *(pFrame + 0));
                            g_sSoundInfo.test = SOUND_VOICE_TEST_FLAG;   //�����洢���ϵ����ã� ������ʾ
                            g_sDeviceParams.voiceSth = *(pFrame + 0);
                             Device_WriteDeviceParamenter();
                            
                         }
                         else
                         {
                             g_sDeviceRspFrame.err = READER_RESPONSE_ERR_FREAM;
                         }
                  }
                  else
                  {
                     g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                  }

                    g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                }
                break;
            case  DEVICE_CMD_GET_CFG:
                if(paramsLen == 0)
                {
                    if(add == DEVICE_SM5001_ID)
                    {
                         g_sDeviceRspFrame.len = Device_ResponseCfg(&g_sDeviceRspFrame);

                    }
                    else
                    {
                         g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                         g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                    }
                }
                break;
            case  DEVICE_CMD_GATE_DISENABLE:
                if(paramsLen == 0)
                {
                    if(add == DEVICE_SM5001_ID)
                    {
                         g_sDeviceTestInfo.flag = GATE_FLAG_DOOR_TEST ;
                         Device_VoiceApoFrame(SOUND_CNT_TIME_1S * 2, SOUND_REPAT_NULL, SOUND_VOICE_DI, SOUND_VOC_DI);

                    }
                    else
                    {
                         g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                    }
                    g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                }
                break;
            case DEVICE_CMD_SET_CFG:
                if(paramsLen == DEVICE_SET_CFG_FRAME_LEN + 4)
                {
                    if(add == DEVICE_SM5001_ID)
                    {
                         Device_SetCfg(pFrame);
                         
                    }
                    else
                    {
                         g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                    }
                }
                g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                break;
            case DEVICE_CMD_CTR_LED :
              if(paramsLen == DEVICE_ACTCTL_CTL_FRAME_LEN + 1)
                {
                    if(add == DEVICE_SM5001_ID)
                    {
                         if(*(pFrame + 0) == DEVICE_MASK_FLAG_MAST)
                         {
                             g_sIoInfo.maskFlag |= IO_DEVICE_MASK_FLAG_LED;
                             if(*(pFrame + 1) & DEVICE_IO_DEVICE_OPEN)
                             {
                             		IO_Led_Open();
                             		a_SetStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_LED); 
                             		
                             }
                             else 
                             {
                             		IO_Led_Close();
                             		a_ClearStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_LED); 
                             }
                         }
                         else if(*(pFrame + 0) == DEVICE_MASK_FLAG_NULL)
                         {
                             g_sIoInfo.maskFlag &= ~IO_DEVICE_MASK_FLAG_LED;
                             if(*(pFrame + 1) & DEVICE_IO_DEVICE_OPEN)
                             {
                             		IO_Led_Open();
                             		a_SetStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_LED); 
                             		
                             }
                             else 
                             {
                             		IO_Led_Close();
                             		a_ClearStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_LED); 
                             }
                         }
                         else
						 {
                           g_sDeviceRspFrame.err = READER_RESPONSE_ERR_FREAM;
                         }
                    }
                    else
                    {
                         g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                    }  
                    g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                }
          
            break;
             case DEVICE_CMD_CTR_RELAY :
                if(paramsLen == DEVICE_ACTCTL_CTL_FRAME_LEN + 1)
                {
                    if(add == DEVICE_SM5001_ID)
                    {
                         if(*(pFrame + 0) == DEVICE_MASK_FLAG_MAST)
                         {
                             g_sIoInfo.maskFlag |= IO_DEVICE_MASK_FLAG_RELAY;
                             if(*(pFrame + 1) & DEVICE_IO_DEVICE_OPEN)
                             {
                             		IO_Realy_Open();
                             		a_SetStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_RELAY); 
                             		
                             }
                             else 
                             {
                                    IO_Realy_Close();
                             		a_ClearStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_RELAY); 
                             }
                         }
                         else if(*(pFrame + 0) == DEVICE_MASK_FLAG_NULL)
                         {
                             g_sIoInfo.maskFlag &= ~IO_DEVICE_MASK_FLAG_RELAY;
                              if(*(pFrame + 1) & DEVICE_IO_DEVICE_OPEN)
                             {
                             		IO_Realy_Open();
                             		a_SetStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_RELAY); 
                             		
                             }
                             else 
                             {
                                    IO_Realy_Close();
                             		a_ClearStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_RELAY); 
                             }
                         }
                         else 
						 {
								g_sDeviceRspFrame.err = READER_RESPONSE_ERR_FREAM;
                         }
                    }
                    else
                    {
                         g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                    }  
                    g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                }
          
            break;
              case DEVICE_CMD_CTR_FAN :
                if(paramsLen == DEVICE_ACTCTL_CTL_FRAME_LEN + 1)
                {
                    if(add == DEVICE_SM5001_ID)
                    {
                         if(*(pFrame + 0) == DEVICE_MASK_FLAG_MAST)
                         {
                             g_sIoInfo.maskFlag |= IO_DEVICE_MASK_FLAG_FAN;
                             if(*(pFrame + 1) & DEVICE_IO_DEVICE_OPEN)
                             {
                                   IO_Fan_Open();
                                   a_SetStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_FAN); 
                              }
                              else 
                              {
                                   IO_Fan_Close();
                                   a_ClearStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_FAN); 
                              }
                         }
                         else if(*(pFrame + 0) == DEVICE_MASK_FLAG_NULL)
                         {
                             g_sIoInfo.maskFlag &= ~IO_DEVICE_MASK_FLAG_FAN;
                             if(*(pFrame + 1) & DEVICE_IO_DEVICE_OPEN)
                             {
                                   IO_Fan_Open();
                                   a_SetStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_FAN); 
                              }
                              else 
                              {
                                   IO_Fan_Close();
                                   a_ClearStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_FAN); 
                              }
                         }
                         else 
						 {
                         
                           g_sDeviceRspFrame.err = READER_RESPONSE_ERR_FREAM;
                         }
                    }
                    else
                    {
                         g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                    }  
                    g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                }
          
            break;
               case DEVICE_CMD_CTR_DOOR :
                if(paramsLen == DEVICE_ACTCTL_CTL_FRAME_LEN + 1)
                {
                    if(add == DEVICE_SM5001_ID)
                    {
                         if(*(pFrame + 0) == DEVICE_MASK_FLAG_MAST)
                         {
                             g_sIoInfo.maskFlag |= IO_DEVICE_MASK_FLAG_DOOR;
                             if(*(pFrame + 1) & DEVICE_IO_DEVICE_OPEN)
                             {
                             		IO_Door_Open();
                             		g_sIoInfo.state |= IO_DEVICE_STAT_DOOR;
                             		g_sIoInfo.ctrlDoorTick = g_nSysTick;
                             }
                         }
                         else if(*(pFrame + 0) == DEVICE_MASK_FLAG_NULL)
                         {
                             g_sIoInfo.maskFlag &= ~IO_DEVICE_MASK_FLAG_DOOR;
                             if(*(pFrame + 1) & DEVICE_IO_DEVICE_OPEN)
                             {
                             		IO_Door_Open();
                             		g_sIoInfo.state |= IO_DEVICE_STAT_DOOR;
                             		g_sIoInfo.ctrlDoorTick = g_nSysTick;
                             }
                         }
                         else 
						 {
                             g_sDeviceRspFrame.err = READER_RESPONSE_ERR_FREAM;
                         }
                    }
                    else
                    {
                         g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                    }  
                    g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                }
          
            break;
            case GATE_FRAME_CMD_SET_OUTINFO:
                if(paramsLen == DEVICE_ACTCTL_CTL_FRAME_LEN)
                {
                    a_SetStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_GATE);

                    if(Device_ChkGate(add))
                    {
                         if(*(pFrame + 0) & DEVICE_OUT_CTRL_POS_CHANG || *(pFrame + 0) & DEVICE_OUT_CTRL_POS_DOOR || *(pFrame + 0) == DEVICE_OUT_NULL)
                         {
                             Device_AnsyFrame(add, GATE_FRAME_CMD_SET_OUTINFO, *(pFrame + 0));
                             Gate_TxFrame(&g_sGateOpInfo, tick);
                         }
                         else
                         {
                             Device_AnsyFrame(add, GATE_FRAME_CMD_SET_OUTINFO, *(pFrame + 0));
                             Gate_TxFrame(&g_sGateOpInfo, tick);
                             g_sDeviceRspFrame.err = READER_RESPONSE_ERR_FREAM;
                         
                         }
                    }
                    else
                    {
                         Device_AnsyFrame(add, GATE_FRAME_CMD_SET_OUTINFO, *(pFrame + 0));
                         Gate_TxFrame(&g_sGateOpInfo, tick);
                         g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                         
                    }  
                    g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                }
                break;
                
            case GATE_FRAME_CMD_CHARGE:
                if(paramsLen == DEVICE_ACTCTL_CTL_FRAME_LEN)
                {
                    a_SetStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_GATE);
                    if(Device_ChkGate(add))
                    {
                         if(*(pFrame + 0) & DEVICE_OUT_CTRL_POS_CHANG || *(pFrame + 0) & DEVICE_OUT_CTRL_POS_DOOR || *(pFrame + 0) == DEVICE_OUT_NULL)
                         {
                             Device_AnsyFrame(add, GATE_FRAME_CMD_CHARGE, *(pFrame + 0));
                             Gate_TxFrame(&g_sGateOpInfo, tick);
                             
                         }
                         else
                         {
                             Device_AnsyFrame(add, GATE_FRAME_CMD_CHARGE, *(pFrame + 0));
                             Gate_TxFrame(&g_sGateOpInfo, tick);
                             g_sDeviceRspFrame.err = READER_RESPONSE_ERR_FREAM;
                         }
                    }
                    else
                    {
                         Device_AnsyFrame(add, GATE_FRAME_CMD_CHARGE, *(pFrame + 0));
                         Gate_TxFrame(&g_sGateOpInfo, tick);
                         g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                         
                    }  
                    g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                }
                break;
    
          case GATE_FRAME_CMD_RTNBAT:
                if(paramsLen == DEVICE_RETURN_BAT_FRAME_LEN)
                {

                    a_SetStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_REBAT);
					
                    if(Device_ChkGate(add))
                    {
						g_sDeviceImpRspInfo.add = add;
						Device_GateBatCtr(add, GATE_FRAME_CMD_RTNBAT, Device_GetBatSnLen(pFrame));     //����У��
						Gate_TxFrame(&g_sGateOpInfo, tick);
                    }
                    else
                    {
                           g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                    }  

                    g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                }
                break;
        case GATE_FRAME_CMD_BRWBAT:
                if(paramsLen == DEVICE_RETURN_BAT_FRAME_LEN)
                {
                    a_SetStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_BWBAT);
					
                    if(Device_ChkGate(add))
                    {
						g_sDeviceImpRspInfo.add = add;
						Device_GateBatCtr(add, GATE_FRAME_CMD_BRWBAT, Device_GetBatSnLen(pFrame));
						Gate_TxFrame(&g_sGateOpInfo, tick);
                    }
                    else
                    {
                           g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                    }
                    g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                }
                break;
          case GATE_FRAME_CMD_PLANE_BAT:
                if(paramsLen == DEVICE_RETURN_BAT_FRAME_LEN + 1)
                {
                    a_SetStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_GATE);
                    
                    if(Device_ChkGate(add))
                    {
                         if(g_sGateOpInfo.batOpState != GATE_OP_BAT_STAT_ING)            //�軹��ع����п�ԤԼ��أ�����
                         {
                           Device_GatePlBat(add, *(pFrame + 1), Device_GetBatSnLen(pFrame));
                           Gate_TxFrame(&g_sGateOpInfo, tick);
                         }
                    }
                    else
                    {                             		                             		            
                         Gate_TxFrame(&g_sGateOpInfo, tick);
                         g_sDeviceRspFrame.err = READER_RESPONSE_ERR_DEVICE;
                         
                    }  
                    g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                }
                break;
          case GATE_FRAME_CMD_GET_ININFO:
                if(paramsLen == DEVICE_ACTCTL_CTL_FRAME_LEN)
                {
                    if( *(pFrame + 0) == DEVICE_GET_GATEINFO_MODE_ONCE || *(pFrame + 0) == DEVICE_GET_GATEINFO_MODE_ALL)
                    {
                         g_sDeviceRspFrame.len = Device_ResponseGateFrame(add -1 , *(pFrame + 0) ,&g_sDeviceRspFrame);
                    }
                    else
                    {
                         g_sDeviceRspFrame.err = READER_RESPONSE_ERR_FREAM;
                         g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
                    }
                }
                break;
            case DEVICE_CMD_GATE_KEY_CTR:
                if(paramsLen == DEVICE_KEY_UID_LEN)
                {
					memcpy(&g_sMqttKey.uid, pFrame , FRAME_UID_LEN);
					g_sDeviceRspFrame.err = READER_RESPONSE_NOERR;
					Device_WriteMqttKey();
                }
				else
				{
					g_sDeviceRspFrame.err = READER_RESPONSE_ERR_LEN;	
				}
				g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);  
                break;

        }
     if(g_sDeviceRspFrame.len == 0 && bRfOperation == FALSE)
    {
        g_sDeviceRspFrame.flag = DEVICE_RSPFRAME_FLAG_FAIL;
        g_sDeviceRspFrame.err = DEVICE_OPTAG_RESPONSE_PARERR;
        g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
    }
 
    return g_sDeviceRspFrame.len ;
}





BOOL Device_SetCfg(u8 *pBuffer)
{
    BOOL bOk = TRUE;
    u8 pos = 0;
  
    g_sDeviceParams.gateTick = (*(pBuffer + pos++) & 0xFF);
    g_sDeviceParams.gateTxTick = (*(pBuffer + pos++) & 0xFF);
    g_sDeviceParams.gateNum = (*(pBuffer + pos++) & 0xFF);
    g_sDeviceParams.rfu1 = (*(pBuffer + pos++) & 0xFF);
    g_sDeviceParams.rfu2 = ((*(pBuffer + pos++) & 0xFF) << 8);
    g_sDeviceParams.rfu2 = g_sDeviceParams.rfu2 | ((*(pBuffer + pos++) & 0xFF) << 0);
    g_sDeviceParams.gateParams.ledLowVolLev = ((*(pBuffer + pos++) & 0xFF) << 8);
    g_sDeviceParams.gateParams.ledLowVolLev = g_sDeviceParams.gateParams.ledLowVolLev | ((*(pBuffer + pos++) & 0xFF) << 0);
    g_sDeviceParams.gateParams.alarmTmpr = ((*(pBuffer + pos++) & 0xFF) << 8);
    g_sDeviceParams.gateParams.alarmTmpr = g_sDeviceParams.gateParams.alarmTmpr | ((*(pBuffer + pos++) & 0xFF) << 0);
    g_sDeviceParams.gateParams.chagParams.fulVolLev = ((*(pBuffer + pos++) & 0xFF) << 8);
    g_sDeviceParams.gateParams.chagParams.fulVolLev = g_sDeviceParams.gateParams.chagParams.fulVolLev | ((*(pBuffer + pos++) & 0xFF) << 0);
    g_sDeviceParams.gateParams.chagParams.lowVolLev = ((*(pBuffer + pos++) & 0xFF) << 8);
    g_sDeviceParams.gateParams.chagParams.lowVolLev = g_sDeviceParams.gateParams.chagParams.lowVolLev | ((*(pBuffer + pos++) & 0xFF) << 0);
    g_sDeviceParams.gateParams.chagParams.higVolLev = ((*(pBuffer + pos++) & 0xFF) << 8);
    g_sDeviceParams.gateParams.chagParams.higVolLev = g_sDeviceParams.gateParams.chagParams.higVolLev | ((*(pBuffer + pos++) & 0xFF) << 0);
    g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX0].vol = ((*(pBuffer + pos++) & 0xFF) << 8);
    g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX0].vol = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX0].vol | ((*(pBuffer + pos++) & 0xFF) << 0);
    g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX0].cur = ((*(pBuffer + pos++) & 0xFF) << 8);
    g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX0].cur = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX0].cur | ((*(pBuffer + pos++) & 0xFF) << 0);
    g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX1].vol = ((*(pBuffer + pos++) & 0xFF) << 8);
    g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX1].vol = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX1].vol | ((*(pBuffer + pos++) & 0xFF) << 0);
    g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX1].cur = ((*(pBuffer + pos++) & 0xFF) << 8);
    g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX1].cur = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX1].cur | ((*(pBuffer + pos++) & 0xFF) << 0);
    g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX2].vol = ((*(pBuffer + pos++) & 0xFF) << 8);
    g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX2].vol = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX2].vol | ((*(pBuffer + pos++) & 0xFF) << 0);
    g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX2].cur = ((*(pBuffer + pos++) & 0xFF) << 8);
    g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX2].cur = g_sDeviceParams.gateParams.chagParams.stepParams[CHAG_STEP_IDX2].cur | ((*(pBuffer + pos++) & 0xFF) << 0);

    Device_WriteDeviceParamenter();

    return bOk;

}

void Device_ServerProcessRxInfo(W232_RCVBUFFER *pRcvBuffer, u32 tick)               //����������·�����
{

    u16 crc1 = 0, crc2 = 0;
     u16 txLen = 0; 
    if(!a_CheckStateBit(g_sDeviceServerTxBuf.state, DEVICE_SERVER_TXSTAT_RX_AT) && !a_CheckStateBit(g_sDeviceServerTxBuf.state, DEVICE_SERVER_TXSTAT_WAIT))
    { 
		if(pRcvBuffer->len >= DEVICE_FREAM_MIN_LEN && strlen(pRcvBuffer->bufferStr) > 0)
		{
			crc1 = Uart_GetFrameCrc(pRcvBuffer->buffer, pRcvBuffer->len);
			crc2 = a_GetCrc(pRcvBuffer->buffer, pRcvBuffer->len - 2);

		}
		else if(pRcvBuffer->len == 0)    //����Ϊ0����У�飿����
		{
		   crc1 = crc2 = 0xFF;
		}
		else
		{
			crc1 = 0xFF;
		}
		
		if(crc1 == crc2)
		{
			txLen = Reader_ProcessUartFrame(pRcvBuffer->buffer, pRcvBuffer->addr, pRcvBuffer->len, tick);
			if(txLen > 0)
			{
				if(!a_CheckStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_GATE) && !a_CheckStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_BWBAT) && !a_CheckStateBit(g_sDeviceRspFrame.mark, DEVIDE_MARK_REBAT))
				{
					Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_CMD);          
				}
			}
		}
		else
		{  
			g_sDeviceRspFrame.err = READER_RESPONSE_ERR_LEN;
			g_sDeviceRspFrame.len = Device_ResponseFrame(NULL, 0, &g_sDeviceRspFrame);
			Device_AtRsp(W232_CNT_TIME_500MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_CMD);

		}
	}
}



void Device_UpHeartData(u32 id, u8 *pBuffer, char *strAtBuff, char *strRspBuff, u16 lenth)
{
    char strRspbuffer[W232_STR_BUFFER_HEART_LEN] ={0};  
    a_Hex2Str(strRspbuffer, pBuffer, lenth);

    sprintf(strRspBuff,"{\"id\":%.8d,\"dp\":{\"device\":[{\"v\":\"FF20%.4d%s\"}]}}",id, lenth, strRspbuffer);
    sprintf(strAtBuff,"AT+QMTPUBEX=0,0,0,0,\"$sys/%.6s/%.15s/dp/post/json\",%d",W232_PRDOCT_ID, g_nImsiStr, strlen(strRspBuff));
}


u16 Device_HeartFormat(u8 *pBuffer, u32 tick)
{
    u16 pos = 0;
    u16 crc = 0;
    u8 index = 0;

    pBuffer[pos++] = 0x1F;
    pBuffer[pos++] = (tick >> 24) & 0xFF;
    pBuffer[pos++] = (tick >> 16) & 0xFF;
    pBuffer[pos++] = (tick >>  8) & 0xFF;
    pBuffer[pos++] = (tick >>  0) & 0xFF;
  //������Ϣ      
    pBuffer[pos++] = 0xFF;
    pBuffer[pos++] = g_nImei[0];
    pBuffer[pos++] = g_nImei[1];
    pBuffer[pos++] = g_nImei[2];
    pBuffer[pos++] = g_nImei[3];
    pBuffer[pos++] = g_nImei[4];
    pBuffer[pos++] = g_nImei[5];
    pBuffer[pos++] = g_nImei[6];
    pBuffer[pos++] = (g_nImei[7] & 0xF0);
         
    memcpy(pBuffer + pos, (u8 *)&DEVICE_VERSION[DEVICE_VERSION_LEN - 1], DEVICE_VERSION_LEN);  //Ӳ���汾
	pos += DEVICE_VERSION_LEN ;
    memcpy(pBuffer + pos, (u8 *)&DEVICE_VERSION, DEVICE_VERSION_LEN - 2);						//����汾
	pos += (DEVICE_VERSION_LEN - 2);
	pBuffer[pos++] = 0x30;
	pBuffer[pos++] = 0x31;					//  ����汾��Ϣ��0
    
    pBuffer[pos++] = (g_sElectInfo.electValue >> 24) & 0xFF;
    pBuffer[pos++] = (g_sElectInfo.electValue >> 16) & 0xFF;
    pBuffer[pos++] = (g_sElectInfo.electValue >>  8) & 0xFF;
    pBuffer[pos++] = (g_sElectInfo.electValue >>  0) & 0xFF;
    pBuffer[pos++] = (g_sDeviceParams.temprUp.t >> 0) & 0xFF;
    pBuffer[pos++] = (g_sDeviceParams.temprDown.t >> 0) & 0xFF;

    pBuffer[pos++] = (g_sIoInfo.sersorState & 0x1F) | 
                     (Device_ChkDeviceStat(IO_DEVICE_STAT_FAN) << 5) | 
                     (Device_ChkDeviceStat(IO_DEVICE_STAT_LED) << 6) | 
                     (Device_ChkDeviceStat(IO_DEVICE_STAT_RELAY) << 7);
    //������Ϣ
    pBuffer[pos++] = 0x00;
    pBuffer[pos++] = 0x00;
    pBuffer[pos++] = 0x00;
    pBuffer[pos++] = 0x00;
    pBuffer[pos++] = 0x00;
    
  //������Ϣ
    for(index = 0 ;index < (GATE_SLAVER_NUM << 1); index ++)
    {
        pBuffer[pos++] = index + 1;
        if(g_aGateSlvInfo[index].state == GATE_STAT_OK)
        {
            memcpy(pBuffer + pos, g_aGateSlvInfo[index].softWare, GATE_VERSION_LEN);
            pos += GATE_VERSION_LEN;
            memcpy(pBuffer + pos, g_aGateSlvInfo[index].hardWare, GATE_VERSION_LEN);
            pos += GATE_VERSION_LEN;
            pBuffer[pos++] = g_aGateSlvInfo[index].sensorInfo.tmpr;
            pBuffer[pos++] = (g_aGateSlvInfo[index].sensorInfo.sensorState.fan << 3) | (g_aGateSlvInfo[index].sensorInfo.sensorState.smoke << 2) | 
                             (g_aGateSlvInfo[index].sensorInfo.sensorState.rfid << 1) | (g_aGateSlvInfo[index].sensorInfo.sensorState.door << 0);
            pBuffer[pos++] = g_aGateSlvInfo[index].sensorInfo.batInfo.state;
            pBuffer[pos++] = g_aGateSlvInfo[index].sensorInfo.batInfo.status.volLev;
            pBuffer[pos++] = g_aGateSlvInfo[index].sensorInfo.chagInfo.state;
        }
        else
        {
            memset(pBuffer + pos, 0xFF, GATE_VERSION_LEN + GATE_VERSION_LEN + 1 + 1 + 1 + 1 + 1);
            pos += GATE_VERSION_LEN + GATE_VERSION_LEN + 1 + 1 + 1 + 1 + 1;
        }
    }
    
    pBuffer[pos++] = g_sW232Connect.sigNum;//�ź�ǿ��
    pBuffer[pos++] = UART_FRAME_PARAM_RFU;
    crc = a_GetCrc(pBuffer, pos); //��LEN��ʼ����crc
    pBuffer[pos++] = crc & 0xFF;
    pBuffer[pos++] = (crc >> 8) & 0xFF;

   return pos;
}



void Device_IoCtr()
{
    static BOOL fireFlag = TRUE;
    //if(!g_nMasterFlag)
    {
          if(a_CheckStateBit(g_sIoInfo.sersorState, IO_SENSOR_STAT_TEMPR_CHANGE) || a_CheckStateBit(g_sIoInfo.sersorState, IO_SENSOR_STAT_SMOKE))
          { 
             if(!(g_sIoInfo.maskFlag & IO_DEVICE_MASK_FLAG_FAN)) 
             {
                if(!a_CheckStateBit(g_sIoInfo.deviceState, IO_DEVICE_STAT_FAN))
                 {
                     IO_Fan_Open();
                    a_SetStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_FAN);   
                 }
             }
          } 
          else
          {
              if(!(g_sIoInfo.maskFlag & IO_DEVICE_MASK_FLAG_FAN)) 
             {
                if(a_CheckStateBit(g_sIoInfo.deviceState, IO_DEVICE_STAT_FAN))
               {
                  IO_Fan_Close();
                  a_ClearStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_FAN);
               }
             }
          }
            
          if((a_CheckStateBit(g_sIoInfo.sersorState, IO_SENSOR_STAT_WATER) || a_CheckStateBit(g_sIoInfo.sersorState, IO_SENSOR_STAT_SMOKE)))
          {
            if(fireFlag)
            {
              if(a_CheckStateBit(g_sIoInfo.sersorState, IO_SENSOR_STAT_SMOKE))
              {
                  fireFlag = FALSE;
              }
            }
             if(!(g_sIoInfo.maskFlag & IO_DEVICE_MASK_FLAG_RELAY)) 
             {
                if(a_CheckStateBit(g_sIoInfo.deviceState, IO_DEVICE_STAT_RELAY))
                {
                    IO_Realy_Close();    //���Թر�
                    a_ClearStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_RELAY); 
                }
              }
          } 
          else
          {
              if(!(g_sIoInfo.maskFlag & IO_DEVICE_MASK_FLAG_RELAY)) 
             {
                if(fireFlag)
                {
                   if(!a_CheckStateBit(g_sIoInfo.deviceState, IO_DEVICE_STAT_RELAY))
                   {
                      IO_Realy_Open();
                      a_SetStateBit(g_sIoInfo.deviceState,IO_DEVICE_STAT_RELAY);
                   }
                }
             }
          }
    }



}

void Device_CtrBatVolce(u16 add, u8 mode, u8 step, u8 flag)
{

  if(mode == DEVICE_BAT_RTN)
  {
      if(step == DEVICE_STEP_OVER)
      {
          switch(flag)
          {
            case DEVICE_STEP_FLAG_OK:
				if(g_sGateOpInfo.batAddr != GATE_BAT_OK)
				{
					g_sGateOpInfo.batAddr = GATE_BAT_OK;
                    Device_VoiceApoFrame(SOUND_CNT_TIME_1S * 8, SOUND_REPAT_NULL, SOUND_VOICE_RTU_BAT_OK, SOUND_VOC_RETURN_OK );
				}  
					//��������
              break;
            case DEVICE_STEP_FLAG_DOOR_FAIL:
              
              break;
            case DEVICE_STEP_FLAG_BAT_NO_RTN:
              
                    Device_VoiceApoFrame(SOUND_CNT_TIME_1S * 8, SOUND_REPAT_NULL, SOUND_VOICE_NO_BAT, SOUND_VOC_RETURN_FAIL);
              break;
            case DEVICE_STEP_FLAG_BAT_FRAME_ERR:
              
              break;
          }
        
      }
      else if(step == DEVICE_STEP_ONE)
      {
          switch(flag)
          {
            case DEVICE_STEP_FLAG_DOOR_OK:
              
              Device_VoiceApoFrame(SOUND_CNT_TIME_1S * 8, SOUND_REPAT_NULL, SOUND_VOICE_RTU_BAT, g_sGateOpInfo.add + 1);
              break;
            case DEVICE_STEP_FLAG_DOOR_LOADING:
              
              break;
          }
      }
      else if(step == DEVICE_STEP_TWICE)  
      {
          switch(flag)
          {
            case DEVICE_STEP_FLAG_DOOR_NOCLOSE_BAT_INSER:

              if(g_sGateOpInfo.batAddr != GATE_BAT_OUT)
              {
                g_sGateOpInfo.batAddr = GATE_BAT_OUT;
                //Device_VoiceApoFrame(SOUND_CNT_TIME_1S , SOUND_REPAT_NULL, SOUND_VOICE_DI, SOUND_VOC_DI);
              }
              break;
            case DEVICE_STEP_FLAG_DOOR_CLOSE_BAT_NOINSER:
              if(g_sGateOpInfo.batAddr != GATE_BAT_NULL)
              {
                   g_sGateOpInfo.batAddr = GATE_BAT_NULL;
              }
              break;
            case DEVICE_STEP_FLAG_DOOR_CLOSE_BAT_FAIL:
             
              if(g_sGateOpInfo.batAddr != GATE_BAT_FAIL)
              {
                g_sGateOpInfo.batAddr = GATE_BAT_FAIL;
                Device_VoiceApoFrame(SOUND_CNT_TIME_1S * 8, SOUND_REPAT_NULL, SOUND_VOICE_BAT_SN_FAIL, SOUND_VOC_BATTER_CODE_FAIL);
              }
              break;
          case DEVICE_STEP_FLAG_DOOR_CLOSE_BAT_LOADING:
              if(g_sGateOpInfo.batAddr != GATE_BAT_NULL)
              {
                   g_sGateOpInfo.batAddr = GATE_BAT_NULL;
              }
              break;
          }
      }
  }
  else if(mode == DEVICE_BAT_BRW)
  {
       if(step == DEVICE_STEP_OVER)
      {
          switch(flag)
          {
            case DEVICE_STEP_FLAG_OK:
				if(g_sGateOpInfo.batAddr != GATE_BAT_OK)
				{
					g_sGateOpInfo.batAddr = GATE_BAT_OK;
					Device_VoiceApoFrame(SOUND_CNT_TIME_1S , SOUND_REPAT_NULL, SOUND_VOICE_DI, SOUND_VOC_DI);
				} //�������
              break;
            case DEVICE_STEP_FLAG_DOOR_FAIL:
              
              break;
            case DEVICE_STEP_FLAG_BAT_NO_RTN:
              //Device_VoiceApoFrame(SOUND_CNT_TIME_1S * 2, SOUND_REPAT_NULL, SOUND_VOICE_DI, SOUND_VOC_DI);
              
              break;
            case DEVICE_STEP_FLAG_BAT_FRAME_ERR:
              
              break;
          }
        
      }
      else if(step == DEVICE_STEP_ONE)
      {
          switch(flag)
          {
            case DEVICE_STEP_FLAG_DOOR_OK:
				
              	Device_VoiceApoFrame(SOUND_CNT_TIME_1S * 8, SOUND_REPAT_NULL, SOUND_VOICE_BRW_BAT, g_sGateOpInfo.add + 1);
			  break;
            case DEVICE_STEP_FLAG_DOOR_LOADING:
              
              break;
          }
      }
      else if(step == DEVICE_STEP_TWICE)  
      {
          switch(flag)
          {
            case DEVICE_STEP_FLAG_DOOR_NOCLOSE_BAT_INSER:
              
              if(g_sGateOpInfo.batAddr != GATE_BAT_IN)
              {
                g_sGateOpInfo.batAddr = GATE_BAT_IN;
                //Device_VoiceApoFrame(SOUND_CNT_TIME_1S * 2, SOUND_REPAT_NULL, SOUND_VOICE_DI, SOUND_VOC_DI);
              }
              break;
            case DEVICE_STEP_FLAG_DOOR_CLOSE_BAT_NOINSER:
              if(g_sGateOpInfo.batAddr != GATE_BAT_NULL)
              {
                   g_sGateOpInfo.batAddr = GATE_BAT_NULL;
              }
             
              break;
            case DEVICE_STEP_FLAG_DOOR_CLOSE_BAT_FAIL:
              
              break;
          case DEVICE_STEP_FLAG_DOOR_CLOSE_BAT_LOADING:
              if(g_sGateOpInfo.batAddr != GATE_BAT_NULL)
              {
                   g_sGateOpInfo.batAddr = GATE_BAT_NULL;
              }
              break;
          }
      }
  
  }

}

char g_aStrBuffRequst[W232_STR_BUFFER_RSP_LEN] = {0};
char g_aStrBuffJsonAccept[W232_STR_BUFFER_RSP_LEN] = {0};
char g_aStrBuffJsonReject[W232_STR_BUFFER_RSP_LEN] = {0};
char g_aStrBuffcmdAccept[W232_STR_BUFFER_RSP_LEN] = {0};
char g_aStrBuffcmdOta[W232_STR_BUFFER_RSP_LEN] = {0};

BOOL Device_CheckRsp(W232_CONNECT *pCntOp, u8 *pRxBuf, u8 len)
{
    BOOL bOK = FALSE;
	
    sprintf(g_aStrBuffRequst,"cmd/request/");
    sprintf(g_aStrBuffJsonAccept,"dp/post/json/accepted");
    sprintf(g_aStrBuffJsonReject,"dp/post/json/rejected");
    sprintf(g_aStrBuffcmdAccept,"cmd/response/%.36s/accepted",pCntOp->requestId);
    sprintf(g_aStrBuffcmdOta,"ota/inform ");
  
    if(strstr((char const *)pRxBuf, g_aStrBuffRequst) != NULL)
    {
        memcpy(pCntOp->requestId, pRxBuf + W232_RQUEST_ID_POS, W232_RQUEST_ID_LEN);

        if((*(pCntOp->requestId +  DEVICE_MQTT_FRAME_CMDID_TAG_1) == DEVICE_MQTT_FRAME_CMDID_MASK) &&        //����������֡�����򱻷������߳�
           (*(pCntOp->requestId +  DEVICE_MQTT_FRAME_CMDID_TAG_2) == DEVICE_MQTT_FRAME_CMDID_MASK) && 
           (*(pCntOp->requestId +  DEVICE_MQTT_FRAME_CMDID_TAG_3) == DEVICE_MQTT_FRAME_CMDID_MASK) &&
           (*(pCntOp->requestId +  DEVICE_MQTT_FRAME_CMDID_TAG_4) == DEVICE_MQTT_FRAME_CMDID_MASK))
        {
              g_sW232RcvBuffer.flag = W232_RESPONES_CMD_GET;
              memcpy(pCntOp->requestBuffer, pRxBuf + W232_RQUEST_ID_POS + W232_RQUEST_ID_LEN + 3, W232_RQUEST_BUFFER_LEN);   //������У��
             if(W232_DataHandle(&g_sW232RcvBuffer, pRxBuf + W232_RQUEST_ID_POS + W232_RQUEST_ID_LEN + 3))
             {
                 bOK = TRUE;
             }
        }
		else
		{
			 bOK = FALSE;													//��ʱ���տ��ܽض����ݣ�
		}
    }
    else if(strstr((char const *)pRxBuf, g_aStrBuffJsonAccept) != NULL)
    {
        bOK = TRUE;
        g_sDeviceParams.offLineTime = 0;
        g_sW232RcvBuffer.flag = W232_RESPONES_CMD_RESP;

    }
    else if(strstr((char const *)pRxBuf, g_aStrBuffJsonReject) != NULL)
    {
        bOK = FALSE;
        g_sW232RcvBuffer.flag = W232_RESPONES_CMD_RESP;

    }
    else if(strstr((char const *)pRxBuf, g_aStrBuffcmdAccept) != NULL)
    {
        bOK = TRUE;
        g_sW232RcvBuffer.flag = W232_RESPONES_CMD_RESP;

    }
    else if(strstr((char const *)pRxBuf, g_aStrBuffcmdOta) != NULL)
    {
        //����֪ͨ
        bOK = TRUE;
        g_sW232RcvBuffer.flag = W232_RESPONES_CMD_RESP;
        g_sFramBootParamenter.appState = FRAM_BOOT_APP_FAIL;
        Fram_WriteBootParamenter();
        Reader_Delayms(5);
        Sys_SoftReset();
    }
   
    return bOK;
}



char g_aMqttAtBuf[W232_STR_BUFFER_HEART_LEN] = {0};
char g_aMqttBuf[W232_STR_BUFFER_HEART_LEN] = {0};

void Device_CommunTxCmd(DEVICE_SENVER_TXBUFFER *pCntOp, u32 sysTick)
{
    u8 op = 0;

    op = pCntOp->op[pCntOp->index];
    
    switch(op)
    {
        case W232_MQTT_TOPIC_HEART:

            pCntOp->len = Device_HeartFormat(pCntOp->buffer, g_nSysTick);
            Device_UpHeartData(sysTick,pCntOp->buffer, g_aMqttAtBuf, g_aMqttBuf, pCntOp->len);
            W232_WriteCmd(g_aMqttAtBuf);
            break;
        case W232_CNT_OP_GET_QSQ:


            W232_WriteCmd("AT+CSQ");
            break;
        case W232_MQTT_TOPIC_REBAT:

            Device_RtuRsp(g_sDeviceImpRspInfo.rtuBuffer, g_aMqttAtBuf, g_aMqttBuf, g_sGateOpInfo.add, g_nSysTick);
            W232_WriteCmd(g_aMqttAtBuf);
            break;
        case W232_MQTT_TOPIC_BWBAT:
          
            Device_BrwRsp(g_sDeviceImpRspInfo.brwBuffer, g_aMqttAtBuf, g_aMqttBuf, g_sGateOpInfo.add, g_nSysTick);
            W232_WriteCmd(g_aMqttAtBuf);
            break;
        case W232_MQTT_TOPIC_WARN:
          
            Device_InfoChgRsp(g_sDeviceImpRspInfo.warnBuffer, g_aMqttAtBuf, g_aMqttBuf, g_sDeviceImpRspInfo.add, g_nSysTick);
            W232_WriteCmd(g_aMqttAtBuf);
            break;
        case W232_MQTT_TOPIC_CMD:
          
            Device_PostRsp(&g_sW232Connect,g_sDeviceRspFrame.buffer, g_aMqttAtBuf, g_aMqttBuf, g_sDeviceRspFrame.len);
            W232_WriteCmd(g_aMqttAtBuf);
            
            break;
            
    }
    pCntOp->tick = sysTick;
}


void Device_PostRsp(W232_CONNECT *pCntOp0,u8 *pBuffer, char *strAtBuff, char *strRspBuff,u16 len)
{  
    char strRspbuffer[W232_STR_BUFFER_LEN] ={0};
    char strRsplen[4] = {0};
    u8 hexLen[2] = {0};
      //0x01D9
    hexLen[0] = ((len & 0xFF00) >> 8);
    hexLen[1] = ((len & 0x00FF) >> 0); 
    a_Hex2Str(strRspbuffer, pBuffer, len);
    a_Hex2Str(strRsplen, hexLen, 2);
    sprintf(strRspBuff,"{\"id\":%.8s,\"dp\":{\"device\":[{\"v\":\"%.2s%.2s%.4s%s\"}]}}", g_sW232RcvBuffer.idStr, g_sW232RcvBuffer.addStr, g_sW232RcvBuffer.cmdStr, strRsplen, strRspbuffer);
    sprintf(strAtBuff,"AT+QMTPUBEX=0,0,0,0,\"$sys/%.6s/%.15s/cmd/response/%.36s\",%d",W232_PRDOCT_ID,g_nImsiStr, pCntOp0->requestId, strlen(strRspBuff));
    
}


BOOL Device_CommunCheckRsp(DEVICE_SENVER_TXBUFFER *pCntOp, u8 *pRxBuf)
{
    u8 op = 0;

    BOOL bOK = FALSE;
    op = pCntOp->op[pCntOp->index];

    switch(op)
    {
        case W232_MQTT_TOPIC_HEART:
            if(strstr((char const *)pRxBuf, ">") != NULL)
            {
                bOK = TRUE;
                W232_WriteCmd(g_aMqttBuf);
            }
            break;
        case  W232_CNT_OP_GET_QSQ:
            if(strstr((char const *)pRxBuf, "OK") != NULL)
            {
                bOK = TRUE;
                g_sW232Connect.sigNum = a_atoi(pRxBuf + 8, 2, STD_LIB_FMT_DEC);
                g_sW232Connect.channelErrNum = a_atoi(pRxBuf + 11, 2,STD_LIB_FMT_DEC);
            }
            break;
        case W232_MQTT_TOPIC_REBAT:
           if(strstr((char const *)pRxBuf, ">") != NULL)
            {
                bOK = TRUE;
                W232_WriteCmd(g_aMqttBuf);

            }
            break;
        case W232_MQTT_TOPIC_BWBAT:
            if(strstr((char const *)pRxBuf, ">") != NULL)
            {
                bOK = TRUE;
                W232_WriteCmd(g_aMqttBuf);
            }
            break;
        case W232_MQTT_TOPIC_WARN:
            if(strstr((char const *)pRxBuf, ">") != NULL)
            {
                bOK = TRUE;
                W232_WriteCmd(g_aMqttBuf);
            }
            break;
        case W232_MQTT_TOPIC_CMD:
            if(strstr((char const *)pRxBuf, ">") != NULL)
            {
                bOK = TRUE;
                W232_WriteCmd(g_aMqttBuf);
                if(g_sDeviceRspFrame.cmd == DEVICE_CMD_RESET && g_sDeviceRspFrame.flag == DEVICE_RESPONSE_FLAG_RESET)
                {
                    g_sFramBootParamenter.appState = FRAM_BOOT_APP_OK;
                    if(Fram_WriteBootParamenter())
                    {
                         W232_CtrlLow();
                         W232_KeyLow();
                         #if SYS_ENABLE_WDT
                         WDG_FeedIWDog();
                         #endif
                         Reader_Delayms(500);
                         #if SYS_ENABLE_WDT
                         WDG_FeedIWDog();
                         #endif
                         Reader_Delayms(500);
                         #if SYS_ENABLE_WDT
                         WDG_FeedIWDog();
                         #endif
                         Reader_Delayms(500);
                         #if SYS_ENABLE_WDT
                         WDG_FeedIWDog();
                         #endif
                         Reader_Delayms(500);
                         Sys_SoftReset();
                    }
                }
                else if(g_sDeviceRspFrame.cmd == DEVICE_CMD_VERSION_UPDATA && g_sDeviceRspFrame.flag == DEVICE_RESPONSE_FLAG_UPDATA)
                {
                    g_sFramBootParamenter.appState = FRAM_BOOT_APP_DATA_DOWD;
                    if(Fram_WriteBootParamenter())
                    {
                         W232_CtrlLow();
                         W232_KeyLow();
                         #if SYS_ENABLE_WDT
                         WDG_FeedIWDog();
                         #endif
                         Reader_Delayms(500);
                         #if SYS_ENABLE_WDT
                         WDG_FeedIWDog();
                         #endif
                         Reader_Delayms(500);
                         #if SYS_ENABLE_WDT
                         WDG_FeedIWDog();
                         #endif
                         Reader_Delayms(500);
                         #if SYS_ENABLE_WDT
                         WDG_FeedIWDog();
                         #endif
                         Sys_SoftReset();
                    }
                  
                }
            }
            break;
    }
	g_sW232Connect.comErr = 0;
    memset(g_aMqttBuf, 0, W232_STR_BUFFER_HEART_LEN);
    
    return bOK;
}



void Device_CommunStep(DEVICE_SENVER_TXBUFFER *pCntOp)
{
    u8 op = 0;
    op = pCntOp->op[pCntOp->index];
	switch(op)
    {
		case W232_MQTT_TOPIC_HEART:
        case  W232_CNT_OP_GET_QSQ:
        case W232_MQTT_TOPIC_REBAT:
        case W232_MQTT_TOPIC_BWBAT:
        case W232_MQTT_TOPIC_WARN:
        case W232_MQTT_TOPIC_CMD:
            if(pCntOp->result == W232_CNT_RESULT_OK)
            {
                pCntOp->repeat[pCntOp->index] = 0;
                pCntOp->index++;
            }
            else
            {
                pCntOp->repeat[pCntOp->index] = 0;
                pCntOp->index++;
                
                //�ϴ�ʧ�ܣ����ߴ洢����
            }
            break;
    }
}


void Device_RtuRsp(u8 *pBuffer, char *strAtBuff, char *strRspBuff, u8 addr, u32 id)
{  
    char strRspbuffer[W232_STR_BUFFER_LEN] ={0};
    char strRspAdd[2] = {0};
    u8 hexAdd[1] = {0};
    
    hexAdd[0] = addr + 1;
    a_Hex2Str(strRspAdd, hexAdd, 1);
    a_Hex2Str(strRspbuffer, pBuffer, 7);
    sprintf(strRspBuff,"{\"id\":%.8d,\"dp\":{\"rebat\":[{\"v\":\"%.2s3A0007%.14s\"}]}}", id, strRspAdd, strRspbuffer);
    sprintf(strAtBuff,"AT+QMTPUBEX=0,0,0,0,\"$sys/%.6s/%.15s/dp/post/json\",%d",W232_PRDOCT_ID, g_nImsiStr, strlen(strRspBuff));

}

void Device_BrwRsp(u8 *pBuffer, char *strAtBuff, char *strRspBuff, u8 addr, u32 id)
{  
    char strRspbuffer[W232_STR_BUFFER_LEN] ={0};
    char strRspAdd[2] = {0};
    u8 hexAdd[1] = {0};
    
    hexAdd[0] = addr + 1;
    a_Hex2Str(strRspAdd, hexAdd, 1);
    a_Hex2Str(strRspbuffer, pBuffer, 7);
    sprintf(strRspBuff,"{\"id\":%.8d,\"dp\":{\"bwbat\":[{\"v\":\"%.2s3B0007%.14s\"}]}}", id, strRspAdd, strRspbuffer);
    sprintf(strAtBuff,"AT+QMTPUBEX=0,0,0,0,\"$sys/%.6s/%.15s/dp/post/json\",%d",W232_PRDOCT_ID, g_nImsiStr, strlen(strRspBuff));
    
}

void Device_InfoChgRsp(u8 *pBuffer, char *strAtBuff, char *strRspBuff, u8 addr, u32 id)
{  
	char strRspbuffer[W232_STR_BUFFER_LEN] ={0};
    char strRspAdd[2] = {0};
    u8 hexAdd[1] = {0};
    
    hexAdd[0] = addr + 1;
    a_Hex2Str(strRspAdd, hexAdd, 1);
    a_Hex2Str(strRspbuffer, pBuffer, 9);
    sprintf(strRspBuff,"{\"id\":%.8d,\"dp\":{\"state\":[{\"v\":\"%.2s220009%.14s\"}]}}", id, strRspAdd, strRspbuffer);
    sprintf(strAtBuff,"AT+QMTPUBEX=0,0,0,0,\"$sys/%.6s/%.15s/dp/post/json\",%d",W232_PRDOCT_ID, g_nImsiStr, strlen(strRspBuff));

}





void Device_GateStateRsp()    //״̬�仯�������������£��Ƿ�ѹ������
{ 
	u8 index = 0;
	if(index < (GATE_SLAVER_NUM << 1))
	{
	    for(index = 0; index < (GATE_SLAVER_NUM << 1); index ++)
	    {
		if(memcmp(&(g_aGateSlvStat[index].sensorState), &(g_aGateSlvInfo[index].sensorInfo.sensorState), sizeof(GATE_STATINFO)) ||
			memcmp(&(g_aGateSlvStat[index].batState), &(g_aGateSlvInfo[index].sensorInfo.batInfo.state), sizeof(u8)) ||
			memcmp(&(g_aGateSlvStat[index].chagState), &(g_aGateSlvInfo[index].sensorInfo.chagInfo.state), sizeof(u8)))     //״̬�仯
		{
		    memcpy(&(g_aGateSlvStat[index].sensorState), &(g_aGateSlvInfo[index].sensorInfo.sensorState), sizeof(GATE_STATINFO));
		    memcpy(&(g_aGateSlvStat[index].batState), &(g_aGateSlvInfo[index].sensorInfo.batInfo.state), sizeof(u8));
		    memcpy(&(g_aGateSlvStat[index ].chagState), &(g_aGateSlvInfo[index].sensorInfo.chagInfo.state), sizeof(u8));
		    Device_AtRsp(W232_CNT_TIME_100MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_HEART);
		}
	    }

	}
	if(g_sIoInfo.tempState1 != ((g_sIoInfo.sersorState & 0x1F) | (Device_ChkDeviceStat(IO_DEVICE_STAT_FAN) << 5) | (Device_ChkDeviceStat(IO_DEVICE_STAT_LED) << 6) | (Device_ChkDeviceStat(IO_DEVICE_STAT_RELAY) << 7)) )
	{
	    g_sIoInfo.tempState1 = ((g_sIoInfo.sersorState & 0x1F) | (Device_ChkDeviceStat(IO_DEVICE_STAT_FAN) << 5) | (Device_ChkDeviceStat(IO_DEVICE_STAT_LED) << 6) | (Device_ChkDeviceStat(IO_DEVICE_STAT_RELAY) << 7))  ;
	    Device_AtRsp(W232_CNT_TIME_100MS, W232_CNT_REPAT_NULL, W232_MQTT_TOPIC_HEART);
	}

}

void Device_GateStateInit()
{ 
	u8 index = 0;

	for(index = 0; index < (GATE_SLAVER_NUM << 1); index ++)
	{
		memcpy(&(g_aGateSlvStat[index].sensorState), &(g_aGateSlvInfo[index].sensorInfo.sensorState), sizeof(GATE_STATINFO));
		memcpy(&(g_aGateSlvStat[index].batState), &(g_aGateSlvInfo[index].sensorInfo.batInfo.state), sizeof(u8));
		memcpy(&(g_aGateSlvStat[index ].chagState), &(g_aGateSlvInfo[index].sensorInfo.chagInfo.state), sizeof(u8));
	}

}



void Device_VoiceCtr()
{
	if(g_sIoInfo.sersorState & IO_SENSOR_STAT_SMOKE)
	{
		if(!a_CheckStateBit(g_sSoundInfo.state, SOUND_STAT_WAIT))
		{
			Device_VoiceApoFrame(SOUND_CNT_TIME_1S * 3, SOUND_REPAT_NULL, SOUND_VOICE_FIRE_WRAN, SOUND_VOC_FIRE_WARN);
		}
	}
}



BOOL Device_ChkSersor()
{
	BOOL bOK = FALSE;
	if(g_sDeviceParams.temprUp.t < AD_TEMPR_NORMAL || g_sDeviceParams.temprDown.t < AD_TEMPR_NORMAL)
	{
		g_sDeviceTestInfo.err |= DEVICE_TEST_ERR_TEMPR;
	}
	else
	{
		g_sDeviceTestInfo.err &= ~DEVICE_TEST_ERR_TEMPR;
	}
	
	if(g_sElectInfo.electValue == ELECT_GET_VALUE_FAIL)
	{
		g_sDeviceTestInfo.err |= DEVICE_TEST_ERR_ELECT;
	}
	else
	{
		g_sDeviceTestInfo.err &= ~DEVICE_TEST_ERR_ELECT;
	}
	
	if(g_sDeviceTestInfo.err == 0)
	{
		bOK = TRUE;
	}
	
	return bOK;
}

/*

u8 Device_UrlEncode(char *sign)
{

	char sign_t[40];
	unsigned char i = 0, j = 0;
	unsigned char sign_len = strlen(sign);
	
	if(sign == (void *)0 || sign_len < 28)
		return 1;
	
	for(; i < sign_len; i++)
	{
		sign_t[i] = sign[i];
		sign[i] = 0;
	}
	sign_t[i] = 0;
	
	for(i = 0, j = 0; i < sign_len; i++)
	{
		switch(sign_t[i])
		{
			case '+':
				strcat(sign + j, "%2B");j += 3;
			break;
			
			case ' ':
				strcat(sign + j, "%20");j += 3;
			break;
			
			case '/':
				strcat(sign + j, "%2F");j += 3;
			break;
			
			case '?':
				strcat(sign + j, "%3F");j += 3;
			break;
			
			case '%':
				strcat(sign + j, "%25");j += 3;
			break;
			
			case '#':
				strcat(sign + j, "%23");j += 3;
			break;
			
			case '&':
				strcat(sign + j, "%26");j += 3;
			break;
			
			case '=':
				strcat(sign + j, "%3D");j += 3;
			break;
			
			default:
				sign[j] = sign_t[i];j++;
			break;
		}
	}
	
	sign[j] = 0;
	
	return 0;

}
*/

/*
u8 Device_ChkDoor()
{
  u8 i = 0,index = 0;
  for(i  = 0; i <= GATE_SLAVER_NUM << 1; i  ++)
  {
  
     if(!g_aGateSlvInfo[i].sensorInfo.sensorState.door)
     {
        index = i;
        break;
     }
  }
   return index;
}
*/

/*
#define METHOD		"sha1"
u8 Device_Ota_Token(char *ver, char *res, unsigned int et, char *access_key, char *authorization_buf, unsigned short authorization_buf_len)
{
	
	size_t olen = 0;
	
	char sign_buf[40];							//����ǩ����Base64������ �� URL������
	char hmac_sha1_buf[40];						//����ǩ��
	char access_key_base64[40];					//����access_key��Base64������
	char string_for_signature[56];				//����string_for_signature������Ǽ��ܵ�key

//----------------------------------------------------�����Ϸ���--------------------------------------------------------------------
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
//----------------------------------------------------��access_key����Base64����----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
	//UsartPrintf(USART_DEBUG, "access_key_base64: %s\r\n", access_key_base64);
	
//----------------------------------------------------����string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	//UsartPrintf(USART_DEBUG, "string_for_signature: %s\r\n", string_for_signature);
	
//----------------------------------------------------����-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	
	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
	//UsartPrintf(USART_DEBUG, "hmac_sha1_buf: %s\r\n", hmac_sha1_buf);
	
//----------------------------------------------------�����ܽ������Base64����------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

//----------------------------------------------------��Base64����������URL����---------------------------------------------------
	Device_UrlEncode(sign_buf);
	//UsartPrintf(USART_DEBUG, "sign_buf: %s\r\n", sign_buf);
	
//----------------------------------------------------����Token--------------------------------------------------------------------
	snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	//UsartPrintf(USART_DEBUG, "Token: %s\r\n", token_buf);
	
	return 0;

}


u16 Device_MqttRequeatSck(u8 *pBuffer)
{
  u8 pos = 0,index = 0;
  
  for(pos = 0; pos <= 50; pos ++)
  {
    if(*(pBuffer + pos) != 0x22)
    {
         index ++;
    }
    else
    {
      break;
    }
  }
  return index;


}
*/

void Device_FormatMainInfo(GATE_OPINFO *pGateOpInfo)
{
	u16 pos = 0;
    u16 crc  = 0;
    u8 *pBuffer = NULL;
    u16 addr = 0;
	u8 gateState = 0;
	u8 index = 0;
	
    addr = 0x0001;
    pBuffer = pGateOpInfo->txFrame.buffer;
    
    pBuffer[pos++] = UART_FRAME_FLAG_HEAD1;     // frame head first byte
    pBuffer[pos++] = UART_FRAME_FLAG_HEAD2;     // frame haed second byte
    pBuffer[pos++] = 0x00;                      // length
    pBuffer[pos++] = 0x00;
    pBuffer[pos++] = 0x00;
    pBuffer[pos++] = (addr >> 0) & 0xFF; 
    pBuffer[pos++] = (addr >> 8) & 0xFF;
    pBuffer[pos++] = DEVICE_CMD_INFOR_MAIN_INFO;                       // cmd
    pBuffer[pos++] = UART_FRAME_PARAM_RFU;      // RFU

	
	for(index = 0 ;index < GATE_SLAVER_NUM; index ++)
	{
		if(g_sGateOpInfo.comErr[index] < DEVICE_GATE_OP_TICK)
		{
			gateState |= (1 << index);
		}
	}
	pBuffer[pos++] = gateState;      // RFU
	memcpy(pBuffer + pos, g_nImei, (W232_IMEI_LEN + 1) / 2);
    pos += (W232_IMEI_LEN + 1) / 2;     
    memcpy(pBuffer + pos, (u8 *)&DEVICE_VERSION[DEVICE_VERSION_LEN - 1], DEVICE_VERSION_LEN);  //Ӳ���汾
	pos += DEVICE_VERSION_LEN ;
    memcpy(pBuffer + pos, (u8 *)&DEVICE_VERSION, DEVICE_VERSION_LEN - 2);						//����汾
	pos += (DEVICE_VERSION_LEN - 2);
	pBuffer[pos++] = 0x30;
	pBuffer[pos++] = 0x31;					//  ����汾��Ϣ��0
    


	if(g_sWaterInfo.comErr <= WATER_COM_OP_TICK)
	{
		 pBuffer[pos++] = 0;
	}
	else
	{
		pBuffer[pos++] = DEVICE_COM_STAT_FAIL;
	}
	
	if(g_sW232Connect.comErr <= DEVICE_GATE_OP_TICK)
	{
		pBuffer[pos++] = 0;
	}
	else
	{
		pBuffer[pos++] = DEVICE_COM_STAT_FAIL;
	}  
	pBuffer[pos++] = (g_sElectInfo.electValue >> 0) & 0xFF;
    pBuffer[pos++] = (g_sElectInfo.electValue >> 8) & 0xFF;
    pBuffer[pos++] = (g_sElectInfo.electValue >> 16) & 0xFF;
    pBuffer[pos++] = (g_sElectInfo.electValue >> 24) & 0xFF;
    pBuffer[pos++] = (g_sDeviceParams.temprUp.t >> 0) & 0xFF;
    pBuffer[pos++] = (g_sDeviceParams.temprDown.t >> 0) & 0xFF;
    pBuffer[pos++] = (g_sIoInfo.sersorState & 0x1F) | 
                     (Device_ChkDeviceStat(IO_DEVICE_STAT_FAN) << 5) | 
                     (Device_ChkDeviceStat(IO_DEVICE_STAT_LED) << 6) | 
                     (Device_ChkDeviceStat(IO_DEVICE_STAT_RELAY) << 7);
    pBuffer[2] = pos - 3 + 2; //��ȥ֡ͷ7E 55 LEN �������ֽڣ�����CRC�������ֽ�

    crc = a_GetCrc(pBuffer + UART_FRAME_POS_LEN, pos - 2); //��LEN��ʼ����crc
    pBuffer[pos++] = (crc >> 0) & 0xFF;
    pBuffer[pos++] = (crc >> 8) & 0xFF;
                                                                     
    pGateOpInfo->txFrame.len = pos;


}

