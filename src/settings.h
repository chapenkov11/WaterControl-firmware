#ifndef settings_h
#define settings_h

#include "gpio.h"

#define f_cpu 1000000 // частота микроконтроллера

#define VREF 2950          // опорное напряжение питания, мВ (подобрать для адекватного измерения напряжения батареи)
#define MIN_BAT_LEVEL 4500 // минимальное напряжение батареи, мВ

// Для отладочной платы
// Установка пинов МК
typedef Pd5 ValvePower;     // PD5 выход вкл преобразователя напряжения
typedef Pd7 ValveDirection; // PD7 выход реверса - включения реле
typedef Pd2 Alarm;          // PD2 вход тревоги с Gidrolock
typedef Pd3 Button;         // PD3 кнопка управления
typedef Pb0 Zummer;         // PD6 подключение базы транзистора пьезоизлучателя
typedef Pd4 Led;            // Сигнальный светодиод - PB0

// Установка пинов МК
// typedef Pd5 ValvePower;     // PD5 выход вкл преобразователя напряжения
// typedef Pd7 ValveDirection; // PD7 выход реверса - включения реле
// typedef Pd2 Alarm;          // PD2 вход тревоги с Gidrolock
// typedef Pd3 Button;         // PD3 кнопка управления
// typedef Pd6 Zummer;         // PD6 подключение базы транзистора пьезоизлучателя
// typedef Pb0 Led;            // Сигнальный светодиод - PB0
// PC3 (ADC3) - датчик тока кранов
// PC2 (ADC2) - напряжение батареи

// DEBUG
// #define SERIAL_LOG_MAIN_ON // лог всех действий в UART, в спящий режим не уходит
// #define DEBUG_INTERVAL // короткие интервалы для проверки функций

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
#define INTERVAL_CHECK_VALV 30    // профилактика закисания кранов
#define INTERVAL_SIGNAL 30        // интервал сигнализирования о проблеме (низкий заряд батареи, тревога)
#define INTERVAL_LED 5            // интервал мигания светодиодом
#endif

#define RELEY_INTERVAL 20 /* время в мс на между выключением преобразователя                                            \
                           и переключением реле. Чтобы не возникала искра, переключение реле только \
                           при выключенном преобразователе */

#define AVG_NUMBER 100 // количество измерений тока крана для усреднения

#endif