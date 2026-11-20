#ifndef __SSD1309_SPI_H
#define __SSD1309_SPI_H

#include "main_head.h"
#include "gpio_head.h"
#include "spi_head.h"

#define OLED_CMD  0 //Ð´ÃüÁî
#define OLED_DATA 1 //Ð´Êý¾Ý

typedef struct SSD1309_SPI_CLASS_STR
{
    SPI_STR  *spi;
    GPIO_STR *res; 
    GPIO_STR *dc;
    uint8_t   high;
    uint8_t   width;
    
    void      (*wr_byte)(struct SSD1309_SPI_CLASS_STR*,uint8_t,uint8_t);
    void      (*color_turn)(struct SSD1309_SPI_CLASS_STR*,uint8_t);
    void      (*diaplay_turn)(struct SSD1309_SPI_CLASS_STR*,uint8_t);
    void      (*display_on)(struct SSD1309_SPI_CLASS_STR*);
    void      (*display_off)(struct SSD1309_SPI_CLASS_STR*);
    void      (*refresh)(struct SSD1309_SPI_CLASS_STR*);
    void      (*clear)(struct SSD1309_SPI_CLASS_STR*);
    void      (*draw_point)(struct SSD1309_SPI_CLASS_STR*,uint8_t,uint8_t,uint8_t);
    void      (*draw_line)(struct SSD1309_SPI_CLASS_STR*,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t);
    void      (*draw_circle)(struct SSD1309_SPI_CLASS_STR*,uint8_t,uint8_t,uint8_t);
    void      (*show_char)(struct SSD1309_SPI_CLASS_STR*,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t); 
    void      (*show_str)(struct SSD1309_SPI_CLASS_STR*,uint8_t,uint8_t,uint8_t*,uint8_t,uint8_t);
    void      (*show_num)(struct SSD1309_SPI_CLASS_STR*,uint8_t,uint8_t,uint32_t,uint8_t,uint8_t,uint8_t,char);
    void      (*show_chinese)(struct SSD1309_SPI_CLASS_STR*,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t);
    void      (*show_pic)(struct SSD1309_SPI_CLASS_STR*,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t*,uint8_t);
    void      (*init)(struct SSD1309_SPI_CLASS_STR*);
    void      (*str_init)(struct SSD1309_SPI_CLASS_STR*);
}SSD1309_SPI_STR;



 void SSD1309_SPI_STR_INIT(SSD1309_SPI_STR *ssd1309_str);



#define SSD1309_SPI_CLASS_CREATE(_spi,_res,_dc,_high,_width)\
{\
    .spi      = _spi,                  \
    .res      = _res,                  \
    .dc       = _dc,                   \
    .high     = _high,                 \
    .width    = _width,                \
    .str_init = SSD1309_SPI_STR_INIT,  \
}\



#define OLED_TASK_PRIO    8
#define OLED_STK_SIZE     120






 
void Oled_Task_Create(void);



/**********************************USER*********************************/


#define OLED_CS_PORT      GPIOB
#define OLED_CS_PIN       GPIO_Pin_12 
#define OLED_SCLK_PORT    GPIOB
#define OLED_SCLK_PIN     GPIO_Pin_13
#define OLED_SDIN_PORT    GPIOB
#define OLED_SDIN_PIN     GPIO_Pin_15
#define OLED_DC_PORT      GPIOB
#define OLED_DC_PIN       GPIO_Pin_9
#define OLED_RES_PORT     GPIOB
#define OLED_RES_PIN      GPIO_Pin_8

























#endif










