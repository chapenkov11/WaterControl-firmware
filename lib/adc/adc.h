#ifndef adc_h
#define adc_h

#include <avr/io.h>

enum ADCinput
{
    ADC0 = 0,
    ADC1 = (1 << MUX0),
    ADC2 = (1 << MUX1),
    ADC3 = (1 << MUX0) | (1 << MUX1),
    ADC4 = (1 << MUX2),
    ADC5 = (1 << MUX0) | (1 << MUX2),
    ADC6 = (1 << MUX1) | (1 << MUX2),
    ADC7 = (1 << MUX0) | (1 << MUX1) | (1 << MUX2),
    V_bg = (1 << MUX1) | (1 << MUX2) | (1 << MUX3)
};

enum ADCref
{
    ref_AREF = 0,
    ref_Vcc = (1 << REFS0),
    ref_Int_2_56V = (1 << REFS1) | (1 << REFS0)
};

enum ADCprescaler
{
    presc_1 = 0,
    presc_2 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0),
    presc_4 = (1 << ADPS0),
    presc_8 = (1 << ADPS1) | (1 << ADPS0),
    presc_16 = (1 << ADPS2),
    presc_32 = (1 << ADPS2) | (1 << ADPS0),
    presc_64 = (1 << ADPS2) | (1 << ADPS1),
    presc_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0)
};

class Adc
{
public:
    static void enable();
    static void init(ADCprescaler prescaler, ADCref ref);
    static void disable();
    static void setInput(ADCinput input);
    static uint16_t getOne();
    static uint16_t getAVGofN(int N);
};

#endif