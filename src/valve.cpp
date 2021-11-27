#include <util/delay.h>
#include "settings.h"
#include "sleepTimer.h"
#include "valve.h"
#include "adc.h"
#include "power.h"
#include "debug.h"
#include "interrupts.h"
#include "zummer.h"

valvePosition valveGoalPosition = CLOSE;   // целевое положение крана (в которое нужно перевести)
valvePosition valveCurrentPosition = OPEN; // =valveStatus // текущее положение крана
valveStatus valveCurrentStatus = RUNNING;  // текущий статус крана
uint16_t startSwitch;                      // время начала переключения крана

// #define valveAdc ADC3

// Стадия переключения крана
enum vavlePeriod
{
    NORMAL,
    PAUSE,
    REVERS
};

// Устанавливает флаги для переключения крана в нужное положение
void valveSetPosition(valvePosition position)
{
    LOG("Переключение крана");
    if (valveCurrentStatus != RUNNING)
    {
        valveGoalPosition = position;
        valveCurrentStatus = RUNNING;
        startSwitch = time;
    }
}

// Получить текущее положение крана
valvePosition valveGetPosition()
{
    return valveCurrentPosition;
}

// Получить текущее состояние крана
valveStatus valveGetStatus()
{
    return valveCurrentStatus;
}

/*
Логика остановки крана 
Проверяет состояние кранов и переводит в нужное
*/
void valveRun()
{
    // Если краны не в состоянии переключения
    if (valveCurrentStatus == DONE)
    {
        // Закрытие крана
        if (valveGoalPosition == CLOSE && valveCurrentPosition == OPEN)
        {
            valveSetPosition(CLOSE);
        }

        // Закрытие крана по тревоге
        if ((alarmFlag == 1 || lowBat == 1) && valveCurrentPosition == OPEN)
        {
            valveSetPosition(CLOSE);
            zummerRun(alarm);
        }

        // Открытие крана по кнопке
        if (valveGoalPosition == OPEN && valveCurrentPosition == CLOSE && (alarmFlag == 0 || lowBat == 0))
        {
            valveSetPosition(OPEN);
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
        Adc::setInput(valvAdc);
        static vavlePeriod period = NORMAL;
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
            if (valveGoalPosition == OPEN)
            {
                valveOnOpen(); // Направление - реверс
            }
            else
            {
                valveOnClose();
            }
        }
        else if (N >= MAX_SWITCH_TIME && N < PAUSE_TIME && period != PAUSE)
        {
            // Стадия 2: пауза
            LOG("Стадия 2: пауза")
            period = PAUSE;
            valveOff();
        }
        else
        {
            // Стадия 3: противозаклинивающий маневр
            LOG("Стадия 3: противозаклинивающий маневр");
            period = REVERS;
            // реверсируем двигатель крана
            if (valveGoalPosition == OPEN)
            {
                valveOff();
                valveOnClose();
            }
            else
            {
                valveOff();
                valveOnOpen();
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
            if (valveGoalPosition == OPEN)
            {
                valveCurrentPosition = OPEN;
                valveCurrentStatus = DONE;
                LOG("Кран открыт");
            }
            else
            {
                valveCurrentPosition = CLOSE;
                valveCurrentStatus = DONE;
                LOG("Кран закрыт");
            }
            Adc::disable();
            valveOff();
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
void valveOff()
{
    ValvePower::Off(); // выкл. преобразователь
    _delay_ms(RELEY_INTERVAL);
    ValveDirection::Off(); // выкл. реверс
    _delay_ms(RELEY_INTERVAL);
}

// Вкл. краны в прямом направлении
void valveOnClose()
{
    ValveDirection::Off(); // выкл. реверс
    _delay_ms(RELEY_INTERVAL);
    ValvePower::On(); // вкл. преобразователь
}

// Вкл. краны в обратном направлении
void valveOnOpen()
{
    ValveDirection::On(); // вкл. реверс
    _delay_ms(RELEY_INTERVAL);
    ValvePower::On(); // вкл. преобразователь
}

valveStatus getValveStatus()
{
    Adc::setInput(valvAdc);
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