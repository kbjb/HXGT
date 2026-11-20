#ifndef __MAIN_H
#define __MAIN_H
/* [B2] HOST TEST ONLY. Not part of either MCU include search path. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef int GPIO_PinState;
typedef enum { HAL_OK=0, HAL_ERROR=1, HAL_BUSY=2, HAL_TIMEOUT=3 } HAL_StatusTypeDef;
typedef struct {
    void *Instance;
    uint8_t *rxbuf;
    uint8_t rx_armed;
    uint8_t idle_flag;
} UART_HandleTypeDef;
#define USART1 ((void *)(uintptr_t)1)
#define USART2 ((void *)(uintptr_t)2)
#define USART3 ((void *)(uintptr_t)3)
#define UART_IT_IDLE 1
#define UART_FLAG_IDLE 1
#define __HAL_UART_ENABLE_IT(h, it) ((void)(h), (void)(it))
#define __HAL_UART_GET_FLAG(h, flag) ((h)->idle_flag)
#define __HAL_UART_CLEAR_IDLEFLAG(h) ((h)->idle_flag=0)
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
uint32_t HAL_GetTick(void);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
#endif
