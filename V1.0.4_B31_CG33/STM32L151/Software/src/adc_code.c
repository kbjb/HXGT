#include "adc_head.h"


extern ADC_HandleTypeDef hadc;

// 配置ADC通道
static void ADC_ConfigureChannel(ADC_HandleTypeDef* hadc, uint32_t channel, uint32_t samplingTime) {
    ADC_ChannelConfTypeDef sConfig = {0};
    
    sConfig.Channel = channel;                    // 通道号
    sConfig.Rank = ADC_REGULAR_RANK_1;            // 序列1
    sConfig.SamplingTime = samplingTime;          // 采样时间
    
    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

// 通用ADC读取函数
uint16_t ADC_ReadChannel(uint32_t channel) {
    uint16_t adc_value = 0;
    
    // 1. 停止当前ADC转换
    HAL_ADC_Stop(&hadc);
    
    // 2. 根据通道选择配置
    if (channel == ADC_CHANNEL_VREFINT) {
        // Vrefint通道需要较长的采样时间
        ADC_ConfigureChannel(&hadc, channel, ADC_SAMPLETIME_384CYCLES);
    } else {
        ADC_ConfigureChannel(&hadc, channel, ADC_SAMPLETIME_384CYCLES);
    }
    
    // 3. 启动ADC转换
    if (HAL_ADC_Start(&hadc) != HAL_OK) {
        return 0;
    }
    
    // 4. 等待转换完成（10ms超时）
    if (HAL_ADC_PollForConversion(&hadc, 10) == HAL_OK) {
        // 5. 读取转换值
        adc_value = HAL_ADC_GetValue(&hadc);
    }
    
    // 6. 停止ADC
    HAL_ADC_Stop(&hadc);
    
    return adc_value;
}

// 读取ADC通道9
uint16_t ADC_ReadChannel9(void) {
    return ADC_ReadChannel(ADC_CHANNEL_9);
}

// 读取Vrefint通道
uint16_t ADC_ReadVrefint(void) {
    return ADC_ReadChannel(ADC_CHANNEL_VREFINT);
}

// 获取通道9电压值（单位：V）
float ADC_GetChannel9Voltage(void) {
    uint16_t adc_value = ADC_ReadChannel9();
    float vdda = ADC_GetVDDAVoltage();  // 获取实际VDDA电压
    
    if (vdda > 0 && adc_value > 0) {
        return (adc_value * vdda) / 4095.0f;  // 12位ADC最大值是4095
    }
    
    return 0.0f;
}

// 获取Vrefint电压（单位：V）
float ADC_GetVrefintVoltage(void) {
    uint16_t vrefint_raw = ADC_ReadVrefint();  // 读取当前Vrefint值
    
    if (vrefint_raw > 0) {
        // 计算实际的Vrefint电压
        return (1.2 * 4096) / vrefint_raw;
    }
    
    return 0.0f;
}

// 获取VDDA电压（单位：V）
float ADC_GetVDDAVoltage(void) {
    uint16_t vrefint_cal = *VREFINT_CAL_ADDR;  // 读取工厂校准值
    uint16_t vrefint_raw = ADC_ReadVrefint();  // 读取当前Vrefint值
    
    if (vrefint_raw > 0) {
        // 计算公式：VDDA = 3.0V * 校准值 / 测量值
        return (VDDA_TYPICAL * vrefint_cal) / vrefint_raw;
    }
    
    return 0.0f;
}





