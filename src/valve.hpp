#ifndef valve_hpp
#define valve_hpp

#include <stdio.h>

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

// Стадия переключения крана
enum valvePeriod
{
    NORMAL,
    PAUSE,
    REVERS
};

extern bool lowBat;
extern uint32_t nextCheckBat, nextCheckValv, nextSignal, nextLed;
extern uint32_t time;

/* Необходимые параметры класса:
- пин включения преобразователя
- пин включения реверса (управление H-мостом)
- вход АЦП для измерения тока
*/
template <class PIN_POWER, class PIN_DIRECTION, uint8_t ADC_INPUT>
class Valve
{
private:
    valvePosition goalPosition = CLOSE;   // целевое положение крана (в которое нужно перевести)
    valvePosition currentPosition = OPEN; // текущее положение крана
    valveStatus status = RUNNING;         // текущий статус крана
    valveStatus getValveStatus();         // измерить ток крана и определить состояние крана

    uint16_t startSwitch; // время начала переключения крана

public:
    Valve();
    void run();
    void setPosition(valvePosition position);
    valvePosition getPosition();
    valveStatus getStatus();
    void off(); // управление питанием мотора и реверсом
    void onClose();
    void onOpen();
};

#endif