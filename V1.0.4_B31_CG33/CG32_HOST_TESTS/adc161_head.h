#ifndef MOCK_ADC161_HEAD
#define MOCK_ADC161_HEAD
#include <stdint.h>
#define Bit_RESET 0U
#define Bit_SET 1U
typedef struct Gpio Gpio;
struct Gpio { void (*write)(Gpio *,uint8_t); uint8_t (*read)(Gpio *); void (*str_init)(Gpio *); };
typedef Gpio GPIO_STR;
typedef struct ADC161_CLASS_STR {
    GPIO_STR *cs, *clk, *out;
    int16_t ad_data;
    void (*str_init)(struct ADC161_CLASS_STR*);
    void (*init)(struct ADC161_CLASS_STR*);
    void (*read)(struct ADC161_CLASS_STR*);
} ADC161_STR;
void ADC161_STR_INIT(ADC161_STR *);
void ADC161_READ(ADC161_STR *);
int16_t uint16_twos_complement_to_int16_original(uint16_t);
#endif
