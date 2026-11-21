#!/usr/bin/env python3
"""Extract and host-test REAL QINGJIAOBAN ACK ISR, ACK consumer and SysTick logic.
Requires cc. Does not modify firmware. UBSan checks the compiled source.
"""
from pathlib import Path
import subprocess, tempfile, os, re
root=Path(__file__).resolve().parent.parent
main=(root/'QINGJIAOBAN/USER/main.c').read_text(encoding='latin1')
tick=(root/'QINGJIAOBAN/USER/stm32f10x_it.c').read_text(encoding='latin1')

# Use the production configuration, not a stale mock period.
cfg=(root/'QINGJIAOBAN/USER/main_head.h').read_text(encoding='latin1')
period_define=re.search(r'^#define[ \t]+SENSOR_SEND_PERIOD_MS[ \t]+\d+U?\b', cfg, re.M)[0]

def extract_function(src, signature):
    start=src.index(signature)
    brace=src.index('{',start)
    depth=0
    for i in range(brace,len(src)):
        if src[i]=='{': depth+=1
        elif src[i]=='}':
            depth-=1
            if depth==0: return src[start:i+1]
    raise RuntimeError(f'unbalanced function {signature}')

ack_start=main.index('        if(usart1_str.rx_ok == 1)')
ack_end=main.index('        // Transmit sensor frame', ack_start)
ack_consumer='void consume_ack(void) {\n' + main[ack_start:ack_end] + '\n}\n'
tx_driver=(root/'QINGJIAOBAN/CLASS/SCR/usart_code.c').read_text(encoding='latin1')
assert 'USART_TrySendReport(&usart1_str, buffer, sizeof(buffer))' in main
functions='\n'.join((extract_function(tx_driver,'uint8_t USART_TrySendReport('),
                     extract_function(main,'uint8_t getAdd(uint8_t'),
                     extract_function(main,'void USART1_IQRN(USATR_STR *usart_str)\n{'),
                     extract_function(tick,'void SysTick_Handler(void)\n{'), ack_consumer))
