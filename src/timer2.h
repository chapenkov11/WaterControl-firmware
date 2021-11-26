#ifndef timer2_h
#define timer2_h

#include <avr/io.h>

enum T2_prescaler
{
    stop = 0,
    presc_1 = (1 << (CS20)),
    presc_8 = (1 << (CS21)),
    presc_32 = (1 << (CS21)) | (1 << (CS20)),
    presc_64 = (1 << (CS22)),
    presc_128 = (1 << (CS22)) | (1 << (CS20)),
    presc_256 = (1 << (CS22)) | (1 << (CS21)),
    presc_1024 = (1 << (CS22)) | (1 << (CS21)) | (1 << (CS20))
};

class Timer2
{
public:
    static void init(T2_prescaler prescaler);
    static void init_async(T2_prescaler prescaler);
};

#endif
