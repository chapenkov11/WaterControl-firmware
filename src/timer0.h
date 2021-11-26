#ifndef timer0_h
#define timer0_h

#include <avr/io.h>

enum T0_prescaler
{
    presc_stop = 0,
    presc_1 = (1 << (CS00)),
    presc_8 = (1 << (CS01)),
    presc_64 = (1 << (CS01)) | (1 << (CS00)),
    presc_256 = (1 << (CS02)),
    presc_1024 = (1 << (CS02)) | (1 << (CS00)),
    extern_falling = (1 << (CS02)) | (1 << (CS01)),
    presc_rising = (1 << (CS02)) | (1 << (CS01)) | (1 << (CS00))
};

class Timer0
{
public:
    static void init(T0_prescaler prescaler);
    static void stop();
};

#endif