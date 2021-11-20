#include <avr/interrupt.h>
#include "time.h"

// Счетчик переполнений таймера Т2
static volatile uint32_t overflowCount;

ISR(TIMER2_OVF_vect)
{
    overflowCount++;
}

void time_update()
{
    uint32_t t1;
    do
    {
        t1 = overflowCount;
    } while (t1 != overflowCount);
    time = t1;
}