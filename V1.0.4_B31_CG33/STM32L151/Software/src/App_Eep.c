#include "App_Eep.h"
#include "at24c512_head.h"
#include "cg_cal_guard.h" /* [CG32-03] stricter check for NEW CG saves only */
#include <string.h>

/* [B3] On-disk snapshot format (64 bytes):
 * [0..3] magic ("GJB3" or "RCB3"), [4] version=3, [5] reserved,
 * [6..9] sequence (BE), [10..60] typed payload + 0xFF pad,
 * [61..62] CRC16-CCITT(FALSE) over bytes [0..60],
 * [63] commit marker 0xA5 (written last).
 *
 * The A and B copies live in separate physical 128-byte pages.
 * A partially written slot is rejected; an older valid copy remains.
 */
#define B3_VERSION         3U
#define B3_COMMIT          0xA5U
#define B3_CRC_OFFSET      61U
#define B3_MARKER_OFFSET   63U
#define B3_MAGIC_CAL0      'G'
#define B3_MAGIC_CAL1      'J'
#define B3_MAGIC_CAL2      'B'
#define B3_MAGIC_CAL3      '3'
#define B3_MAGIC_REC0      'R'
#define B3_MAGIC_REC1      'C'
#define B3_MAGIC_REC2      'B'
#define B3_MAGIC_REC3      '3'

typedef char EEP_NODE_Must_Be_15_Bytes[(sizeof(EEP_NODE) == EEP_NODE_ENCODED_LEN) ? 1 : -1];
typedef char EEP_Float_Must_Be_32_Bit[(sizeof(float) == 4U) ? 1 : -1];

typedef struct
{
    uint32_t password;
    CG_Calibration_Param_Str cg;
    GJ_Calibration_Param_Str gj;
    CH_Calibration_Param_Str ch;
} B3_CAL_DATA;

static EEP_STORAGE_STATE cal_state = EEP_STATE_NOT_READY;
static EEP_RECORD_STATE record_state = EEP_RECORD_NOT_READY;
static int8_t cal_active_slot = -1;
static int8_t record_active_slot = -1;
static uint32_t cal_sequence = 0U;
static uint32_t record_sequence = 0U;
static uint16_t record_count = 0U;

static uint16_t B3_Crc16(const uint8_t *data, uint8_t length)
{
    uint16_t crc = 0xFFFFU;
    uint8_t i, bit;
    for (i = 0U; i < length; ++i)
    {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (bit = 0U; bit < 8U; ++bit)
            crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U)
                                  : (uint16_t)(crc << 1);
    }
    return crc;
}

static void Put16(uint8_t *dest, uint16_t value)
{
    dest[0] = (uint8_t)(value >> 8);
    dest[1] = (uint8_t)value;
}

static uint16_t Get16(const uint8_t *src)
{
    return (uint16_t)(((uint16_t)src[0] << 8) | src[1]);
}

static void Put32(uint8_t *dest, uint32_t value)
{
    dest[0] = (uint8_t)(value >> 24);
    dest[1] = (uint8_t)(value >> 16);
    dest[2] = (uint8_t)(value >> 8);
    dest[3] = (uint8_t)value;
}

static uint32_t Get32(const uint8_t *src)
{
    return ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) |
           ((uint32_t)src[2] << 8) | (uint32_t)src[3];
}

static int16_t GetSigned16(const uint8_t *src)
{
    uint16_t raw = Get16(src);
    return (raw < 0x8000U) ? (int16_t)raw :
           (int16_t)((int32_t)raw - 65536L);
}

static void PutFloat(uint8_t *dest, float value)
{
    uint32_t raw;
    memcpy(&raw, &value, sizeof(raw));
    Put32(dest, raw);
}

static float GetFloat(const uint8_t *src)
{
    uint32_t raw = Get32(src);
    float value;
    memcpy(&value, &raw, sizeof(value));
    return value;
}

static uint8_t ValidTemp(float t)
{
    /* NaN fails all comparisons; infinities fail the bounded range. */
    return (t >= -40.0f && t <= 100.0f) ? 1U : 0U;
}

