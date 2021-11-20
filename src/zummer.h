#ifndef zummer_h
#define zummer_h

// Макросы включения зуммера
#define zummerOn() TIMSK |= (1 << TOIE0); // вкл. прерывание переполнения
#define zummerOff()         \
    TIMSK &= ~(1 << TOIE0); \
    Zummer::Clear();
// выкл. прерывание совпадения

// Без зуммера
//#define zummerOn() PORTD &= ~(1<<PORT_ZUMMER); // вкл. прерывание переполнения
//#define zummerOff() PORTD &= ~(1<<PORT_ZUMMER); // выкл. прерывание совпадения

void initTimer0();
void stopTimer0();

#endif