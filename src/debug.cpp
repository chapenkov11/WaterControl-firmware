#ifndef DEBUG_CPP
#define DEBUG_CPP

#ifdef SERIAL_LOG_MAIN_ON

#include <Arduino.h>

/*
#define LOG_BEGIN() Serial.begin(9600);


#define LOG(x)               \
    Serial.print(mainTimer); \
    Serial.print(": ");      \
    Serial.println(x);
*/

void LOG_BEGIN()
{
    Serial.begin(9600);
}

void LOG(String msg)
{
    // Serial.print(mainTimer);
    Serial.print(": ");
    Serial.println(msg);
}

#endif
#endif