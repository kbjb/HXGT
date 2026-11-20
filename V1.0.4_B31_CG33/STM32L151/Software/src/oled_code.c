#include "oled_head.h"
#include "oledfont.h"



// GPIO控制宏
#define OLED_CS_LOW()     HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_RESET)
#define OLED_CS_HIGH()    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET)
#define OLED_DC_CMD()     HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_RESET)
#define OLED_DC_DATA()    HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_SET)
#define OLED_RESET_LOW()  HAL_GPIO_WritePin(OLED_RESET_PORT, OLED_RESET_PIN, GPIO_PIN_RESET)
#define OLED_RESET_HIGH() HAL_GPIO_WritePin(OLED_RESET_PORT, OLED_RESET_PIN, GPIO_PIN_SET)

// 显示缓冲区
static uint8_t oled_buffer[OLED_PAGE_NUM][OLED_WIDTH];
static volatile uint8_t dma_ready = 1;

// SSD1309初始化命令序列[6](@ref)
const uint8_t oled_init_cmds[] = {
    0xAE,        // 关闭显示
    0xD5, 0x80,  // 设置时钟分频
    0xA8, 0x3F,  // 设置复用率(1/64)
    0xD3, 0x00,  // 设置显示偏移
    0x40,        // 设置起始行
    0xA1,        // 段重映射(水平翻转)
    0xC8,        // 扫描方向重映射(垂直翻转)
    0xDA, 0x12,  // 设置COM硬件配置
    0x81, 0x15,  // 设置对比度
    0xA4,        // 全部像素点亮
    0xA6,        // 正常显示
    0x8D, 0x14,  // 启用电荷泵[6](@ref)
    0xAF         // 打开显示
};

// 发送命令[5](@ref)
void OLED_WriteCommand(uint8_t cmd) {
    while(!dma_ready);  // 等待DMA就绪
    OLED_DC_CMD();
    OLED_CS_LOW();
    HAL_SPI_Transmit(&OLED_SPI_HANDLE, &cmd, 1, 100);
    OLED_CS_HIGH();
}

// 发送数据(阻塞)
void OLED_WriteData(uint8_t data) {
    OLED_DC_DATA();
    OLED_CS_LOW();
    HAL_SPI_Transmit(&OLED_SPI_HANDLE, &data, 1, 100);
    OLED_CS_HIGH();
}

// DMA发送数据[1](@ref)
void OLED_WriteData_DMA(uint8_t *data, uint16_t size) {
    if(size == 0 || !dma_ready) return;
    
    dma_ready = 0;
    OLED_DC_DATA();
    OLED_CS_LOW();
    HAL_SPI_Transmit_DMA(&OLED_SPI_HANDLE, data, size);
}

// 设置光标位置
void OLED_SetCursor(uint8_t page, uint8_t col) {
    OLED_WriteCommand(0xB0 + page);      // 设置页地址
    OLED_WriteCommand(col & 0x0F);       // 设置列低地址
    OLED_WriteCommand(0x10 + (col >> 4)); // 设置列高地址
}

// 初始化OLED[6](@ref)
void OLED_Init(void) {
    // 硬件复位
    OLED_RESET_LOW();
    HAL_Delay(10);
    OLED_RESET_HIGH();
    HAL_Delay(10);
    
    // 发送初始化命令序列
    for(uint8_t i = 0; i < sizeof(oled_init_cmds); i++) {
        OLED_WriteCommand(oled_init_cmds[i]);
    }
    OLED_Clear();
    dma_ready = 1;
}

// 清屏
void OLED_Clear(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
    OLED_Refresh();
}

// 显示字符[6](@ref)
void OLED_PrintChar(uint8_t x, uint8_t y, char ch, uint8_t font_size) {
    if(ch < 32 || ch > 126) return;
    
    uint16_t char_index = ch - 32;
    
    switch(font_size) {
        case FONT_6X8:
            if(x > OLED_WIDTH - 6 || y > OLED_PAGE_NUM - 1) return;
            OLED_SetCursor(y, x);
            for(uint8_t i = 0; i < 6; i++) {
                OLED_WriteData(ascii_6x8[char_index][i]);
            }
            break;
            
        case FONT_8X16:
            if(x > OLED_WIDTH - 8 || y > OLED_PAGE_NUM - 2) return;
            // 上半部分
            OLED_SetCursor(y, x);
            for(uint8_t i = 0; i < 8; i++) {
                OLED_WriteData(ascii_8x16[char_index][i]);
            }
            // 下半部分
            OLED_SetCursor(y + 1, x);
            for(uint8_t i = 8; i < 16; i++) {
                OLED_WriteData(ascii_8x16[char_index][i]);
            }
            break;
    }
}

