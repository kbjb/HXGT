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
static unsigned writes, reads, polls, assertions, page_crosses, writes_to_old;
static int fail_after_writes=-1, fail_after_reads=-1;
static uint8_t wp, bus_off, ack_timeout, enforce_layout;
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
    memset(mem,0xFF,sizeof(mem)); reset_globals();
    writes=reads=polls=page_crosses=writes_to_old=0;
    fail_after_writes=fail_after_reads=-1; wp=bus_off=ack_timeout=0;
    enforce_layout=1;
}
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *h, uint16_t d,
                                        uint32_t trials,uint32_t t)
{
    (void)h; (void)trials; (void)t;
    assert(d==0xA0U); polls++;
    return (bus_off || ack_timeout) ? HAL_TIMEOUT : HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h,uint16_t d,
        uint16_t a,uint16_t mode,uint8_t *src,uint16_t n,uint32_t t)
{
    (void)h;
    assert(d==0xA0U && mode==I2C_MEMADD_SIZE_16BIT && t<=100U);
    assert(n && src && (uint32_t)a+n<=65536UL);
    if ((uint32_t)(a%128U)+n>128U) {page_crosses++;return HAL_ERROR;}
    if (enforce_layout)
    {
        uint32_t end=(uint32_t)a+n;
        assert(((a >= EEP_B3_RECORD_START) &&
                end <= EEP_B3_RECORD_START + MAX_INDEX_ADDR * EEP_NODE_ENCODED_LEN) ||
               ((a >= EEP_B3_CAL_SLOT_A) && end <= EEP_B3_CAL_SLOT_A + EEP_B3_SLOT_LEN) ||
               ((a >= EEP_B3_CAL_SLOT_B) && end <= EEP_B3_CAL_SLOT_B + EEP_B3_SLOT_LEN) ||
               ((a >= EEP_B3_NODE_SLOT_A) && end <= EEP_B3_NODE_SLOT_A + EEP_B3_SLOT_LEN) ||
               ((a >= EEP_B3_NODE_SLOT_B) && end <= EEP_B3_NODE_SLOT_B + EEP_B3_SLOT_LEN));
    }
    if (bus_off) return HAL_TIMEOUT;
    if (fail_after_writes>=0 && writes>=(unsigned)fail_after_writes)
        return HAL_ERROR;
    writes++;
    if (!wp)
    {
        memcpy(mem+a,src,n);
        if (a<1500U || a>=65501U) writes_to_old++;
    }
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h,uint16_t d,
        uint16_t a,uint16_t mode,uint8_t *dst,uint16_t n,uint32_t t)
{
    (void)h;
    assert(d==0xA0U && mode==I2C_MEMADD_SIZE_16BIT && t<=100U);
    assert(n && dst && (uint32_t)a+n<=65536UL);
    if (bus_off) return HAL_TIMEOUT;
    if (fail_after_reads>=0 && reads>=(unsigned)fail_after_reads)
        return HAL_ERROR;
    reads++; memcpy(dst,mem+a,n); return HAL_OK;
}
void HAL_Delay(uint32_t delay) { CHECK(delay<100U); }

static void test_driver(void)
{
    uint8_t a[5]={1,2,3,4,5},b[5]={0};
    reset_all(); enforce_layout=0; mem[0]=0xC3;
    CHECK(AT24C512_Init()==EEPROM_OK);
    CHECK(writes==0 && mem[0]==0xC3);
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
    CHECK(AT24C512_ReadPage(512U,a)==EEPROM_INVALID_ARG);
    {
        uint8_t data[260], check[260];
        for(unsigned i=0;i<260;i++) data[i]=(uint8_t)(i*17U);
        CHECK(AT24C512_WriteBuffer(0x017FU,data,260U)==EEPROM_OK);
        CHECK(AT24C512_ReadBuffer(0x017FU,check,260U)==EEPROM_OK);
        CHECK(memcmp(data,check,260U)==0 && page_crosses==0);
    }
    CHECK(AT24C512_WriteBuffer(0U,NULL,0)==EEPROM_OK);
    CHECK(AT24C512_ReadBuffer(0U,NULL,0)==EEPROM_OK);
    bus_off=1; CHECK(AT24C512_Init()==EEPROM_TIMEOUT); bus_off=0;
    ack_timeout=1; CHECK(AT24C512_WriteByte(0x40U,0x33U)==EEPROM_TIMEOUT);
}

