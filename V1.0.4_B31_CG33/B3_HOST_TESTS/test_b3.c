#include "main.h"
#include "at24c512_head.h"
#include "App_Eep.h"
#include "param_head.h"

I2C_HandleTypeDef hi2c1 = {0};
uint32_t Driver_Calibration_Password = 997900UL;
CG_Calibration_Param_Str CG_Param = {1,-12445,50,12513,20.0f};
GJ_Calibration_Param_Str GJ_Param = {7781,15946,27395,20.0f};
CH_Calibration_Param_Str CH_Param = {10000,13000,20.0f};

static uint8_t mem[65536];
static unsigned writes, reads, polls, checks, assertions, page_crosses;
static unsigned writes_to_old;
static int fail_after_writes=-1;
static int fail_after_reads=-1;
static uint8_t wp, bus_off, ack_timeout;
#define CHECK(x) do { assert(x); assertions++; } while(0)

static void reset_globals(void)
{
    Driver_Calibration_Password=997900UL;
    CG_Param.CG_Zero_Left_Ad=1; CG_Param.CG_Max_Left_Ad=-12445;
    CG_Param.CG_Zero_Right_Ad=50; CG_Param.CG_Max_Right_Ad=12513;
    CG_Param.temp=20.0f;
    GJ_Param.GJ_1410_Ad=7781; GJ_Param.GJ_1435_Ad=15946;
    GJ_Param.GJ_1470_Ad=27395; GJ_Param.temp=20.0f;
    CH_Param.CZ_Ad=10000; CH_Param.HB_Ad=13000; CH_Param.temp=20.0f;
}
static void reset_all(void)
{
    memset(mem, 0xFF, sizeof(mem));
    reset_globals();
    writes=reads=polls=checks=page_crosses=0;
    writes_to_old=0; fail_after_writes=-1; fail_after_reads=-1;
    wp=bus_off=ack_timeout=0;
}
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *h, uint16_t d, uint32_t trials, uint32_t t)
{
    (void)h; (void)trials; (void)t;
    assert(d == 0xA0U); polls++;
    return (bus_off || ack_timeout) ? HAL_TIMEOUT : HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t d,
            uint16_t a, uint16_t mode, uint8_t *src, uint16_t n, uint32_t t)
{
    (void)h;
    assert(d==0xA0U && mode==I2C_MEMADD_SIZE_16BIT && t<=100U);
    assert(n && src && (uint32_t)a+n<=65536UL);
    if ((uint32_t)(a%128U)+n>128U) {page_crosses++; return HAL_ERROR;}
    if (bus_off) return HAL_TIMEOUT;
    if (fail_after_writes>=0 && writes>=(unsigned)fail_after_writes) return HAL_ERROR;
    writes++;
    if (!wp) {
        memcpy(mem+a,src,n);
        if (a<1500U || a>=65501U) writes_to_old++;
    }
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h, uint16_t d,
            uint16_t a, uint16_t mode, uint8_t *dst, uint16_t n, uint32_t t)
{
    (void)h;
    assert(d==0xA0U && mode==I2C_MEMADD_SIZE_16BIT && t<=100U);
    assert(n && dst && (uint32_t)a+n<=65536UL);
    if (bus_off) return HAL_TIMEOUT;
    if (fail_after_reads>=0 && reads>=(unsigned)fail_after_reads) return HAL_ERROR;
    reads++;
    memcpy(dst,mem+a,n);
    return HAL_OK;
}
void HAL_Delay(uint32_t delay) { CHECK(delay<100U); }

static void put16(uint16_t a, uint16_t v)
{
    mem[a]=(uint8_t)(v>>8);
    mem[a+1]=(uint8_t)v;
}
static void put32(uint16_t a, uint32_t v)
{
    mem[a]=(uint8_t)(v>>24); mem[a+1]=(uint8_t)(v>>16);
    mem[a+2]=(uint8_t)(v>>8); mem[a+3]=(uint8_t)v;
}

