#include "esp8266.h"

#define F_CPU 16000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include "usart.h"

int8_t responseStatus;
volatile int16_t responseLen = 0, pointer = 0;
char responseBuf[BUF_SIZE];

void ESP8266_ReadResponse(const char *expectedResponse) {
    uint8_t expectedLen = strlen(expectedResponse);
    uint32_t timeCount = 0, responseLength;
    char RECEIVED_CRLF_BUF[expectedLen];

    while (true) {
        if (timeCount >= DEFAULT_TIMEOUT) {
            responseStatus = ESP8266_RESPONSE_TIMEOUT;
            return;
        }

        responseLength = strlen(responseBuf);
        if (responseLength) {
            _delay_ms(1);
            timeCount++;
            if (responseLength == strlen(responseBuf)) {
                for (uint16_t i = 0; i < responseLength; i++) {
                    memmove(RECEIVED_CRLF_BUF, RECEIVED_CRLF_BUF + 1, expectedLen - 1);
                    RECEIVED_CRLF_BUF[expectedLen - 1] = responseBuf[i];
                    if (!strncmp(RECEIVED_CRLF_BUF, expectedResponse, expectedLen)) {
                        responseStatus = ESP8266_RESPONSE_FINISHED;
                        return;
                    }
                }
            }
        }
        _delay_ms(1);
        timeCount++;
    }
}

void ESP8266_Clear() {
    memset(responseBuf, 0, BUF_SIZE);
    responseLen = 0;
    pointer = 0;
}

/** @return true for success, false otherwise */
bool WaitForExpectedResponse(const char *expectedResponse) {
    ESP8266_ReadResponse(expectedResponse);

    return responseStatus != ESP8266_RESPONSE_TIMEOUT;
}

void sendAT(const char *ATCmd) {
    ESP8266_Clear();

    /* send AT command to ESP8266 */
    USART0_print(ATCmd);
    USART0_print("\r\n");
}

bool SendATandExpectResponse(const char *ATCommand, const char *expectedResponse) {
    ESP8266_Clear();

    /* send AT command to ESP8266 */
    USART0_print(ATCommand);
    USART0_print("\r\n");

    return WaitForExpectedResponse(expectedResponse);
}

bool ESP8266_ApplicationMode(uint8_t Mode) {
    char ATCmd[20];
    memset(ATCmd, 0, 20);
    sprintf(ATCmd, "AT+CIPMODE=%d", Mode);
    ATCmd[19] = 0;
    return SendATandExpectResponse(ATCmd, "\r\nOK\r\n");
}

void ESP8266_deepSleep() {
    char ATCmd[20];
    memset(ATCmd, 0, 20);
    sprintf(ATCmd, "AT+GSLP=5000");
    ATCmd[19] = 0;

    sendAT(ATCmd);
}

bool ESP8266_ConnectionMode(uint8_t mode) {
    char ATCmd[20];
    memset(ATCmd, 0, 20);
    sprintf(ATCmd, "AT+CIPMUX=%d", mode);
    ATCmd[19] = 0;
    return SendATandExpectResponse(ATCmd, "\r\nOK\r\n");
}

bool ESP8266_Begin() {
    for (uint8_t i = 0; i < 5; i++) {
        if (SendATandExpectResponse("ATE0", "\r\nOK\r\n") ||
            SendATandExpectResponse("AT", "\r\nOK\r\n"))
            return true;
    }
    return false;
}

bool ESP8266_Close() {
    return SendATandExpectResponse("AT+CIPCLOSE=1", "\r\nOK\r\n");
}

bool ESP8266_WIFIMode(uint8_t mode) {
    char ATCmd[20];
    memset(ATCmd, 0, 20);
    sprintf(ATCmd, "AT+CWMODE=%d", mode);
    ATCmd[19] = 0;
    return SendATandExpectResponse(ATCmd, "\r\nOK\r\n");
}