static void test_factory_init_once(void)
{
    unsigned before, rbefore;
    reset_all();
    CHECK(EEP_B3_SERVICE_ENABLE_LEGACY_CAL_UPGRADE==0);
    CHECK(EEP_B3_SERVICE_IMPORT_LEGACY_COUNT==0);
    CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Get_Storage_State()==EEP_STATE_B3_VALID);
    CHECK(Eep_Get_Record_State()==EEP_RECORD_EMPTY);
    CHECK(Eep_Get_Legacy_Warnings()==0);
    CHECK(reads>=512U && writes==6U);
    CHECK(mem[EEP_B3_CAL_SLOT_A+63U]==0xA5U);
    CHECK(mem[EEP_B3_CAL_SLOT_B+63U]==0xA5U);
    CHECK(mem[0]==0xFFU && mem[0xFFFFU]==0xFFU);
    CHECK(mem[EEP_B3_NODE_SLOT_A]==0xFFU);
    CHECK(writes_to_old==0);
    before=writes; rbefore=reads;
    reset_globals();
    Driver_Calibration_Password=123456UL; CG_Param.temp=42.0f;
    CHECK(Eep_Data_Get()==1);
    CHECK(writes==before && reads-rbefore==4U);
    CHECK(Driver_Calibration_Password==997900UL && CG_Param.temp==20.0f);
    CHECK(GJ_Param.GJ_1435_Ad==15946U && CG_Param.temp==20.0f);
    CHECK(Eep_Approve_Legacy_Calibration_Upgrade()==0);
    CHECK(Eep_Approve_Legacy_Record_Import()==0);
    CHECK(Eep_Read_Legacy_Record(0U,NULL)==0);
    CHECK(Eep_Import_Legacy_Records(0U)==0);
    CHECK(Eep_Write_For_Unsigned32(0xFFFBU,123U)==0);
    CHECK(Eep_Write_For_Unsigned16(0xFFFFU,123U)==0);
    CHECK(Eep_Write_For_Signed16(0xFFFFU,-123)==0);
    CHECK(Eep_Write_For_Float(0xFFFFU,42.0f)==0);
    CHECK(writes==before);
}

static void test_nonblank_locked(void)
{
    static const uint16_t addrs[]={0,0x05DBU,0x1000U,0x2000U,
                                     0xE100U,0xE180U,0xFFDDU,0xFFFFU};
    for (unsigned i=0;i<sizeof(addrs)/sizeof(addrs[0]);i++)
    {
        reset_all(); mem[addrs[i]]=0x77U;
        CHECK(Eep_Data_Get()==0);
        CHECK(Eep_Get_Storage_State()==EEP_STATE_NONBLANK_LOCKED);
        CHECK(writes==0 && mem[addrs[i]]==0x77U);
        CHECK(Eep_Save_Password(123456UL)==0);
        { EEP_NODE test_node={0}; CHECK(Eep_Input_Node(0U,test_node)==3); }
        CHECK(Eep_Clear_Node()==0);
        CHECK(writes==0);
        CHECK(Driver_Calibration_Password==997900UL);
    }
    reset_all(); mem[EEP_B3_CAL_SLOT_A+3]=0x52U;
    CHECK(Eep_Data_Get()==0);
    CHECK(Eep_Get_Storage_State()==EEP_STATE_B3_CORRUPT);
    CHECK(writes==0);
    /* A/B calibration slots erased while a B3 record counter is present
     * cannot be interpreted as a newly manufactured chip. */
    reset_all(); mem[EEP_B3_NODE_SLOT_B]=0x41U;
    CHECK(Eep_Data_Get()==0 && writes==0);
}

static void test_failures_and_factory_recovery(void)
{
    for(int limit=0;limit<=3;limit++)
    {
        reset_all(); fail_after_writes=limit;
        CHECK(Eep_Data_Get()==0);
        CHECK(Eep_Get_Storage_State()==EEP_STATE_IO_ERROR);
        fail_after_writes=-1;
        reset_globals();
        CHECK(Eep_Data_Get()==1);
        CHECK(Eep_Get_Storage_State()==EEP_STATE_B3_VALID);
        CHECK(Driver_Calibration_Password==997900UL);
        CHECK(mem[EEP_B3_CAL_SLOT_A+63U]==0xA5U);
        CHECK(writes_to_old==0);
    }
    reset_all(); fail_after_reads=5;
    CHECK(Eep_Data_Get()==0 && writes==0);
    CHECK(Eep_Get_Storage_State()==EEP_STATE_IO_ERROR);
    fail_after_reads=-1;
    CHECK(Eep_Data_Get()==1);
    reset_all(); wp=1;
    CHECK(Eep_Data_Get()==0);
    CHECK(Eep_Get_Storage_State()==EEP_STATE_IO_ERROR);
    CHECK(mem[EEP_B3_CAL_SLOT_A+63U]==0xFFU);
    wp=0; CHECK(Eep_Data_Get()==1);
    reset_all(); CG_Param.CG_Max_Left_Ad=10;
    CHECK(Eep_Data_Get()==0 && writes==0);
    CHECK(Eep_Get_Storage_State()==EEP_STATE_IO_ERROR);
    /* Partial factory image PLUS any unrelated byte must be refused. */
    reset_all(); fail_after_writes=1;
    CHECK(Eep_Data_Get()==0);
    fail_after_writes=-1;
    mem[0x6000U]=0x20U;
    CHECK(Eep_Data_Get()==0);
    CHECK(Eep_Get_Storage_State()==EEP_STATE_B3_CORRUPT);
}

