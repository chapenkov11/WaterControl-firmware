#include <util/delay.h>
#include "settings.h"
#include "sleepTimer.h"
#include "valve.h"
#include "adc.h"
#include "power.h"

// Выкл. краны
void valveOff()
{
    ValvePower::Off(); // выкл. преобразователь
    _delay_ms(RELEY_INTERVAL);
    ValveDirection::Off(); // выкл. реверс
    _delay_ms(RELEY_INTERVAL);
}

// Вкл. краны в прямом направлении
void valveOnDirect()
{
    ValveDirection::Off(); // выкл. реверс
    _delay_ms(RELEY_INTERVAL);
    ValvePower::On(); // вкл. преобразователь
}

// Вкл. краны в обратном направлении
void valveOnRevers()
{
    ValveDirection::On(); // вкл. реверс
    _delay_ms(RELEY_INTERVAL);
    ValvePower::On(); // вкл. преобразователь
}

void setValve(bool status)
{
#ifdef SERIAL_LOG_MAIN_ON
    LOG("Переключение крана");
#endif
    Adc::enable();
    Adc::setInput(ADC3);

    uint8_t count = 5;
    uint16_t timeWork = 20; // при цикле через 500 мс - время включения противозастревательного
    // маневра - ~ 10 сек

    if (status == OPEN)
    {
        valveOnRevers(); // Направление - реверс
    }
    else
    {
        valveOnDirect();
    }
    while (count > 0)
    {
        if (getValveStatus() == 0)
        {
            count--;
        }

        // Получение тока крана
        // if (getValveCurr() == 0) {
        //         count = 0;
        //   }

        if (timeWork == 0)
        {
#ifdef SERIAL_LOG_MAIN_ON
            LOG("Противозастревательный маневр");
#endif
            if (status == OPEN)
            {
                valveOff();
                valveOnDirect();
            }
            else
            {
                valveOff();
                valveOnRevers();
            }
            _delay_ms(2000);
            if (status == OPEN)
            {
                valveOff();
                valveOnRevers(); // Направление - реверс
            }
            else
            {
                valveOff();
                valveOnDirect();
            }
            count = 5;
            timeWork = 20;
        }
        timeWork--;
        _delay_ms(500);
    }

    if (status == OPEN)
    {
        valveStatus = OPEN;
#ifdef SERIAL_LOG_MAIN_ON
        LOG("Кран открыт");
#endif
    }
    else
    {
        valveStatus = CLOSE;
#ifdef SERIAL_LOG_MAIN_ON
        LOG("Кран закрыт");
#endif
    }

#ifdef SERIAL_LOG_MAIN_ON
    LOG("Измерение батареи");
#endif
    // Измерение напряжения на батарее
    if (getVCC() <= MIN_BAT_LEVEL)
    {
        lowBat = 1;
    }

    Adc::disable();
    valveOff();
    nextCheckValv = time + INTERVAL_CHECK_VALV; // отложить проверку на закисание
    nextCheckBat = time + INTERVAL_CHECK_BAT;   // отложить проверку батареи
}

bool getValveStatus()
{
    uint16_t ADCavg = Adc::getAVGofN(AVG_NUMBER);
    if (ADCavg <= 1)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}