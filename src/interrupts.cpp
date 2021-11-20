#include <avr/io.h>
#include <avr/interrupt.h>
#include "settings.h"
#include "interrupts.h"

void INT0init()
{
    MCUCR &= ~((1 << ISC00) | (1 << ISC01)); // прерывание по низкому уровню
    GICR |= 1 << INT0;                       // вкл. INT0
    SREG |= 1 << SREG_I;                     // вкл. прерывания
}

ISR(INT0_vect)
{
    alarmFlag = 1;
    GICR &= ~(1 << INT0); // выкл. INT0
#ifdef SERIAL_LOG_MAIN_ON
    LOG("Тревога");
#endif
}

ISR(TIMER2_OVF_vect)
{
    sleepCount++;
}

ISR(TIMER0_OVF_vect)
{
    Zummer::Toggle();
}