static void test_driver(void)
{
    uint8_t a[5]={0x01,0x02,0x03,0x04,0x05}, b[5]={0};
    unsigned old_writes;
    reset_all();
    mem[0]=0xC3;
    old_writes=writes;
    CHECK(AT24C512_Init()==EEPROM_OK);
    CHECK(writes==old_writes && mem[0]==0xC3);
    CHECK(AT24C512_WriteBuffer(0x007FU,a,5)==EEPROM_OK);
    CHECK(page_crosses==0 && writes==2);
    CHECK(mem[0x007F]==1 && mem[0x0080]==2 && mem[0x0083]==5);
    CHECK(AT24C512_ReadBuffer(0x007FU,b,5)==EEPROM_OK);
    CHECK(memcmp(a,b,5)==0);
    CHECK(AT24C512_WriteByte(0xFFFFU,0xDE)==EEPROM_OK);
    CHECK(mem[0xFFFF]==0xDE);
    CHECK(AT24C512_WriteBuffer(0xFFFFU,a,2)==EEPROM_INVALID_ARG);
    CHECK(AT24C512_ReadBuffer(0xFFFFU,b,2)==EEPROM_INVALID_ARG);
    CHECK(AT24C512_WriteBuffer(0U,NULL,1)==EEPROM_INVALID_ARG);
    CHECK(AT24C512_ReadBuffer(0U,NULL,1)==EEPROM_INVALID_ARG);
    CHECK(AT24C512_WritePage(512U,a)==EEPROM_INVALID_ARG);
    {
        uint8_t data[260], check[260];
        for(unsigned i=0;i<260;i++) data[i]=(uint8_t)(i*17U);
        CHECK(AT24C512_WriteBuffer(0x017FU,data,260U)==EEPROM_OK);
        CHECK(AT24C512_ReadBuffer(0x017FU,check,260U)==EEPROM_OK);
        CHECK(memcmp(data,check,260U)==0 && page_crosses==0);
    }
    CHECK(AT24C512_ReadPage(512U,a)==EEPROM_INVALID_ARG);
    CHECK(AT24C512_WriteBuffer(0U,NULL,0)==EEPROM_OK);
    CHECK(AT24C512_ReadBuffer(0U,NULL,0)==EEPROM_OK);
    bus_off=1; CHECK(AT24C512_Init()==EEPROM_TIMEOUT);
    bus_off=0;
    ack_timeout=1;
    CHECK(AT24C512_WriteByte(0x0040U,0x33U)==EEPROM_TIMEOUT);
    ack_timeout=0;
}

static void test_factory_blank(void)
{
    unsigned before;
    reset_all();
    mem[0]=0xCC;
    before=writes;
    CHECK(Eep_Data_Get()==1);
    CHECK(writes==before);
    CHECK(mem[0]==0xCC);
    CHECK(Eep_Get_Storage_State()==EEP_STATE_FACTORY_EMPTY);
    CHECK(Eep_Get_Record_State()==EEP_RECORD_EMPTY);
    CHECK(Eep_Approve_Legacy_Calibration_Upgrade()==0);
    CHECK(Eep_Save_Password(333333UL)==1);
    CHECK(Eep_Get_Storage_State()==EEP_STATE_B3_VALID);
    CHECK(Driver_Calibration_Password==333333UL);
    CHECK(writes_to_old==0);
    reset_globals();
    CHECK(Eep_Data_Get()==1);
    CHECK(Driver_Calibration_Password==333333UL);
    CHECK(mem[0]==0xCC);
}

static void test_legacy_locked_and_upgrade(void)
{
    unsigned before;
    GJ_Calibration_Param_Str p;
    reset_all();
    mem[EEP_USE_FLAG_START]=0xAA;
    put32(EEP_PASSWORD_START,123456UL);
    put16(CG_LEFT_180_START,(uint16_t)-12000);
    put16(CG_LEFT_0_START,1U);
    put16(CG_RIGHT_180_START,12000U);
    put16(CG_RIGHT_0_START,50U);
    put16(GJ_1410_AD_START,7000U);
    put16(GJ_1435_AD_START,15000U);
    put16(GJ_1470_AD_START,27000U);
    put16(CZ_AD_START,10000U);
    put16(HB_AD_START,13000U);
    before=writes;
    CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Get_Storage_State()==EEP_STATE_LEGACY_LOCKED);
    CHECK(Eep_Get_Legacy_Warnings() & EEP_LEGACY_TEMP_UNCERTAIN);
    CHECK(writes==before);
    CHECK(CG_Param.CG_Max_Left_Ad==-12000 && GJ_Param.GJ_1435_Ad==15000);
    CHECK(Driver_Calibration_Password==123456UL);
    p=GJ_Param; p.GJ_1435_Ad=15123; p.temp=24.5f;
    CHECK(Eep_Save_GJ_Calibration(p)==0);
    CHECK(writes==before);
    CHECK(Eep_Approve_Legacy_Calibration_Upgrade()==1);
    CHECK(Eep_Save_GJ_Calibration(p)==1);
    CHECK(writes_to_old==0);
    CHECK(GJ_Param.GJ_1435_Ad==15123);
    CHECK(Eep_Get_Storage_State()==EEP_STATE_B3_VALID);
    reset_globals();
    CHECK(Eep_Data_Get()==1);
    CHECK(Driver_Calibration_Password==123456UL);
    CHECK(GJ_Param.GJ_1435_Ad==15123 && GJ_Param.temp==24.5f);
    CHECK(mem[EEP_USE_FLAG_START]==0xAA);
    CHECK(mem[GJ_1435_AD_START]==(uint8_t)(15000>>8));
}

