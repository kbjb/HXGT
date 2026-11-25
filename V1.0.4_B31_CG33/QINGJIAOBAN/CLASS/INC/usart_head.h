#ifndef __USART_HEAD_H
#define __USART_HEAD_H

#include "main_head.h"
#include "gpio_head.h"


#define USART_RX_LEN    64
#define USART_TX_LEN    64




typedef struct USART_CLASS_STR
{
    uint8_t             *rx_buffer;
    volatile uint8_t    rx_i;      /* [B1] Shared with USART ISR. */
    volatile uint8_t    rx_len;
    volatile uint8_t    rx_ok      :1;
    uint8_t             time_en    :1;
    uint8_t             time_top   :3;
    uint8_t             time_cnt   :3;
    uint32_t            boud;
    USART_TypeDef       *usart;
    
    void (*str_init)(struct USART_CLASS_STR*);
    void (*init)(struct USART_CLASS_STR*);
    void (*send_byte)(struct USART_CLASS_STR*,uint8_t);
    void (*send_string)(struct USART_CLASS_STR*,char*,uint8_t);
    void (*sysRec)(struct USART_CLASS_STR*);
    void (*iqrn)(struct USART_CLASS_STR*);
}USATR_STR;




void USART_STR_INIT(USATR_STR *usart_str);

#define USART_CLASS_CREATE(_usart,_rx_buffer,_boud)\
{\
    .usart                                          = _usart,                           \
    .rx_buffer                                      = _rx_buffer,                       \
    .boud                                           = _boud,                            \
    .rx_i                                           = 0,                                \
    .rx_len                                         = 0,                                \
    .time_top                                       = 0,                                \
    .time_en                                        = 0,                                \
    .time_cnt                                       = 0,                                \
    .str_init                                       = USART_STR_INIT,                   \
}







#define USART_STK_SIZE  120
#define USART_TASK_PRIO 10


extern USATR_STR     usart1_str;
extern USATR_STR     usart2_str;
extern USATR_STR     usart3_str;



void Usart_Task_Create(void);



















#endif

