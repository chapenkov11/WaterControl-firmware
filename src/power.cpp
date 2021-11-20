#include <util/delay.h>
#include "adc.h"
#include "valve.h"
#include "settings.h"

uint16_t getVCC()
{
    Adc::enable();
    Adc::setInput(ADC2);
    if (valveStatus == OPEN)
    {
        valveOnRevers();
    }
    else
    {
        valveOnDirect();
    }
    // Замеряем напряжение батареи
    uint16_t AVG = Adc::getAVGofN(50);
    // Выкл. преобзователь и реле
    valveOff();
    Adc::disable();
    // Делитель 2000/1000 Om
    // r1 = 2000, r2 = 1000
    // VCC = U2*(r1+r2)/r2
    // U2=Vref*ADCavg/1024
    // VCC = Vref*ADCavg*(r1+r2)/1024*r2
    // VCC = Vref*ADCavg*(3000/1024*1000)
    // VCC = Vref*ADCavg*(3/1024)

    uint16_t volt = round((VREF * AVG * 3) / 1024);

#ifdef SERIAL_LOG_MAIN_ON
    Serial.print("Напряжение батареи: ");
    Serial.println(volt);
#endif
    return volt;
}