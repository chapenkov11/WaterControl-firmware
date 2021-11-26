#include <avr/io.h>
#include <avr/pgmspace.h>

#include "zummer.h"
#include "timer0.h"
#include <avr/interrupt.h>
#include "settings.h"

#define ZUMMER_FREQ f_cpu / (256 * 2)

#define NUMBER_SEMIPERIOD(ms) (int)ms * 2 * ZUMMER_FREQ / 1000L

volatile uint8_t sound_index = 0; // индекс текущего сигнала

// Паттерны сигналов
const int PROGMEM alarm[] = {NUMBER_SEMIPERIOD(500), NUMBER_SEMIPERIOD(-100), NUMBER_SEMIPERIOD(500), NUMBER_SEMIPERIOD(-100), NUMBER_SEMIPERIOD(500), 0};
const int PROGMEM battery_low[] = {NUMBER_SEMIPERIOD(500), NUMBER_SEMIPERIOD(-100), NUMBER_SEMIPERIOD(500), 0};
const int PROGMEM button[] = {NUMBER_SEMIPERIOD(200), 0};
const int PROGMEM alarm_and_battery_low[] = {NUMBER_SEMIPERIOD(500), NUMBER_SEMIPERIOD(-100), NUMBER_SEMIPERIOD(500), NUMBER_SEMIPERIOD(-100), NUMBER_SEMIPERIOD(500), NUMBER_SEMIPERIOD(-500), NUMBER_SEMIPERIOD(500), NUMBER_SEMIPERIOD(-100), NUMBER_SEMIPERIOD(500), 0};
const int PROGMEM bip_1000[] = {NUMBER_SEMIPERIOD(1000), 0};
const int PROGMEM bip_2000[] = {7812, 0};

volatile const int *sound_list; // указатель на текущую длительность сигнала
volatile int period_counter;

void zummerInit()
{
    // Частота тактирвоания 1000000 Гц
    // Частота счетчика част. такт./предделитель
    // Длительность 1 тика = предделитель/част. такт. (сек)
    // Длительность 256 тиков (время между переполнениями) = 256*предделитель/част. такт. (сек)
    // Длительность 256 тиков (время между переполнениями) для такта 1МГц = 256*предделитель/1000000
    // Частота 1 тика = частота тактирования/предделитель
    // Частота 256 тиков = частота тактирования/(предделитель*256)

    // Предделитель      1 тик, сек         256 тиков, сек (переполнений)
    // 1                 1 мкс (1000000 Гц)              256 мкс (3906,25 Гц)
    // 8                 8 мкс              0,02048 сек (48 Гц)
    // 64                64 мкс             0,16384 сек
    // 256               256 мкс            0,065536 сек
    // 1024              1,024 мс           0,262144 сек

    Timer0::init(presc_1); // частота 3906,25 Гц/2 = 1953,125
    TIFR &= ~(1 << TOV0);  // сброс флаг переполнения
}

void zummerStart()
{
    TIFR &= ~(1 << TOV0);  // сброс флаг переполнения
    TIMSK |= (1 << TOIE0); // вкл. прерывание
}

void zummerStop()
{
    Zummer::Off();
    Led::Off();
    TIMSK &= ~(1 << TOIE0); // откл. прерывание переполнения
}

void zummerRun(const int *pattern)
{
    sound_list = pattern;
    sound_index = 0;
    period_counter = pgm_read_word(sound_list);
    zummerStart();
}

bool zummerIsBusy()
{
    return TIMSK & (1 << TOIE0);
}

ISR(TIMER0_OVF_vect)
{
    if (period_counter == 0)
    {
        sound_index++;
        period_counter = (int)pgm_read_word(sound_list + sound_index);
        if (period_counter == 0)
        {
            zummerStop();
        }
    }
    else if (period_counter > 0)
    {
        period_counter--;
        Zummer::Toggle();
        Led::On();
    }
    else if (period_counter < 0)
    {
        period_counter++;
        Zummer::Off();
        Led::Off();
    }
}