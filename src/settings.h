#ifndef settings_h
#define settings_h

#include "gpio.h"

#define f_cpu 1000000 // частота микроконтроллера

/* DEBUG */
#define SERIAL_LOG_ON  // лог всех действий в UART, в спящий режим не уходит
#define DEBUG_INTERVAL // короткие интервалы для проверки функций

// Настройка измерения напряжения батареи
#define VCC 3000UL           // напряжение питания МК, мВ (подобрать для адекватного измерения напряжения батареи)
#define MIN_BAT_LEVEL 4500UL // минимальное напряжение батареи, мВ
#define V_BAT_CORR 0         // поправка к отображаемому напряжению (прибавляется к нему) (подобрать для правильного отображения напряжения батареи)
#define DIVIDER_COEF 20      // 3     // коэффициент делителя напряжения (верхний резистор разделить на нижний)
#define BAT_AVG_NUMBER 50    // количество измерений напряжения батареи для усреднения
#define battery_adc ADC2     // вход АЦП для измерения напряжения батареи
typedef Pd5 BatteryDivider;  // пин подключения делителя напряжения при измерении

// Установка пинов МК
// Кран
typedef Pc5 VALVE_POWER;  // вкл. dc-преобразователя
typedef Pd7 VALVE_REL_ON; // вкл. реле реверса
#define valve1_adc ADC3   // датчик тока крана

/* Входы датчиков и кнопок, выходы индикаторов */
typedef Pb1 AlarmInput;      // вход тревоги с Gidrolock
typedef Pb2 AlarmInputPower; // подача питания на схему детекции тревоги
typedef Pd3 Button;          // кнопка управления
typedef Pd6 Zummer;          // подключение базы транзистора пьезоизлучателя
typedef Pb0 Led;             // cигнальный светодиод

// Установка пинов МК
// typedef Pd5 ValvePower;     // PD5 выход вкл преобразователя напряжения
// typedef Pd7 ValveDirection; // PD7 выход реверса - включения реле
// typedef Pd2 Alarm;          // PD2 вход тревоги с Gidrolock
// typedef Pd3 Button;         // PD3 кнопка управления
// typedef Pd6 Zummer;         // PD6 подключение базы транзистора пьезоизлучателя
// typedef Pb0 Led;            // Сигнальный светодиод - PB0
// PC3 (ADC3) - датчик тока кранов
// PC2 (ADC2) - напряжение батареи

// Интервалы времени
#ifndef DEBUG_INTERVAL

// Нормальный режим
#define SLEEP_PERIOD_PER_MINUT 30L                                // количество периодов таймера сна в минуту
#define INTERVAL_CHECK_BAT SLEEP_PERIOD_PER_MINUT * 60 * 24       // 1 раз в сутки - проверка напряжения батареи
#define INTERVAL_CHECK_VALV SLEEP_PERIOD_PER_MINUT * 60 * 24 * 14 // 2 недели - профилактика закисания кранов
#define INTERVAL_SIGNAL SLEEP_PERIOD_PER_MINUT * 15               // 15 мин - интервал сигнализирования о проблеме (низкий заряд батареи, тревога)
#define INTERVAL_LED 10                                           // 20 сек - интервал мигания светодиодом
#else

// Отладка
#define SLEEP_PERIOD_PER_MINUT 30 // количество периодов режима сна в минуту
#define INTERVAL_CHECK_BAT 60     // проверка напряжения батареи
#define INTERVAL_CHECK_VALV 15    // профилактика закисания кранов
#define INTERVAL_SIGNAL 15        // интервал сигнализирования о проблеме (низкий заряд батареи, тревога)
#define INTERVAL_LED 10           // интервал мигания светодиодом
#endif

#define RELEY_INTERVAL 20 /* время в мс на между выключением преобразователя                                            \
                           и переключением реле. Чтобы не возникала искра, переключение реле только \
                           при выключенном преобразователе */

#define VALVE_AVG_NUMBER 100 // количество измерений тока крана для усреднения
#define DONE_NUMBER 5        // количество получения нулевых измерения тока крана, чтобы кран считался остановившимся
#define MAX_SWITCH_TIME 6    // (12 сек) максимальное время на переключение крана, в циклах time
#define PAUSE_TIME 2         // (4 сек) длительность паузы на остывание преобразователя, в циклах time
#define REVERS_TIME 2        // (4 сек) длительность противозаклинивающего маневра, в циклах time

#endif