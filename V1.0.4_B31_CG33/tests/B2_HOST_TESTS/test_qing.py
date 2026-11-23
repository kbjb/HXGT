#!/usr/bin/env python3
"""Extract and host-test REAL QINGJIAOBAN ACK ISR, ACK consumer and SysTick logic.
Requires cc. Does not modify firmware. ASan/UBSan check the compiled source.
"""
from pathlib import Path
import subprocess, tempfile, os
root=Path(__file__).resolve().parents[2]
main=(root/'QINGJIAOBAN/USER/main.c').read_bytes().decode('latin1')
tick=(root/'QINGJIAOBAN/USER/stm32f10x_it.c').read_bytes().decode('latin1')

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
functions='\n'.join((extract_function(main,'uint8_t getAdd(uint8_t'),
                     extract_function(main,'void USART1_IQRN(USATR_STR *usart_str)\r\n{'),
                     extract_function(tick,'void SysTick_Handler(void)\r\n{'), ack_consumer))
header=r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#define USART_RX_LEN 64
#define SENSOR_SEND_PERIOD_MS 500U
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
volatile uint32_t uart1_ack_ok_count = 0;
volatile uint32_t uart1_ack_error_count = 0;
volatile uint32_t uart1_ack_bad_count = 0;
static uint16_t USART_GetITStatus(USART_TypeDef *u, int flag) {(void)u; (void)flag; return 1;}
static uint16_t USART_ReceiveData(USART_TypeDef *u) {(void)u; return fake_rx_byte;}
'''
maintext=r'''
static void feed(const uint8_t *data, unsigned len) { unsigned i; for(i=0;i<len;i++) {fake_rx_byte=data[i]; USART1_IQRN(&usart1_str);} }
static void make_ack(uint8_t a, uint8_t b, uint8_t *f) { f[0]=0x5A; f[1]=0x05; f[2]=a; f[3]=b; f[4]=getAdd(f,4); f[5]=0x0D; f[6]=0x0A; }
int main(void) {
    uint8_t f[7], bad[7]; unsigned i; uint32_t seed=3333;
    for(i=0;i<499;i++) SysTick_Handler();
    assert(!send_en && send_cnt==499);
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
    for(i=0;i<500;i++) SysTick_Handler();
    assert(send_en && send_cnt==0); send_en=0;
    for(i=0;i<120000;i++) {
       seed=1664525UL*seed+1013904223UL;
       fake_rx_byte=(uint8_t)(seed>>24);
       USART1_IQRN(&usart1_str);
       if(usart1_str.rx_ok) consume_ack();
       if((i%211)==0) {for(unsigned j=0;j<500;j++) SysTick_Handler(); send_en=0;}
    }
    /* An interrupted ACK must be recoverable on the next 0x5A start. */
    usart1_str.rx_i=0;
    {uint8_t noise[]={0x5A,0x05,0x12,0x5A}; feed(noise,4);}
    make_ack(0x4F,0x4B,f); feed(f,7);
    assert(usart1_str.rx_ok && usart1_str.rx_len==7);
    consume_ack();
    printf("B2 QING: PASS; ACK ok=%lu error=%lu, 120,000 noise bytes, 500ms scheduler\n",
           (unsigned long)uart1_ack_ok_count,(unsigned long)uart1_ack_error_count);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='b2_qing_') as d:
    c=Path(d)/'test_qing_generated.c'
    exe=Path(d)/'run_qing'
    c.write_text(header+functions+'\n'+maintext,encoding='latin1')
    args=['cc','-std=c99','-O1','-g','-Wall','-Wextra','-fsanitize=undefined','-fstack-protector-all','-fno-omit-frame-pointer',str(c),'-o',str(exe)]
    cp=subprocess.run(args,check=True,capture_output=True,text=True)
    if cp.stderr: print(cp.stderr)
    subprocess.run([str(exe)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
