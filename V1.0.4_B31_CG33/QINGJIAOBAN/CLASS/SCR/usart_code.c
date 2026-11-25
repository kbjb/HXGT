#include "usart_head.h"



void USART_INIT(USATR_STR *usart_str)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    if(usart_str->usart == USART1)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);
        
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        
        NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    }
    else if(usart_str->usart == USART2)
    {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);    //使能USART2，GPIOA时钟
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
        
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        
        NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    }
    else if(usart_str->usart == USART3)
    {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);    //使能USART2，GPIOA时钟
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
        
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        
        NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    }
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=5 ;    //抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;          //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;            //IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);    //根据指定的参数初始化VIC寄存器
  
   //USART 初始化设置

    USART_InitStructure.USART_BaudRate = usart_str->boud;                               //串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                         //字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                              //一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;                                 //无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;     //无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                     //收发模式

    USART_Init(usart_str->usart, &USART_InitStructure);                                           //初始化串口
    USART_ITConfig(usart_str->usart, USART_IT_RXNE, ENABLE);                                      //开启串口接受中断
    USART_Cmd(usart_str->usart, ENABLE);                                                          //使能串口
}

void USART_SEND_BYTE(USATR_STR *usart_str,uint8_t byte)
{
    while(USART_GetFlagStatus(usart_str->usart,USART_FLAG_TXE) == RESET);
    USART_SendData(usart_str->usart,byte);
}

void USART_SEND_STRING(USATR_STR *usart_str,char*charp,uint8_t len)
{
    for(uint8_t i=0;i<len;i++)
        usart_str->send_byte(usart_str,charp[i]);
    while(USART_GetFlagStatus(usart_str->usart,USART_FLAG_TC) == RESET);
}





void USART_IQRN(USATR_STR *usart_str) 
{
    uint8_t c;
    if (USART_GetITStatus(usart_str->usart, USART_IT_RXNE) != RESET) 
    {
        usart_str->time_en = 1;
        usart_str->time_cnt = 0;
        if(usart_str->rx_i<64)
        {
            c = USART_ReceiveData(usart_str->usart);
            usart_str->rx_buffer[usart_str->rx_i] = c;
            usart_str->rx_i++;
        }
        USART_ClearFlag(usart_str->usart, USART_IT_RXNE);
        
    }
}

void USART_SYSTIC_RECEIVE(USATR_STR *usart_str)
{
    if(usart_str->time_en == 1)
    {
        usart_str->time_cnt++;
        if(usart_str->time_cnt>5)
        {
            usart_str->rx_ok    = 1;
            usart_str->time_cnt = 0;
            usart_str->rx_len   = usart_str->rx_i;
            usart_str->rx_i     = 0;
            usart_str->time_en  = 0;
        }
    }
}

void USART_STR_INIT(USATR_STR *usart_str)
{
    usart_str->init = USART_INIT;
    usart_str->send_byte = USART_SEND_BYTE;
    usart_str->send_string = USART_SEND_STRING;
    usart_str->sysRec = USART_SYSTIC_RECEIVE;
    usart_str->iqrn = USART_IQRN;
    usart_str->init(usart_str);
    
}

///*********************************************************OS*********************************************************/





void USART1_IRQHandler(void) 
{
    usart1_str.iqrn(&usart1_str);
}
void USART2_IRQHandler(void)
{
    usart2_str.iqrn(&usart2_str);
}
void USART3_IRQHandler(void) 
{
    usart3_str.iqrn(&usart3_str);
}


