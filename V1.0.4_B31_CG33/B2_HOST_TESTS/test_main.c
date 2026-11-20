#include "main.h"
#include "usart_head.h"
#include "param_head.h"
#include "agreement_head.h"
#include "App_Eep.h"
#include "at24c512_head.h"
/* B2 UART fixture keeps its own fake EEPROM counter; B3.1 firmware
 * does NOT use this address. This definition exists in the TEST only. */
#define NODE_INPUT_NUM_ADDR_START 65505U

UART_HandleTypeDef huart1 = {USART1, NULL, 0, 0};
UART_HandleTypeDef huart2 = {USART2, NULL, 0, 0};
uint16_t GJ_Ad = 0;
int16_t CG_Ad = 0;
float Temp = 0;
uint32_t Driver_Calibration_Password = 0x12345678;
uint8_t U2_PC_Work_Mode = 0;
static uint32_t fake_tick = 0;
static uint8_t eep[65536];
static uint32_t writes = 0;
static uint8_t tx1[100000], tx2[100000];
static size_t ntx1 = 0, ntx2 = 0;
static uint32_t tx_in_isr = 0;
static uint8_t in_isr = 0;
static uint32_t tests = 0;

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *u, uint8_t *buf, uint16_t len) {
    assert(len == 1);
    assert(!u->rx_armed);
    u->rxbuf = buf;
    u->rx_armed = 1;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *u) {
    u->rx_armed = 0;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *u, uint8_t *data, uint16_t len, uint32_t timeout) {
    uint8_t *p; size_t *n;
    assert(timeout != 0xFFFFFFFFUL);
    if(in_isr) tx_in_isr++;
    if (u == &huart1) {p=tx1; n=&ntx1;} else {p=tx2; n=&ntx2;}
    assert((*n)+len<100000);
    memcpy(p+*n, data, len);
    *n+=len;
    return HAL_OK;
}
uint32_t HAL_GetTick(void) { return fake_tick; }
uint8_t AT24C512_WriteByte(uint16_t addr, uint8_t data) { eep[addr]=data; writes++; return EEPROM_OK; }
uint8_t AT24C512_ReadByte(uint16_t addr, uint8_t *data) { *data=eep[addr]; return EEPROM_OK; }
/* [B3] Match new EEPROM API using a lightweight test double. */
uint8_t Eep_Save_Password(uint32_t password) {
    if (password > 999999UL) return 0;
    Driver_Calibration_Password=password;
    writes++;
    return 1;
}
uint8_t Eep_Get_Node_Count(uint16_t *count) {
    uint16_t n;
    if (!count) return 0;
    n=(uint16_t)(((uint16_t)eep[NODE_INPUT_NUM_ADDR_START]<<8) |
                 (uint16_t)eep[NODE_INPUT_NUM_ADDR_START+1]);
    if (n > MAX_INDEX_ADDR) return 0;
    *count=n;
    return 1;
}
uint8_t Eep_Output_Node(uint16_t index, void *out) {
    uint8_t *p=(uint8_t*)out; unsigned i;
    for(i=0;i<sizeof(EEP_NODE);i++) p[i]=(uint8_t)(0xA0U + i + index);
    return 0;
}
static void raw(UART_HandleTypeDef *u, uint8_t b) {
    assert(u->rx_armed && u->rxbuf);
    *u->rxbuf=b;
    u->rx_armed=0;
    in_isr=1;
    HAL_UART_RxCpltCallback(u);
    in_isr=0;
}
static void feed(UART_HandleTypeDef *u, const uint8_t *p, size_t len) {
    size_t i;
    for(i=0;i<len;i++) raw(u,p[i]);
}
static uint8_t checksum(const uint8_t *p, size_t n) {
    size_t i; uint32_t sum=0;
    for(i=0;i<n;i++) sum+=p[i];
    return (uint8_t)sum;
}
static void u1(uint16_t gj, int16_t cg, float temperature, uint8_t *out) {
    union { uint32_t u; float f; } v;
    uint16_t signed_bits=(uint16_t)cg;
    v.f=temperature;
    out[0]=0x5A; out[1]=0x0A;
    out[2]=(uint8_t)(gj>>8); out[3]=(uint8_t)gj;
    out[4]=(uint8_t)(signed_bits>>8); out[5]=(uint8_t)signed_bits;
    out[6]=(uint8_t)(v.u>>24); out[7]=(uint8_t)(v.u>>16);
    out[8]=(uint8_t)(v.u>>8); out[9]=(uint8_t)v.u;
    out[10]=checksum(out,10);
    out[11]=0x0D; out[12]=0x0A;
}
static unsigned u2(uint8_t cmd, const uint8_t *data, unsigned dlen, uint8_t *out) {
    assert(dlen+7<=256);
    out[0]=0x5A; out[1]=cmd; out[2]=(uint8_t)(dlen>>8); out[3]=(uint8_t)dlen;
    if(dlen) memcpy(out+4,data,dlen);
    out[4+dlen]=checksum(out,4+dlen);
    out[5+dlen]=0x0D; out[6+dlen]=0x0A;
    return dlen+7;
}
#define CHECK(pred) do { assert(pred); tests++; } while(0)
int main(void) {
    uint8_t f[256], g[256], dat[256];
    unsigned len; size_t before;
    UART_Init();
    CHECK(huart1.rx_armed && huart2.rx_armed);
    CHECK(!Sensor_Data_Is_Fresh());

    u1(15946,-127,25.5f,f); feed(&huart1,f,13);
    CHECK(UART_Is_Frame_Ready(UART1_ID));
    CHECK(ntx1==0); /* packet callback must not transmit inside IRQ */
    u1(7777,42,11.0f,g); feed(&huart1,g,13);
    CHECK(UART_Get_Dropped_Frame_Count(UART1_ID)==1);
    UART_Process_Received(UART1_ID);
    CHECK(!UART_Is_Frame_Ready(UART1_ID));
    CHECK(GJ_Ad==15946 && CG_Ad==-127 && Temp==25.5f);
    CHECK(ntx1==7 && tx1[0]==0x5A && tx1[1]==0x05 && tx1[2]==0x4F && tx1[3]==0x4B);
    CHECK(tx1[4]==checksum(tx1,4) && tx1[5]==0x0D && tx1[6]==0x0A);
    CHECK(Sensor_Data_Is_Fresh());
    fake_tick=1499; CHECK(Sensor_Data_Is_Fresh());
    fake_tick=1500; CHECK(!Sensor_Data_Is_Fresh());
    u1(15123,-1,-2.5f,f); feed(&huart1,f,13); UART_Process_Received(UART1_ID);
    CHECK(GJ_Ad==15123 && CG_Ad==-1 && Temp==-2.5f);
    f[10]^=0x40;
    before=ntx1;
    feed(&huart1,f,13); UART_Process_Received(UART1_ID);
    CHECK(ntx1==before);
    CHECK(UART_Get_Bad_Frame_Count(UART1_ID)>=1);

    len=u2(0x01,NULL,0,f); before=ntx2;
    feed(&huart2,f,len); CHECK(ntx2==before);
    UART_Process_Received(UART2_ID);
    CHECK(ntx2==before+7 && tx2[before+1]==0x81);
    f[4]^=1; before=ntx2;
    feed(&huart2,f,len); UART_Process_Received(UART2_ID);
    CHECK(ntx2==before && UART_Get_Bad_Frame_Count(UART2_ID)>=1);
    dat[0]=0x00; dat[1]=0x10; dat[2]=0x20;
    len=u2(0x03,dat,3,f); before=ntx2;
    feed(&huart2,f,len); UART_Process_Received(UART2_ID);
    CHECK(ntx2==before+7 && tx2[before+1]==0xC3 && writes==0);
    dat[1]=0x0A; dat[2]=0x12; dat[3]=0x34; /* Valid six-digit password */
    len=u2(0x03,dat,4,f); before=ntx2;
    feed(&huart2,f,len); UART_Process_Received(UART2_ID);
    CHECK(ntx2==before+7 && tx2[before+1]==0x83 && writes==1);

    len=u2(0x05,NULL,0,f); before=ntx2;
    feed(&huart2,f,len); UART_Process_Received(UART2_ID);
    CHECK(ntx2==before+7 && tx2[before+1]==0xC5 && writes==1);

    eep[NODE_INPUT_NUM_ADDR_START]=0; eep[NODE_INPUT_NUM_ADDR_START+1]=2;
    dat[0]=0; dat[1]=1;
    len=u2(0x04,dat,2,f); before=ntx2;
    feed(&huart2,f,len); UART_Process_Received(UART2_ID);
    CHECK(ntx2==before+7+2+sizeof(EEP_NODE) && tx2[before+1]==0x84);
    CHECK(tx2[before+4]==0 && tx2[before+5]==1 && tx2[before+6]==0xA1);
    CHECK(tx2[before+4+2+sizeof(EEP_NODE)]==checksum(tx2+before,4+2+sizeof(EEP_NODE)));

    len=u2(0x09,NULL,0,f); before=ntx2;
    feed(&huart2,f,len); UART_Process_Received(UART2_ID);
    CHECK(ntx2==before+4+2*sizeof(EEP_NODE)+3);
    CHECK(tx2[before+1]==0x89 && tx2[before+2]==0 && tx2[before+3]==2*sizeof(EEP_NODE));
    CHECK(tx2[ntx2-2]==0x0D && tx2[ntx2-1]==0x0A);
    CHECK(tx2[ntx2-3]==checksum(tx2+before,ntx2-before-3));

    /* [B2] Maximal (256-byte) UART2 frame with binary CR/LF/5A payload:
     * framing is length-driven and must never split at an embedded delimiter. */
    { unsigned j;
      for(j=0;j<249;j++) dat[j]=(uint8_t)(j*3U);
      dat[4]=0x0D; dat[5]=0x0A; dat[6]=0x5A;
      len=u2(0x7E,dat,249,f); before=ntx2;
      CHECK(len==256);
      feed(&huart2,f,len); CHECK(UART_Is_Frame_Ready(UART2_ID));
      UART_Process_Received(UART2_ID);
      CHECK(ntx2==before+7 && tx2[before+1]==0x80);
    }
    /* [B2] Complete-frame drop is explicit and bounded while mailbox busy. */
    len=u2(0x01,NULL,0,f);
    before=ntx2;
    feed(&huart2,f,len); feed(&huart2,f,len);
    CHECK(UART_Get_Dropped_Frame_Count(UART2_ID)>=1);
    UART_Process_Received(UART2_ID);
    CHECK(ntx2==before+7);
    /* [B2] Corrupted stored record count must never initiate a big stream. */
    eep[NODE_INPUT_NUM_ADDR_START]=1;
    eep[NODE_INPUT_NUM_ADDR_START+1]=0; /* 256 > MAX_INDEX_ADDR */
    len=u2(0x09,NULL,0,f); before=ntx2;
    feed(&huart2,f,len); UART_Process_Received(UART2_ID);
    CHECK(ntx2==before+7 && tx2[before+1]==0xC9);
    eep[NODE_INPUT_NUM_ADDR_START]=0;
    eep[NODE_INPUT_NUM_ADDR_START+1]=2;

    /* Oversize length rejected at the 4th byte; valid frame still accepted. */
    dat[0]=0x5A; dat[1]=0x01; dat[2]=1; dat[3]=0;
    feed(&huart2,dat,4);
    len=u2(0x01,NULL,0,f); before=ntx2;
    feed(&huart2,f,len); UART_Process_Received(UART2_ID);
    CHECK(ntx2==before+7 && tx2[before+1]==0x81);

    /* IDLE tears down incomplete assembly, preserving a future valid packet. */
    feed(&huart2,f,3);
    huart2.idle_flag=1;
    UART_IDLE_IRQHandler(&huart2);
    CHECK(!huart2.idle_flag);
    before=ntx2; feed(&huart2,f,len); UART_Process_Received(UART2_ID);
    CHECK(ntx2==before+7);

    /* HAL error must rearm the byte stream, then accept another command. */
    HAL_UART_ErrorCallback(&huart2);
    CHECK(huart2.rx_armed && UART_Get_Error_Count(UART2_ID)>0);
    before=ntx2; feed(&huart2,f,len); UART_Process_Received(UART2_ID);
    CHECK(ntx2==before+7);

    /* Noise + partial packets; bounded mailbox and parser must remain safe. */
    {unsigned i; uint32_t seed=0x1248137; for(i=0;i<50000;i++) {
        seed=seed*1664525UL+1013904223UL;
        raw(&huart1,(uint8_t)(seed>>24));
        if((i % 257)==0) UART_Process_Received(UART1_ID);
        if((i % 433)==0) {huart1.idle_flag=1; UART_IDLE_IRQHandler(&huart1);}
    }}
    CHECK(huart1.rx_armed && huart2.rx_armed);
    CHECK(tx_in_isr==0);
    printf("B2 HOST: PASS %u assertions; UART1 bad=%lu dropped=%lu; UART2 bad=%lu; 50,000 noise bytes; no IRQ TX\n",tests,
        (unsigned long)UART_Get_Bad_Frame_Count(UART1_ID),
        (unsigned long)UART_Get_Dropped_Frame_Count(UART1_ID),
        (unsigned long)UART_Get_Bad_Frame_Count(UART2_ID));
    return 0;
}
