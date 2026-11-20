#include "at24c512_head.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

/* [B3] All I2C transfers use finite timeouts, never HAL_MAX_DELAY.
 * AT24C512C supports 16-bit word addresses and 128-byte page writes.
 * HAL_I2C_Mem_{Read,Write} use a repeated START for random reads.
 */
static uint8_t Eep_Map_Status(HAL_StatusTypeDef status)
{
    if (status == HAL_OK) return EEPROM_OK;
    if (status == HAL_TIMEOUT) return EEPROM_TIMEOUT;
    return EEPROM_ERROR;
}

static uint8_t Eep_Check_Range(uint16_t addr, uint16_t len)
{
    return ((uint32_t)addr + (uint32_t)len <= EEPROM_MAX_SIZE) ? 1U : 0U;
}

static uint8_t Eep_Wait_Write_Complete(void)
{
    uint8_t i;
    /* [B3] ACK polling after STOP, not a blind fixed 5 ms sleep.
     * Each attempt is bounded and we also bound the number of retries. */
    for (i = 0; i < EEPROM_ACK_POLL_TRIES; ++i)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c1, AT24C512_ADDR, 1U, 2U) == HAL_OK)
            return EEPROM_OK;
        HAL_Delay(1U);
    }
    return EEPROM_TIMEOUT;
}

uint8_t AT24C512_Init(void)
{
    return AT24C512_Check();
}

uint8_t AT24C512_Check(void)
{
    /* [B3] OLD CODE WROTE 0x55 TO ADDRESS 0x0000, DESTROYING A RECORD.
     * The probe is now strictly read-only. A successful probe does NOT
     * demonstrate that WP is low; high-level writers read back data. */
    return Eep_Map_Status(HAL_I2C_IsDeviceReady(&hi2c1,
                      AT24C512_ADDR, 3U, EEPROM_IO_TIMEOUT_MS));
}

uint8_t AT24C512_WriteByte(uint16_t addr, uint8_t data)
{
    return AT24C512_WriteBuffer(addr, &data, 1U);
}

uint8_t AT24C512_ReadByte(uint16_t addr, uint8_t *data)
{
    return AT24C512_ReadBuffer(addr, data, 1U);
}

uint8_t AT24C512_WritePage(uint16_t page_num, uint8_t *data)
{
    if (page_num >= EEPROM_MAX_SIZE / EEPROM_PAGE_SIZE || data == NULL)
        return EEPROM_INVALID_ARG;
    return AT24C512_WriteBuffer((uint16_t)((uint32_t)page_num *
                      EEPROM_PAGE_SIZE), data, EEPROM_PAGE_SIZE);
}

uint8_t AT24C512_ReadPage(uint16_t page_num, uint8_t *data)
{
    if (page_num >= EEPROM_MAX_SIZE / EEPROM_PAGE_SIZE || data == NULL)
        return EEPROM_INVALID_ARG;
    return AT24C512_ReadBuffer((uint16_t)((uint32_t)page_num *
                      EEPROM_PAGE_SIZE), data, EEPROM_PAGE_SIZE);
}

uint8_t AT24C512_WriteBuffer(uint16_t addr, uint8_t *data, uint16_t len)
{
    uint32_t current = addr;
    uint16_t done = 0U;
    uint16_t n;
    uint32_t in_page;
    HAL_StatusTypeDef status;

    if (!Eep_Check_Range(addr, len) || (len != 0U && data == NULL))
        return EEPROM_INVALID_ARG;

    while (done < len)
    {
        /* [B3] Never cross a physical 128-byte write-page boundary
         * in the same I2C transaction: otherwise the chip wraps in-page. */
        in_page = current % EEPROM_PAGE_SIZE;
        n = (uint16_t)(EEPROM_PAGE_SIZE - in_page);
        if (n > (uint16_t)(len - done)) n = (uint16_t)(len - done);

        status = HAL_I2C_Mem_Write(&hi2c1, AT24C512_ADDR, (uint16_t)current,
                                  I2C_MEMADD_SIZE_16BIT, data + done, n,
                                  EEPROM_IO_TIMEOUT_MS);
        if (status != HAL_OK) return Eep_Map_Status(status);
        if (Eep_Wait_Write_Complete() != EEPROM_OK) return EEPROM_TIMEOUT;

        current += n;
        done = (uint16_t)(done + n);
    }
    return EEPROM_OK;
}

uint8_t AT24C512_ReadBuffer(uint16_t addr, uint8_t *data, uint16_t len)
{
    uint32_t current = addr;
    uint16_t done = 0U;
    uint16_t n;
    HAL_StatusTypeDef status;
    if (!Eep_Check_Range(addr, len) || (len != 0U && data == NULL))
        return EEPROM_INVALID_ARG;

    /* [B3] Limit each HAL read to 128 bytes for predictable I2C timing.
     * Sequential reads MAY cross pages; the controller advances across
     * pages correctly, unlike page writes. */
    while (done < len)
    {
        n = (uint16_t)(len - done);
        if (n > EEPROM_PAGE_SIZE) n = EEPROM_PAGE_SIZE;

        status = HAL_I2C_Mem_Read(&hi2c1, AT24C512_ADDR, (uint16_t)current,
                                 I2C_MEMADD_SIZE_16BIT, data + done, n,
                                 EEPROM_IO_TIMEOUT_MS);
        if (status != HAL_OK) return Eep_Map_Status(status);
        current += n;
        done = (uint16_t)(done + n);
    }
    return EEPROM_OK;
}

uint8_t AT24C512_WriteCrossPages(uint16_t start_addr,
                                 uint8_t *data, uint16_t len)
{
    return AT24C512_WriteBuffer(start_addr, data, len);
}

void AT24C512_Delay(uint32_t delay)
{
    HAL_Delay(delay);
}
