#include "usart.h"

void USART0_init() {
    /* seteaza baud rate la 115200 */
    UBRR0H = 0;
    UBRR0L = 8;

    /* porneste transmitatorul */
    UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);

    /* seteaza formatul frame-ului: 8 biti de date, 1 bit de stop */
    UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01);
}

void USART0_transmit(char data) {
    /* asteapta pana bufferul e gol */
    while (!(UCSR0A & (1 << UDRE0)));

    /* pune datele in buffer; transmisia va porni automat in urma scrierii */
    UDR0 = data;
}

char USART0_receive() {
    /* asteapta cat timp bufferul e gol */
    while (!(UCSR0A & (1 << RXC0)));

    /* returneaza datele din buffer */
    return UDR0;
}

void USART0_print(const char *data) {
    while (*data != '\0')
        USART0_transmit(*data++);
}

int USART0_printf(char data, FILE *stream) {
    /* asteapta pana bufferul e gol */
    while (!(UCSR0A & (1 << UDRE0)));

    /* pune datele in buffer; transmisia va porni automat in urma scrierii */
    UDR0 = data;
    return 0;
}

/* USART 1 --------------------------------------------------------------------------------*/
void USART1_init() {
    /* seteaza baud rate la 115200 */
    UBRR1H = 0;
    UBRR1L = 8;

    /* porneste transmitatorul */
    UCSR1B = (1 << TXEN1) | (1 << RXEN1);

    /* seteaza formatul frame-ului: 8 biti de date, 1 bit de stop, paritate even */
    UCSR1C |= (3 << UCSZ10) | (2 << UPM10);
}

void USART1_transmit(char data) {
    /* asteapta pana bufferul e gol */
    while (!(UCSR1A & (1 << UDRE1)));

    /* pune datele in buffer; transmisia va porni automat in urma scrierii */
    UDR1 = data;
}

char USART1_receive() {
    /* asteapta cat timp bufferul e gol */
    while (!(UCSR1A & (1 << RXC1)));

    /* returneaza datele din buffer */
    return UDR1;
}

void USART1_print(const char *data) {
    while (*data != '\0')
        USART1_transmit(*data++);
}

int USART1_printf(char data, FILE *stream) {
    /* asteapta pana bufferul e gol */
    while (!(UCSR1A & (1 << UDRE1)));

    /* pune datele in buffer; transmisia va porni automat in urma scrierii */
    UDR1 = data;
    return 0;
}