static void test_cal_dual_and_powerloss(void)
{
    GJ_Calibration_Param_Str p;
    unsigned done;
    reset_all();
    CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Save_Password(111111UL)==1);
    CHECK(Eep_Save_Password(222222UL)==1);
    CHECK(mem[EEP_B3_CAL_SLOT_A+63]==0xA5 &&
          mem[EEP_B3_CAL_SLOT_B+63]==0xA5);
    CHECK(Eep_Save_Password(9999999UL)==0);
    CHECK(Driver_Calibration_Password==222222UL);
    /* One copy corrupted; older valid copy must be selected. */
    mem[EEP_B3_CAL_SLOT_B+63]=0;
    reset_globals();
    CHECK(Eep_Data_Get()==1);
    CHECK(Driver_Calibration_Password==111111UL);
    CHECK(Eep_Save_Password(333333UL)==1);
    CHECK(Driver_Calibration_Password==333333UL);
    /* Simulated write failure between invalidate and payload commit. */
    done=writes;
    fail_after_writes=(int)done+1;
    CHECK(Eep_Save_Password(444444UL)==0);
    fail_after_writes=-1;
    reset_globals();
    CHECK(Eep_Data_Get()==1);
    CHECK(Driver_Calibration_Password==333333UL);
    p=GJ_Param;
    p.GJ_1435_Ad=p.GJ_1410_Ad; /* invalid monotonic points */
    CHECK(Eep_Save_GJ_Calibration(p)==0);
    CHECK(GJ_Param.GJ_1435_Ad==15946U);
    /* WP may ACK a write but ignore it; verify must reject. */
    wp=1;
    CHECK(Eep_Save_Password(555555UL)==0);
    CHECK(Driver_Calibration_Password==333333UL);
    wp=0;
    /* Both journals invalid => explicit corrupt state, no writes allowed. */
    mem[EEP_B3_CAL_SLOT_A+63]=0;
    mem[EEP_B3_CAL_SLOT_B+63]=0;
    CHECK(Eep_Data_Get()==0);
    CHECK(Eep_Get_Storage_State()==EEP_STATE_B3_CORRUPT);
    CHECK(Eep_Save_Password(123UL)==0);
}

static EEP_NODE node(unsigned i)
{
    EEP_NODE n;
    n.year=26; n.month=11; n.day=(uint8_t)(i%28+1);
    n.hour=12; n.min=34; n.sec=56; n.type=1;
    n.data1=0x12345678U+i; n.data2=0xABCDEF00U+i;
    return n;
}

static void test_records(void)
{
    EEP_NODE a,b;
    uint16_t count;
    unsigned before;
    reset_all();
    mem[0]=0x6E; mem[1]=0x5F;
    CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==0);
    a=node(0);
    CHECK(Eep_Input_Node(3U,a)==2);
    CHECK(Eep_Input_Node(0U,a)==0);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==1);
    CHECK(Eep_Output_Node(1U,&b)==2);
    CHECK(Eep_Output_Node(0U,&b)==0 && b.data1==a.data1);
    CHECK(mem[0]==0x6E && mem[1]==0x5F);
    CHECK(writes_to_old==0);
    a=node(1);
    CHECK(Eep_Input_Node(1U,a)==0);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==2);
    reset_globals();
    CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==2);
    CHECK(Eep_Output_Node(1U,&b)==0 && b.data2==a.data2);
    CHECK(Eep_Clear_Node()==1);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==0);
    CHECK(Eep_Output_Node(0U,&b)==1);
    CHECK(mem[0]==0x6E);
    /* A failed write must never increase the committed count. */
    before=writes;
    fail_after_writes=(int)before;
    CHECK(Eep_Input_Node(0U,a)==3);
    fail_after_writes=-1;
    CHECK(Eep_Get_Node_Count(&count)==1 && count==0);
    /* Boundary maximum of 100 records. */
    for (unsigned i=0;i<100;i++) CHECK(Eep_Input_Node((uint16_t)i,node(i))==0);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==100);
    CHECK(Eep_Input_Node(100U,a)==1);
    reset_globals();
    CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==100);
    CHECK(Eep_Output_Node(99U,&b)==0 && b.data1==node(99).data1);
    CHECK(page_crosses==0 && writes_to_old==0);
}