static EEP_NODE node(unsigned i)
{
    EEP_NODE n;
    n.year=26; n.month=11; n.day=(uint8_t)(i%28+1);
    n.hour=12; n.min=34; n.sec=56; n.type=1;
    n.data1=0x12345678U+i; n.data2=0xABCDEF00U+i;
    return n;
}

static void test_calibration_records_separation(void)
{
    CG_Calibration_Param_Str cg;
    GJ_Calibration_Param_Str gj;
    CH_Calibration_Param_Str ch;
    EEP_NODE n;
    uint16_t count;
    unsigned before;
    reset_all(); CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==0);
    CHECK(Eep_Input_Node(4U,node(0))==2);
    CHECK(Eep_Input_Node(0U,node(0))==0);
    CHECK(Eep_Input_Node(1U,node(1))==0);
    cg=CG_Param; cg.CG_Max_Left_Ad=-12500; cg.temp=23.5f;
    gj=GJ_Param; gj.GJ_1435_Ad=16800; gj.temp=25.25f;
    ch=CH_Param; ch.CZ_Ad=10222; ch.temp=26.0f;
    CHECK(Eep_Save_CG_Calibration(cg)==1);
    CHECK(Eep_Save_GJ_Calibration(gj)==1);
    CHECK(Eep_Save_CH_Calibration(ch)==1);
    CHECK(Eep_Save_Password(654321UL)==1);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==2);
    CHECK(Eep_Output_Node(1U,&n)==0 && n.data2==node(1).data2);
    CHECK(writes_to_old==0 && page_crosses==0);
    before=writes;
    reset_globals(); CHECK(Eep_Data_Get()==1);
    CHECK(writes==before);
    CHECK(Driver_Calibration_Password==654321UL);
    CHECK(CG_Param.CG_Max_Left_Ad==-12500 && CG_Param.temp==23.5f);
    CHECK(GJ_Param.GJ_1435_Ad==16800U && GJ_Param.temp==25.25f);
    CHECK(CH_Param.CZ_Ad==10222U && CH_Param.temp==26.0f);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==2);
    CHECK(Eep_Output_Node(0U,&n)==0 && n.data1==node(0).data1);
    CHECK(Eep_Save_Password(9999999UL)==0);
    CHECK(Driver_Calibration_Password==654321UL && writes==before);
    /* [CG32] New bad calibration must never write or replace good slots. */
    cg=CG_Param; cg.CG_Max_Left_Ad = (int16_t)(cg.CG_Zero_Left_Ad-10);
    CHECK(Eep_Save_CG_Calibration(cg)==0U && writes==before);
    cg=CG_Param; cg.CG_Zero_Right_Ad = (int16_t)(cg.CG_Zero_Left_Ad+2000);
    CHECK(Eep_Save_CG_Calibration(cg)==0U && writes==before);
}

static void test_cal_rollback_and_wp(void)
{
    unsigned done;
    GJ_Calibration_Param_Str gj;
    reset_all(); CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Save_Password(111111UL)==1); /* slot A, seq 3 */
    CHECK(Eep_Save_Password(222222UL)==1); /* slot B, seq 4 */
    mem[EEP_B3_CAL_SLOT_B+63U]=0;
    reset_globals(); CHECK(Eep_Data_Get()==1);
    CHECK(Driver_Calibration_Password==111111UL);
    CHECK(Eep_Save_Password(333333UL)==1);
    done=writes;
    fail_after_writes=(int)done+1;
    CHECK(Eep_Save_Password(444444UL)==0);
    fail_after_writes=-1;
    reset_globals(); CHECK(Eep_Data_Get()==1);
    CHECK(Driver_Calibration_Password==333333UL);
    gj=GJ_Param; gj.GJ_1435_Ad=gj.GJ_1410_Ad;
    CHECK(Eep_Save_GJ_Calibration(gj)==0);
    CHECK(GJ_Param.GJ_1435_Ad==15946U);
    wp=1;
    CHECK(Eep_Save_Password(555555UL)==0);
    CHECK(Driver_Calibration_Password==333333UL);
    wp=0;
    mem[EEP_B3_CAL_SLOT_A+63U]=0;
    mem[EEP_B3_CAL_SLOT_B+63U]=0;
    CHECK(Eep_Data_Get()==0);
    CHECK(Eep_Get_Storage_State()==EEP_STATE_B3_CORRUPT);
    CHECK(Eep_Save_Password(123UL)==0);
}

