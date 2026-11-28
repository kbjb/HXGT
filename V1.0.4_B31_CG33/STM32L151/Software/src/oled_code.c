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
static uint8_t oled_sent[OLED_PAGE_NUM][OLED_WIDTH];
static uint8_t cursor_page, cursor_column;
static uint8_t known_pages;
static uint8_t dirty_pages;
static uint8_t tx_page, tx_column, tx_length;
static uint8_t tx_data[OLED_WIDTH];
static uint32_t tx_tick;
enum { OLED_DMA_IDLE, OLED_DMA_ACTIVE, OLED_DMA_DONE, OLED_DMA_ERROR };
static volatile uint8_t dma_state;

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
    0xA4,        // Resume RAM display
    0xA6,        // 正常显示
    0x8D, 0x14,  // 启用电荷泵[6](@ref)
    0xAF         // 打开显示
};

static HAL_StatusTypeDef OLED_Transmit(uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef result;
    OLED_DC_CMD();
    OLED_CS_LOW();
    result = HAL_SPI_Transmit(&OLED_SPI_HANDLE, data, size, 5U);
    OLED_CS_HIGH();
    return result;
}

void OLED_SetCursor(uint8_t page, uint8_t col)
{
    cursor_page = page;
    cursor_column = col;
}

void OLED_WriteData(uint8_t data)
{
    if (cursor_page >= OLED_PAGE_NUM || cursor_column >= OLED_WIDTH) return;
    if (oled_buffer[cursor_page][cursor_column] != data)
    {
        oled_buffer[cursor_page][cursor_column] = data;
        dirty_pages |= (uint8_t)(1U << cursor_page);
    }
    cursor_column++;
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
        uint8_t command = oled_init_cmds[i];
        (void)OLED_Transmit(&command, 1U);
    }
    OLED_Clear();
    known_pages = 0U;
    dma_state = OLED_DMA_IDLE;
}

// 清屏
void OLED_Clear(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
    dirty_pages = (uint8_t)((1U << OLED_PAGE_NUM) - 1U);
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



















/* Called from foreground. Transmit one dirty page range per call. */
void OLED_Refresh(void)
{
    uint8_t page;
    if (dma_state == OLED_DMA_ACTIVE)
    {
        if ((uint32_t)(HAL_GetTick() - tx_tick) < 10U) return;
        (void)HAL_SPI_Abort(&OLED_SPI_HANDLE);
        OLED_CS_HIGH();
        dma_state = OLED_DMA_ERROR;
    }
    if (dma_state == OLED_DMA_DONE)
    {
        memcpy(&oled_sent[tx_page][tx_column], tx_data, tx_length);
        known_pages |= (uint8_t)(1U << tx_page);
        dma_state = OLED_DMA_IDLE;
    }
    if (dma_state == OLED_DMA_ERROR)
    {
        known_pages &= (uint8_t)~(1U << tx_page);
        dirty_pages |= (uint8_t)(1U << tx_page);
        dma_state = OLED_DMA_IDLE;
        return; /* Retry in a later pass. */
    }

    for (page = 0; page < OLED_PAGE_NUM; ++page)
    {
        uint8_t start = 0U, end = OLED_WIDTH;
        uint8_t address[3];
        uint8_t page_mask = (uint8_t)(1U << page);
        if (!(dirty_pages & page_mask)) continue;
        if (known_pages & page_mask)
        {
            while (start < end && oled_buffer[page][start] == oled_sent[page][start])
                start++;
            if (start == end)
            {
                dirty_pages &= (uint8_t)~page_mask;
                continue;
            }
            while (end > start && oled_buffer[page][end-1U] == oled_sent[page][end-1U])
                end--;
        }
        address[0] = (uint8_t)(0xB0U + page);
        address[1] = start & 0x0FU;
        address[2] = (uint8_t)(0x10U | (start >> 4));
        if (OLED_Transmit(address, sizeof(address)) != HAL_OK) return;

        dirty_pages &= (uint8_t)~page_mask;
        tx_page = page;
        tx_column = start;
        tx_length = end - start;
        memcpy(tx_data, &oled_buffer[page][start], tx_length);
        OLED_DC_DATA();
        OLED_CS_LOW();
        tx_tick = HAL_GetTick();
        dma_state = OLED_DMA_ACTIVE;
        if (HAL_SPI_Transmit_DMA(&OLED_SPI_HANDLE, tx_data, tx_length) != HAL_OK)
        {
            OLED_CS_HIGH();
            dma_state = OLED_DMA_ERROR;
        }
        return;
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2 && dma_state == OLED_DMA_ACTIVE)
    {
        OLED_CS_HIGH();
        dma_state = OLED_DMA_DONE;
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2 && dma_state == OLED_DMA_ACTIVE)
    {
        OLED_CS_HIGH();
        dma_state = OLED_DMA_ERROR;
    }
}
