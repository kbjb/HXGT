#include "main_head.h"
#include "gpio_head.h"
#include "usart_head.h"
#include "ads1110_head.h"
#include "adc161_head.h"
#include "cg_angle_filter.h" /* [CG32-01] */
#include "lm9507_head.h"



/*对象创建区*/
//串口对象创建
uint8_t   usart1_rx_buffer[USART_RX_LEN];
uint8_t   usart2_rx_buffer[USART_RX_LEN];
uint8_t   usart3_rx_buffer[USART_RX_LEN];

USATR_STR usart1_str = USART_CLASS_CREATE(USART1,usart1_rx_buffer,115200);
USATR_STR usart2_str = USART_CLASS_CREATE(USART2,usart2_rx_buffer,115200);
USATR_STR usart3_str = USART_CLASS_CREATE(USART3,usart3_rx_buffer,115200);

GPIO_STR testLed_gpio   = GPIO_CLASS_CREATE(GPIOC,GPIO_Pin_13,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
GPIO_STR Beep_gpio      = GPIO_CLASS_CREATE(GPIOA,GPIO_Pin_8,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
//ADS1110对象创建
GPIO_STR ads1110_scl_str   = GPIO_CLASS_CREATE(ADS1110_SCL_PORT,ADS1110_SCL_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz); 
GPIO_STR ads1110_sda_str   = GPIO_CLASS_CREATE(ADS1110_SDA_PORT,ADS1110_SDA_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);

IIC_STR  ads1110_iic_str   = IIC_CLASS_CREATE(&ads1110_scl_str,&ads1110_sda_str,10000,NULL);

ADS1110_STR ads1110_str    = ADS1110_CLASS_STR(&ads1110_iic_str);
//ADC161对象创建
GPIO_STR   adc161_cs_str   = GPIO_CLASS_CREATE(ADC161_CS_PORT,ADC161_CS_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz); 
GPIO_STR   adc161_scl_str   = GPIO_CLASS_CREATE(ADC161_CLK_PORT,ADC161_CLK_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
GPIO_STR   adc161_out_str   = GPIO_CLASS_CREATE(ADC161_OUT_PORT,ADC161_OUT_PIN,GPIO_Mode_IN_FLOATING,GPIO_Speed_50MHz);

ADC161_STR adc161_str = ADC161_CLASS_CREATE(&adc161_cs_str,&adc161_scl_str,&adc161_out_str);
//LM9507对象创建
GPIO_STR   lm9507_cs_gpio = GPIO_CLASS_CREATE(LM9507_CS_PORT,LM9507_CS_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
GPIO_STR   lm9507_clk_gpio = GPIO_CLASS_CREATE(LM9507_CLK_PORT,LM9507_CLK_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
GPIO_STR   lm9507_mosi_gpio = GPIO_CLASS_CREATE(LM9507_MOSI_PORT,LM9507_MOSI_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
GPIO_STR   lm9507_miso_gpio = GPIO_CLASS_CREATE(LM9507_MISO_PORT,LM9507_MISO_PIN,GPIO_Mode_IPU,GPIO_Speed_50MHz);

LM9507_STR lm9531_str = LM9507_CLASS_STR(&lm9507_clk_gpio,&lm9507_cs_gpio,&lm9507_mosi_gpio,&lm9507_miso_gpio);


//函数创建区

void USART1_IQRN(USATR_STR *usart_str) ;

uint16_t umove_average(uint16_t*datap,uint8_t len,uint8_t *i,uint16_t data )
{
    uint32_t sum = 0;
    uint16_t max = 0,min=0xFFFF;
    datap[*i] = data;
    (*i)++;
    if((*i) == len)
        (*i) = 0;
    for(uint8_t j=0;j<len;j++)
    {
        if(max<datap[j])max = datap[j];
        if(min>datap[j])min = datap[j];
        sum+=datap[j];
    }
    sum = sum-max-min;
    return sum/(len-2);
}

//

uint8_t getAdd(uint8_t *buffp,uint8_t len)
{
    uint32_t sum = 0;
    for(uint8_t i=0;i<len;i++)
        sum+=buffp[i];
    return sum&0x000000FF;
}

/* [CG32-01] Signed inclination filtering is in cg_angle_filter.h.
 * The old move_average() used +/-20000 extrema and an uninitialized
 * 30-slot startup window, both of which corrupt signed endpoint results. */
/*
    根据角度计算弧度值
*/
float degrees_to_radians(float degrees)
{
    return degrees * 0.0174532925f; // π/180 ≈ 0.0174532925
}
/*
    快速余弦近似（-15°~15°误差<0.1%）
*/
float fast_cos(float x)
{
    x = fabs(x); // 处理负角度
    float x2 = x * x;
    float x4 = x2 * x2;
    return 1.0f - x2 / 2.0f + x4 / 24.0f;
}
/*根据角度与长度计算水平距离*/
float calculate_horizontal(float slant_m, float angle_deg)
{
    float rad = degrees_to_radians(angle_deg);
    return slant_m * sinf(rad);
}

typedef union
{
    uint32_t inf;
    float    flo;
}FToI;

typedef union
{
    int16_t  inf;
    uint16_t uinf;
}UToI;


void idwg_Init(void)
{
    // 1. 使能寄存器写权限
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    // 2. 设置预分频器
    IWDG_SetPrescaler(PRESCALER);
    // 3. 设置重载值
    IWDG_SetReload(RELOAD_VALUE);
    // 4. 首次喂狗
    IWDG_ReloadCounter();
    // 5. 启动看门狗
    IWDG_Enable();
}
//变量定义区
#define ADS1110_BUFFER_SIZE 50


volatile uint8_t  send_en                           = 0; /* [B2] SysTick -> main */
volatile uint16_t send_cnt                           = 0; /* [B2] 1 ms tick */
volatile uint32_t uart1_ack_ok_count                  = 0; /* [B2] diagnostic */
volatile uint32_t uart1_ack_error_count               = 0; /* [B2] diagnostic */
volatile uint32_t uart1_ack_bad_count                 = 0; /* [B2] malformed ACK */
uint8_t     ads1110_cnt                             = 0;
uint16_t    ads1110_buffer[ADS1110_BUFFER_SIZE];
uint16_t    ads1110_aver;
float       temper                                  = 0;

int16_t adc161_aver; /* UART1 inclination payload */
CG_ANGLE_FILTER CG_Angle_Filter; /* Keil Watch: raw/filtered/output */





typedef struct
{
    uint8_t flag;       //标准
    uint8_t tempCnt;    //临时计数器
    int16_t StableValue;//稳定数据
    int16_t realValue;  //实时数据
    int16_t PreTempValue;//临时上一次的值
    int32_t tempSumValue;//临时和数据
}DataParm;




void DataParamDefult(DataParm * dataParam)
{
    dataParam->flag = 0;
    dataParam->tempCnt = 0;
    dataParam->PreTempValue = 0xffff;
    dataParam->StableValue  = 0xffff;
    dataParam->tempSumValue = 0;
}


DataParm AngleParam;
/*******************************************************************************
* 函 数 名         : main
* 函数功能         : 主函数
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
int main(void)
{
    /*配置串口1*/ 
    usart1_str.str_init(&usart1_str);
    usart1_str.iqrn = USART1_IQRN; 
    /*配置ADS1110*/
    ads1110_str.str_init(&ads1110_str);
    ads1110_str.check(&ads1110_str);
    /*配置ADC161*/
    adc161_str.str_init(&adc161_str);
    /*配置LM9531*/
    lm9531_str.str_init(&lm9531_str);
    /*配置看门狗*/
//    idwg_Init();
    //开启systic中断
    SysTick_Config(SystemCoreClock / 1000);
    //角度数据处理初始化
    DataParamDefult(&AngleParam);
    CG_AngleFilter_Init(&CG_Angle_Filter);
    while(1)
    {
        
        /*
            距离传感器
        */
        ads1110_str.read(&ads1110_str);     //获取ADS1110数据
        if((ads1110_str.config>>7) == 0)    //判断数据是否刷新
        {
            //ads1110_aver    = umove_average(ads1110_buffer,ADS1110_BUFFER_SIZE,&ads1110_cnt,(ads1110_str.ad_data>0x7FFF?0:ads1110_str.ad_data));//滑动均值滤波  
            ads1110_aver    = ads1110_aver*0.5+0.5*(ads1110_str.ad_data>0x7FFF?0:ads1110_str.ad_data);//一阶低通滤波
        }
        
        /*
            倾角传感器
        */
        //获取ADC161数据
        
        
        adc161_str.read(&adc161_str);
        /* [CG32-01] Smooth all signed 16-bit samples, with no 30-AD blind zone.
         * Remove the old unstable 5-sample accumulator entirely: it could
         * retain an old sum, and its state machine caused unwanted latching.
         * Keep AngleParam fields for existing Keil Watch procedures. */
        adc161_aver = CG_AngleFilter_Push(&CG_Angle_Filter, adc161_str.ad_data);
        AngleParam.realValue = CG_Angle_Filter.filtered_value;
        AngleParam.StableValue = CG_Angle_Filter.output_value;
        AngleParam.PreTempValue = CG_Angle_Filter.raw_value;
        AngleParam.tempCnt = 0U;
        AngleParam.tempSumValue = 0;
        AngleParam.flag = 0U;

        
        /*
            温度传感器
        */
        //获取LM6509温度数据
        temper = lm9531_str.readTemp(&lm9531_str);
        
        if(usart1_str.rx_ok == 1) /* [B1] Only a 7-byte mainboard ACK is expected. */
        {
            uint8_t uLen = usart1_str.rx_len;
            uint8_t *ubuffer = usart1_str.rx_buffer; /* No short temporary copy. */
            if(uLen == 7 && uLen <= USART_RX_LEN &&
               ubuffer[0] == 0x5A && ubuffer[1] == 0x05 &&
               ubuffer[5] == 0x0D && ubuffer[6] == 0x0A &&
               ubuffer[4] == getAdd(ubuffer,4))
            {
                if (ubuffer[2] == 0x4F && ubuffer[3] == 0x4B)
                    uart1_ack_ok_count++; /* [B2] ACK never triggers immediate TX. */
                else
                    uart1_ack_error_count++;
            }
            usart1_str.rx_ok = 0;
        }
        // Transmit sensor frame (original 13-byte protocol retained).
        if(send_en == 1)
        {
            send_en  = 0; /* [B2] SysTick is the sole TX scheduler. */
            FToI fToI;
            UToI uToI;
            uint8_t buffer[13] = {0x5A};
            buffer[2] = (ads1110_aver>>8)&0xFF;
            buffer[3] =  ads1110_aver&0xFF;
            uToI.inf = adc161_aver;
            buffer[4] = (uToI.uinf>>8)&0xFF;
            buffer[5] =  uToI.uinf&0xFF;
            fToI.flo   = temper;
            buffer[6] = (fToI.inf>>24)&0xFF;
            buffer[7] = (fToI.inf>>16)&0xFF;
            buffer[8] = (fToI.inf>>8)&0xFF;
            buffer[9] = (fToI.inf)&0xFF;
            buffer[1]  = 0x0A;
            buffer[10] = getAdd(buffer,10);
            buffer[11] = 0x0D;
            buffer[12] = 0x0A;
            usart1_str.send_string(&usart1_str,(char *)buffer,13);
        }
        
//        IWDG_ReloadCounter();  // 重载计数器
    }
}





/* [B2] USART1 RX IRQ: read DR exactly once, then assemble ONLY the
 * existing seven-byte ACK: 5A 05 AA BB CHECK 0D 0A. No blocking TX here. */
void USART1_IQRN(USATR_STR *usart_str)
{
    uint8_t c;
    if (USART_GetITStatus(usart_str->usart, USART_IT_RXNE) == RESET) return;
    c = (uint8_t)USART_ReceiveData(usart_str->usart);
    if (usart_str->rx_ok) return; /* Foreground owns the complete ACK buffer. */
    /* [B2] A fresh 0x5A resynchronizes even after a truncated/noisy ACK.
     * Valid OK/status ACKs from this protocol never contain 0x5A later. */
    if (c == 0x5A)
    {
        usart_str->rx_buffer[0] = c;
        usart_str->rx_i = 1;
        return;
    }
    if (usart_str->rx_i == 0) return;
    if (usart_str->rx_i == 1)
    {
        if (c != 0x05)
        {
            uart1_ack_bad_count++; /* [B2] Invalid ACK length marker. */
            usart_str->rx_i = 0;
            if (c == 0x5A)
            {
                usart_str->rx_buffer[0] = c;
                usart_str->rx_i = 1;
            }
            return;
        }
    }
    /* ACK is never longer than seven bytes; no 64-byte unbounded scan. */
    if (usart_str->rx_i < 7U)
        usart_str->rx_buffer[usart_str->rx_i++] = c;
    if (usart_str->rx_i == 7U)
    {
        if (usart_str->rx_buffer[5] == 0x0D &&
            usart_str->rx_buffer[6] == 0x0A &&
            usart_str->rx_buffer[4] == getAdd(usart_str->rx_buffer, 4))
        {
            usart_str->rx_len = 7;
            usart_str->rx_ok = 1;
        }
        else
            uart1_ack_bad_count++; /* [B2] Invalid tail/checksum. */
        usart_str->rx_i = 0;
    }
}
