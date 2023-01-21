#include "power.h"
#include <util/delay.h>
#include "adc.h"
#include "settings.h"
#include "zummer.h"
#include "debug.h"

bool lowBat = 0; // заряд батареи, 1 - низкий

uint16_t getVCC()
{
    Adc::enable();
    Adc::setInput(battery_adc);
    BatteryDivider::On(); // подключить делитель

    // Замеряем напряжение батареи
    uint16_t ADCavg = Adc::getAVGofN(BAT_AVG_NUMBER);
    // LOG(ADCavg);
    Adc::disable();
    BatteryDivider::Off(); // выключить делитель

    // Делитель 2000/1000 Om
    // r1 = 2000, r2 = 1000
    // VCC = U2*(r1+r2)/r2
    // U2=Vref*ADCavg/1024
    // VCC = Vref*ADCavg*(r1+r2)/1024*r2
    // VCC = Vref*ADCavg*(3000/1024*1000)
    // VCC = Vref*ADCavg*(3/1024)

#ifdef SERIAL_LOG_ON
    LOG("U_bat:");
    LOG((ADCavg * VCC * DIVIDER_COEF / 1023) - V_BAT_CORR);
#endif

    if (ADCavg <= (MIN_BAT_LEVEL + V_BAT_CORR) * 1023 / (VCC * DIVIDER_COEF))
    {
        lowBat = 1;
        zummerRun(battery_low);
        LOG("lowBat = 1");
    }
    nextCheckBat = time + INTERVAL_CHECK_BAT; // отложить проверку батареи
    return ADCavg;
}