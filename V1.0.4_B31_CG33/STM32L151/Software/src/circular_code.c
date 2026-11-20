#include "circular_head.h"


/**
  * @brief  环形缓冲区初始化
  * @param  cb: 环形缓冲区指针
  * @retval 无
  */
void CircularBuffer_Init(CircularBuffer *cb) {
    cb->head = 0;
    cb->tail = 0;
}

/**
  * @brief  向环形缓冲区写入数据
  * @param  cb: 环形缓冲区指针
  * @param  data: 要写入的数据
  * @retval 1: 成功, 0: 缓冲区已满
  */
int CircularBuffer_Write(CircularBuffer *cb, uint8_t data) {
    uint16_t next_head = (cb->head + 1) % CIRCULAR_BUFFER_SIZE;
    
    if (next_head == cb->tail) 
        return 0;  // 缓冲区已满
    
    cb->buffer[cb->head] = data;
    cb->head = next_head;
    return 1;
}

/**
  * @brief  从环形缓冲区读取数据
  * @param  cb: 环形缓冲区指针
  * @param  data: 存储读取数据的指针
  * @retval 1: 成功, 0: 缓冲区为空
  */
int CircularBuffer_Read(CircularBuffer *cb, uint8_t *data) {
    if (cb->head == cb->tail) 
        return 0;  // 缓冲区为空
    
    *data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % CIRCULAR_BUFFER_SIZE;
    return 1;
}

/**
  * @brief  获取缓冲区中可读数据量
  * @param  cb: 环形缓冲区指针
  * @retval 可读数据字节数
  */
uint16_t CircularBuffer_Available(CircularBuffer *cb) {
    return (cb->head - cb->tail + CIRCULAR_BUFFER_SIZE) % CIRCULAR_BUFFER_SIZE;
}

/**
  * @brief  检查缓冲区是否为空
  * @param  cb: 环形缓冲区指针
  * @retval 1: 空, 0: 非空
  */
int CircularBuffer_IsEmpty(CircularBuffer *cb) {
    return (cb->head == cb->tail);
}

/**
  * @brief  检查缓冲区是否已满
  * @param  cb: 环形缓冲区指针
  * @retval 1: 满, 0: 未满
  */
int CircularBuffer_IsFull(CircularBuffer *cb) {
    return ((cb->head + 1) % CIRCULAR_BUFFER_SIZE == cb->tail);
}




