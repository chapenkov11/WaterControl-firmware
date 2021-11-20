#ifndef time_h
#define time_h

#include <stdint.h>

// #define T0_PRESC (1 << CS02)
#define Time_FREQ (F_CPU / 256.0)

#define Time_US(n) (uint16_t)(1e-6 * (n)*Time_FREQ + 0.5)
#define Time_MS(n) (uint16_t)(1e-3 * (n)*Time_FREQ + 0.5)
#define Time_SEC(n) (uint16_t)(1.0 * (n)*Time_FREQ + 0.5)

extern uint32_t time;

// Обновляет системное время
void time_update();

#endif