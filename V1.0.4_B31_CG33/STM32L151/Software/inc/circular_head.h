#ifndef __CIRCULAR_HEAR_H
#define __CIRCULAR_HEAR_H

#include "main.h"
#include <stdint.h>

/* 环形缓冲区配置 */
#define CIRCULAR_BUFFER_SIZE 64  // 缓冲区大小，可根据需要修改

/* 环形缓冲区结构体 */
typedef struct {
    uint8_t buffer[CIRCULAR_BUFFER_SIZE];
    volatile uint16_t head;  // 写指针（新数据位置）
    volatile uint16_t tail;  // 读指针（旧数据位置）
} CircularBuffer;

/* 函数声明 */
void CircularBuffer_Init(CircularBuffer *cb);
int CircularBuffer_Write(CircularBuffer *cb, uint8_t data);
int CircularBuffer_Read(CircularBuffer *cb, uint8_t *data);
uint16_t CircularBuffer_Available(CircularBuffer *cb);
int CircularBuffer_IsEmpty(CircularBuffer *cb);
int CircularBuffer_IsFull(CircularBuffer *cb);


#endif

