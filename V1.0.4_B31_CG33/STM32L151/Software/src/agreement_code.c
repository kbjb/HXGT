#include "agreement_head.h"
#include "param_head.h"
#include "usart_head.h"
#include "App_Eep.h"
#include "at24c512_head.h"
/* [B2] Link freshness is separate from the last displayed measurement.
 * Do not overwrite calibration or measurement values on timeout. */
static volatile uint32_t Sensor_Last_Valid_Tick = 0;
static volatile uint8_t Sensor_Frame_Seen = 0;

uint8_t Sensor_Data_Is_Fresh(void)
{
    return (uint8_t)(Sensor_Frame_Seen &&
        (uint32_t)(HAL_GetTick() - Sensor_Last_Valid_Tick) < SENSOR_LINK_TIMEOUT_MS);
}


static int16_t CG_Capture_Samples[CG_CAL_STABLE_SAMPLES];
static uint32_t CG_Capture_Ticks[CG_CAL_STABLE_SAMPLES];
static uint8_t CG_Capture_Index = 0U;
static uint8_t CG_Capture_Count = 0U;
volatile int32_t CG_Calibration_Range_AD = 0;

void Sensor_CG_Stability_Reset(void)
{
    CG_Capture_Count = 0U;
    CG_Capture_Index = 0U;
    CG_Calibration_Range_AD = 0;
}

uint8_t Sensor_Get_Stable_CG(int16_t *value)
{
    int16_t lowest = INT16_MAX, highest = INT16_MIN;
    int32_t sum = 0;
    uint8_t i;
    if (value == NULL || CG_Capture_Count < CG_CAL_STABLE_SAMPLES ||
        !Sensor_Data_Is_Fresh() ||
        (uint32_t)(HAL_GetTick() - Sensor_Last_Valid_Tick) >= CG_CAL_MAX_GAP_MS)
        return 0U;
    for (i = 0U; i < CG_CAL_STABLE_SAMPLES; ++i)
    {
        int16_t sample = CG_Capture_Samples[i];
        if (sample < lowest) lowest = sample;
        if (sample > highest) highest = sample;
        sum += sample;
    }
    CG_Calibration_Range_AD = (int32_t)highest - (int32_t)lowest;
    if (CG_Calibration_Range_AD > CG_CAL_STABLE_RANGE_AD ||
        (uint32_t)(Sensor_Last_Valid_Tick - CG_Capture_Ticks[CG_Capture_Index]) <
            CG_CAL_STABLE_TIME_MS)
        return 0U;
    /* Round the mean symmetrically for positive and negative ADC values. */
    sum += sum >= 0 ? (int32_t)CG_CAL_STABLE_SAMPLES / 2 :
                    -(int32_t)CG_CAL_STABLE_SAMPLES / 2;
    *value = (int16_t)(sum / (int32_t)CG_CAL_STABLE_SAMPLES);
    return 1U;
}

int16_t Unsigned_Six_To_Signed_Six(uint16_t data)
{
    UToI uToi_Temp;
    uToi_Temp.uinf = data;
    return uToi_Temp.inf;
}
uint16_t Signed_Six_To_Unsigned_Six(int16_t data)
{
    UToI uToi_Temp;
    uToi_Temp.inf = data;
    return uToi_Temp.uinf;
}
float Unsigned_Six_To_Float(uint32_t data)
{
    FToI fToi_Temp;
    fToi_Temp.inf = data;
    return fToi_Temp.flo;
}
uint32_t Float_To_Unsigned_Six(float data)
{
    FToI fToi_Temp;
    fToi_Temp.flo = data;
    return fToi_Temp.inf;
}



uint8_t Get_Add_Verify(uint8_t *buffp,uint8_t len)
{
    uint32_t sum = 0;
    for(uint8_t i=0;i<len;i++)
        sum+=buffp[i];
    return sum&0x000000FF;
}



