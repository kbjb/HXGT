#include "adc_head.h"

ADC_CH_STR *ADC_CHS[MAX_CH_NUM]={0};

void ADC_INIT(ADC_STR *adc_str)
{
    RCC_APB2PeriphClockCmd(adc_str->rcc,ENABLE);                                //使能ADC时钟
    RCC_ADCCLKConfig(adc_str->rcc_pclk);                                        //ADC 分频
    ADC_DeInit(adc_str->adc);                                                   //复位ADC
    adc_str->adc_info.ADC_ContinuousConvMode = ENABLE;                         //ADC工作在连续转换模式
    adc_str->adc_info.ADC_DataAlign =ADC_DataAlign_Right;                      //设置数据右对齐
    adc_str->adc_info.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;        //转换由软件而不是外部触发启动
    adc_str->adc_info.ADC_Mode =ADC_Mode_Independent;                          //设置ADC为独立工作模式
    adc_str->adc_info.ADC_NbrOfChannel =1;                                     //顺序进行规则转换的ADC通道的数目
    adc_str->adc_info.ADC_ScanConvMode =DISABLE;                               //模数转换工作在非扫描模式
    ADC_Init(adc_str->adc,&adc_str->adc_info);                                   //初始化ADC参数
    ADC_Cmd(adc_str->adc,ENABLE);                                               //使能ADC
    ADC_ResetCalibration(adc_str->adc);                                         //使能复位校准
    while(ADC_GetResetCalibrationStatus(adc_str->adc));                         //等待ADC复位校准完成
    ADC_StartCalibration(adc_str->adc);                                         //开启AD校准
    while(ADC_GetCalibrationStatus(adc_str->adc));                              //等待ADC校准完成
}

uint16_t ADC_GET_VALUE(ADC_STR *adc_str,ADC_CH_STR ch_str)
{
    uint16_t data;
    ADC_RegularChannelConfig(adc_str->adc,ch_str.ch,1,ch_str.SampleTime);//配置ADC数模转换通道
    ADC_SoftwareStartConvCmd(adc_str->adc,ENABLE);
    while(!ADC_GetFlagStatus(adc_str->adc,ADC_FLAG_EOC));
    data = ADC_GetConversionValue(adc_str->adc);
    return data;
}

uint16_t ADC_GET_AVERAGE_VALUE(ADC_STR *adc_str,ADC_CH_STR ch_str,uint16_t num)
{
    uint32_t sum_temp = 0;
    uint16_t max = 0,min = 0,temp;
    if(num < 3)num = 3;
    temp = adc_str->get_adc(adc_str,ch_str);
    if(temp > max)max = temp;
    if(temp < min)min = temp;
    sum_temp += temp;
    return (sum_temp / (num-2));
}

void ADC_CH_ADD(ADC_STR *adc_str,ADC_CH_STR *ch_str)
{
    ch_str->init(ch_str);
    adc_str->chs[ch_str->ch] = ch_str;
    adc_str->ch_num ++;
}

void ADC_STR_INIT(ADC_STR *adc_str)
{
    adc_str->init       = ADC_INIT;
    adc_str->ch_add     = ADC_CH_ADD;
    adc_str->get_adc    = ADC_GET_VALUE;
    adc_str->get_aver   = ADC_GET_AVERAGE_VALUE;
    adc_str->init(adc_str);
}
void ADC_CH_INIT(ADC_CH_STR *adc_ch_str)
{
    adc_ch_str->ch_gpio->str_init(adc_ch_str->ch_gpio);
}


/*********************************************************OS*********************************************************/


GPIO_STR   charge_gpio = GPIO_CLASS_CREATE(CHARGE_PORT,CHARGE_PIN,GPIO_Mode_AIN,GPIO_Speed_50MHz);
ADC_CH_STR charge_ch = ADC_CH_CLASS_CREATE(&charge_gpio,ADC_Channel_9,ADC_SampleTime_55Cycles5);
ADC_STR    adc_str = ADC_CALSS_CREATE(ADC1,ADC_CHS,RCC_PCLK2_Div6);


void adc_task(void *pvParameters)
{
    adc_str.str_init(&adc_str);
    adc_str.ch_add(&adc_str,&charge_ch);
    while(1)
    {
        adc_str.get_adc(&adc_str,charge_ch);
        vTaskDelay(10);
    }
}

 
void Adc_Task_Create(void)
{
    //任务句柄
    TaskHandle_t AdcTask_Handler;
        //创建LED1任务
    xTaskCreate((TaskFunction_t )adc_task,
                (const char*    )"adc_task",
                (uint16_t       )ADC_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )ADC_TASK_PRIO,
                (TaskHandle_t*  )&AdcTask_Handler);
}





