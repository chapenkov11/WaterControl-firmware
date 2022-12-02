#include <avr/io.h>
#include "adc.h"

void Adc::enable()
{
    ADCSRA |= (1 << ADEN);
}

void Adc::disable()
{
    ADCSRA &= ~(1 << ADEN); // выкл. АЦП
}

void Adc::setInput(ADCinput input)
{

    ADMUX &= ~((1 << MUX0) | (1 << MUX1) | (1 << MUX2) | (1 << MUX3)); // установка всех бит в 0 = вход ADC0
    ADMUX |= input;
    // пробное преобразование для стабилизации результата
    Adc::getOne();
}

void Adc::init(ADCprescaler prescaler, ADCref ref)
{
    ADCSRA |= prescaler;
    ADMUX |= ref;
    // Adc::getOne();
}

uint16_t Adc::getOne()
{
    ADCSRA |= (1 << ADSC); //Начинаем преобразование
    while ((ADCSRA & (1 << ADSC)))
    {
    }
    return ADC;
}

uint16_t Adc::getAVGofN(int N)
{
    uint32_t ADCsumm = 0;
    for (uint8_t i = 0; i < N; i++)
    {
        ADCsumm += Adc::getOne();
    }
    return (uint16_t)(ADCsumm / N);
}