uint8_t Sensor_Data_Receive_For_Uart1(uint8_t* data, uint16_t length)
{
    /* [B1] Exactly 13 bytes are needed before any fixed-position read. */
    if(data == NULL || length != 13) return 0;
    if(data[0] != 0x5A || data[length-2]!=0x0D || data[length-1]!=0x0A)
        return 0;
    if(data[1] != (length-3))
        return 1;
    if(data[length-3] != Get_Add_Verify(data,length-3))
        return 2;
    /* [B2] Shift unsigned 32-bit fields, avoiding signed-shift UB. */
    GJ_Ad = (uint16_t)(((uint16_t)data[2] << 8) | data[3]);
    CG_Ad = Unsigned_Six_To_Signed_Six((uint16_t)(((uint16_t)data[4] << 8) | data[5]));
    Temp  = Unsigned_Six_To_Float(((uint32_t)data[6] << 24) |
                                  ((uint32_t)data[7] << 16) |
                                  ((uint32_t)data[8] << 8) | (uint32_t)data[9]);
    {
        uint32_t now = HAL_GetTick();
        if (Sensor_Frame_Seen &&
            (uint32_t)(now - Sensor_Last_Valid_Tick) >= CG_CAL_MAX_GAP_MS)
            Sensor_CG_Stability_Reset();
        CG_Capture_Samples[CG_Capture_Index] = CG_Ad;
        CG_Capture_Ticks[CG_Capture_Index] = now;
        CG_Capture_Index = (uint8_t)((CG_Capture_Index + 1U) % CG_CAL_STABLE_SAMPLES);
        if (CG_Capture_Count < CG_CAL_STABLE_SAMPLES) CG_Capture_Count++;
        Sensor_Last_Valid_Tick = now;
    }
    Sensor_Frame_Seen = 1;
    return 3;
}

void Agreement_Ack_For_Uart1(uint8_t uart_id,uint8_t* data, uint16_t length)
{
    uint8_t res = Sensor_Data_Receive_For_Uart1(data,length);
    char Tx_Buffer[7] = {0x5A,0x05,0x00,0x00,0x5F,0x0D,0x0A};
    if(res == 3)
    {
        Tx_Buffer[2] = 0x4F;
        Tx_Buffer[3] = 0x4B;
    }
    else
    {
        Tx_Buffer[2] = 0;
        Tx_Buffer[3] = res;
    }
    Tx_Buffer[4] = Get_Add_Verify((uint8_t *)Tx_Buffer,4);
    UART_Send_Data((UART_ID_t)uart_id,(uint8_t*)Tx_Buffer,7);
    
    /* [B2] Receiver stays armed; UART_Process_Received frees its mailbox. */
}






typedef struct 
{
    uint8_t     uart_id;        //串口ID
    volatile uint8_t cmd;   //协议命令
    uint8_t*    CmdBuffer;      //指令缓存区
    uint8_t     DataOffset;     //数据偏移位置
    uint16_t    dataNumber;     //数据大小
    uint16_t    BufferSize;     //缓冲区尺寸
    uint32_t    Sum;            //校验和
}AGREEMEN_PARAM;

/* [B1] 0xFF is the idle state; zero-initialization is NOT idle. */
AGREEMEN_PARAM CmdParam = {UART2_ID, 0xFF, NULL, 4, 0, 0, 0};



uint8_t UART2_PC_Read_Password(AGREEMEN_PARAM * AgreementParam)
{
    uint8_t *buffer = AgreementParam->CmdBuffer+AgreementParam->DataOffset;
    AgreementParam->dataNumber = 4;
    buffer[0] = (Driver_Calibration_Password>>24)&0xFF;
    buffer[1] = (Driver_Calibration_Password>>16)&0xFF;
    buffer[2] = (Driver_Calibration_Password>>8)&0xFF;
    buffer[3] = (Driver_Calibration_Password)&0xFF;
    return 1;
}

uint8_t UART2_PC_Write_Password(AGREEMEN_PARAM *AgreementParam)
{
    uint8_t *buffer = AgreementParam->CmdBuffer + AgreementParam->DataOffset;
    uint32_t new_password;
    /* [B3] No more four unverified byte writes to the legacy region.
     * The password is committed with the whole calibration snapshot. */
    new_password = ((uint32_t)buffer[0] << 24) |
                   ((uint32_t)buffer[1] << 16) |
                   ((uint32_t)buffer[2] << 8) | (uint32_t)buffer[3];
    AgreementParam->dataNumber = 0U;
    return Eep_Save_Password(new_password);
}

