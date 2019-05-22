#ifndef PMAPI_H
#define PMAPI_H

#define onB(b) (PORTB) |= (1 << (b))
#define onC(x) (PORTC) |= (1 << (x))
#define onD(b) (PORTD) |= (1 << (b))

#define offB(b) (PORTB) &= ~(1 << (b))
#define offC(x) (PORTC) &= ~(1 << (x))
#define offD(b) (PORTD) &= ~(1 << (b))

// -------------------------------------------------------

#define setBtnA(x) DDRA &= ~(1 << x), PORTA |= (1 << x)
#define setBtnB(x) DDRB &= ~(1 << x), PORTB |= (1 << x)
#define setBtnC(x) DDRC &= ~(1 << x), PORTC |= (1 << x)
#define setBtnD(x) DDRD &= ~(1 << x), PORTD |= (1 << x)

#define isBtnPressedA(x) (!(PINA & (1 << x)))
#define isBtnPressedB(x) (!(PINB & (1 << x)))
#define isBtnPressedC(x) (PINC & (1 << x))
#define isBtnPressedD(x) (PIND & (1 << x))

#define setOutA(x) DDRA |= (1u << x)
#define setOutB(x) DDRB |= (1u << x)
#define setOutC(x) DDRC |= (1u << x)
#define setOutD(x) DDRD |= (1u << x)

// -------------------------------------------------------

#define stateA(x) (PINA & (1 << x))
#define stateB(x) (PINB & (1 << x))
#define stateC(x) (PINC & (1 << x))
#define stateD(x) (PIND & (1 << x))

#endif // PMAPI_H
