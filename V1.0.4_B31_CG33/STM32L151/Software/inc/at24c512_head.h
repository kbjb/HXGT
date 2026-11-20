#ifndef __AT24C512_HEAD_H
#define __AT24C512_HEAD_H

#include "main.h"

/* [B3] AT24C512: 512 Kbit = 65536 bytes, 512 physical pages x 128 bytes.
 * Address pins A2/A1/A0 are assumed LOW: 7-bit address 0x50.
 * STM32 HAL expects the LEFT-SHIFTED 8-bit address (0xA0).
 * Verify the actual hardware pins and WP before enabling writes.
 */
#define AT24C512_ADDR             0xA0U
#define EEPROM_PAGE_SIZE          128U
#define EEPROM_MAX_SIZE           65536UL
#define EEPROM_IO_TIMEOUT_MS      100U
#define EEPROM_ACK_POLL_TRIES     10U

#define EEPROM_OK                 0U
#define EEPROM_ERROR              1U
#define EEPROM_TIMEOUT            2U
#define EEPROM_VERIFY_ERROR       3U
#define EEPROM_INVALID_ARG        4U

uint8_t AT24C512_Init(void);
uint8_t AT24C512_Check(void); /* [B3] Non-destructive I2C ACK probe */
uint8_t AT24C512_WriteByte(uint16_t addr, uint8_t data);
uint8_t AT24C512_ReadByte(uint16_t addr, uint8_t *data);
uint8_t AT24C512_WritePage(uint16_t page_num, uint8_t *data);
uint8_t AT24C512_ReadPage(uint16_t page_num, uint8_t *data);
uint8_t AT24C512_WriteBuffer(uint16_t addr, uint8_t *data, uint16_t len);
uint8_t AT24C512_ReadBuffer(uint16_t addr, uint8_t *data, uint16_t len);
uint8_t AT24C512_WriteCrossPages(uint16_t start_addr, uint8_t *data, uint16_t len);
void AT24C512_Delay(uint32_t delay);

#endif
