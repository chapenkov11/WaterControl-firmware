// #include <Arduino.h>
//#include <avr/pgmspace.h>
//#include <Wire.h>
//#include <avr/wdt.h>
#include <avr/sleep.h>
//#include <avr/power.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "settings.cpp"
#include "debug.cpp"
#include "gpio.h"

// TPin<PortB, 4> Led1;
// TPin<PortB, 3> Led2;

// Led1.SetDirWrite();
// Led2.SetDirWrite();
// Led1.Set(1);
// LED_PORT |= 1 << LED_PIN;
// // LED::Set();
// // Led1.Set();
// Led2.Set();
// LED_PORT &= ~(1 << LED_PIN);
// // LED::Clear();
// // Led1.Clear();
// Led2.Clear();

// Положение крана
#define CLOSE 0
#define OPEN 1

volatile uint32_t sleepCount = 0;
uint32_t mainTimer;
uint32_t nextCheckBat = INTERVAL_CHECK_BAT, nextCheckValv = INTERVAL_CHECK_VALV, nextSignal = INTERVAL_SIGNAL, nextLed = INTERVAL_LED;

// Макросы включения зуммера
#define zummerOn() TIMSK |= (1 << TOIE0); // вкл. прерывание переполнения
#define zummerOff()       \
  TIMSK &= ~(1 << TOIE0); \
  Zummer::Clear();
// PORTD &= ~(1 << PORT_ZUMMER); // выкл. прерывание совпадения

// Без зуммера
//#define zummerOn() PORTD &= ~(1<<PORT_ZUMMER); // вкл. прерывание переполнения
//#define zummerOff() PORTD &= ~(1<<PORT_ZUMMER); // выкл. прерывание совпадения

bool valveFlag = CLOSE // целевое положение крана (в которое нужно перевести)
    ,
     valveStatus = OPEN // текущее положение крана
    ,
     lowBat = 0; // заряд батареи, 1 - низкий

volatile bool alarmFlag = 0; // 1 - тревога

//---Прототипы---------------------------
void setValve(bool status);
//bool getValveCurr();
bool getValveStatus();
void ADCinit();
void INT0init();
void initSleepTimer();
void resetSleepTimer();
void initTimer0();
void stopTimer0();
void setADCinput(uint8_t numADC);
uint16_t getVCC();
void valveOff();
void valveOnDirect();
void valveOnRevers();
//---------------------------

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
  ADCinit();
  INT0init();
  // Главный цикл
  while (1)
  {
    cli();
    mainTimer = sleepCount;
    sei();

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
          nextSignal = mainTimer + 43200; // Отложить сигналы на сутки
        }
      }
    }

    // Проверка напряжения батареи
    if (mainTimer >= nextCheckBat)
    {
#ifdef SERIAL_LOG_MAIN_ON
      LOG("Проверка напряжения батареи");
#endif
      if (getVCC() <= MIN_BAT_LEVEL)
      {
        lowBat = 1;
      }
      nextCheckBat = mainTimer + INTERVAL_CHECK_BAT;
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
    if ((mainTimer >= nextCheckValv) && (alarmFlag == 0 || lowBat == 0))
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
      nextCheckValv = mainTimer + INTERVAL_CHECK_VALV;
    }

    // Звуковая сигнализация при тревоге и низком заряде батареи
    if (mainTimer >= nextSignal)
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
      nextSignal = mainTimer + INTERVAL_SIGNAL;
    }

    // Мигание светодиодом
    if (mainTimer >= nextLed)
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
      nextLed = mainTimer + INTERVAL_LED;
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

//--------------------------------------

void INT0init()
{
  MCUCR &= ~((1 << ISC00) | (1 << ISC01)); // прерывание по низкому уровню
  GICR |= 1 << INT0;                       // вкл. INT0
  SREG |= 1 << SREG_I;                     // вкл. прерывания
}

ISR(INT0_vect)
{
  alarmFlag = 1;
  GICR &= ~(1 << INT0); // выкл. INT0
#ifdef SERIAL_LOG_MAIN_ON
  LOG("Тревога");
#endif
}

void setValve(bool status)
{
#ifdef SERIAL_LOG_MAIN_ON
  LOG("Переключение крана");
#endif
  ADCSRA |= (1 << ADEN); // вкл. АЦП
  setADCinput(3);

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

  ADCSRA &= ~(1 << ADEN); // выкл. АЦП
  valveOff();
  nextCheckValv = mainTimer + INTERVAL_CHECK_VALV; // отложить проверку на закисание
  nextCheckBat = mainTimer + INTERVAL_CHECK_BAT;   // отложить проверку батареи
}

