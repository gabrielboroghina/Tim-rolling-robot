#define F_CPU 16000000

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <string.h>

#include "pmapi.h"
#include "usart.h"
#include "esp8266.h"

const char *UPDATE_REQUEST =
        "GET /get HTTP/1.1\r\n"
        "Host: tim-tim.7e14.starter-us-west-2.openshiftapps.com\r\n"
        "User-Agent: esp\r\n"
        "Accept: */*\r\n\r\n";

void initDebug_USART1() {
    USART1_init();

    // redirect STDOUT to USART1
    FILE *serial = (FILE *) calloc(1, sizeof(FILE));
    serial->flags = __SWR;
    serial->put = USART1_printf;
    stdout = serial;
}

void drive(int speed, int dir) {
    if (speed > 0) {
        // go forward
        onB(PB5);
        offB(PB6);

        OCR1A = speed * 16;
    } else if (speed < 0) {
        // go backward
        offB(PB5);
        onB(PB6);

        OCR1A = -speed * 16;
    } else {
        // brake
        offB(PB5);
        offB(PB6);

        OCR1A = 0;
    }
}

void initEnginePWM() {
    setOutB(PB5);
    setOutB(PB6);

    setOutD(PD5);
    TCCR1A = (2 << COM1A0) | (1 << WGM11); // phase correct PWM
    TCCR1B = (1 << CS10) | (1 << WGM13);   // prescaler clk/1

    TCNT1 = 0;    // Set timer1 count zero
    ICR1 = 1600;  // Set TOP count for timer1 in ICR1 register
    OCR1A = 0;

    setBtnB(PB2);
    setOutD(PD7);
}

ISR(INT2_vect) {
    if (OCR0B == 8) {
        OCR0B = 15;
    } else {
        OCR0B = 8;
    }
    PORTD ^= (1 << PD7);
    _delay_ms(1000);
}

void initServoPWMs() {
    // LEFT servo
    setOutB(PB4);  /* Make OC0B pin output */
    TCNT0 = 0;     /* Set timer0 count zero */
    OCR0B = 8;     /* init */

    /* Set Fast PWM, Clear OC0B on compare match, clk/1024 */
    TCCR0A = (1 << WGM01) | (1 << WGM00) | (1 << COM0B1);
    TCCR0B = (1 << CS02) | (1 << CS00);

    EICRA = (1 << ISC21);
    EIMSK = (1 << INT2);
}

/** Start WiFi module and try to connect to AP */
void initWiFi() {
    while (!ESP8266_Begin());

    ESP8266_WIFIMode(WIFI_MODE_STATION);
    ESP8266_ConnectionMode(SINGLE_CONN);
    ESP8266_ApplicationMode(NORMAL);
    if (ESP8266_Connected() == ESP8266_NOT_CONNECTED_TO_AP)
        ESP8266_JoinAccessPoint(SSID, PASSWORD);

    _delay_ms(2000);
}

int main() {
    USART0_init();
    sei(); // enable global interrupts

    initDebug_USART1();

    initEnginePWM();
    initServoPWMs();

    initWiFi();

    // connect to driving server
    ESP8266_Start(0, DOMAIN, PORT);
    _delay_ms(3000);

    char buffer[BUF_SIZE];
    uint8_t connStatus;
    int speed, dir;

    // continuously get parameters updates from server
    while (true) {
        connStatus = ESP8266_Connected();
        if (connStatus == ESP8266_NOT_CONNECTED_TO_AP)
            ESP8266_JoinAccessPoint(SSID, PASSWORD);
        if (connStatus == ESP8266_TRANSMISSION_DISCONNECTED)
            ESP8266_Start(0, DOMAIN, PORT);

        memset(buffer, 0, BUF_SIZE);
        ESP8266_Send(UPDATE_REQUEST);
        ESP8266_Read(buffer);

        // extract parameters from server's response
        char *beg = strchr(buffer, '*');
        char *sec = strchr(beg, ';');

        *sec = '\0';
        speed = atoi(beg + 3);
        dir = atoi(sec + 1);

        drive(speed, dir);
    }

    return 0;
}
