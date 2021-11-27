#ifndef power_h
#define power_h

#include <avr/io.h>

extern bool lowBat; // заряд батареи, 1 - низкий
// extern Valve valve;

uint16_t getVCC();

#endif