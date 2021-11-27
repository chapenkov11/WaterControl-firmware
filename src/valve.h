#ifndef valve_h
#define valve_h

// Возможное положение крана
enum valvePosition
{
    CLOSE,
    OPEN
};

// Возможные статусы крана (RUNNING - в процессе переключения, DONE - не переключается)
enum valveStatus
{
    RUNNING,
    DONE
};

extern valvePosition valveGoalPosition;    // целевое положение крана (в которое нужно перевести)
extern valvePosition valveCurrentPosition; // текущее положение крана
extern valveStatus valveCurrentStatus;     // текущий статус крана
extern bool lowBat;
extern uint32_t nextCheckBat, nextCheckValv, nextSignal, nextLed;
extern uint32_t time;

// void setValve(valvePosition position);
valveStatus getValveStatus();
void valveOff();
void valveOnClose();
void valveOnOpen();
void valveRun();
void valveSetPosition(valvePosition position);
valvePosition valveGetPosition();
valveStatus valveGetStatus();

#endif