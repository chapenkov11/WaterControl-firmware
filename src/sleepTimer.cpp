#include <avr/io.h>
#include <util/delay.h>
#include "timer2.h"
#include "sleepTimer.h"

void initSleepTimer()
{
    // Частота кварца 32768 Гц
    // Частота счетчика 32678/предделитель
    // Длительность 1 тика = предделитель/32768 (сек)
    // Длительность 256 тиков = 256*предделитель/32768 = предделитель/128 (сек)

    // Предделитель      1 тик, сек         256 тиков, сек (переполнений)
    // 1024              0,03125            8
    // 256               0,0078125          2
    // 128               0,00390625         1
    // 64                ~0,001953          0,5
    // 32                ~0,0009765625      0,25
    // 8                 ~0,0002441         0,0625
    // 1                 ~0,000030517       0,0078125

    Timer2::init_async(presc_256);
    // разрешаем прерывания
    SREG |= (1 << (SREG_I)); // глобально разрешить прерывания
    //TIMSK |= (1<<(OCIE2)); // вкл. прерывание совпадения
    TIMSK |= (1 << (TOIE2)); // вкл. прерывание переполнения
}