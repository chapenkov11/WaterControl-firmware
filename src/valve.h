#ifndef valve_h
#define valve_h

// Положение крана
#define CLOSE 0
#define OPEN 1

extern bool valveFlag;   // целевое положение крана (в которое нужно перевести)
extern bool valveStatus; // текущее положение крана
extern bool lowBat;
extern uint32_t nextCheckBat, nextCheckValv, nextSignal, nextLed;
extern uint32_t mainTimer;

void setValve(bool status);
bool getValveStatus();
void valveOff();
void valveOnDirect();
void valveOnRevers();

#endif