//#include <Wire.h>
//#include <avr/wdt.h>
#include <avr/sleep.h>
//#include <avr/power.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <gpio.h>
#include "settings.h"
#include "debug.cpp"
#include "sleepTimer.h"

#include "adc.h"
#include "valve.h"
#include "power.h"
#include "zummer.h"
#include "interrupts.h"
#include "time.h"

// Глобальные переменные
uint32_t nextCheckBat = INTERVAL_CHECK_BAT, nextCheckValv = INTERVAL_CHECK_VALV, nextSignal = INTERVAL_SIGNAL, nextLed = INTERVAL_LED;
// bool preventOn = 0;

Valve<valve1_power, valve1_driver_in_1, valve1_driver_in_2, valve1_adc> valve;
// Valve<valve2_power, valve2_direction, valve2_adc> valve2;

int main()
{
  _delay_ms(5000);
  LOG_BEGIN(9600);
  LOG("Start")
  SREG |= (1 << (SREG_I)); // глобально разрешить прерывания

  // DDRB = 0b00000001;
  // PORTB = 0b11111110; // неиспользуемые порты - вход с подтяжкоей к VCC, PB0 - выход светодиода

  // DDRD = 0b11100000;
  // PORTD = 0b00011111; // PD2, PD3 - вход с подтяжкой, PD5, PD7 - выход, PD6 - выход зуммера

  // DDRC = 0b00000000;
  // PORTC = 0b11110011; // PC2, PC3 - вход без подтяжки

  Led::SetDir(1);
  Zummer::SetDir(1);
  Button::SetDir(0); // вход
  Button::Set(1);    // вкл. подтяжку
  AlarmInput::SetDir(0);
  AlarmInput::Set(1);

  zummerInit();
  zummerRun(bip_2000);
  initSleepTimer();
  Adc::init(presc_128, ref_Vcc); //Делитель 128 = 64 кГц, опора на VCC
  // INT0init();
  getVCC(); // измерение батареи
  // Главный цикл
  while (1)
  {
    LOG("Loop start");
    time_update();

    // Проверка входа тревоги
    if (!AlarmInput::IsSet() && alarmFlag != 1)
    // && alarmFlag == 0)
    {
      LOG("Leak...");
      alarmFlag = 1;
      zummerRun(alarm);
    }

    // Обработка нажатия кнопки
    if (!Button::IsSet() && valve.getStatus() != RUNNING)
    {
      _delay_ms(20);
      if (!Button::IsSet())
      {
        LOG("Btn:click");
        if ((alarmFlag == 0) && (lowBat == 0))
        {
          // Нормальный режим работы
          // Переключение крана
          LOG("Btn:valve");
          if (valve.getPosition() == OPEN)
          {
            valve.setPosition(CLOSE);
          }
          else
          {
            valve.setPosition(OPEN);
          }
          zummerRun(button);
        }
        else
        {
          // сброс тревоги и отложить на сутки сигнал низкого заряда батареи
          LOG("Btn:rst alarm");
          if (alarmFlag == 1)
          {
            alarmFlag = 0;
            // GICR |= 1 << INT0; // вкл. INT0 прерывание
            zummerRun(bip_1000);
          }
          if (lowBat == 1)
          {
            zummerRun(battery_low);
          }
          nextSignal = time + 43200; // Отложить сигналы на сутки
        }
      }
    }

    // Проверка напряжения батареи
    if (time >= nextCheckBat)
    {
      LOG("Bat volt mes");
      getVCC();
      nextCheckBat = time + INTERVAL_CHECK_BAT;
    }

    valve.run();

    // Профилактика закисания крана - закрыть-открыть
    // if ((time >= nextCheckValv) && (alarmFlag == 0 || lowBat == 0))
    // {
    //   LOG("Prevent...");

    //   if (valve.getPosition() == OPEN && valve.getStatus() != RUNNING)
    //   {

    //     valve.setPosition(CLOSE);
    //   }

    //   if (valve.getPosition() == CLOSE && valve.getStatus() != RUNNING)
    //   {
    //     // preventOn == 0;
    //     valve.setPosition(OPEN);
    //     nextCheckValv = time + INTERVAL_CHECK_VALV;
    //   }
    //   nextCheckValv = time + INTERVAL_CHECK_VALV;
    // }

    // Звуковая сигнализация при тревоге и низком заряде батареи
    if (time >= nextSignal)
    {
      LOG("Sound signal");
      if (alarmFlag == 1)
      {
        zummerRun(alarm);
      }

      if (lowBat == 1)
      {
        zummerRun(battery_low);
      }
      if (lowBat && alarmFlag)
      {
        zummerRun(alarm_and_battery_low);
      }
      nextSignal = time + INTERVAL_SIGNAL;
    }

    // Мигание светодиодом
    if (time >= nextLed)
    {
      LOG("Led signal");
      if ((alarmFlag == 0) && (lowBat == 0))
      {
        Led::On();
        _delay_ms(1);
        Led::Off();
      }
      else
      {
        if (alarmFlag == 1)
        {
          for (uint8_t i = 3; i > 0; i--)
          {
            Led::On();
            _delay_ms(100);
            Led::Off();
            _delay_ms(100);
          }
        }
        if (lowBat == 1)
        {
          _delay_ms(500);
          for (uint8_t i = 2; i > 0; i--)
          {
            Led::On();
            _delay_ms(100);
            Led::Off();
            _delay_ms(100);
          }
        }
      }
      nextLed = time + INTERVAL_LED;
    }

#ifdef SERIAL_LOG_ON
    _delay_ms(1000);
    // Led::On();
    // Led::Off();
#endif

#ifndef SERIAL_LOG_ON
    // Если краны и зуммер отработали - уход в сон
    if (valve.getStatus() == DONE && !(zummerIsBusy()))
    {
      // Засыпаем
      set_sleep_mode(SLEEP_MODE_PWR_SAVE);
      sleep_enable();
      sleep_cpu();
      sleep_disable();
    }
#endif
  }
}
