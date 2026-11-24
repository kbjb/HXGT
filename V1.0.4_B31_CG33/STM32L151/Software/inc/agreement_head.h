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
#define SENSOR_LINK_TIMEOUT_MS 1500U
uint8_t Sensor_Data_Is_Fresh(void);

/* [CG36] Ten nominal 100 ms reports. Allow timestamp jitter around the
 * nominal 900 ms span; reject movement by range AND half-window mean drift. */
#define CG_CAL_STABLE_SAMPLES      10U
#define CG_CAL_STABLE_TIME_MS      800U
#define CG_CAL_MAX_GAP_MS          250U
#define CG_CAL_STABLE_RANGE_AD     6L
#define CG_CAL_MAX_MEAN_DRIFT_AD   2L
typedef enum
{
    CG_CAL_CAPTURE_READY = 0,
    CG_CAL_WAIT_DATA,
    CG_CAL_WAIT_LINK,
    CG_CAL_WAIT_TIME,
    CG_CAL_WAIT_NOISE,
    CG_CAL_WAIT_DRIFT
} CG_CAL_WAIT_STATE;
void Sensor_CG_Stability_Reset(void);
uint8_t Sensor_Get_Stable_CG(int16_t *value);
extern volatile int32_t CG_Calibration_Range_AD;
/* Ceil of the absolute difference between the two five-frame means. */
extern volatile int32_t CG_Calibration_Trend_AD;
extern volatile CG_CAL_WAIT_STATE CG_Calibration_Wait_State;




/* [CG38] Calibration-only window of the already filtered displacement AD.
 * Thresholds are in mm using the actual gauge scale; not CG deadband counts.
 * Provisional TEST limits; validate noise/repeatability on the actual gauge. */
#define CH_CAL_STABLE_SAMPLES       10U
#define CH_CAL_STABLE_TIME_MS       800U
#define CH_CAL_MAX_GAP_MS           250U
#define CH_CAL_STABLE_RANGE_MM      0.03f
#define CH_CAL_MAX_MEAN_DRIFT_MM    0.01f
typedef enum
{
    CH_CAL_CAPTURE_READY = 0,
    CH_CAL_WAIT_DATA,
    CH_CAL_WAIT_LINK,
    CH_CAL_WAIT_TIME,
    CH_CAL_WAIT_NOISE,
    CH_CAL_WAIT_DRIFT,
    CH_CAL_WAIT_SCALE
} CH_CAL_WAIT_STATE;
void Sensor_GJ_Stability_Reset(void);
uint8_t Sensor_Get_Stable_GJ(uint16_t *value, float mm_per_ad);
extern volatile uint32_t CH_Calibration_Range_AD;
extern volatile float CH_Calibration_Range_MM;
extern volatile float CH_Calibration_Trend_MM;
extern volatile CH_CAL_WAIT_STATE CH_Calibration_Wait_State;

#endif

