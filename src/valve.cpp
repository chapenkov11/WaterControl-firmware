#include <util/delay.h>
#include "settings.h"
#include "sleepTimer.h"
#include "adc.h"
#include "power.h"
#include "debug.h"
#include "interrupts.h"
#include "zummer.h"
#include "valve.hpp"

// #define valveAdc ADC3
template <class PIN_POWER, class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
Valve<PIN_POWER, PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::Valve()
{
    PIN_POWER::SetDir(1);
    PIN_DRIVER_IN_1::SetDir(1);
    PIN_DRIVER_IN_2::SetDir(1);
}

// Устанавливает флаги для переключения крана в нужное положение
template <class PIN_POWER, class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
void Valve<PIN_POWER, PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::setPosition(ValvePosition position)
{
    if (status != RUNNING)
    {
        goalPosition = position;
        status = RUNNING;
        startSwitch = time;
        LOG("Valve:set pos");
    }
}

// Получить текущее положение крана
template <class PIN_POWER, class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
ValvePosition Valve<PIN_POWER, PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::getPosition()
{
    return currentPosition;
}

// Получить текущее состояние крана
template <class PIN_POWER, class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
ValveStatus Valve<PIN_POWER, PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::getStatus()
{
    return status;
}

/*
Логика остановки крана 
Проверяет состояние кранов и переводит в нужное
*/
template <class PIN_POWER, class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
void Valve<PIN_POWER, PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::run()
{
    // Если краны не в состоянии переключения
    if (status == DONE)
    {
        // Закрытие крана
        if (goalPosition == CLOSE && currentPosition == OPEN)
        {
            setPosition(CLOSE);
        }

        // Закрытие крана по тревоге
        if ((alarmFlag == 1 || lowBat == 1) && currentPosition == OPEN)
        {
            setPosition(CLOSE);
        }

        // Открытие крана по кнопке
        if (goalPosition == OPEN && currentPosition == CLOSE && (alarmFlag == 0 || lowBat == 0))
        {
            setPosition(OPEN);
        }

        // Профилактика закисания крана - закрыть-открыть
        // if ((time >= nextCheckValv) && (alarmFlag == 0 || lowBat == 0))
        // {
        //     LOG("Prevent...");

        //     if (valve.getPosition() == OPEN)
        //     {

        //         valve.setPosition(CLOSE);
        //     }

        //     if (valve.getPosition() == CLOSE)
        //     {
        //         // preventOn == 0;
        //         valve.setPosition(OPEN);
        //         nextCheckValv = time + INTERVAL_CHECK_VALV;
        //     }
        //     nextCheckValv = time + INTERVAL_CHECK_VALV;
        // }
    }
    else
    {
        /* Если краны в состоянии переключения
        В проверяем, остановился ли кран
        Если не останавливается за нужное время - противозаклинивающий маневр
        (сместить немного назад, остановить для остывания преобразователя, докрыть кран)
        */
        LOG("Valve:run");
        Adc::enable();
        Adc::setInput(ADC_INPUT);
        static ValvePeriod period = PAUSE;
        static uint8_t count = DONE_NUMBER; // кран считается остановившимся, когда получено столько статусов DONE при измерении тока крана

        /*
        Переключение крана состоит из циклов,
        в цикл входит:
         попытка закрытия -> 
         пауза на остывание преобразователя + сигнал тревоги ->
         противозаклинивающий маневр (кратковременный реверс) ->
         цикл повторяется пока кран не закроется
        */

        // получаем остаток от деления времени переключения на длительность цикла
        int N = (time - startSwitch) % (MAX_SWITCH_TIME + PAUSE_TIME + REVERS_TIME);

        if (N < MAX_SWITCH_TIME && period != NORMAL)
        {
            // Стадия 1: попытка закрытия
            LOG("St1:running");
            period = NORMAL;
            count = DONE_NUMBER;
            // включаем двигатель
            if (goalPosition == OPEN)
            {
                onOpen(); // Направление - реверс
            }
            else
            {
                onClose();
            }
        }
        else if (N >= MAX_SWITCH_TIME && N < (MAX_SWITCH_TIME + PAUSE_TIME) && period != PAUSE)
        {
            // Стадия 2: пауза
            LOG("St2:pause")
            period = PAUSE;
            off();
        }
        else if (N >= (MAX_SWITCH_TIME + PAUSE_TIME) && period != REVERS)
        {
            // Стадия 3: противозаклинивающий маневр
            LOG("St3:reverse");
            period = REVERS;
            // реверсируем двигатель крана
            if (goalPosition == OPEN)
            {
                off();
                onClose();
            }
            else
            {
                off();
                onOpen();
            }
        }

        if (getValveStatus() == DONE && period == NORMAL)
        {
            count--;
            LOG("count--");
            // LOG(count);
        }

        // Переключение крана закончено
        if (count == 0)
        {
            getVCC();
            if (goalPosition == OPEN)
            {
                currentPosition = OPEN;
                status = DONE;
                LOG("Valve:opened");
            }
            else
            {
                currentPosition = CLOSE;
                status = DONE;
                LOG("Valve:closed");
            }
            Adc::disable();
            off();
            nextCheckValv = time + INTERVAL_CHECK_VALV; // отложить проверку на закисание
            nextCheckBat = time + INTERVAL_CHECK_BAT;   // отложить проверку батареи
            period = PAUSE;
        }
    }
}

// Выкл. краны
template <class PIN_POWER, class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
void Valve<PIN_POWER, PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::off()
{
    PIN_POWER::Off(); // выкл. преобразователь
    // ValvePower::Off();
    // _delay_ms(RELEY_INTERVAL);
    // PIN_DIRECTION::Off(); // выкл. реверс
    PIN_DRIVER_IN_1::Off();
    PIN_DRIVER_IN_1::Off();
    // ValveDirection::Off();
    // _delay_ms(RELEY_INTERVAL);
}

// Вкл. краны в прямом направлении
template <class PIN_POWER, class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
void Valve<PIN_POWER, PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::onClose()
{
    // ValveDirection::Off();
    // PIN_DIRECTION::Off(); // выкл. реверс
    // _delay_ms(RELEY_INTERVAL);
    // ValvePower::On();
    PIN_DRIVER_IN_1::On();
    PIN_POWER::On(); // вкл. преобразователь
}

// Вкл. краны в обратном направлении
template <class PIN_POWER, class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
void Valve<PIN_POWER, PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::onOpen()
{
    // ValveDirection::On();
    // PIN_DIRECTION::On(); // вкл. реверс
    // _delay_ms(RELEY_INTERVAL);
    // ValvePower::On();
    PIN_DRIVER_IN_2::On();
    PIN_POWER::On(); // вкл. преобразователь
}

template <class PIN_POWER, class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
ValveStatus Valve<PIN_POWER, PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::getValveStatus()
{
    Adc::setInput(ADC_INPUT);
    uint16_t ADCavg = Adc::getAVGofN(VALVE_AVG_NUMBER);
    // LOG("ADCavg = ");
    // LOG(ADCavg);
    if (ADCavg <= 1)
    {
        return DONE;
    }
    else
    {
        return RUNNING;
    }
}