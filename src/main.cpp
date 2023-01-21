#include <avr/sleep.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <gpio.h>
#include "settings.h"
#include "debug.cpp"
#include "sleepTimer.h"
#include "adc.h"
#include "valve_relay.h"
#include "power.h"
#include "zummer.h"
#include "interrupts.h"
#include "time.h"
#include <avr/wdt.h>

// Статический класс для управления краном
Valve<VALVE_POWER, VALVE_REL_ON, valve1_adc> valve;

int main()
{
  LOG_BEGIN(9600);
  LOG("Start")
  SREG |= (1 << (SREG_I)); // глобально разрешить прерывания

  /* Неиспользуемые порты - вход с подтяжкой (снижение энергопотребления) */
  // DDRC = 0b00000000;
  PORTC = 0b11010011;

  // DDRB = 0b00000000;
  PORTB = 0b11111000;

  // DDRD = 0b00000000;
  PORTD = 0b00010111;

  Led::SetDir(1);
  Zummer::SetDir(1);
  Button::SetDir(0);          // вход
  Button::Set(1);             // вкл. подтяжку
  AlarmInputPower::SetDir(0); // вход
  AlarmInputPower::Set(0);    // без подтяжки
  AlarmInput::SetDir(0);      // вход
  AlarmInput::Set(1);         // без подтяжки
  BatteryDivider::SetDir(1);  // выход
  BatteryDivider::Set(0);     // низкий уровень
  zummerInit();
  zummerRun(bip_2000);
  initSleepTimer();
  Adc::init(presc_128, ref_Vcc); // Делитель 128 = 64 кГц, опора на VCC
  getVCC();                      // измерение батареи
  wdt_enable(WDTO_2S);           // на VCC 3V период около 2.2 сек

  /* Главный цикл */
  while (1)
  {
    wdt_reset();
    LOG("Loop start");
    time_update();

    // Проверка входа тревоги
    if (alarmFlag != 1 && (valve.getPosition() == OPEN) && !AlarmInput::IsSet())
    {
      LOG("Leak...");
      alarmFlag = 1;
      zummerRun(alarm);
      AlarmInput::Set(0);         // выкл. подтяжку, входы для уменьшения энергопотребления
      AlarmInputPower::SetDir(0); // вход
      AlarmInputPower::Set(0);    // выкл. подтяжку
    }

    if (valve.getStatus() == STOPPED)
    {
      // Закрытие крана по тревоге
      if ((alarmFlag == 1 || lowBat == 1) && (valve.getPosition() == OPEN))
      {
        valve.setPosition(CLOSE);
      }

      // Обработка нажатия кнопки
      if (!Button::IsSet())
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
              zummerRun(button);
            }
            else
            {
              getVCC();
              if (lowBat == 0) // при низком заряде батареи не открывать
              {
                valve.setPosition(OPEN);
                // вкл. вход тревоги
                AlarmInputPower::Set(1);    // вкл. подтяжку
                AlarmInputPower::SetDir(1); // выход
                AlarmInput::Set(1);         // вкл. подтяжку
                zummerRun(button);
              }
            }
          }
          else
          {
            // сброс тревоги и отложить на сутки сигнал низкого заряда батареи
            LOG("Btn:rst alarm");
            if (alarmFlag == 1)
            {
              alarmFlag = 0;
              zummerRun(bip_1000);
              _delay_ms(1000);
            }
            if (lowBat == 1)
            {
              zummerRun(battery_low);
              nextSignal = time + 43200; // Отложить звуковой сигнал на сутки
            }
          }
        }
      }
    }

    valve.run();

    /* Проверка напряжения батареи */
    if (time >= nextCheckBat)
    {
      LOG("Bat volt mes");
      getVCC();
    }

    // Звуковая сигнализация при тревоге и низком заряде батареи
    if (time >= nextSignal)
    {
      LOG("Sound signal");
      if (alarmFlag == 1)
      {
        zummerRun(alarm);
      }
      else if (lowBat == 1)
      {
        zummerRun(battery_low);
      }
      else if (lowBat && alarmFlag)
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

    // Если краны и зуммер отработали - уход в сон
    if (valve.getStatus() == STOPPED && !(zummerIsBusy()))
    {
#ifdef SERIAL_LOG_ON
      Serial.flush(); // ждем окончания отправки по UART
#endif
      // Засыпаем
      wdt_reset();
      set_sleep_mode(SLEEP_MODE_PWR_SAVE);
      sleep_enable();
      sleep_cpu();
      sleep_disable();
      wdt_reset();
    }
  }
}