static uint8_t ValidCG(const CG_Calibration_Param_Str *p)
{
    return (p->CG_Max_Left_Ad < p->CG_Zero_Left_Ad &&
            p->CG_Max_Right_Ad > p->CG_Zero_Right_Ad &&
            ValidTemp(p->temp)) ? 1U : 0U;
}

static uint8_t ValidGJ(const GJ_Calibration_Param_Str *p)
{
    uint16_t a = p->GJ_1410_Ad, b = p->GJ_1435_Ad,
             c = p->GJ_1470_Ad;
    return ((((a < b) && (b < c)) || ((a > b) && (b > c))) &&
             ValidTemp(p->temp)) ? 1U : 0U;
}

/* [CG38] Independent zero references may be equal; no CH slope is fitted.
 * Keep the B3 layout/read compatibility. New writes also check ADC bounds. */
static uint8_t ValidCH(const CH_Calibration_Param_Str *p)
{
    return (p->CZ_Ad != 0xFFFFU && p->HB_Ad != 0xFFFFU &&
            ValidTemp(p->temp)) ? 1U : 0U;
}

static uint8_t ValidCal(const B3_CAL_DATA *p)
{
    return (p->password <= 999999UL && ValidCG(&p->cg) &&
            ValidGJ(&p->gj) && ValidCH(&p->ch)) ? 1U : 0U;
}

static void B3_MakeCal(uint8_t buf[EEP_B3_SLOT_LEN],
                       const B3_CAL_DATA *c, uint32_t sequence)
{
    memset(buf, 0xFF, EEP_B3_SLOT_LEN);
    buf[0]='G'; buf[1]='J'; buf[2]='B'; buf[3]='3';
    buf[4]=B3_VERSION; buf[5]=0U;
    Put32(buf+6, sequence);
    Put32(buf+10, c->password);
    Put16(buf+14, (uint16_t)c->cg.CG_Zero_Left_Ad);
    Put16(buf+16, (uint16_t)c->cg.CG_Max_Left_Ad);
    Put16(buf+18, (uint16_t)c->cg.CG_Zero_Right_Ad);
    Put16(buf+20, (uint16_t)c->cg.CG_Max_Right_Ad);
    PutFloat(buf+22, c->cg.temp);
    Put16(buf+26, c->gj.GJ_1410_Ad);
    Put16(buf+28, c->gj.GJ_1435_Ad);
    Put16(buf+30, c->gj.GJ_1470_Ad);
    PutFloat(buf+32, c->gj.temp);
    Put16(buf+36, c->ch.CZ_Ad);
    Put16(buf+38, c->ch.HB_Ad);
    PutFloat(buf+40, c->ch.temp);
    Put16(buf+B3_CRC_OFFSET, B3_Crc16(buf, B3_CRC_OFFSET));
    buf[B3_MARKER_OFFSET] = B3_COMMIT;
}

static void B3_ReadCal(const uint8_t *buf, B3_CAL_DATA *c)
{
    c->password = Get32(buf+10);
    c->cg.CG_Zero_Left_Ad  = GetSigned16(buf+14);
    c->cg.CG_Max_Left_Ad   = GetSigned16(buf+16);
    c->cg.CG_Zero_Right_Ad = GetSigned16(buf+18);
    c->cg.CG_Max_Right_Ad  = GetSigned16(buf+20);
    c->cg.temp             = GetFloat(buf+22);
    c->gj.GJ_1410_Ad      = Get16(buf+26);
    c->gj.GJ_1435_Ad      = Get16(buf+28);
    c->gj.GJ_1470_Ad      = Get16(buf+30);
    c->gj.temp             = GetFloat(buf+32);
    c->ch.CZ_Ad           = Get16(buf+36);
    c->ch.HB_Ad           = Get16(buf+38);
    c->ch.temp            = GetFloat(buf+40);
}

static void B3_MakeRecordMeta(uint8_t buf[EEP_B3_SLOT_LEN],
                              uint16_t count, uint32_t sequence)
{
    memset(buf, 0xFF, EEP_B3_SLOT_LEN);
    buf[0]='R'; buf[1]='C'; buf[2]='B'; buf[3]='3';
    buf[4]=B3_VERSION; buf[5]=0U;
    Put32(buf+6, sequence);
    Put16(buf+10, count);
    Put16(buf+B3_CRC_OFFSET, B3_Crc16(buf, B3_CRC_OFFSET));
    buf[B3_MARKER_OFFSET] = B3_COMMIT;
}

