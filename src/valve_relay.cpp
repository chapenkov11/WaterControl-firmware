#include <util/delay.h>
#include "settings.h"
#include "sleepTimer.h"
#include "adc.h"
#include "debug.h"
#include "interrupts.h"
#include "zummer.h"
#include "valve_relay.hpp"

template <class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
Valve<PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::Valve()
{
    /* установка как выходов */
    // PIN_POWER::SetDir(1);
    PIN_DRIVER_IN_1::SetDir(1);
    PIN_DRIVER_IN_2::SetDir(1);
}

// Устанавливает флаги для переключения крана в нужное положение
template <class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
void Valve<PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::setPosition(ValvePosition position)
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
template <class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
ValvePosition Valve<PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::getPosition()
{
    return currentPosition;
}

// Получить текущее состояние крана
template <class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
ValveStatus Valve<PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::getStatus()
{
    return status;
}

/*
Логика остановки крана
Проверяет состояние кранов и переводит в нужное
*/
template <class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
void Valve<PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::run()
{
    // Если краны не в состоянии переключения
    if (status == DONE)
    {
        // Закрытие крана
        if (goalPosition == CLOSE && currentPosition == OPEN)
        {
            setPosition(CLOSE);
        }

        // Открытие крана
        if (goalPosition == OPEN && currentPosition == CLOSE)
        {
            setPosition(OPEN);
        }
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
         пауза на остывание преобразователя
         цикл повторяется пока кран не закроется
        */

        // получаем остаток от деления времени переключения на длительность цикла
        int N = (time - startSwitch) % (MAX_SWITCH_TIME + PAUSE_TIME + REVERS_TIME + PAUSE_TIME);

        if (N < MAX_SWITCH_TIME && period != NORMAL)
        {
            // Стадия 1: попытка закрытия
            LOG("St1:running");
            period = NORMAL;
            count = DONE_NUMBER;

            if (goalPosition == OPEN)
            {
                onOpen(); // Направление - реверс
            }
            else
            {
                onClose();
            }
            // включаем преобразователь
            // powerOn();
        }
        else if (N >= MAX_SWITCH_TIME && N < (MAX_SWITCH_TIME + PAUSE_TIME) && period != PAUSE)
        {
            // Стадия 2: пауза
            LOG("St2:pause")
            period = PAUSE;
            onStop();
        }
        else if (N >= (MAX_SWITCH_TIME + PAUSE_TIME) && N < (MAX_SWITCH_TIME + REVERS_TIME + PAUSE_TIME) && period != REVERS)
        {
            // Стадия 3: противозаклинивающий маневр
            LOG("St3:reverse");
            period = REVERS;
            // реверсируем двигатель крана
            if (goalPosition == OPEN)
            {
                onClose();
            }
            else
            {
                onOpen();
            }
        }
        else if (N >= (MAX_SWITCH_TIME + REVERS_TIME + PAUSE_TIME) && period != PAUSE)
        {
            // Стадия 4: пауза
            LOG("St4:pause")
            period = PAUSE;
            onStop();
        }

        if (getValveStatus() == DONE && period == NORMAL)
        {
            count--;
            LOG("count--");
        }

        // Переключение крана закончено
        if (count == 0)
        {
            // getVCC();
            if (goalPosition == OPEN)
            {
                currentPosition = OPEN;
                LOG("Valve:opened");
            }
            else
            {
                currentPosition = CLOSE;
                LOG("Valve:closed");
            }
            status = DONE;
            onStop();
            // powerOff();
            Adc::disable();
            nextCheckValv = time + INTERVAL_CHECK_VALV; // отложить проверку на закисание
            nextCheckBat = time + INTERVAL_CHECK_BAT;   // отложить проверку батареи
            period = PAUSE;
        }
    }
}

// template <class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
// void Valve<PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::powerOn()
// {
//     PIN_POWER::On(); // вкл. преобразователь
// }

// template <class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
// void Valve<PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::powerOff()
// {
//     PIN_POWER::Off(); // выкл. преобразователь
// }

template <class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
void Valve<PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::onStop()
{
    PIN_DRIVER_IN_1::Off();
    PIN_DRIVER_IN_2::Off();
}

// Вкл. краны в прямом направлении
template <class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
void Valve<PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::onClose()
{
    PIN_DRIVER_IN_2::Off();
    PIN_DRIVER_IN_1::On();
}

// Вкл. краны в обратном направлении
template <class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
void Valve<PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::onOpen()
{
    PIN_DRIVER_IN_1::Off();
    PIN_DRIVER_IN_2::On();
}

template <class PIN_DRIVER_IN_1, class PIN_DRIVER_IN_2, ADCinput ADC_INPUT>
ValveStatus Valve<PIN_DRIVER_IN_1, PIN_DRIVER_IN_2, ADC_INPUT>::getValveStatus()
{
    Adc::setInput(ADC_INPUT);
    uint16_t ADCavg = Adc::getAVGofN(VALVE_AVG_NUMBER);
    LOG("VavlCurADC = ");
    LOG(ADCavg);
    if (ADCavg > 0)
    {
        return RUNNING;
    }
    else
    {
        return DONE;
    }
}