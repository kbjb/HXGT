#include "adc161_head.h"

void ADC161_DELAY(uint32_t tim)
{
    /* [CG32-02] Volatile prevents armcc optimizing the bit-bang delay away.
     * The exact ADC161 CLK frequency still requires a scope check. */
    volatile uint32_t remaining = tim;
    while (remaining != 0U) remaining--;
}

void ADC161_INIT(ADC161_STR *adc161_str)
{
    adc161_str->clk->str_init(adc161_str->clk);
    adc161_str->out->str_init(adc161_str->out);
    adc161_str->clk->write(adc161_str->clk, Bit_SET);
    adc161_str->cs->str_init(adc161_str->cs);
    /* Begin the first sample with the same idle levels as later samples.
     * A GPIO reset latch is low; do not leave CS selected after init. */
    adc161_str->cs->write(adc161_str->cs, Bit_SET);
}

/* [CG32-02] Standard 16-bit two's-complement interpretation.
 * The previous signed shift was undefined when bit 15 became set, and
 * adding 1 to all negative results gave an off-by-one error. */
int16_t uint16_twos_complement_to_int16_original(uint16_t tc)
{
    if ((tc & 0x8000U) != 0U)
        return (int16_t)((int32_t)tc - 65536L);
    return (int16_t)tc;
}

void ADC161_READ(ADC161_STR *adc161_str)
{
    uint16_t raw = 0U;
    uint8_t i;
    adc161_str->cs->write(adc161_str->cs,Bit_RESET);
    ADC161_DELAY(10U);
    for (i = 0U; i < 18U; ++i)
    {
        adc161_str->clk->write(adc161_str->clk,Bit_RESET);
        ADC161_DELAY(10U);
        adc161_str->clk->write(adc161_str->clk,Bit_SET);
        ADC161_DELAY(10U);
        if (i < 2U) continue; /* preserve the original 2 dummy clocks */
        raw = (uint16_t)((uint16_t)(raw << 1) |
                         (adc161_str->out->read(adc161_str->out) ? 1U : 0U));
    }
    adc161_str->cs->write(adc161_str->cs,Bit_SET);
    adc161_str->ad_data = uint16_twos_complement_to_int16_original(raw);
}

void ADC161_STR_INIT(ADC161_STR *adc161_str)
{
    adc161_str->init = ADC161_INIT;
    adc161_str->read = ADC161_READ;
    adc161_str->init(adc161_str);
}

//GPIO_STR   adc161_cs_str   = GPIO_CLASS_CREATE(ADC161_CS_PORT,ADC161_CS_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz); 
//GPIO_STR   adc161_scl_str   = GPIO_CLASS_CREATE(ADC161_CLK_PORT,ADC161_CLK_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
//GPIO_STR   adc161_out_str   = GPIO_CLASS_CREATE(ADC161_OUT_PORT,ADC161_OUT_PIN,GPIO_Mode_IN_FLOATING,GPIO_Speed_50MHz);

//ADC161_STR adc161_str = ADC161_CLASS_CREATE(&adc161_cs_str,&adc161_scl_str,&adc161_out_str);






//float jiaodu = 0;
//float jiaodus[50];
//uint8_t jiaodus_i = 0,jiaodu_refresh = 0,jiaodu_len;
//double jiaodu_sum = 0;

//void ADC161_task(void *pvParameters)
//{   
//    adc161_str.str_init(&adc161_str);
//    while(1)
//    {
//         float min = 0,max = 0;
//         adc161_str.read(&adc161_str);
//         jiaodu = (adc161_str.ad_data)*1.0/32768.0*15;
//        
//         jiaodus[jiaodus_i] = jiaodu;
//         jiaodus_i++;
//         if(jiaodus_i == 50)
//             jiaodus_i = 0;
//         if(jiaodu_refresh == 0)
//             jiaodu_len = jiaodus_i+1;
//         if(jiaodu_len == 50)
//             jiaodu_refresh = 1;
//         jiaodu_sum = 0;
//         for(uint8_t i=0;i<jiaodu_len;i++)
//         {
//             if(jiaodus[i]<min)min = jiaodus[i];
//             if(jiaodus[i]>max)max = jiaodus[i];
//             jiaodu_sum+=jiaodus[i];
//         }
//         jiaodu_sum = jiaodu_sum-max-min;
//         angle = jiaodu_sum/(jiaodu_len-2);
//         vTaskDelay(10);
//    }
//}

// 
//void ADC161_Task_Create(void)
//{
//    //ÈÎÎñ¾ä±ú
//    TaskHandle_t ADC161_Handler;
//    
//    xTaskCreate((TaskFunction_t )ADC161_task,
//                (const char*    )"ADC161_task",
//                (uint16_t       )ADC161_STK_SIZE,
//                (void*          )NULL,
//                (UBaseType_t    )ADC161_TASK_PRO,
//                (TaskHandle_t*  )&ADC161_Handler);
//}