static uint8_t B3_IsBlank(const uint8_t *buf)
{
    uint8_t i;
    for (i = 0U; i < EEP_B3_SLOT_LEN; ++i)
        if (buf[i] != 0xFFU) return 0U;
    return 1U;
}

static uint8_t B3_SlotValid(const uint8_t *buf, uint8_t is_cal)
{
    B3_CAL_DATA decoded;
    uint16_t checksum;
    if (buf[3] != '3' || buf[4] != B3_VERSION ||
        buf[5] != 0U || buf[B3_MARKER_OFFSET] != B3_COMMIT) return 0U;
    if (is_cal)
    {
        if (buf[0] != 'G' || buf[1] != 'J' || buf[2] != 'B') return 0U;
    }
    else
    {
        if (buf[0] != 'R' || buf[1] != 'C' || buf[2] != 'B') return 0U;
    }
    checksum = B3_Crc16(buf, B3_CRC_OFFSET);
    if (Get16(buf+B3_CRC_OFFSET) != checksum) return 0U;
    if (is_cal)
    {
        B3_ReadCal(buf, &decoded);
        return ValidCal(&decoded);
    }
    return (Get16(buf+10) <= MAX_INDEX_ADDR) ? 1U : 0U;
}

static uint8_t B3_SequenceNewer(uint32_t a, uint32_t b)
{
    /* Unsigned subtraction handles a simple 32-bit sequence wraparound. */
    uint32_t delta = a - b;
    return (delta != 0U && delta < 0x80000000UL) ? 1U : 0U;
}

/* return: 0=both blank, 1=valid slot, 2=corrupt/no valid,
 * 3=read failure. active: 0=A / 1=B / -1=none. */
static uint8_t B3_FindSlot(uint16_t slot_a, uint16_t slot_b,
                           uint8_t is_cal, uint8_t chosen[EEP_B3_SLOT_LEN],
                           int8_t *active, uint32_t *sequence)
{
    uint8_t a[EEP_B3_SLOT_LEN], b[EEP_B3_SLOT_LEN];
    uint8_t valid_a, valid_b;
    if (AT24C512_ReadBuffer(slot_a, a, EEP_B3_SLOT_LEN) != EEPROM_OK ||
        AT24C512_ReadBuffer(slot_b, b, EEP_B3_SLOT_LEN) != EEPROM_OK)
        return 3U;
    valid_a = B3_SlotValid(a, is_cal);
    valid_b = B3_SlotValid(b, is_cal);
    if (!valid_a && !valid_b)
    {
        *active = -1;
        *sequence = 0U;
        return (B3_IsBlank(a) && B3_IsBlank(b)) ? 0U : 2U;
    }
    *active = valid_b && (!valid_a ||
              B3_SequenceNewer(Get32(b+6), Get32(a+6))) ? 1 : 0;
    memcpy(chosen, (*active == 0) ? a : b, EEP_B3_SLOT_LEN);
    *sequence = Get32(chosen+6);
    return 1U;
}

/* [B3] Atomic-ish A/B journal commit:
 * (1) invalidate TARGET marker; (2) write new CRC-covered payload;
 * (3) read back and compare; (4) write the 0xA5 commit marker LAST;
 * (5) read back and validate. The old valid page is untouched.
 */