void OLED_PrintLine(uint8_t start_x, uint8_t end_x,uint8_t y,uint8_t data)
{
    OLED_SetCursor(y, start_x);
    
    for(uint8_t i = 0; i < (end_x-start_x); i++) {
        OLED_WriteData(data);
    }
}

// 显示字符串[6](@ref)
void OLED_PrintString(uint8_t x, uint8_t y, char *str, uint8_t font_size) {
    uint8_t char_width = (font_size == FONT_6X8) ? 6 : 8;
    
    while(*str) {
        OLED_PrintChar(x, y, *str, font_size);
        x += char_width;
        
        // 换行处理
        if(x >= OLED_WIDTH - char_width) {
            x = 0;
            y += (font_size == FONT_6X8) ? 1 : 2;
            if(y >= OLED_PAGE_NUM) break;
        }
        str++;
    }
}

void OLED_PrintChinese(uint8_t x, uint8_t y, char* str) {
    uint8_t current_x = x;
    uint8_t current_y = y;
//    uint16_t gbk_code = 0;
    int16_t font_index = -1;

    // 遍历字符串
    while (*str != '\0') {
        // 检查是否为汉字首字节(GBK汉字首字节 > 0xA0)
        if ((uint8_t)(*str) > 0xA0) {
            // 组合GBK编码: 高8位为第一个字节，低8位为第二个字节
            //gbk_code = ((uint16_t)((uint8_t)*str) << 8) | (uint8_t)(*(str + 1));
            
            // 在字库中查找对应汉字
            font_index = FindChineseIndex(str);
            
            if (font_index >= 0) {
                // 显示找到的汉字(分上下两页写入)
                OLED_SetCursor(current_y, current_x);
                for (uint8_t t = 0; t < 16; t++) {
                    OLED_WriteData(ChineseLib[font_index].bitmap[t]); // 上半部分
                }
                
                OLED_SetCursor(current_y + 1, current_x);
                for (uint8_t t = 16; t < 32; t++) {
                    OLED_WriteData(ChineseLib[font_index].bitmap[t]); // 下半部分
                }
                
                // 下一个汉字位置(右移16像素)
                current_x += 16;
                
                // 跳过当前汉字第二个字节
                str += 2;
            } else {
                // 字库中未找到的汉字，跳过但保留位置
                current_x += 16;
                str += 2;
            }
        } else {
            // 非汉字字符(如ASCII)，可以选择跳过或用其他方式处理
            str++; // 只跳过一个字节
        }

        // 换行检查：如果当前行显示不下，换到下一行
        if (current_x > OLED_WIDTH - 16) {
            current_x = 0; // 回到最左侧
            current_y += 2; // 下移2页(16像素)
            
            // 超出屏幕底部则停止显示
            if (current_y > OLED_PAGE_NUM - 2) {
                break;
            }
        }
    }
}

//// 显示图片(全屏64x128)[6](@ref)
//void OLED_ShowImage(const uint8_t *img) {
//    for(uint8_t page = 0; page < OLED_PAGE_NUM; page++) {
//        OLED_SetCursor(page, 0);
//        for(uint16_t col = 0; col < OLED_WIDTH; col++) {
//            OLED_WriteData(img[page * OLED_WIDTH + col]);
//        }
//    }
//}

//void OLED_ShowImage(int16_t x, int16_t y, uint8_t width, uint8_t height, const uint8_t *img_data) {
//    // 1. 基础参数检查
//    if (x >= OLED_WIDTH || y >= OLED_HEIGHT || width == 0 || height == 0 || img_data == NULL) {
//        return;
//    }

//    // 2. 动态裁剪：计算图片在屏幕内的实际显示区域
//    int16_t display_x = (x < 0) ? 0 : x; // 显示起始X坐标
//    int16_t display_y = (y < 0) ? 0 : y; // 显示起始Y坐标

//    // 计算实际显示的宽度和高度（防止越界）
//    int16_t display_width = width;
//    int16_t display_height = height;