uint8_t ESP8266_JoinAccessPoint(const char *_SSID, const char *_PASSWORD) {
    char ATCmd[60];
    memset(ATCmd, 0, 60);
    sprintf(ATCmd, "AT+CWJAP=\"%s\",\"%s\"", _SSID, _PASSWORD);
    ATCmd[59] = 0;
    if (SendATandExpectResponse(ATCmd, "\r\nWIFI CONNECTED\r\n")) {
        return ESP8266_WIFI_CONNECTED;
    } else {
        if (strstr(responseBuf, "+CWJAP:1"))
            return ESP8266_CONNECTION_TIMEOUT;
        else if (strstr(responseBuf, "+CWJAP:2"))
            return ESP8266_WRONG_PASSWORD;
        else if (strstr(responseBuf, "+CWJAP:3"))
            return ESP8266_NOT_FOUND_TARGET_AP;
        else if (strstr(responseBuf, "+CWJAP:4"))
            return ESP8266_CONNECTION_FAILED;
        else
            return ESP8266_JOIN_UNKNOWN_ERROR;
    }
}

uint8_t ESP8266_Connected() {
    SendATandExpectResponse("AT+CIPSTATUS", "\r\nOK\r\n");
    if (strstr(responseBuf, "STATUS:2"))
        return ESP8266_CONNECTED_TO_AP;
    else if (strstr(responseBuf, "STATUS:3"))
        return ESP8266_CREATED_TRANSMISSION;
    else if (strstr(responseBuf, "STATUS:4"))
        return ESP8266_TRANSMISSION_DISCONNECTED;
    else if (strstr(responseBuf, "STATUS:5"))
        return ESP8266_NOT_CONNECTED_TO_AP;
    else
        return ESP8266_CONNECT_UNKNOWN_ERROR;
}

uint8_t ESP8266_Start(uint8_t _ConnectionNumber, const char *Domain, const char *Port) {
    bool _startResponse;
    char ATCmd[100];
    memset(ATCmd, 0, 100);

    _delay_ms(3000);
    if (SendATandExpectResponse("AT+CIPMUX?", "CIPMUX:0"))
        sprintf(ATCmd, "AT+CIPSTART=\"TCP\",\"%s\",%s", Domain, Port);
    else
        sprintf(ATCmd, "AT+CIPSTART=\"TCP\",\"%s\",%s", Domain, Port);

    _startResponse = SendATandExpectResponse(ATCmd, "CONNECT\r\n");
    if (!_startResponse) {
        if (responseStatus == ESP8266_RESPONSE_TIMEOUT)
            return ESP8266_RESPONSE_TIMEOUT;
        return ESP8266_RESPONSE_ERROR;
    }
    return ESP8266_RESPONSE_FINISHED;
}

uint8_t ESP8266_Send(const char *Data) {
    char ATCmd[20];
    memset(ATCmd, 0, 20);
    sprintf(ATCmd, "AT+CIPSEND=%d", (strlen(Data) + 2));

    sendAT(ATCmd);
    while (responseBuf[responseLen - 2] != '>')
        _delay_ms(5);

    sendAT(Data);
    while (!strstr(responseBuf, "IPD"))
        _delay_ms(10);

    return ESP8266_RESPONSE_FINISHED;
}

uint8_t ESP8266_DataRead() {
    if (pointer < responseLen)
        return responseBuf[pointer++];

    ESP8266_Clear();
    return 0;
}

/** Get received data from ESP8266 */
uint16_t ESP8266_Read(char *buffer) {
    uint16_t len = 0;
    _delay_ms(30);
    while (pointer < responseLen)
        buffer[len++] = ESP8266_DataRead();
    buffer[len] = '\0';
    return len;
}

/** Received data ISR */
ISR (USART0_RX_vect) {
    uint8_t oldsrg = SREG;
    cli();
    responseBuf[responseLen++] = UDR0;

    if (responseLen == BUF_SIZE) {
        responseLen = 0;
        pointer = 0;
    }
    SREG = oldsrg;
}
