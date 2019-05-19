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
uint32_t timeout = 0;
char responseBuf[BUF_SIZE];

void Read_Response(char *expectedResponse) {
    uint8_t EXPECTED_RESPONSE_LENGTH = strlen(expectedResponse);
    uint32_t timeCount = 0, ResponseBufferLength;
    char RECEIVED_CRLF_BUF[EXPECTED_RESPONSE_LENGTH];

    while (true) {
        if (timeCount >= DEFAULT_TIMEOUT + timeout) {
            timeout = 0;
            responseStatus = ESP8266_RESPONSE_TIMEOUT;
            return;
        }

        ResponseBufferLength = strlen(responseBuf);
        if (ResponseBufferLength) {
            _delay_ms(1);
            timeCount++;
            if (ResponseBufferLength == strlen(responseBuf)) {
                for (uint16_t i = 0; i < ResponseBufferLength; i++) {
                    memmove(RECEIVED_CRLF_BUF, RECEIVED_CRLF_BUF + 1, EXPECTED_RESPONSE_LENGTH - 1);
                    RECEIVED_CRLF_BUF[EXPECTED_RESPONSE_LENGTH - 1] = responseBuf[i];
                    if (!strncmp(RECEIVED_CRLF_BUF, expectedResponse, EXPECTED_RESPONSE_LENGTH)) {
                        timeout = 0;
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

void Start_Read_Response(char *expectedResponse) {
    Read_Response(expectedResponse);
}

void GetResponseBody(char *Response, uint16_t ResponseLength) {

    uint16_t i = 12;
    char buffer[5];
    while (Response[i] != '\r')
        ++i;

    strncpy(buffer, Response + 12, (i - 12));
    ResponseLength = atoi(buffer);

    i += 2;
    uint16_t tmp = strlen(Response) - i;
    memcpy(Response, Response + i, tmp);

    if (!strncmp(Response + tmp - 6, "\r\nOK\r\n", 6))
        memset(Response + tmp - 6, 0, i + 6);
}

/** @return true for success, false otherwise */
bool WaitForExpectedResponse(char *expectedResponse) {
    Start_Read_Response(expectedResponse); /* read response */

    if (responseStatus != ESP8266_RESPONSE_TIMEOUT)
        return true;
    return false;
}

void sendAT(char *ATCmd) {
    ESP8266_Clear();

    /* send AT command to ESP8266 */
    USART0_print(ATCmd);
    USART0_print("\r\n");
}

bool SendATandExpectResponse(char *ATCommand, char *expectedResponse) {
    ESP8266_Clear();

    /* send AT command to ESP8266 */
    USART0_print(ATCommand);
    USART0_print("\r\n");

    return WaitForExpectedResponse(expectedResponse);
}

bool ESP8266_ApplicationMode(uint8_t Mode) {
    char _atCommand[20];
    memset(_atCommand, 0, 20);
    sprintf(_atCommand, "AT+CIPMODE=%d", Mode);
    _atCommand[19] = 0;
    return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

void ESP8266_deepSleep() {
    char _atCommand[20];
    memset(_atCommand, 0, 20);
    sprintf(_atCommand, "AT+GSLP=5000");
    _atCommand[19] = 0;

    sendAT(_atCommand);
}

bool ESP8266_ConnectionMode(uint8_t mode) {
    char _atCommand[20];
    memset(_atCommand, 0, 20);
    sprintf(_atCommand, "AT+CIPMUX=%d", mode);
    _atCommand[19] = 0;
    return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
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
    char _atCommand[20];
    memset(_atCommand, 0, 20);
    sprintf(_atCommand, "AT+CWMODE=%d", mode);
    _atCommand[19] = 0;
    return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

uint8_t ESP8266_JoinAccessPoint(char *_SSID, char *_PASSWORD) {
    char _atCommand[60];
    memset(_atCommand, 0, 60);
    sprintf(_atCommand, "AT+CWJAP=\"%s\",\"%s\"", _SSID, _PASSWORD);
    _atCommand[59] = 0;
    if (SendATandExpectResponse(_atCommand, "\r\nWIFI CONNECTED\r\n")) {
        return ESP8266_WIFI_CONNECTED;
    } else {
        printf("WiFi connecting error\r\n");
        printf("%s\n", responseBuf);
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

uint8_t ESP8266_connected() {
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

uint8_t ESP8266_Start(uint8_t _ConnectionNumber, char *Domain, char *Port) {
    bool _startResponse;
    char _atCommand[100];
    memset(_atCommand, 0, 100);

    _delay_ms(3000);
    if (SendATandExpectResponse("AT+CIPMUX?", "CIPMUX:0"))
        sprintf(_atCommand, "AT+CIPSTART=\"TCP\",\"%s\",%s", Domain, Port);
    else
        sprintf(_atCommand, "AT+CIPSTART=\"TCP\",\"%s\",%s", Domain, Port);
//        sprintf(_atCommand, "AT+CIPSTART=\"%d\",\"TCP\",\"%s\",%s", _ConnectionNumber, Domain,
//                Port);

    _startResponse = SendATandExpectResponse(_atCommand, "CONNECT\r\n");
    if (!_startResponse) {
        if (responseStatus == ESP8266_RESPONSE_TIMEOUT)
            return ESP8266_RESPONSE_TIMEOUT;
        return ESP8266_RESPONSE_ERROR;
    }
    return ESP8266_RESPONSE_FINISHED;
}

uint8_t ESP8266_Send(char *Data) {
    char _atCommand[20];
    memset(_atCommand, 0, 20);
    sprintf(_atCommand, "AT+CIPSEND=%d", (strlen(Data) + 2));

    SendATandExpectResponse(_atCommand, "\r\nOK\r\n>");
    if (!SendATandExpectResponse(Data, "\r\nSEND OK\r\n")) {
        if (responseStatus == ESP8266_RESPONSE_TIMEOUT)
            return ESP8266_RESPONSE_TIMEOUT;
        return ESP8266_RESPONSE_ERROR;
    }
    return ESP8266_RESPONSE_FINISHED;
}

int16_t ESP8266_DataAvailable() {
    return (responseLen - pointer);
}

uint8_t ESP8266_DataRead() {
    if (pointer < responseLen)
        return responseBuf[pointer++];
    else {
        ESP8266_Clear();
        return 0;
    }
}

uint16_t Read_Data(char *buffer) {
    uint16_t len = 0;
    _delay_ms(100);
    while (ESP8266_DataAvailable() > 0)
        buffer[len++] = ESP8266_DataRead();
    buffer[len] = '\0';
    return len;
}

/** Received data ISR */
ISR (USART0_RX_vect) {
    uint8_t oldsrg = SREG;
    cli();
    responseBuf[responseLen++] = UDR0;

    //printf("%c", UDR0); // print all ESP8266 responses to debug serial

    if (responseLen == BUF_SIZE) {
        responseLen = 0;
        pointer = 0;
    }
    SREG = oldsrg;
}
