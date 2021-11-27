#ifndef debug_h
#define debug_h

#include <stdint.h>

extern uint32_t time;

#ifndef SERIAL_LOG_ON

#define LOG(x)
#define LOG_BEGIN()

#else
#include <Arduino.h>
#undef LOG
#undef LOG_BEGIN

#define LOG_BEGIN() Serial.begin(9600);

#define LOG(x)             \
    Serial.print(time);    \
    Serial.print(F(": ")); \
    Serial.println(F(x));

#endif

#endif