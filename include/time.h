#ifndef time_h
#define time_h

#include <stdint.h>

extern uint32_t time;

// Обновляет системное время
void time_update();

#endif