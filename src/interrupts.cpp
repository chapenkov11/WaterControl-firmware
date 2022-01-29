#include <avr/io.h>
#include <avr/interrupt.h>
#include "settings.h"
#include "interrupts.h"
#include "debug.h"

bool alarmFlag = 0; // 1 - тревога

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
    LOG("Тревога");
}
