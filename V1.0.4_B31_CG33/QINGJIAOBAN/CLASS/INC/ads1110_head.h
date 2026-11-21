#ifndef __ADS1110_HEAD_H
#define __ADS1110_HEAD_H

#include "main_head.h"
#include "iic_head.h"

#define ADS1110_READ_REG          0x91U
#define ADS1110_WRITE_REG         0x90U
#define ADS1110_CONFIG_REG_DATA   0x0CU /* 15 SPS, continuous, PGA=1 */
#define ADS1110_POLL_MS           10U
#define ADS1110_STALE_MS          300U
/* The legacy gauge path emitted only 0..0x7FFF. This value invalidates
 * gauge alone on a paired RC2 mainboard; CG/temperature remain in frame. */
#define ADS1110_GAUGE_INVALID     0xFFFFU

typedef enum
{
    ADS1110_WAITING = 0,
    ADS1110_VALID,
    ADS1110_BUS_ERROR,
    ADS1110_CONFIG_ERROR,
    ADS1110_NEGATIVE,
    ADS1110_STALE
} ADS1110_STATUS;

typedef struct
{
    IIC_STR *iic_str;
    uint32_t last_poll_ms;
    uint32_t last_valid_ms;
    uint16_t filtered;
    uint8_t valid;
    uint8_t config;              /* Keil Watch: last received configuration */
    ADS1110_STATUS status;       /* Keil Watch: most recent acquisition state */
} ADS1110_STR;

void ADS1110_Init(ADS1110_STR *adc, uint32_t now_ms);
void ADS1110_Poll(ADS1110_STR *adc, uint32_t now_ms);
uint16_t ADS1110_WireValue(const ADS1110_STR *adc, uint32_t now_ms);

#define ADS1110_CLASS_STR(_iic) { .iic_str = (_iic) }
#define ADS1110_SCL_PORT GPIOB
#define ADS1110_SCL_PIN  GPIO_Pin_6
#define ADS1110_SDA_PORT GPIOB
#define ADS1110_SDA_PIN  GPIO_Pin_7

#endif
