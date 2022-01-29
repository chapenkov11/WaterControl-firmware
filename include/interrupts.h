#ifndef interrupts_h
#define interrupts_h

extern bool alarmFlag;
extern volatile uint32_t sleepCount;

void INT0init();

#endif