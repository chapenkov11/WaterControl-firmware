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
template <class PIN_POWER, class PIN_DIRECTION, ADCinput ADC_INPUT>
Valve<PIN_POWER, PIN_DIRECTION, ADC_INPUT>::Valve()
{
    PIN_POWER::SetDir(1);
    PIN_DIRECTION::SetDir(1);
}

// Устанавливает флаги для переключения крана в нужное положение
template <class PIN_POWER, class PIN_DIRECTION, ADCinput ADC_INPUT>
void Valve<PIN_POWER, PIN_DIRECTION, ADC_INPUT>::setPosition(valvePosition position)
{
    LOG("Переключение крана");
    if (status != RUNNING)
    {
        goalPosition = position;
        status = RUNNING;
        startSwitch = time;
    }
}

// Получить текущее положение крана
template <class PIN_POWER, class PIN_DIRECTION, ADCinput ADC_INPUT>
valvePosition Valve<PIN_POWER, PIN_DIRECTION, ADC_INPUT>::getPosition()
{
    return currentPosition;
}

// Получить текущее состояние крана
template <class PIN_POWER, class PIN_DIRECTION, ADCinput ADC_INPUT>
valveStatus Valve<PIN_POWER, PIN_DIRECTION, ADC_INPUT>::getStatus()
{
    return status;
}

/*
Логика остановки крана 
Проверяет состояние кранов и переводит в нужное
*/
template <class PIN_POWER, class PIN_DIRECTION, ADCinput ADC_INPUT>
void Valve<PIN_POWER, PIN_DIRECTION, ADC_INPUT>::run()
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
            zummerRun(alarm);
        }

        // Открытие крана по кнопке
        if (goalPosition == OPEN && currentPosition == CLOSE && (alarmFlag == 0 || lowBat == 0))
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
        LOG("Переключение крана");
        Adc::enable();
        Adc::setInput(ADC_INPUT);
        static valvePeriod period = NORMAL;
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
            LOG("Стадия 1 : попытка закрытия");
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
        else if (N >= MAX_SWITCH_TIME && N < PAUSE_TIME && period != PAUSE)
        {
            // Стадия 2: пауза
            LOG("Стадия 2: пауза")
            period = PAUSE;
            off();
        }
        else
        {
            // Стадия 3: противозаклинивающий маневр
            LOG("Стадия 3: противозаклинивающий маневр");
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
        }

        // Переключение крана закончено
        if (count == 0)
        {
            LOG("Измерение батареи");
            getVCC();
            if (goalPosition == OPEN)
            {
                currentPosition = OPEN;
                status = DONE;
                LOG("Кран открыт");
            }
            else
            {
                currentPosition = CLOSE;
                status = DONE;
                LOG("Кран закрыт");
            }
            Adc::disable();
            off();
            nextCheckValv = time + INTERVAL_CHECK_VALV; // отложить проверку на закисание
            nextCheckBat = time + INTERVAL_CHECK_BAT;   // отложить проверку батареи
            period = NORMAL;
        }
    }
}

// профилактика закисания
// TODO: доделать профилактику заклинивания
// void valvePrevention()
// {
//     if (valveCurrentPosition == OPEN)
//     {
//         valveSetPosition(CLOSE);
//         if (lowBat == 0)
//         {
//             valveSetPosition(OPEN);
//         }
//     }
// }

// Выкл. краны
template <class PIN_POWER, class PIN_DIRECTION, ADCinput ADC_INPUT>
void Valve<PIN_POWER, PIN_DIRECTION, ADC_INPUT>::off()
{
    PIN_POWER::Off(); // выкл. преобразователь
    // ValvePower::Off();
    _delay_ms(RELEY_INTERVAL);
    PIN_DIRECTION::Off(); // выкл. реверс
    // ValveDirection::Off();
    _delay_ms(RELEY_INTERVAL);
}

// Вкл. краны в прямом направлении
template <class PIN_POWER, class PIN_DIRECTION, ADCinput ADC_INPUT>
void Valve<PIN_POWER, PIN_DIRECTION, ADC_INPUT>::onClose()
{
    // ValveDirection::Off();
    PIN_DIRECTION::Off(); // выкл. реверс
    _delay_ms(RELEY_INTERVAL);
    // ValvePower::On();
    PIN_POWER::On(); // вкл. преобразователь
}

// Вкл. краны в обратном направлении
template <class PIN_POWER, class PIN_DIRECTION, ADCinput ADC_INPUT>
void Valve<PIN_POWER, PIN_DIRECTION, ADC_INPUT>::onOpen()
{
    // ValveDirection::On();
    PIN_DIRECTION::On(); // вкл. реверс
    _delay_ms(RELEY_INTERVAL);
    // ValvePower::On();
    PIN_POWER::On(); // вкл. преобразователь
}

template <class PIN_POWER, class PIN_DIRECTION, ADCinput ADC_INPUT>
valveStatus Valve<PIN_POWER, PIN_DIRECTION, ADC_INPUT>::getValveStatus()
{
    Adc::setInput(ADC_INPUT);
    uint16_t ADCavg = Adc::getAVGofN(AVG_NUMBER);
    if (ADCavg <= 1)
    {
        return DONE;
    }
    else
    {
        return RUNNING;
    }
}