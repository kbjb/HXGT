#ifndef __APP_EEP_H
#define __APP_EEP_H

#include "main.h"
#include "param_head.h"

/* [B3.1] AT24C512 production layout (64 KiB), disjoint regions:
 * records 0x2000..0x25DB (100 x 15 bytes), calibration A/B 0xE000 / 0xE080,
 * record-count journal A/B 0xE100 / 0xE180 (64 bytes/slot).
 * Each A/B slot starts on a different physical 128-byte EEPROM page.
 */
#define EEP_MAX_ADDR                 65536UL
#define EEP_B3_RECORD_START          0x2000U
#define EEP_B3_CAL_SLOT_A            0xE000U
#define EEP_B3_CAL_SLOT_B            0xE080U
#define EEP_B3_NODE_SLOT_A           0xE100U
#define EEP_B3_NODE_SLOT_B           0xE180U
#define EEP_B3_SLOT_LEN              64U
#define MAX_INDEX_ADDR              100U
#define EEP_NODE_ENCODED_LEN         15U

/* Obsolete service switches MUST remain zero in the production firmware.
 * B3.1 intentionally has NO legacy decoding or migration implementation.
 * The factory-only initialization requires a full 64KiB erased check;
 * these two switches are NOT involved in initialization or saving. */
#define EEP_B3_SERVICE_ENABLE_LEGACY_CAL_UPGRADE 0
#define EEP_B3_SERVICE_IMPORT_LEGACY_COUNT       0
#if EEP_B3_SERVICE_ENABLE_LEGACY_CAL_UPGRADE || EEP_B3_SERVICE_IMPORT_LEGACY_COUNT
#error "B3.1 production firmware does not support legacy EEPROM migration"
#endif

/* Compile-time physical address and slot separation checks. */
#if (EEP_B3_RECORD_START + MAX_INDEX_ADDR * EEP_NODE_ENCODED_LEN) > EEP_B3_CAL_SLOT_A
#error "B3.1 record data overlaps calibration"
#endif
#if (EEP_B3_CAL_SLOT_A + EEP_B3_SLOT_LEN > EEP_B3_CAL_SLOT_B) || \
    (EEP_B3_CAL_SLOT_B + EEP_B3_SLOT_LEN > EEP_B3_NODE_SLOT_A) || \
    (EEP_B3_NODE_SLOT_A + EEP_B3_SLOT_LEN > EEP_B3_NODE_SLOT_B) || \
    (EEP_B3_NODE_SLOT_B + EEP_B3_SLOT_LEN > EEP_MAX_ADDR)
#error "B3.1 journal slots overlap or extend beyond EEPROM"
#endif

/* Existing numeric values are retained for Keil Watch compatibility. */
typedef enum
{
    EEP_STATE_NOT_READY = 0,
    EEP_STATE_B3_VALID = 1,
    EEP_STATE_NONBLANK_LOCKED = 2, /* no valid B3 cal; auto write refused */
    EEP_STATE_FACTORY_EMPTY = 3,  /* transient only during factory init */
    EEP_STATE_B3_CORRUPT = 4,
    EEP_STATE_IO_ERROR = 5
} EEP_STORAGE_STATE;

typedef enum
{
    EEP_RECORD_NOT_READY = 0,
    EEP_RECORD_B3_VALID = 1,
    EEP_RECORD_EMPTY = 2,
    EEP_RECORD_CORRUPT = 3,
    EEP_RECORD_IO_ERROR = 4
} EEP_RECORD_STATE;

/* Return 1 only if calibration is valid and records are usable.
 * Factory blank: checks EVERY byte is 0xFF and initializes calibration
 * into TWO CRC/commit-validated copies. Existing B3 snapshots are loaded
 * without writes. Nonblank with no valid calibration remains locked.
 */
uint8_t Eep_Data_Get(void);
EEP_STORAGE_STATE Eep_Get_Storage_State(void);
EEP_RECORD_STATE Eep_Get_Record_State(void);
uint8_t Eep_Get_Legacy_Warnings(void); /* always 0 in B3.1 */

/* These APIs stay as safe no-ops for source compatibility. */
uint8_t Eep_Approve_Legacy_Calibration_Upgrade(void);
uint8_t Eep_Approve_Legacy_Record_Import(void);
uint8_t Eep_Read_Legacy_Record(uint16_t index, void *node);
uint8_t Eep_Import_Legacy_Records(uint16_t verified_count);

/* User-confirmed calibration/password are persisted as whole snapshots.
 * Each function returns 1 only after write AND CRC/readback verification.
 */
uint8_t Eep_Save_CG_Calibration(CG_Calibration_Param_Str data);
uint8_t Eep_Save_GJ_Calibration(GJ_Calibration_Param_Str data);
uint8_t Eep_Save_CH_Calibration(CH_Calibration_Param_Str data);
uint8_t Eep_Save_Password(uint32_t password);

/* Obsolete single-field/raw writes are disabled in B3.1. */
uint8_t Eep_Write_For_Signed16(uint16_t addr, int16_t data);
uint8_t Eep_Write_For_Unsigned16(uint16_t addr, uint16_t data);
uint8_t Eep_Write_For_Float(uint16_t addr, float data);
uint8_t Eep_Write_For_Unsigned32(uint16_t addr, uint32_t data);

/* Records: append at index == current count, max 100; 0=success for
 * Eep_Input/Output_Node, 1=success for Eep_Clear_Node.
 * Clear is logical: new records overwrite old payloads from index 0.
 */
uint8_t Eep_Get_Node_Count(uint16_t *count);
uint8_t Eep_Input_Node(uint16_t index, EEP_NODE node);
uint8_t Eep_Output_Node(uint16_t index, void *node);
uint8_t Eep_Clear_Node(void);

#endif