static uint8_t B3_Commit(uint16_t slot_a, uint16_t slot_b, uint8_t is_cal,
                         const uint8_t data[EEP_B3_SLOT_LEN],
                         int8_t *active, uint32_t *sequence)
{
    uint8_t verify[EEP_B3_SLOT_LEN];
    uint16_t target = (*active == 0) ? slot_b : slot_a;
    uint8_t invalid = 0U;
    uint8_t marker = B3_COMMIT;

    if (AT24C512_WriteByte((uint16_t)(target+B3_MARKER_OFFSET), invalid)
        != EEPROM_OK) return 0U;
    if (AT24C512_WriteBuffer(target, (uint8_t *)data, B3_MARKER_OFFSET)
        != EEPROM_OK) return 0U;
    if (AT24C512_ReadBuffer(target, verify, B3_MARKER_OFFSET)
        != EEPROM_OK) return 0U;
    if (memcmp(data, verify, B3_MARKER_OFFSET) != 0) return 0U;
    if (AT24C512_WriteByte((uint16_t)(target+B3_MARKER_OFFSET), marker)
        != EEPROM_OK) return 0U;
    if (AT24C512_ReadBuffer(target, verify, EEP_B3_SLOT_LEN)
        != EEPROM_OK) return 0U;
    if (memcmp(data, verify, EEP_B3_SLOT_LEN) != 0 ||
        !B3_SlotValid(verify, is_cal)) return 0U;

    *active = (*active == 0) ? 1 : 0;
    *sequence = Get32(data+6);
    return 1U;
}

static void B3_CalibrationToGlobals(const B3_CAL_DATA *c)
{
    Driver_Calibration_Password = c->password;
    CG_Param = c->cg;
    GJ_Param = c->gj;
    CH_Param = c->ch;
}

static void B3_CalibrationFromGlobals(B3_CAL_DATA *c)
{
    c->password = Driver_Calibration_Password;
    c->cg = CG_Param;
    c->gj = GJ_Param;
    c->ch = CH_Param;
}

static uint8_t B3_SaveCalibration(const B3_CAL_DATA *new_data)
{
    uint8_t buf[EEP_B3_SLOT_LEN];
    if (!ValidCal(new_data)) return 0U;
    /* A nonblank chip without valid B3 calibration is quarantined. */
    if (cal_state != EEP_STATE_B3_VALID) return 0U;
    B3_MakeCal(buf, new_data, cal_sequence + 1U);
    if (!B3_Commit(EEP_B3_CAL_SLOT_A, EEP_B3_CAL_SLOT_B, 1U,
                   buf, &cal_active_slot, &cal_sequence)) return 0U;
    B3_CalibrationToGlobals(new_data);
    cal_state = EEP_STATE_B3_VALID;
    return 1U;
}

/* [B3.1] The production build does not import or decode pre-B3 data.
 * A new chip is auto-initialized ONLY when the whole 64 KiB array is
 * erased to 0xFF. Do not infer "factory new" from empty A/B slots.
 * Result: 1=factory blank, 0=nonblank, 2=I2C/read error.
 * The scan runs only for a missing/corrupt calibration journal.
 * It does NOT write to EEPROM.
 */
static uint8_t B31_FactoryScan(uint8_t allow_partial_cal_a)
{
    uint8_t raw[128];
    uint8_t expected[EEP_B3_SLOT_LEN];
    B3_CAL_DATA defaults;
    uint32_t offset;
    uint16_t j;
    uint8_t byte;

    B3_CalibrationFromGlobals(&defaults);
    if (!ValidCal(&defaults)) return 2U; /* never store invalid defaults */
    B3_MakeCal(expected, &defaults, 1U);

    for (offset = 0U; offset < EEP_MAX_ADDR; offset += sizeof(raw))
    {
        if (AT24C512_ReadBuffer((uint16_t)offset, raw, sizeof(raw))
            != EEPROM_OK) return 2U;
        for (j = 0U; j < (uint16_t)sizeof(raw); ++j)
        {
            byte = raw[j];
            if (byte == 0xFFU) continue;

            /* An interrupted first-ever initialization can leave only
             * the A slot's provisional marker/payload programmed.
             * Recover it ONLY when every programmed byte matches the
             * exact factory image (or marker=0), and the rest of the
             * entire EEPROM is blank. This NEVER enables legacy import.
             */
            if (allow_partial_cal_a &&
                offset + j >= EEP_B3_CAL_SLOT_A &&
                offset + j < (uint32_t)EEP_B3_CAL_SLOT_A + EEP_B3_SLOT_LEN)
            {
                uint16_t pos = (uint16_t)(offset + j - EEP_B3_CAL_SLOT_A);
                if (pos == 63U && byte == 0U) continue;
                if (pos < 63U && byte == expected[pos]) continue;
            }
            return 0U;
        }
    }
    return 1U;
}