static void test_legacy_import(void)
{
    EEP_NODE a,b;
    uint8_t rec[15];
    unsigned before;
    reset_all();
    mem[EEP_USE_FLAG_START]=0xAA;
    put32(EEP_PASSWORD_START,997900UL);
    a=node(0);
    rec[0]=a.year; rec[1]=a.month; rec[2]=a.day;
    rec[3]=a.hour; rec[4]=a.min; rec[5]=a.sec; rec[6]=a.type;
    for(unsigned i=0;i<4;i++) {
        rec[7+i]=(uint8_t)(a.data1>>(8*i));
        rec[11+i]=(uint8_t)(a.data2>>(8*i));
    }
    memcpy(mem,rec,15);
    b=node(1); rec[2]=b.day;
    for(unsigned i=0;i<4;i++) {
        rec[7+i]=(uint8_t)(b.data1>>(8*i));
        rec[11+i]=(uint8_t)(b.data2>>(8*i));
    }
    memcpy(mem+15,rec,15);
    before=writes;
    CHECK(Eep_Data_Get()==1 && Eep_Get_Storage_State()==EEP_STATE_LEGACY_LOCKED);
    CHECK(Eep_Read_Legacy_Record(1U,&a)==1 && a.day==b.day);
    CHECK(Eep_Import_Legacy_Records(2U)==0); /* no approval */
    CHECK(Eep_Approve_Legacy_Calibration_Upgrade()==1);
    CHECK(Eep_Approve_Legacy_Record_Import()==1);
    CHECK(Eep_Import_Legacy_Records(101U)==0);
    CHECK(Eep_Import_Legacy_Records(2U)==1);
    CHECK(writes>before && writes_to_old==0);
    CHECK(Eep_Output_Node(1U,&a)==0 && a.data2==b.data2);
    CHECK(Eep_Import_Legacy_Records(2U)==0); /* no double import */
    CHECK(memcmp(mem,mem+EEP_B3_RECORD_START,30)==0);
}


static void test_record_meta_rollback(void)
{
    uint16_t count;
    EEP_NODE x=node(0);
    reset_all();
    CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Input_Node(0,x)==0); /* rec slot A, count=1 */
    CHECK(Eep_Input_Node(1,node(1))==0); /* rec slot B, count=2 */
    mem[EEP_B3_NODE_SLOT_B+61]^=0x10U; /* CRC-corrupted NEW slot */
    CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==1);
    CHECK(Eep_Input_Node(1,node(8))==0);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==2);
    /* Both copies bad: refuse to interpret raw record data as valid. */
    mem[EEP_B3_NODE_SLOT_A+62]^=0x7FU;
    mem[EEP_B3_NODE_SLOT_B+62]^=0x7FU;
    CHECK(Eep_Data_Get()==0);
    CHECK(Eep_Get_Record_State()==EEP_RECORD_CORRUPT);
    CHECK(Eep_Get_Node_Count(&count)==0);
    CHECK(Eep_Input_Node(2,node(2))==3);
    CHECK(Eep_Clear_Node()==0);
}

int main(void)
{
    test_driver();
    test_factory_blank();
    test_legacy_locked_and_upgrade();
    test_cal_dual_and_powerloss();
    test_records();
    test_record_meta_rollback();
    test_legacy_import();
    printf("B3 EEPROM: PASS %u assertions, 100 records, dual-slot/CRC,"
           " legacy locking/import, WP failure, boundary and read-only boot\n",
           assertions);
    return 0;
}
