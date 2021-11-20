//#include <avr/pgmspace.h>
//#include <Wire.h>
//#include <avr/wdt.h>
#include <avr/sleep.h>
//#include <avr/power.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "settings.h"
#include "debug.cpp"
#include "sleepTimer.h"
#include "gpio.h"
#include "adc.h"
#include "valve.h"
#include "power.h"
#include "zummer.h"
#include "interrupts.h"
#include "time.h"

// Глобальные переменные

uint32_t time = 0; // Системное время
uint32_t nextCheckBat = INTERVAL_CHECK_BAT, nextCheckValv = INTERVAL_CHECK_VALV, nextSignal = INTERVAL_SIGNAL, nextLed = INTERVAL_LED;
bool valveFlag = CLOSE;      // целевое положение крана (в которое нужно перевести)
bool valveStatus = OPEN;     // текущее положение крана
bool lowBat = 0;             // заряд батареи, 1 - низкий
volatile bool alarmFlag = 0; // 1 - тревога

int main()
{

  // DDRB = 0b00000001;
  // PORTB = 0b11111110; // неиспользуемые порты - вход с подтяжкоей к VCC, PB0 - выход светодиода

  // DDRD = 0b11100000;
  // PORTD = 0b00011111; // PD2, PD3 - вход с подтяжкой, PD5, PD7 - выход, PD6 - выход зуммера

  // DDRC = 0b00000000;
  // PORTC = 0b11110011; // PC2, PC3 - вход без подтяжки

#ifdef SERIAL_LOG_MAIN_ON
  LOG_BEGIN();
#endif
  Led::On(); // Вкл. сигнальный светодиод
  initTimer0();
  zummerOn();
  initSleepTimer();
  zummerOff();
  stopTimer0();
  Led::Off();
  Adc::init(presc_128, ref_Vcc); //Делитель 128 = 64 кГц, VCC
  INT0init();

  // Главный цикл
  while (1)
  {
    time_update();

    // Обработка нажатия кнопки
    if (!Button::IsSet())
    {
      _delay_ms(20);
      if (!Button::IsSet())
      {
        if ((alarmFlag == 0) && (lowBat == 0))
        {
// Нормальный режим работы
// Переключение крана
#ifdef SERIAL_LOG_MAIN_ON
          LOG("Кнопка: переключение");
#endif
          valveFlag = !valveFlag;
          // Звуковой сигнал
          initTimer0();
          zummerOn();
          Led::On();
          _delay_ms(200);
          zummerOff();
          Led::Off();
          stopTimer0();
        }
        else
        {
// сброс тревоги
#ifdef SERIAL_LOG_MAIN_ON
          LOG("Кнопка: сброс тревоги");
#endif
          if (alarmFlag == 1)
          {
            valveFlag = CLOSE;
            alarmFlag = 0;
            GICR |= 1 << INT0; // вкл. INT0 прерывание
            // Звуковой сигнал
            initTimer0();
            zummerOn();
            Led::On();
            _delay_ms(1000);
            zummerOff();
            Led::Off();
            stopTimer0();
          }
          if (lowBat == 1)
          {
            // Звуковой сигнал
            initTimer0();
            for (uint8_t i = 2; i > 0; i--)
            {
              zummerOn();
              Led::On();
              _delay_ms(500);
              zummerOff();
              Led::Off();
              _delay_ms(100);
            }
            stopTimer0();
          }
          nextSignal = time + 43200; // Отложить сигналы на сутки
        }
      }
    }

    // Проверка напряжения батареи
    if (time >= nextCheckBat)
    {
#ifdef SERIAL_LOG_MAIN_ON
      LOG("Проверка напряжения батареи");
#endif
      if (getVCC() <= MIN_BAT_LEVEL)
      {
        lowBat = 1;
      }
      nextCheckBat = time + INTERVAL_CHECK_BAT;
    }

    // Закрытие крана
    if (valveFlag == CLOSE && valveStatus == OPEN)
    {
      setValve(CLOSE);
    }

    // Закрытие крана по тревоге
    if ((alarmFlag == 1 || lowBat == 1) && valveStatus == OPEN)
    {
      setValve(CLOSE);
      valveFlag = CLOSE;
      // Звуковой сигнал
      initTimer0();
      for (uint8_t i = 3; i > 0; i--)
      {
        zummerOn();
        Led::On();
        _delay_ms(500);
        zummerOff();
        Led::Off();
        _delay_ms(100);
      }
      stopTimer0();
    }

    // Открытие крана
    if (valveFlag == OPEN && valveStatus == CLOSE && (alarmFlag == 0 || lowBat == 0))
    {
      setValve(OPEN);
    }

    // Профилактика закисания крана - закрыть-открыть
    if ((time >= nextCheckValv) && (alarmFlag == 0 || lowBat == 0))
    {
#ifdef SERIAL_LOG_MAIN_ON
      LOG("Профилактика закисания");
#endif
      if (valveStatus == OPEN)
      {
        setValve(CLOSE);
        if (lowBat == 0)
        {
          setValve(OPEN);
        }
      }
      nextCheckValv = time + INTERVAL_CHECK_VALV;
    }

    // Звуковая сигнализация при тревоге и низком заряде батареи
    if (time >= nextSignal)
    {
#ifdef SERIAL_LOG_MAIN_ON
      LOG("Сигнал");
#endif
      if (alarmFlag == 1)
      {
        initTimer0();
        for (uint8_t i = 3; i > 0; i--)
        {
          zummerOn();
          _delay_ms(500);
          zummerOff();
          _delay_ms(100);
        }
        stopTimer0();
      }

      if (lowBat == 1)
      {
        _delay_ms(1000);
        initTimer0();
        for (uint8_t i = 2; i > 0; i--)
        {
          zummerOn();
          _delay_ms(500);
          zummerOff();
          _delay_ms(100);
        }
        stopTimer0();
      }
      nextSignal = time + INTERVAL_SIGNAL;
    }

    // Мигание светодиодом
    if (time >= nextLed)
    {
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

#ifdef SERIAL_LOG_MAIN_ON
    Led::On();
    _delay_ms(2000);
    Led::Off();
#endif

#ifndef SERIAL_LOG_MAIN_ON
    // Если все хорошо - уход в сон
    if (valveFlag == valveStatus)
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
