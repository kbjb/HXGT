#ifndef __AGREEMENT_HEAD_H
#define __AGREEMENT_HEAD_H

#include "main.h"


typedef enum 
{
    Err,
}U2_PC_ACK_T;

typedef struct
{
    uint8_t  Command;
    uint16_t DataLen;
    uint8_t  data[128];
    
}U2_PC_AGREEMENT_PARAM;



#define UART2_PC_HANDSHAKE                  0x01
#define UART2_PC_READ_PASSWORD              0x02
#define UART2_PC_WRITE_PASSWORD             0x03
#define UART2_PC_READ_EEP_DATA              0x04
#define UART2_PC_CLEAR_EEP_DATA             0x05
#define UART2_PC_GET_EEP_NUM_DATA           0x06
#define UART2_PC_CHANGE_DATA_MODE           0x07
#define UART2_PC_GET_ALL_EEP_DATA           0x09





#define UART2_PC_HANDSHAKE_ACK              0x81
#define UART2_PC_READ_PASSWORD_ACK          0x82
#define UART2_PC_WRITE_PASSWORD_ACK         0x83
#define UART2_PC_READ_EEP_DATA_ACK          0x84
#define UART2_PC_CLEAR_EEP_DATA_ACK         0x85
#define UART2_PC_GET_EEP_NUM_DATA_ACK       0x86
#define UART2_PC_CHANGE_DATA_MODE_ACK       0x87
#define UART2_PC_GET_ALL_EEP_DATA_ACK       0x89









int16_t Unsigned_Six_To_Signed_Six(uint16_t data);
uint16_t Signed_Six_To_Unsigned_Six(int16_t data);
float Unsigned_Six_To_Float(uint32_t data);
uint32_t Float_To_Unsigned_Six(float data);




void Agreement_Ack_For_Uart1(uint8_t uart_id,uint8_t* data, uint16_t length);
void Agreement_Ack_For_Uart2(uint8_t uart_id,uint8_t* data, uint16_t length);

void Agreement_Ack_For_Fun(void);
/* [B2] Fresh if a correctly checked sensor frame was received in last 1.5s. */
#define SENSOR_LINK_TIMEOUT_MS 1500U /* [B2] Keep compatible with older 500ms sensor boards. */
uint8_t Sensor_Data_Is_Fresh(void);



#endif