header=r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#define USART_RX_LEN 64
#include "cg_angle_filter.h"
#include "cg_timing_watch.h"
#define RESET 0
#define USART_IT_RXNE 1
typedef struct USART_TypeDef { int unused; } USART_TypeDef;
typedef struct {
    USART_TypeDef *usart;
    uint8_t *rx_buffer;
    volatile uint8_t rx_i, rx_len, rx_ok;
} USATR_STR;
static USART_TypeDef fake_dev;
static uint8_t fake_rx_byte;
static uint8_t recv_buf[64];
USATR_STR usart1_str = {&fake_dev, recv_buf, 0, 0, 0};
volatile uint8_t send_en = 0;
volatile uint16_t send_cnt = 0;
volatile uint32_t sensor_tick_ms = 0;
volatile uint32_t uart1_ack_ok_count = 0;
volatile uint32_t uart1_ack_error_count = 0;
volatile uint32_t uart1_ack_bad_count = 0;
#define USART_FLAG_TXE 0x80
#define USART_FLAG_TC 0x40
static unsigned tx_fault, tx_count;
static uint8_t tx_bytes[13];
static uint16_t USART_GetFlagStatus(USART_TypeDef *u, uint16_t flag) {
    (void)u;
    if ((tx_fault==1U && tx_count==3U && flag==USART_FLAG_TXE) ||
        (tx_fault==2U && flag==USART_FLAG_TC)) { ++sensor_tick_ms; return RESET; }
    if (tx_fault==3U) return RESET; /* stopped tick AND TX peripheral */
    return 1U;
}
static void USART_SendData(USART_TypeDef *u, uint16_t data) {
    (void)u; assert(tx_count<sizeof(tx_bytes)); tx_bytes[tx_count++]=(uint8_t)data;
}
static uint16_t USART_GetITStatus(USART_TypeDef *u, int flag) {(void)u; (void)flag; return 1;}
static uint16_t USART_ReceiveData(USART_TypeDef *u) {(void)u; return fake_rx_byte;}
'''
maintext=r'''
static void feed(const uint8_t *data, unsigned len) { unsigned i; for(i=0;i<len;i++) {fake_rx_byte=data[i]; USART1_IQRN(&usart1_str);} }
static void make_ack(uint8_t a, uint8_t b, uint8_t *f) { f[0]=0x5A; f[1]=0x05; f[2]=a; f[3]=b; f[4]=getAdd(f,4); f[5]=0x0D; f[6]=0x0A; }
static void test_step(int16_t from, int16_t to, unsigned sample_ms) {
    unsigned phase, ms, i, worst=0U;
    for(phase=0U; phase<SENSOR_SEND_PERIOD_MS; ++phase) {
        CG_ANGLE_FILTER filter;
        CG_AngleFilter_Init(&filter);
        for(i=0U;i<CG_ANGLE_WINDOW_SIZE;++i) CG_AngleFilter_Push(&filter,from);
        send_en=0; send_cnt=(uint16_t)phase;
        /* Step just after an ADC read. TX is serviced only during foreground,
           just after a sample, as in production. No simulated OLED or ADC delays. */
        for(ms=1U; ms <= CG_ANGLE_WINDOW_SIZE*sample_ms+SENSOR_SEND_PERIOD_MS+sample_ms; ++ms) {
            SysTick_Handler();
            if(ms % sample_ms) continue;
            CG_AngleFilter_Push(&filter,to);
            if(send_en) {
                send_en=0;
                if(filter.output_value==to) break;
            }
        }
        assert(ms <= CG_ANGLE_WINDOW_SIZE*sample_ms+SENSOR_SEND_PERIOD_MS+sample_ms);
        if(ms>worst) worst=ms;
    }
    printf("Simulated settled report (%d -> %d AD), assumed %u ms/loop: %u ms worst TX phase\n",
           (int)from,(int)to,sample_ms,worst);
    send_en=0; send_cnt=0;
}
int main(void) {
    uint8_t f[7], bad[7]; unsigned i; uint32_t seed=3333;
    const uint8_t report[]={0x5a,10,0,1,0x80,0,0x41,0xa0,0,0,0xc6,13,10};
    CG_TIMING_WATCH w={0};
    tx_fault=1; sensor_tick_ms=UINT32_MAX-5U; /* TXE fails midframe across wrap */
    assert(!USART_TrySendReport(&usart1_str,report,sizeof(report)));
    assert(tx_count==3U && sensor_tick_ms==4U);
    tx_fault=2; tx_count=0; /* all data accepted, TC never asserts */
    assert(!USART_TrySendReport(&usart1_str,report,sizeof(report)));
    assert(tx_count==sizeof(report) && sensor_tick_ms==14U);
    tx_fault=3; tx_count=0;
    assert(!USART_TrySendReport(&usart1_str,report,sizeof(report)) && tx_count==0U);
    tx_fault=0;
    assert(USART_TrySendReport(&usart1_str,report,sizeof(report)));
    assert(tx_count==sizeof(report) && memcmp(report,tx_bytes,sizeof(report))==0);
    puts("F103 report TX: TXE/TC deadlines, tick wrap/stall, next complete report PASS");

    CG_RecordGap(&w,UINT32_MAX-3U);
    assert(w.seen && w.last_gap_ms==0U && w.max_gap_ms==0U);
    CG_RecordGap(&w,4U); CG_RecordGap(&w,6U);
    assert(w.last_tick_ms==6U && w.last_gap_ms==2U && w.max_gap_ms==8U);
    test_step(-14000,14000,10U); test_step(14000,-14000,10U);
    test_step(0,3,20U);
    assert(SENSOR_SEND_PERIOD_MS == 100U); /* regression: configured 10 Hz */
    for(i=1;i<SENSOR_SEND_PERIOD_MS;i++) SysTick_Handler();
    assert(!send_en && send_cnt==SENSOR_SEND_PERIOD_MS-1U);
    SysTick_Handler(); assert(send_en && send_cnt==0);
    send_en=0;
    make_ack(0x4F,0x4B,f);
    feed(f,7); assert(usart1_str.rx_ok==1 && usart1_str.rx_len==7);
    consume_ack();
    assert(!send_en && uart1_ack_ok_count==1 && !usart1_str.rx_ok);
    make_ack(0x00,0x02,f); feed(f,7); consume_ack();
    assert(!send_en && uart1_ack_error_count==1);
    memcpy(bad,f,7); bad[4]^=0x20;
    feed(bad,7); assert(!usart1_str.rx_ok);
    make_ack(0x4F,0x4B,f);
    feed(f,7); consume_ack(); assert(uart1_ack_ok_count==2);
    /* Valid ACK must not create another send event; only SysTick does. */
    for(i=0;i<SENSOR_SEND_PERIOD_MS;i++) SysTick_Handler();
    assert(send_en && send_cnt==0); send_en=0;
    for(i=0;i<120000;i++) {
       seed=1664525UL*seed+1013904223UL;
       fake_rx_byte=(uint8_t)(seed>>24);
       USART1_IQRN(&usart1_str);
       if(usart1_str.rx_ok) consume_ack();
       if((i%211)==0) {for(unsigned j=0;j<SENSOR_SEND_PERIOD_MS;j++) SysTick_Handler(); send_en=0;}
    }
    /* An interrupted ACK must be recoverable on the next 0x5A start. */
    usart1_str.rx_i=0;
    {uint8_t noise[]={0x5A,0x05,0x12,0x5A}; feed(noise,4);}
    make_ack(0x4F,0x4B,f); feed(f,7);
    assert(usart1_str.rx_ok && usart1_str.rx_len==7);
    consume_ack();
    printf("B2 QING: PASS; ACK ok=%lu error=%lu, 120,000 noise bytes, %ums scheduler\n",
           (unsigned long)uart1_ack_ok_count,(unsigned long)uart1_ack_error_count,(unsigned)SENSOR_SEND_PERIOD_MS);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='b2_qing_') as d:
    c=Path(d)/'test_qing_generated.c'
    exe=Path(d)/'run_qing'
    c.write_text(period_define+'\n'+header+functions+'\n'+maintext,encoding='latin1')
    args=['cc','-std=c99','-I',str(root/'QINGJIAOBAN/USER'),'-O1','-g','-Wall','-Wextra','-fsanitize=undefined','-fstack-protector-all','-fno-omit-frame-pointer',str(c),'-o',str(exe)]
    cp=subprocess.run(args,check=True,capture_output=True,text=True)
    if cp.stderr: print(cp.stderr)
    subprocess.run([str(exe)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
