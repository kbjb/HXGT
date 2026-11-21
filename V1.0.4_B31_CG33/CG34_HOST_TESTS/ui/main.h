#ifndef MAIN_H
#define MAIN_H
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
typedef int GPIO_PinState;
typedef int I2C_HandleTypeDef;
typedef int DMA_HandleTypeDef;
typedef int SPI_HandleTypeDef;
#define Power_Ctrl_GPIO_Port 0
#define Power_Ctrl_Pin 0
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
#define HAL_OK 0
void HAL_GPIO_WritePin(int port, int pin, int level);
uint32_t HAL_GetTick(void);
#endif