uint8_t UART2_PC_Read_Eep(AGREEMEN_PARAM *AgreementParam)
{
    uint16_t number = 0U, index;
    if (AgreementParam->dataNumber != 2U) return 0U;
    index = (uint16_t)(((uint16_t)AgreementParam->CmdBuffer[AgreementParam->DataOffset] << 8) |
              AgreementParam->CmdBuffer[AgreementParam->DataOffset + 1U]);
    /* [B3] NEVER read the legacy counter from 0xFFE1/0xFFE2:
     * those bytes overlap the old CH calibration. */
    if (!Eep_Get_Node_Count(&number) || index >= number ||
        index >= MAX_INDEX_ADDR) return 0U;
    if ((uint32_t)AgreementParam->DataOffset + 2U + sizeof(EEP_NODE) + 3U >
        AgreementParam->BufferSize) return 0U;
    if (Eep_Output_Node(index, AgreementParam->CmdBuffer +
          AgreementParam->DataOffset + 2U) != 0U) return 0U;
    AgreementParam->dataNumber += sizeof(EEP_NODE);
    return 1U;
}

/* [B3] UART2 has no authentication for destructive commands.
 * The local Eep_Clear_Node() API is functional, but remote clear remains
 * disabled by default. Enable only after an authentication design review. */
#ifndef EEPROM_UART2_ENABLE_CLEAR
#define EEPROM_UART2_ENABLE_CLEAR 0
#endif
uint8_t UART2_PC_Clear_Eep(AGREEMEN_PARAM *AgreementParam)
{
    (void)AgreementParam;
#if EEPROM_UART2_ENABLE_CLEAR
    return Eep_Clear_Node();
#else
    return 0U;
#endif
}

uint8_t UART2_PC_Get_Eep_Number(AGREEMEN_PARAM *AgreementParam)
{
    uint16_t count;
    /* [B3] B3's CRC-protected journal is the sole source of the count. */
    if (!Eep_Get_Node_Count(&count)) return 0U;
    AgreementParam->CmdBuffer[AgreementParam->DataOffset] = (uint8_t)(count >> 8);
    AgreementParam->CmdBuffer[AgreementParam->DataOffset + 1U] = (uint8_t)count;
    AgreementParam->dataNumber = 2U;
    return 1U;
}
uint8_t UART2_PC_Change_Data_Mode(AGREEMEN_PARAM * AgreementParam)
{
    uint8_t data;
    if(AgreementParam->dataNumber!=1)
        return 0;
    data = AgreementParam->CmdBuffer[AgreementParam->DataOffset];
    if(data>1)
        return 0;
    U2_PC_Work_Mode = data;
    AgreementParam->dataNumber = 0;
    return 1;
}
uint8_t UART2_PC_Get_All_Eep(AGREEMEN_PARAM * AgreementParam)
{
    uint16_t number = 0, size;
    uint32_t sum = 0;
    /* [B3] Only use validated B3 metadata, not the overlapped legacy count. */
    if (!Eep_Get_Node_Count(&number)) return 0;
    if (number > MAX_INDEX_ADDR ||
        (uint32_t)number * sizeof(EEP_NODE) > 65535UL) return 0;
    size = (uint16_t)((uint32_t)number * sizeof(EEP_NODE));
    //和
    sum+=0x5A;
    sum+=AgreementParam->cmd;
    sum+=(size>>8)&0xFF;
    sum+=size&0xFF;
    
    AgreementParam->CmdBuffer[0] = 0x5A;//发送帧头和应答码
    AgreementParam->CmdBuffer[1] = AgreementParam->cmd;
    AgreementParam->CmdBuffer[2] = (size>>8)&0xFF;//发送数据个数
    AgreementParam->CmdBuffer[3] = size&0xFF;
    UART_Send_Data((UART_ID_t)AgreementParam->uart_id,(uint8_t*)AgreementParam->CmdBuffer,4);
    //发送数据
    for(uint16_t i=0;i<number;i++)
    {
        //获取存储节点数据
        if (Eep_Output_Node(i, AgreementParam->CmdBuffer) != 0)
            return 0; /* [B2] Stop streaming on read failure. */
        //累加存储节点数据值
        for(uint8_t j=0;j<sizeof(EEP_NODE);j++)
            sum += AgreementParam->CmdBuffer[j];
        //发送存储节点数据
        UART_Send_Data((UART_ID_t)AgreementParam->uart_id,(uint8_t*)AgreementParam->CmdBuffer,sizeof(EEP_NODE));
    }
    //校验
    AgreementParam->CmdBuffer[0] = sum&0x000000FF;
    UART_Send_Data((UART_ID_t)AgreementParam->uart_id,(uint8_t*)AgreementParam->CmdBuffer,1);
    //帧尾
    AgreementParam->CmdBuffer[0] = 0x0D;
    AgreementParam->CmdBuffer[1] = 0x0A;
    UART_Send_Data((UART_ID_t)AgreementParam->uart_id,(uint8_t*)AgreementParam->CmdBuffer,2);
    
    
    return 1;
}






