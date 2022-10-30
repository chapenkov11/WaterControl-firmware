#ifndef time_h
#define time_h

#include <stdint.h>
#include <avr/interrupt.h>
#include "settings.h"

extern uint32_t nextCheckBat, nextCheckValv, nextSignal, nextLed;
extern uint32_t time;

// Обновляет системное время
void time_update();

#endif