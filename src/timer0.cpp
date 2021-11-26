#include "timer0.h"
#include <avr/io.h>

// #define T0_PRESC (1 << CS02)
// #define Time_FREQ (F_CPU / 256.0)

// #define Time_US(n) (uint16_t)(1e-6 * (n)*Time_FREQ + 0.5)
// #define Time_MS(n) (uint16_t)(1e-3 * (n)*Time_FREQ + 0.5)
// #define Time_SEC(n) (uint16_t)(1.0 * (n)*Time_FREQ + 0.5)

void Timer0::init(T0_prescaler prescaler)
{
    TIMSK &= ~(1 << TOIE0); // откл. прерывание переполнения
    TCNT0 = 0;              // регистр счетчика
    TCCR0 |= prescaler;     // предделитель
}

void Timer0::stop()
{
    TCCR0 &= ~((1 << CS00) | (1 << CS01) | (1 << CS02));
}