//    if (x < 0)
//        display_width += x; // 如果x为负，宽度要减去超出部分
//    if (y < 0)
//        display_height += y; // 如果y为负，高度要减去超出部分
//    if (display_x + display_width > OLED_WIDTH)
//        display_width = OLED_WIDTH - display_x; // 防止右侧超出屏幕
//    if (display_y + display_height > OLED_HEIGHT)
//        display_height = OLED_HEIGHT - display_y; // 防止底部超出屏幕

//    // 如果裁剪后无需显示，则直接返回
//    if (display_width <= 0 || display_height <= 0)
//        return;

//    // 3. 计算源图片数据的起始偏移量（如果因裁剪导致）
//    uint32_t src_index = 0;
//    if (x < 0)
//        src_index -= x; // X方向偏移：跳过左侧超出部分的数据
//    if (y < 0)
//        src_index -= (y * width); // Y方向偏移：跳过顶部超出部分的数据行

//    // 4. 计算涉及的起始页和结束页（基于显示区域的Y坐标）
//    uint8_t start_page = display_y / 8;     // 起始页
//    uint8_t end_page = (display_y + display_height - 1) / 8; // 结束页
////    uint8_t total_pages = end_page - start_page + 1;

//    // 5. 核心循环：逐页更新显示缓冲区
//    for (uint8_t page = start_page; page <= end_page; page++) {
//        // 计算当前页在屏幕上的像素Y范围
//        uint8_t page_pixel_start_y = page * 8;
//        uint8_t page_pixel_end_y = page_pixel_start_y + 7;

//        // 计算当前页需要显示图片数据的行范围（在当前页内的局部偏移）
//        uint8_t page_data_start_row = (display_y > page_pixel_start_y) ? (display_y - page_pixel_start_y) : 0;
//        uint8_t page_data_end_row = 7;
//        if ((display_y + display_height - 1) < page_pixel_end_y) {
//            page_data_end_row = (display_y + display_height - 1) - page_pixel_start_y;
//        }

//        // 设置OLED光标到当前页的起始显示列
//        OLED_SetCursor(page, display_x);

//        // 遍历当前页的每一列（X方向）
//        for (int16_t col = 0; col < display_width; col++) {
//            uint8_t display_byte = 0x00; // 准备写入到当前（页，列）的最终数据字节

//            // 遍历当前页中需要被图片填充的行（Y方向，8个像素）
//            for (int8_t bit_row = page_data_start_row; bit_row <= page_data_end_row; bit_row++) {
//                // 计算当前像素点在原始图片数据中的位置
//                uint32_t current_src_index = src_index + col + (bit_row + (page - start_page) * 8) * width;

//                // 安全检查：确保不读取超出图片数组范围的数据[1](@ref)
//                if (current_src_index >= (uint32_t)(width * height)) {
//                    continue; // 如果超出，跳过该像素
//                }

//                // 从图片数据中获取当前像素点的状态（1或0）
//                uint8_t pixel_value = (img_data[current_src_index / 8] >> (current_src_index % 8)) & 0x01;

//                // 根据取模方式（阴码/阳码）和字节顺序，将像素点设置到目标字节的相应位
//                // 假设阴码（1点亮像素），且目标屏幕字节的位0对应顶部像素（可能需要调整位移方向）
//                // 例如，如果SSD1309是位0对应最上方像素，而你的取模软件是位0对应最下方，则需要反转 bit_row
//                // 这里假设取模方式为“低位在前”，且与屏幕页模式匹配。根据实际情况调整 (7 - bit_row) 部分。
//                display_byte |= (pixel_value << bit_row);
//            }

//            // 将组合好的一个数据字节写入OLED（或先存入缓冲区，最后统一刷新）
//            OLED_WriteData(display_byte);
//        }
//    }

//    // 6. 如果您使用显存缓冲区，记得在此处调用刷新函数（如OLED_Refresh()）
//    // OLED_Refresh();
//}






