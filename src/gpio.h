#ifndef gpio_h
#define gpio_h

#include <avr/io.h>
#include "gpio.cpp"

#ifdef PORTA
MAKE_PORT(PORTA, DDRA, PINA, PortA, 'A');

typedef TPin<PortA, 0> Pa0;
typedef TPin<PortA, 1> Pa1;
typedef TPin<PortA, 2> Pa2;
typedef TPin<PortA, 3> Pa3;
typedef TPin<PortA, 4> Pa4;
typedef TPin<PortA, 5> Pa5;
typedef TPin<PortA, 6> Pa6;
typedef TPin<PortA, 7> Pa7;
#endif

#ifdef PORTB
MAKE_PORT(PORTB, DDRB, PINB, PortB, 'B');
typedef TPin<PortB, 0> Pb0;
typedef TPin<PortB, 1> Pb1;
typedef TPin<PortB, 2> Pb2;
typedef TPin<PortB, 3> Pb3;
typedef TPin<PortB, 4> Pb4;
typedef TPin<PortB, 5> Pb5;
typedef TPin<PortB, 6> Pb6;
typedef TPin<PortB, 7> Pb7;
#endif

#ifdef PORTC
MAKE_PORT(PORTC, DDRC, PINC, PortC, 'C');
typedef TPin<PortC, 0> Pc0;
typedef TPin<PortC, 1> Pc1;
typedef TPin<PortC, 2> Pc2;
typedef TPin<PortC, 3> Pc3;
typedef TPin<PortC, 4> Pc4;
typedef TPin<PortC, 5> Pc5;
typedef TPin<PortC, 6> Pc6;
typedef TPin<PortC, 7> Pc7;
#endif

#ifdef PORTD
MAKE_PORT(PORTD, DDRD, PIND, PortD, 'D');
typedef TPin<PortD, 0> Pd0;
typedef TPin<PortD, 1> Pd1;
typedef TPin<PortD, 2> Pd2;
typedef TPin<PortD, 3> Pd3;
typedef TPin<PortD, 4> Pd4;
typedef TPin<PortD, 5> Pd5;
typedef TPin<PortD, 6> Pd6;
typedef TPin<PortD, 7> Pd7;
#endif

#endif