/* The initializer commits the verified compile-time defaults into both
 * independent A/B calibration pages. The first commit makes the data
 * durable; the second creates a redundant backup. Power loss between
 * them leaves at least one valid copy. Record metadata stays EMPTY (0)
 * until the first actual append, and is never needlessly rewritten.
 */
static uint8_t B31_InitializeFactoryCalibration(void)
{
    B3_CAL_DATA defaults;
    uint8_t snapshot[EEP_B3_SLOT_LEN];
    uint8_t copy;
    B3_CalibrationFromGlobals(&defaults);
    if (!ValidCal(&defaults)) return 0U;

    for (copy = 0U; copy < 2U; ++copy)
    {
        B3_MakeCal(snapshot, &defaults, cal_sequence + 1U);
        if (!B3_Commit(EEP_B3_CAL_SLOT_A, EEP_B3_CAL_SLOT_B,
                       1U, snapshot, &cal_active_slot, &cal_sequence))
            return 0U;
    }
    cal_state = EEP_STATE_B3_VALID;
    return 1U;
}

uint8_t Eep_Data_Get(void)
{
    uint8_t cal_slot[EEP_B3_SLOT_LEN];
    uint8_t rec_slot[EEP_B3_SLOT_LEN];
    uint8_t result, factory;
    B3_CAL_DATA decoded;

    cal_state = EEP_STATE_IO_ERROR;
    record_state = EEP_RECORD_IO_ERROR;
    cal_active_slot = -1;
    record_active_slot = -1;
    cal_sequence = 0U;
    record_sequence = 0U;
    record_count = 0U;

    /* A read-only probe; a failed probe never becomes an empty-chip test. */
    if (AT24C512_Init() != EEPROM_OK) return 0U;

    result = B3_FindSlot(EEP_B3_CAL_SLOT_A, EEP_B3_CAL_SLOT_B, 1U,
                         cal_slot, &cal_active_slot, &cal_sequence);
    if (result == 3U) return 0U;
    if (result == 1U)
    {
        B3_ReadCal(cal_slot, &decoded);
        B3_CalibrationToGlobals(&decoded);
        cal_state = EEP_STATE_B3_VALID;
    }
    else
    {
        /* Both A/B blank: accept strictly blank 64KiB only.
         * Corrupt A/B: allow recovery of the EXACT interrupted first
         * factory A commit, but never other arbitrary/corrupt data. */
        factory = B31_FactoryScan((result == 2U) ? 1U : 0U);
        if (factory == 2U)
        {
            cal_state = EEP_STATE_IO_ERROR;
            return 0U;
        }
        if (factory == 0U)
        {
            cal_state = (result == 2U) ? EEP_STATE_B3_CORRUPT :
                                      EEP_STATE_NONBLANK_LOCKED;
            /* Inspect record status below, without touching EEPROM. */
        }
        else
        {
            /* State is not marked valid until both writes verify. */
            cal_state = EEP_STATE_FACTORY_EMPTY;
            if (!B31_InitializeFactoryCalibration())
            {
                cal_state = EEP_STATE_IO_ERROR;
                return 0U;
            }
        }
    }

    result = B3_FindSlot(EEP_B3_NODE_SLOT_A, EEP_B3_NODE_SLOT_B, 0U,
                         rec_slot, &record_active_slot, &record_sequence);
    if (result == 3U) { record_state = EEP_RECORD_IO_ERROR; return 0U; }
    if (result == 1U)
    {
        record_count = Get16(rec_slot + 10);
        record_state = EEP_RECORD_B3_VALID;
    }
    else if (result == 2U) record_state = EEP_RECORD_CORRUPT;
    else record_state = EEP_RECORD_EMPTY;

    return (cal_state == EEP_STATE_B3_VALID &&
            record_state != EEP_RECORD_CORRUPT) ? 1U : 0U;
}

EEP_STORAGE_STATE Eep_Get_Storage_State(void) { return cal_state; }
EEP_RECORD_STATE Eep_Get_Record_State(void) { return record_state; }
/* Legacy migration is intentionally removed from the new-product build. */
uint8_t Eep_Get_Legacy_Warnings(void) { return 0U; }
uint8_t Eep_Approve_Legacy_Calibration_Upgrade(void) { return 0U; }
uint8_t Eep_Approve_Legacy_Record_Import(void) { return 0U; }

