#define F_CPU 16000000

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <string.h>

#include "pmapi.h"
#include "usart.h"
#include "esp8266.h"

void initDebug_USART1() {
    USART1_init();

    // redirect STDOUT to USART1
    FILE *serial = (FILE *) calloc(1, sizeof(FILE));
    serial->flags = __SWR;
    serial->put = USART1_printf;
    stdout = serial;
}

void initServoPWMs() {
    setOutC(PC2);
    onC(PC2);
    setOutC(PC3);
    offC(PC3);

    setOutD(PD5);  /* Make OC1A pin as output */
    TCNT1 = 0;     /* Set timer1 count zero */
    ICR1 = 5000;   /* Set TOP count for timer1 in ICR1 register */
    OCR1A = 5000;

    /* Set Fast PWM, TOP in ICR1, Clear OC1A on compare match, clk/64 */
    TCCR1A = (1 << WGM11) | (1 << COM1A1);
    TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS10) | (1 << CS11);
    while (1) {
        OCR1A = 70 * 2;    /* Set servo shaft at -90° position */
        _delay_ms(1500);
        OCR1A = 175 * 2;    /* Set servo shaft at 0° position */
        _delay_ms(1500);
        OCR1A = 300 * 2;    /* Set servo at +90° position */
        _delay_ms(3000);
    }
}

void initWiFi() {

}

int main() {
    USART0_init();
    sei(); // enable global interrupts

    initDebug_USART1();
    _delay_ms(3000);

    //initServoPWMs();

    initWiFi();

    char buffer[200];
    uint8_t connStatus;

    while (!ESP8266_Begin());

    ESP8266_WIFIMode(WIFI_MODE_STATION);
    ESP8266_ConnectionMode(SINGLE_CONN);
    ESP8266_ApplicationMode(NORMAL);
    if (ESP8266_connected() == ESP8266_NOT_CONNECTED_TO_AP)
        ESP8266_JoinAccessPoint(SSID, PASSWORD);

    _delay_ms(2000);

    if (ESP8266_Start(0, DOMAIN, PORT) != ESP8266_RESPONSE_FINISHED)
        printf("CONN not successful\n");
    printf(">>> TCP conn ready\n");

    _delay_ms(3000);

    while (true) {
        connStatus = ESP8266_connected();
        if (connStatus == ESP8266_NOT_CONNECTED_TO_AP)
            ESP8266_JoinAccessPoint(SSID, PASSWORD);
        if (connStatus == ESP8266_TRANSMISSION_DISCONNECTED)
            ESP8266_Start(0, DOMAIN, PORT);

        _delay_ms(3000);

        printf("sending UPDATE request\n");

        memset(buffer, 0, 200);
        sprintf(buffer,
                "GET / HTTP/1.1\r\nHost: servl.gear.host\r\nUser-Agent: esp\r\nAccept: */*\r\n\r\n");
        ESP8266_Send(buffer);
        Read_Data(buffer);
        printf(">>> Response%s\n", buffer);
    }

    return 0;
}
