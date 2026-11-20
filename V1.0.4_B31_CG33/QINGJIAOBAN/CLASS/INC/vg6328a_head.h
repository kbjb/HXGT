#ifndef __VG6328A_HEAD_H
#define __VG6328A_HEAD_H

#include "main_head.h"
#include "usart_head.h"

#define VG6328A_RX_LEN  64
#define VG6328A_TX_LEN  64

typedef enum
{
    cmd_mode = 0,
    data_receive_mode,
    data_send_mode,
    other,
}VG6328A_MODE;

#define SETTING_CMD     0
#define AT_ENAT         "AT+ENAT\r\n"   //进入命令模式
#define AT_EXAT         "AT+EXAT\r\n"   //进入数据模式
#define AT_LEON         "AT+LEON\r\n"   //开启BLE广播
#define AT_LEOF         "AT+LEOF\r\n"   //关闭BLE广播
#define AT_SPON         "AT+SPON\r\n"   //开始SPP广播
#define AT_SPOF         "AT+SPOF\r\n"   //关闭SPP广播
#define AT_SPNC         "AT+SPNC\r\n"   //断开SPP连接
#define AT_LENC         "AT+LENC\r\n"   //断开BLE连接
#define AT_BAUD0        "AT+BAUD0\r\n"  //设置波特率为9600
#define AT_BAUD1        "AT+BAUD1\r\n"  //设置波特率为19200
#define AT_BAUD2        "AT+BAUD2\r\n"  //设置波特率为38400
#define AT_BAUD3        "AT+BAUD3\r\n"  //设置波特率为57600
#define AT_BAUD4        "AT+BAUD4\r\n"  //设置波特率为115200
#define AT_REST         "AT+REST\r\n"   //蓝牙模块复位
#define AT_RDEF         "AT+RDEF\r\n"   //恢复出厂设置

#define CHANGEING_CMD   1

#define AT_SPNA         "AT+SPNA\r\n"   //修改SPP名称
#define AT_LENA         "AT+LENA\r\n"   //修改BLE名称
#define AT_SPAD         "AT+SPAD\r\n"   //设置SPP地址
#define AT_LEAD         "AT+LEAD\r\n"   //设置BLE地址


#define INQUIRY_CMD     2


typedef struct VG6328A_CLASS_STR
{
    USATR_STR        *usart_str;
    uint8_t          *rx_buffer;
    uint8_t          rx_len;
    uint8_t          *tx_buffer;
    uint8_t          tx_len;
    VG6328A_MODE     mode;
    void             (*str_init)(struct VG6328A_CLASS_STR*);
    void             (*init)(struct VG6328A_CLASS_STR*);
    uint8_t          (*send_set_cmd)(struct VG6328A_CLASS_STR*,char *,uint8_t,uint8_t);
}VG6328A_STR;

void VGA6328A_STR_INIT(VG6328A_STR *vga6328_str);

#define VG6283A_CLASS_CREATE(_usatr)\
{\
    .usart_str = _usatr,            \
    .rx_len    = 0,                 \
    .tx_len    = 0,                 \
    .mode      = data_send_mode, \
    .str_init  = VGA6328A_STR_INIT, \
}




#define VGA6328A_STK_SIZE   120
#define VGA6328A_TASK_PRIO  10


void VGA6328a_Task_Create(void);


#endif