/* [B2] Validate command and its exact request payload before dispatch.
 * This callback runs in the main loop (not in USART IRQ). The pending
 * manager mailbox is valid throughout the synchronous response. */
void Agreement_Ack_For_Uart2(uint8_t uart_id, uint8_t *data, uint16_t length)
{
    uint16_t data_len, i;
    uint16_t sum = 0;
    uint8_t response = 0x80;
    uint16_t required_len = 0;
    uint8_t supported = 1;

    if (uart_id != UART2_ID || data == NULL || length < 7 ||
        length > UART_RX_BUFFER_SIZE) return;
    data_len = (uint16_t)(((uint16_t)data[2] << 8) | data[3]);
    if (data[0] != 0x5A || (uint32_t)data_len + 7U != length ||
        data[length - 2U] != 0x0D || data[length - 1U] != 0x0A) return;
    for (i = 0; i < length - 3U; ++i) sum = (uint16_t)(sum + data[i]);
    if ((uint8_t)sum != data[length - 3U]) return;

    /* No persistent command pointer is needed after the response completes. */
    CmdParam.uart_id = UART2_ID;
    CmdParam.CmdBuffer = data;
    CmdParam.BufferSize = UART_RX_BUFFER_SIZE;
    CmdParam.DataOffset = 4;
    CmdParam.dataNumber = data_len;

    switch (data[1])
    {
        case UART2_PC_HANDSHAKE:        response = UART2_PC_HANDSHAKE_ACK; break;
        case UART2_PC_READ_PASSWORD:    response = UART2_PC_READ_PASSWORD_ACK; break;
        case UART2_PC_WRITE_PASSWORD:   response = UART2_PC_WRITE_PASSWORD_ACK; required_len = 4; break;
        case UART2_PC_READ_EEP_DATA:    response = UART2_PC_READ_EEP_DATA_ACK; required_len = 2; break;
        case UART2_PC_CLEAR_EEP_DATA:   response = UART2_PC_CLEAR_EEP_DATA_ACK; break;
        case UART2_PC_GET_EEP_NUM_DATA: response = UART2_PC_GET_EEP_NUM_DATA_ACK; break;
        case UART2_PC_CHANGE_DATA_MODE: response = UART2_PC_CHANGE_DATA_MODE_ACK; required_len = 1; break;
        case UART2_PC_GET_ALL_EEP_DATA: response = UART2_PC_GET_ALL_EEP_DATA_ACK; break;
        default: supported = 0; break;
    }
    if (!supported)
    {
        CmdParam.cmd = 0x80;
        CmdParam.dataNumber = 0;
    }
    else if (data_len != required_len)
    {
        /* [B2] Reject mismatched command lengths before any EEPROM write. */
        CmdParam.cmd = (uint8_t)(response | 0x40U);
        CmdParam.dataNumber = 0;
    }
    else
    {
        CmdParam.cmd = response;
    }
    /* [B2] The handler is synchronous and foreground-only. Avoid retaining
     * pointers into the released single-frame RX mailbox. */
    Agreement_Ack_For_Fun();
}

void Agereement_Reset_CmdParam(AGREEMEN_PARAM * AgreementParam)
{
    AgreementParam->cmd = 0xFF;
    AgreementParam->CmdBuffer = NULL; /* [B1] Never retain the reused UART RX pointer. */
    AgreementParam->BufferSize = 0;
    AgreementParam->dataNumber = 0;
    AgreementParam->DataOffset = 4;
    AgreementParam->uart_id = UART2_ID;
}










