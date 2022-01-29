#include <avr/interrupt.h>
#include "time.h"
#include "settings.h"

uint32_t time = 0;                      // Системное время - инкремент через 2 сек
static volatile uint32_t overflowCount; // Счетчик переполнений таймера Т2

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