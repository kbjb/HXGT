#ifndef __OLEDFONT_H
#define __OLEDFONT_H

#include <stdint.h>
#include <string.h>

// 字库结构体：每个汉字对应一个GBK码和32字节点阵数据
typedef struct {
    char* gbk_code;    // 汉字的GBK编码
    uint8_t bitmap[32];    // 16x16点阵数据(32字节)
} ChineseFont_t;



extern const uint8_t ascii_6x8[][6];
extern const uint8_t ascii_8x16[][16];
extern const uint8_t chinese_16x16[][32];
extern const ChineseFont_t ChineseLib[];
extern const uint16_t CHINESE_LIB_SIZE; // 字库中汉字总数
extern char logo_ico[];
extern char power_4_ico[];
extern char power_3_ico[];
extern char power_2_ico[];
extern char power_1_ico[];
extern char power_0_ico[];
extern char power_n_ico[];

// 根据GBK编码查找汉字索引
int16_t FindChineseIndex(char* gbk_code);

#endif