static void test_record_boundaries_and_meta_rollback(void)
{
    EEP_NODE a;
    uint16_t count;
    unsigned before;
    reset_all(); CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Input_Node(0U,node(0))==0);
    CHECK(Eep_Input_Node(1U,node(1))==0);
    mem[EEP_B3_NODE_SLOT_B+61U]^=0x10U;
    CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==1);
    CHECK(Eep_Input_Node(1U,node(8))==0);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==2);
    CHECK(Eep_Clear_Node()==1);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==0);
    CHECK(Eep_Output_Node(0U,&a)==1);
    before=writes; fail_after_writes=(int)before;
    CHECK(Eep_Input_Node(0U,node(1))==3);
    fail_after_writes=-1;
    CHECK(Eep_Get_Node_Count(&count)==1 && count==0);
    for(unsigned i=0;i<MAX_INDEX_ADDR;i++)
        CHECK(Eep_Input_Node((uint16_t)i,node(i))==0);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==MAX_INDEX_ADDR);
    CHECK(Eep_Input_Node(MAX_INDEX_ADDR,node(0))==1);
    reset_globals(); CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Get_Node_Count(&count)==1 && count==MAX_INDEX_ADDR);
    CHECK(Eep_Output_Node(99U,&a)==0 && a.data1==node(99).data1);
    CHECK(writes_to_old==0 && page_crosses==0);
    mem[EEP_B3_NODE_SLOT_A+62U]^=0x7FU;
    mem[EEP_B3_NODE_SLOT_B+62U]^=0x7FU;
    CHECK(Eep_Data_Get()==0);
    CHECK(Eep_Get_Record_State()==EEP_RECORD_CORRUPT);
    CHECK(Eep_Get_Node_Count(&count)==0);
    CHECK(Eep_Input_Node(1U,node(0))==3);
    CHECK(Eep_Clear_Node()==0);
}


static void test_ch_equal_and_failure(void)
{
    CH_Calibration_Param_Str ch={10318,10318,23.0f};
    unsigned before,cut;
    reset_all(); CHECK(Eep_Data_Get()==1);
    CHECK(Eep_Save_CH_Calibration(ch)==1);
    CHECK(CH_Param.CZ_Ad==10318 && CH_Param.HB_Ad==10318);
    before=writes; reset_globals(); CHECK(Eep_Data_Get()==1);
    CHECK(writes==before && CH_Param.CZ_Ad==10318 && CH_Param.HB_Ad==10318);
    CHECK(CH_Param.temp==23.0f);
    ch.CZ_Ad=0; CHECK(Eep_Save_CH_Calibration(ch)==0 && writes==before);
    ch.CZ_Ad=0x8000; CHECK(Eep_Save_CH_Calibration(ch)==0 && writes==before);
    ch.CZ_Ad=10318; ch.HB_Ad=0xFFFF; CHECK(Eep_Save_CH_Calibration(ch)==0 && writes==before);
    ch.HB_Ad=10318; ch.temp=101; CHECK(Eep_Save_CH_Calibration(ch)==0 && writes==before);
    ch.temp=23; ch.CZ_Ad=10400; wp=1;
    CHECK(Eep_Save_CH_Calibration(ch)==0 && CH_Param.CZ_Ad==10318);
    wp=0; reset_globals(); CHECK(Eep_Data_Get()==1 && CH_Param.CZ_Ad==10318);
    for(cut=0;cut<3;cut++) {
        reset_all(); CHECK(Eep_Data_Get()==1);
        fail_after_writes=(int)writes+(int)cut;
        CHECK(Eep_Save_CH_Calibration(ch)==0);
        CHECK(CH_Param.CZ_Ad==10000 && CH_Param.HB_Ad==13000);
        fail_after_writes=-1; reset_globals(); CHECK(Eep_Data_Get()==1);
        CHECK(CH_Param.CZ_Ad==10000 && CH_Param.HB_Ad==13000);
    }
    CHECK(Eep_Save_CH_Calibration(ch)==1);
    reset_globals(); CHECK(Eep_Data_Get()==1 && CH_Param.CZ_Ad==10400 && CH_Param.HB_Ad==10318);
}

int main(void)
{
    test_ch_equal_and_failure();
    test_driver();
    test_factory_init_once();
    test_nonblank_locked();
    test_failures_and_factory_recovery();
    test_calibration_records_separation();
    test_cal_rollback_and_wp();
    test_record_boundaries_and_meta_rollback();
    printf("B3.1 EEPROM production tests: PASS %u assertions; "
           "64KiB blank scan, init recovery, A/B persistence, "
           "100 records, nonblank lock, I2C/WP/powerloss, disjoint layout.\n",
           assertions);
    return 0;
}
