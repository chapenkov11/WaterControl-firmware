#ifndef power_h
#define power_h

#include <avr/io.h>

extern bool lowBat; // заряд батареи, 1 - низкий

uint16_t getVCC();

#endif