void Agreement_Ack_For_Fun(void)
{
//    uint8_t *u8Temp;
    if(CmdParam.cmd == 0xFF)return;
    /* [B1] A command may only run when a real UART2 frame supplied a buffer. */
    if(CmdParam.CmdBuffer == NULL || CmdParam.BufferSize < 7 ||
       CmdParam.BufferSize > UART_RX_BUFFER_SIZE || CmdParam.uart_id != UART2_ID)
    {
        /* [B2] Transport continues independent of command state. */
        Agereement_Reset_CmdParam(&CmdParam);
        return;
    }
    switch(CmdParam.cmd)
    {
        case UART2_PC_HANDSHAKE_ACK:
            CmdParam.dataNumber = 0;
            break;
        case UART2_PC_READ_PASSWORD_ACK:
            UART2_PC_Read_Password(&CmdParam);
            break;
        case UART2_PC_WRITE_PASSWORD_ACK:
            if(UART2_PC_Write_Password(&CmdParam) == 0)
            {
                CmdParam.cmd = CmdParam.cmd | (1<<6);
                CmdParam.dataNumber = 0;
            }
            break;
        case UART2_PC_READ_EEP_DATA_ACK:
            if(UART2_PC_Read_Eep(&CmdParam) == 0)
            {
                CmdParam.cmd = CmdParam.cmd | (1<<6);
                CmdParam.dataNumber = 0;
            }
            break;
        case UART2_PC_CLEAR_EEP_DATA_ACK:
            /* [B3] Remote destructive clear is guarded by a disabled-by-
             * default compile-time flag; no false success is reported. */
            if (UART2_PC_Clear_Eep(&CmdParam) == 0)
                CmdParam.cmd |= (1U << 6);
            CmdParam.dataNumber = 0;
            break;
        case UART2_PC_GET_EEP_NUM_DATA_ACK:
            if (UART2_PC_Get_Eep_Number(&CmdParam) == 0)
            {
                CmdParam.cmd |= (1U << 6);
                CmdParam.dataNumber = 0;
            }
            break;
        case UART2_PC_CHANGE_DATA_MODE_ACK:
            if(UART2_PC_Change_Data_Mode(&CmdParam) == 0)
            {
                CmdParam.cmd = CmdParam.cmd | (1<<6);
                CmdParam.dataNumber = 0;
            }
            break;
        case UART2_PC_GET_ALL_EEP_DATA_ACK:
            if (UART2_PC_Get_All_Eep(&CmdParam) != 0)
                goto Uart_Rest;
            CmdParam.cmd |= (1U << 6);
            CmdParam.dataNumber = 0;
            break;
        default:
            CmdParam.dataNumber = 0; /* [B1] Unknown/error command has no payload. */
            break;
    }
    
    //帧头
    /* [B1] The ordinary reply uses the RX buffer and UART TX size limit. */
    if((uint32_t)CmdParam.dataNumber + 7U > CmdParam.BufferSize ||
       (uint32_t)CmdParam.dataNumber + 7U > UART_TX_BUFFER_SIZE)
    {
        CmdParam.cmd = 0x80;
        CmdParam.dataNumber = 0;
    }
    CmdParam.CmdBuffer[0] = 0x5A;
    //应答码/命令码
    CmdParam.CmdBuffer[1] = CmdParam.cmd;
    //数据长度
    CmdParam.CmdBuffer[2] = (CmdParam.dataNumber>>8)&0xFF;
    CmdParam.CmdBuffer[3] = CmdParam.dataNumber&0xFF;
    //校验码
    CmdParam.CmdBuffer[CmdParam.dataNumber+4] = Get_Add_Verify((uint8_t *)CmdParam.CmdBuffer,CmdParam.dataNumber+4);
    //帧尾
    CmdParam.CmdBuffer[CmdParam.dataNumber+5] = 0x0D;
    CmdParam.CmdBuffer[CmdParam.dataNumber+6] = 0x0A;
    //发送数据
    UART_Send_Data((UART_ID_t)CmdParam.uart_id,(uint8_t*)CmdParam.CmdBuffer,CmdParam.dataNumber+7);
    Uart_Rest:
    /* [B2] No HAL reception abort between packets. */
    //复位参数
    Agereement_Reset_CmdParam(&CmdParam);
}

/*





*/