uint8_t Eep_Save_CG_Calibration(CG_Calibration_Param_Str data)
{
    B3_CAL_DATA staged;
    B3_CalibrationFromGlobals(&staged);
    staged.cg=data;
    /* [CG32-03] Do NOT change B3_SlotValid/ValidCG: existing CRC-valid
     * B3 data is read intact, even when its CG values need re-calibration.
     * Invalid NEW calibration must not consume an EEPROM commit. */
    if (!CG_Calibration_Sane(&data)) return 0U;
    return B3_SaveCalibration(&staged);
}

uint8_t Eep_Save_GJ_Calibration(GJ_Calibration_Param_Str data)
{
    B3_CAL_DATA staged;
    B3_CalibrationFromGlobals(&staged);
    staged.gj=data;
    return B3_SaveCalibration(&staged);
}

uint8_t Eep_Save_CH_Calibration(CH_Calibration_Param_Str data)
{
    B3_CAL_DATA staged;
    if (data.CZ_Ad==0U || data.CZ_Ad>0x7FFFU ||
        data.HB_Ad==0U || data.HB_Ad>0x7FFFU) return 0U;
    B3_CalibrationFromGlobals(&staged);
    staged.ch=data;
    return B3_SaveCalibration(&staged);
}

uint8_t Eep_Save_Password(uint32_t password)
{
    B3_CAL_DATA staged;
    B3_CalibrationFromGlobals(&staged);
    staged.password=password;
    return B3_SaveCalibration(&staged);
}

/* [B3] Deprecated unsafe individual-parameter interfaces are
 * deliberately blocked; a calibration group must commit together. */
uint8_t Eep_Write_For_Signed16(uint16_t addr, int16_t data)
{
    (void)addr; (void)data;
    return 0U;
}
uint8_t Eep_Write_For_Unsigned16(uint16_t addr, uint16_t data)
{
    (void)addr; (void)data;
    return 0U;
}
uint8_t Eep_Write_For_Float(uint16_t addr, float data)
{
    (void)addr; (void)data;
    return 0U;
}
uint8_t Eep_Write_For_Unsigned32(uint16_t addr, uint32_t data)
{
    (void)addr; (void)data;
    return 0U;
}

static uint8_t B3_SaveRecordCount(uint16_t count)
{
    uint8_t buf[EEP_B3_SLOT_LEN];
    if (cal_state != EEP_STATE_B3_VALID ||
        count > MAX_INDEX_ADDR || record_state == EEP_RECORD_CORRUPT ||
        record_state == EEP_RECORD_IO_ERROR ||
        record_state == EEP_RECORD_NOT_READY) return 0U;
    B3_MakeRecordMeta(buf, count, record_sequence + 1U);
    if (!B3_Commit(EEP_B3_NODE_SLOT_A, EEP_B3_NODE_SLOT_B, 0U,
                   buf, &record_active_slot, &record_sequence)) return 0U;
    record_count=count;
    record_state=EEP_RECORD_B3_VALID;
    return 1U;
}

uint8_t Eep_Get_Node_Count(uint16_t *count)
{
    if (cal_state != EEP_STATE_B3_VALID ||
        count == NULL || record_state == EEP_RECORD_CORRUPT ||
        record_state == EEP_RECORD_IO_ERROR ||
        record_state == EEP_RECORD_NOT_READY) return 0U;
    *count=record_count;
    return 1U;
}

/* The 15-byte STM32 record payload is deliberately stored exactly as
 * the existing UART2 protocol transmits it: 7 8-bit fields followed
 * by two little-endian uint32_t measurement words. */
static void EncodeNode(uint8_t out[EEP_NODE_ENCODED_LEN], const EEP_NODE *n)
{
    out[0]=n->year; out[1]=n->month; out[2]=n->day;
    out[3]=n->hour; out[4]=n->min; out[5]=n->sec; out[6]=n->type;
    out[7]=(uint8_t)n->data1;
    out[8]=(uint8_t)(n->data1 >> 8);
    out[9]=(uint8_t)(n->data1 >> 16);
    out[10]=(uint8_t)(n->data1 >> 24);
    out[11]=(uint8_t)n->data2;
    out[12]=(uint8_t)(n->data2 >> 8);
    out[13]=(uint8_t)(n->data2 >> 16);
    out[14]=(uint8_t)(n->data2 >> 24);
}

