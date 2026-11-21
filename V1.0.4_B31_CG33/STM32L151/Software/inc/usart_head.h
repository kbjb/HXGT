#ifndef __USART_HEAD_H
#define __USART_HEAD_H


#include "main.h"
#include "agreement_head.h"

// 缓冲区大小定义
#define UART_RX_BUFFER_SIZE    256
#define UART_TX_BUFFER_SIZE    128


// 串口管理结构体
typedef struct {
    UART_HandleTypeDef *huart;
    
    // 接收缓冲区
    uint8_t  rx_buffer[UART_RX_BUFFER_SIZE];
    volatile uint16_t rx_index;
    uint16_t expected_length;              /* [B2] Protocol length, not idle gap. */
    uint8_t  rx_byte;                       /* [B2] HAL one-byte staging. */
    volatile uint8_t rx_armed;
    uint8_t  frame_buffer[UART_RX_BUFFER_SIZE]; /* [B2] Foreground mailbox. */
    volatile uint8_t frame_ready;
    volatile uint16_t frame_length;
    volatile uint32_t frame_rx_tick; /* Last byte of THIS accepted mailbox frame. */
    volatile uint32_t bad_frame_count;
    volatile uint32_t dropped_frame_count;
    volatile uint32_t error_count;
    
    // 发送缓冲区
    uint8_t  tx_buffer[UART_TX_BUFFER_SIZE];
    uint16_t tx_head;
    uint16_t tx_tail;
    uint8_t  tx_busy;
    
    // 回调函数
    void (*frame_received_callback)(uint8_t uart_id, uint8_t* data, uint16_t length);
} UART_Manager_t;

// 串口ID定义
typedef enum {
    UART1_ID = 0,
    UART2_ID,
    UART3_ID,
    UART_MAX
} UART_ID_t;

// 函数声明

void     UART_Init(void);
/* [B2] Run from main loop, NEVER from UART IRQ. */
void     UART_Process_Received(UART_ID_t uart_id);
uint32_t UART_Get_Bad_Frame_Count(UART_ID_t uart_id);
uint32_t UART_Get_Dropped_Frame_Count(UART_ID_t uart_id);
uint32_t UART_Get_Error_Count(UART_ID_t uart_id);
void     UART_Manager_Init(UART_ID_t usart_id);
void     UART_Start_Receive(UART_ID_t uart_id);
void     UART_Send_Data(UART_ID_t uart_id, uint8_t* data, uint16_t length);
uint8_t  UART_Try_Send_Data(UART_ID_t uart_id, uint8_t *data, uint16_t length);
uint8_t  UART_Is_Frame_Ready(UART_ID_t uart_id);
uint32_t UART_Get_Frame_Rx_Tick(UART_ID_t uart_id);
uint16_t UART_Get_Frame_Length(UART_ID_t uart_id);
uint8_t* UART_Get_Frame_Data(UART_ID_t uart_id);
void     UART_Clear_Frame_Flag(UART_ID_t uart_id);
void     UART_Register_Frame_Callback(UART_ID_t uart_id, void (*callback)(uint8_t, uint8_t*, uint16_t));

// 中断处理函数
void UART_IDLE_IRQHandler(UART_HandleTypeDef *huart);

#endif