// Измерение тока крана (для теста)
bool getValveCurr()
{
#define AVG_N 50
  // Сделать 1000 измерений и отправить в COM-порт
  for (uint32_t i = 0; i < 100; i++)
  {
    uint64_t ADCsumm = 0;
    for (uint16_t c = 0; c < AVG_N; c++)
    {
      ADCSRA |= (1 << ADSC); //Начинаем преобразование
      while ((ADCSRA & (1 << ADSC)))
        ; //проверим закончилось ли аналого-цифровое преобразование
      ADCsumm += ADC;
    }
    //Serial.println(round(ADCsumm/AVG_N)); // результат преобразования
    _delay_ms(100);
  }
  return 0;
}

void ADCinit()
{
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); //Делитель 128 = 64 кГц
  ADMUX &= ~((1 << REFS1) | (1 << REFS0));              // Varef
  ADMUX |= 1 << REFS0;                                  // ИОН = VCC, вход ADC0
}

bool getValveStatus()
{
  uint64_t ADCsumm = 0;
  for (uint8_t i = 0; i <= AVG_NUMBER; i++)
  {
    ADCSRA |= (1 << ADSC); // Начинаем преобразование
    while ((ADCSRA & (1 << ADSC)))
      ; // проверим закончилось ли аналого-цифровое преобразование
    ADCsumm += ADC;
  }
  uint16_t ADCavg = round(ADCsumm / AVG_NUMBER);
  if (ADCavg <= 1)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

void initSleepTimer()
{
  // atmega8a просыпается по таймеру2 только если он работает в асинхронном
  // режиме с тактированием от отдельного часового кварца
  // Частота тактирования от часового кварца 32,768 кГц (32768 Гц)
  // Подключается кварц так же как и обычный, при этом МК должен быть настроен
  // на внутреннее тактирование
  // При этом при частоте <400 кГц конденсаторы на землю ставить не надо
  // Установкой CKOPT Fuse можно подключить внутренние конденсаторы на 36pF

  //1. Отключите прерывания таймера/счетчика 2, очистив OCIE2 и TOIE2.
  TIMSK &= ~(1 << (OCIE2)); // откл. прерывание совпадения
  TIMSK &= ~(1 << (TOIE2)); // откл. прерывание переполнения
  //2. Выберите источник синхронизации, установив соответствующий параметр AS2.
  ASSR |= (1 << (AS2)); // вкл. асинхронный режим
  _delay_ms(2000);      // ожидание стабилизации частоты
  //3. Запишите новые значения в TCNT2, OCR2 и TCCR2.
  TCNT2 = (0); // регистр счетчика
  OCR2 = (255);
  // Частота кварца 32768 Гц
  // Частота счетчика 32678/предделитель
  // Длительность 1 тика = предделитель/32768 (сек)
  // Длительность 256 тиков = 256*предделитель/32768 = предделитель/128 (сек)

  // Установка предделителя 1024
  // CS 22-21-20 1-1-1
  // Итого 1/32768*1024 = 0,03125 сек - длительность счетчика
  // 8 сек можно посчитать на 256 тиков

  // Предделитель 256
  // CS 22-21-20 1-1-0
  // 1 тик = 256/32768 = 0,0078125 секунд
  // 256 тиков = 2 сек.

  // Предделитель 128
  // CS 22-21-20 1-0-1
  // 1 тик = 128/32768 = 0,00390625 секунд
  // 256 тиков = 1 сек.

  //TCCR2 |= (1<<(CS20))|(1<<(CS21))|(1<<(CS22)); // предделитель 1024
  TCCR2 |= (1 << (CS21)) | (1 << (CS22)); // предделитель 256
  //TCCR2 |= (1<<(CS20))|(1<<(CS22)); // предделитель 128

  //4. Чтобы переключиться на асинхронную работу: дождитесь TCN2UB, OCR2UB и TCR2UB.
  while ((ASSR & (1 << (TCN2UB))) | (ASSR & (1 << (OCR2UB))) | (ASSR & (1 << (TCR2UB))))
    ;

  //5. Снимите флаги прерывания Timer/Counter2.
  TIFR &= ~(1 << (OCF2)); // сброс флаг сравнения
  TIFR &= ~(1 << (TOV2)); // сброс флаг переполнения
  //6. Разрешить прерывания, если это необходимо.
  SREG |= (1 << (SREG_I)); // глобально разрешить прерывания
  //TIMSK |= (1<<(OCIE2)); // вкл. прерывание совпадения
  TIMSK |= (1 << (TOIE2)); // вкл. прерывание переполнения
}

ISR(TIMER2_OVF_vect)
{
  sleepCount++;
}

ISR(TIMER0_OVF_vect)
{
  // PORTD ^= (1 << PORT_ZUMMER);
  Zummer::Toggle();
}

void initTimer0()
{
  TIMSK &= ~(1 << TOIE0); // откл. прерывание переполнения
  //3. Запишите новые значения в TCNT2, OCR2 и TCCR2.
  TCNT0 = 0; // регистр счетчика

  // Частота тактирвоания 1000000 Гц
  // Частота счетчика част. такт./предделитель
  // Длительность 1 тика = предделитель/част. такт. (сек)
  // Длительность 256 тиков (время между переполнениями) = 256*предделитель/част. такт. (сек)
  // Длительность 256 тиков (время между переполнениями) для такта 1МГц = 256*предделитель/1000000

  // Установка предделителя 1024
  // CS 22-21-20 1-1-1
  // Итого 1024*256/1000000 = 0,262144 сек.

  // Предделитель 256
  // CS 22-21-20 1-1-0
  // 256 тиков = 256*256/1000000 = 0,065536 сек.

  // Предделитель 64
  // CS 22-21-20 1-0-1
  // 256 тиков = 64*256/1000000 = 0,016384 сек.

  // Предделитель 8
  // CS 02-01-00 0-1-0
  // 256 тиков = 8*256/1000000 = 0,002048 сек.

  // Предделитель 1
  // CS 22-21-20 1-0-1
  // 256 тиков = 1*256/1000000 = 0,000256 сек.

  TCCR0 |= (1 << (CS00)); // предделитель 1
  //TCCR0 |= (1<<(CS01)); // предделитель 8
  //TCCR0 |= (1<<CS02); // предделитель 256
  //TCCR0 |= (1<<CS00)|(1<<CS01); // предделитель 64
  //TCCR0 |= (1<<CS00)|(1<<CS02); // предделитель 1024

  //5. Снимите флаги прерывания Timer/Counter0.
  TIFR &= ~(1 << TOV0); // сброс флаг переполнения
  //6. Разрешить прерывания, если это необходимо.
  SREG |= (1 << (SREG_I)); // глобально разрешить прерывания
  //TIMSK |= (1 << TOIE0); // вкл. прерывание переполнения
}

void stopTimer0()
{
  TIMSK &= ~(1 << TOIE0);                              // откл. прерывание переполнения
  TCCR0 &= ~((1 << CS00) | (1 << CS01) | (1 << CS02)); // остановка таймера
  TIFR &= ~(1 << TOV0);                                // сброс флаг переполнения
}

void setADCinput(uint8_t numADC)
{
  ADMUX &= ~((1 << MUX0) | (1 << MUX1) | (1 << MUX2) | (1 << MUX3)); // установка всех бит в 0 = вход ADC0
  switch (numADC)
  {
  case 0:
    break;
  case 2:
    ADMUX |= (1 << MUX1); // ADC2
    break;
  case 3:
    ADMUX |= (1 << MUX0) | (1 << MUX1); // вход ADC3;
    break;
  }
  // пробное преобразование для стабилизации результата
  ADCSRA |= (1 << ADSC); //Начинаем преобразование
  while ((ADCSRA & (1 << ADSC)))
    ; //проверим закончилось ли аналого-цифровое преобразование
}

uint16_t getVCC()
{
  ADCSRA |= (1 << ADEN); // вкл. АЦП
  setADCinput(2);
  if (valveStatus == OPEN)
  {
    valveOnRevers();
  }
  else
  {
    valveOnDirect();
  }
  // Замеряем напряжение батареи
  uint32_t ADCsumm = 0;
  for (uint8_t i = 0; i <= 50; i++)
  {
    ADCSRA |= (1 << ADSC); //Начинаем преобразование
    while ((ADCSRA & (1 << ADSC)))
      ; //проверим закончилось ли аналого-цифровое преобразование
    ADCsumm += ADC;
  }
  // Выкл. преобзователь и реле
  valveOff();
  ADCSRA &= ~(1 << ADEN); // выкл. АЦП
  // Делитель 2000/1000 Om
  // r1 = 2000, r2 = 1000
  // VCC = U2*(r1+r2)/r2
  // U2=Vref*ADCavg/1024
  // VCC = Vref*ADCavg*(r1+r2)/1024*r2
  // VCC = Vref*ADCavg*(3000/1024*1000)
  // VCC = Vref*ADCavg*(3/1024)

  uint16_t volt = round((VREF * ADCsumm * 3) / 51200);

#ifdef SERIAL_LOG_MAIN_ON
  Serial.print("Напряжение батареи: ");
  Serial.println(volt);
#endif
  return volt;
}

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
