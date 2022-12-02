#ifndef valve_hpp
#define valve_hpp

#include <stdio.h>
#include "adc.h"
#include <util/delay.h>
#include "settings.h"
#include "debug.h"

// Возможное положение крана
enum ValvePosition : bool
{
    CLOSE,
    OPEN
};

// Возможные статусы крана (RUNNING - в процессе переключения, STOPPED - не переключается)
enum ValveStatus : bool
{
    RUNNING,
    STOPPED
};

// Стадия переключения крана
enum ValvePeriod : uint8_t
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
- вход управление H-моста 1
- вход управление H-моста 2
- вход АЦП для измерения тока
*/
template <class POWER_PIN, class REVERSE_PIN, ADCinput ADC_INPUT>
class Valve
{
private:
    ValvePosition goalPosition = CLOSE;   // целевое положение крана (в которое нужно перевести)
    ValvePosition currentPosition = OPEN; // текущее положение крана
    ValveStatus status = RUNNING;         // текущий статус крана
    ValveStatus getValveStatus();         // измерить ток крана и определить состояние крана

    uint16_t startSwitch; // время начала переключения крана

public:
    Valve();
    void run();
    void setPosition(ValvePosition position);
    ValvePosition getPosition();
    ValveStatus getStatus();
    // void powerOn(); // вкл. преобразователь
    // void powerOff();
    void onClose(); // направление на закрытие
    void onOpen();  // направление на открытие
    void onStop();  // остановить двигатель Н-мостом
};

#endif