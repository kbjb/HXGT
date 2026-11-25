#ifndef __OLED_HEAD_H
#define __OLED_HEAD_H

#include "main.h"

#define OLED_SPI_HANDLE    hspi2
#define OLED_DMA_HANDLE    hdma_spi2_tx
#define OLED_WIDTH        128
#define OLED_HEIGHT       64
#define OLED_PAGE_NUM     (OLED_HEIGHT / 8)

// GPIO定义
#define OLED_CS_PORT      GPIOB
#define OLED_CS_PIN       GPIO_PIN_12
#define OLED_DC_PORT      GPIOB
#define OLED_DC_PIN       GPIO_PIN_9
#define OLED_RESET_PORT   GPIOB
#define OLED_RESET_PIN    GPIO_PIN_8

// 字体大小定义
#define FONT_6X8          0
#define FONT_8X16         1
#define FONT_CHINESE_16X16 2

// 函数声明
void OLED_Init(void);
void OLED_Clear(void);
void OLED_SetCursor(uint8_t page, uint8_t col);
void OLED_WriteCommand(uint8_t cmd);
void OLED_WriteData(uint8_t data);
void OLED_WriteData_DMA(uint8_t *data, uint16_t size);
void OLED_PrintChar(uint8_t x, uint8_t y, char ch, uint8_t font_size);
void OLED_PrintString(uint8_t x, uint8_t y, char *str, uint8_t font_size);
void OLED_PrintChinese(uint8_t x, uint8_t y, char* str);
//void OLED_ShowImage(const uint8_t *img);
void OLED_ShowImage(int16_t x, int16_t y, uint8_t width, uint8_t height, const uint8_t *img_data);
void OLED_Refresh(void);
uint8_t OLED_IsDMAReady(void);


void OLED_PrintLine(uint8_t start_x, uint8_t end_x,uint8_t y,uint8_t data);




extern SPI_HandleTypeDef hspi2;
extern DMA_HandleTypeDef hdma_spi2_tx;

#endif