static void DecodeNode(EEP_NODE *n, const uint8_t in[EEP_NODE_ENCODED_LEN])
{
    n->year=in[0]; n->month=in[1]; n->day=in[2];
    n->hour=in[3]; n->min=in[4]; n->sec=in[5]; n->type=in[6];
    n->data1=(uint32_t)in[7] | ((uint32_t)in[8]<<8) |
             ((uint32_t)in[9]<<16) | ((uint32_t)in[10]<<24);
    n->data2=(uint32_t)in[11] | ((uint32_t)in[12]<<8) |
             ((uint32_t)in[13]<<16) | ((uint32_t)in[14]<<24);
}

static uint8_t B3_WriteRecord(uint16_t index, const uint8_t *raw)
{
    uint8_t verify[EEP_NODE_ENCODED_LEN];
    uint16_t addr;
    if (index >= MAX_INDEX_ADDR) return 0U;
    addr=(uint16_t)(EEP_B3_RECORD_START +
                    (uint32_t)index*EEP_NODE_ENCODED_LEN);
    if (AT24C512_WriteBuffer(addr, (uint8_t *)raw, EEP_NODE_ENCODED_LEN)
        != EEPROM_OK) return 0U;
    if (AT24C512_ReadBuffer(addr, verify, EEP_NODE_ENCODED_LEN)
        != EEPROM_OK) return 0U;
    return (memcmp(raw, verify, EEP_NODE_ENCODED_LEN) == 0) ? 1U : 0U;
}

uint8_t Eep_Input_Node(uint16_t index, EEP_NODE node)
{
    uint8_t raw[EEP_NODE_ENCODED_LEN];
    if (cal_state != EEP_STATE_B3_VALID ||
        record_state == EEP_RECORD_CORRUPT ||
        record_state == EEP_RECORD_IO_ERROR ||
        record_state == EEP_RECORD_NOT_READY) return 3U;
    if (record_count >= MAX_INDEX_ADDR) return 1U;
    if (index != record_count) return 2U;
    EncodeNode(raw, &node);
    if (!B3_WriteRecord(index, raw)) return 3U;
    /* [B3] Metadata is committed LAST. Power loss before this line
     * leaves an unreferenced record, never an inflated record count. */
    if (!B3_SaveRecordCount((uint16_t)(record_count+1U))) return 3U;
    return 0U;
}

uint8_t Eep_Output_Node(uint16_t index, void *node)
{
    EEP_NODE decoded;
    uint8_t raw[EEP_NODE_ENCODED_LEN];
    uint16_t addr;
    if (cal_state != EEP_STATE_B3_VALID ||
        node == NULL || record_state == EEP_RECORD_CORRUPT ||
        record_state == EEP_RECORD_IO_ERROR ||
        record_state == EEP_RECORD_NOT_READY) return 3U;
    if (record_count == 0U) return 1U;
    if (index >= record_count || index >= MAX_INDEX_ADDR) return 2U;
    addr=(uint16_t)(EEP_B3_RECORD_START +
                    (uint32_t)index*EEP_NODE_ENCODED_LEN);
    if (AT24C512_ReadBuffer(addr, raw, EEP_NODE_ENCODED_LEN)
        != EEPROM_OK) return 3U;
    DecodeNode(&decoded, raw);
    memcpy(node, &decoded, sizeof(decoded));
    return 0U;
}

uint8_t Eep_Clear_Node(void)
{
    /* [B3] Logical clear only. Old records and B3 record payload bytes
     * remain physically present; only the CRC-checked count is reset. */
    return B3_SaveRecordCount(0U);
}

/* Legacy import/read are disabled and do not access physical EEPROM. */
uint8_t Eep_Read_Legacy_Record(uint16_t index, void *node)
{
    (void)index; (void)node;
    return 0U;
}

uint8_t Eep_Import_Legacy_Records(uint16_t verified_count)
{
    (void)verified_count;
    return 0U;
}