void OLED_ShowImage(int16_t x, int16_t y, uint8_t width, uint8_t height, const uint8_t *img_data) {
    // 1. 基础参数检查
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT || width == 0 || height == 0 || img_data == NULL) {
        return;
    }

    // 2. 计算实际显示区域（裁剪）
    int16_t display_x, display_y;
    uint16_t display_width, display_height;
    uint32_t src_x_offset = 0, src_y_offset = 0;

    // 处理X方向裁剪
    if (x < 0) {
        display_x = 0;
        src_x_offset = -x;  // 跳过左侧被裁剪的部分
        display_width = (width + x > OLED_WIDTH) ? OLED_WIDTH : (width + x);
    } else {
        display_x = x;
        display_width = (x + width > OLED_WIDTH) ? (OLED_WIDTH - x) : width;
    }

    // 处理Y方向裁剪
    if (y < 0) {
        display_y = 0;
        src_y_offset = -y;  // 跳过顶部被裁剪的部分
        display_height = (height + y > OLED_HEIGHT) ? OLED_HEIGHT : (height + y);
    } else {
        display_y = y;
        display_height = (y + height > OLED_HEIGHT) ? (OLED_HEIGHT - y) : height;
    }

    // 检查是否需要显示
    if (display_width <= 0 || display_height <= 0) {
        return;
    }

    // 3. 计算字节宽度（每列需要多少字节）
    uint8_t bytes_per_column = (height + 7) / 8;  // 向上取整

    // 4. 核心显示循环
    for (uint16_t col = 0; col < display_width; col++) {
        // 计算当前列在原始图片中的列索引
        uint16_t src_col = col + src_x_offset;
        
        // 获取当前页范围
        uint8_t start_page = display_y / 8;
        uint8_t end_page = (display_y + display_height - 1) / 8;
        
        for (uint8_t page = start_page; page <= end_page; page++) {
            // 计算当前页在屏幕上的Y坐标范围
            uint8_t page_y_start = page * 8;
            uint8_t page_y_end = page_y_start + 7;
            
            // 计算当前页在显示区域内的Y范围
            uint8_t display_y_start = (display_y > page_y_start) ? display_y : page_y_start;
            uint8_t display_y_end = (display_y + display_height - 1 < page_y_end) ? 
                                    (display_y + display_height - 1) : page_y_end;
            
            // 初始化当前页的显示数据字节
            uint8_t display_byte = 0x00;
            
            // 遍历当前页的8个像素行
            for (uint8_t row_in_page = 0; row_in_page < 8; row_in_page++) {
                uint8_t screen_y = page_y_start + row_in_page;
                
                // 检查当前像素是否在显示区域内
                if (screen_y < display_y_start || screen_y > display_y_end) {
                    continue;  // 跳过不在显示区域内的像素
                }
                
                // 计算当前像素在原始图片中的Y坐标
                uint16_t src_y = (screen_y - display_y) + src_y_offset;
                
                // 安全检查
                if (src_y >= height) {
                    continue;
                }
                
                // 计算在图片数据中的位置
                // 假设图片数据是按列存储的，每列8个像素（1字节）为一组
                uint32_t byte_index = (src_y / 8) * width + src_col;
                uint8_t bit_pos = src_y % 8;  // 位位置（0=LSB, 7=MSB）
                
                // 安全检查
                if (byte_index >= (uint32_t)width * bytes_per_column) {
                    continue;
                }
                
                // 获取像素值并设置到对应位
                // 假设：1=点亮，0=熄灭（阴码显示）
                uint8_t pixel_value = (img_data[byte_index] >> bit_pos) & 0x01;
                
                // 将像素值设置到显示字节的对应位
                // 注意：OLED通常位0对应页面顶部，位7对应页面底部
                display_byte |= (pixel_value << row_in_page);
            }
            
            // 设置光标位置并写入数据
            OLED_SetCursor(page, display_x + col);
            OLED_WriteData(display_byte);
        }
    }
    
    // 5. 如果有刷新函数，在此处调用
    // OLED_Refresh();
}



















// 刷新整个显示缓冲区(使用DMA)
void OLED_Refresh(void) {
    if(!dma_ready) return;
    
    for(uint8_t page = 0; page < OLED_PAGE_NUM; page++) {
        OLED_SetCursor(page, 0);
        OLED_WriteData_DMA(oled_buffer[page], OLED_WIDTH);
        while(!dma_ready);  // 等待DMA完成
    }
}

// 检查DMA状态
uint8_t OLED_IsDMAReady(void) {
    return dma_ready;
}

// DMA传输完成回调函数[1](@ref)
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if(hspi->Instance == SPI2) {
        OLED_CS_HIGH();
        dma_ready = 1;
    }
}




