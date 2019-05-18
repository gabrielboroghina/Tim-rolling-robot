#include "esp8266.h"

#define F_CPU 16000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include "usart.h"

int8_t Response_Status;
volatile int16_t responseLen = 0, pointer = 0;
uint32_t TimeOut = 0;
char RESPONSE_BUFFER[BUF_SIZE];

void Read_Response(char *_Expected_Response) {
    uint8_t EXPECTED_RESPONSE_LENGTH = strlen(_Expected_Response);
    uint32_t TimeCount = 0, ResponseBufferLength;
    char RECEIVED_CRLF_BUF[EXPECTED_RESPONSE_LENGTH];

    while (true) {
        if (TimeCount >= (DEFAULT_TIMEOUT + TimeOut)) {
            TimeOut = 0;
            Response_Status = ESP8266_RESPONSE_TIMEOUT;
            return;
        }

        if (Response_Status == ESP8266_RESPONSE_STARTING) {
            Response_Status = ESP8266_RESPONSE_WAITING;
        }
        ResponseBufferLength = strlen(RESPONSE_BUFFER);
        if (ResponseBufferLength) {
            _delay_ms(1);
            TimeCount++;
            if (ResponseBufferLength == strlen(RESPONSE_BUFFER)) {
                for (uint16_t i = 0; i < ResponseBufferLength; i++) {
                    memmove(RECEIVED_CRLF_BUF, RECEIVED_CRLF_BUF + 1, EXPECTED_RESPONSE_LENGTH - 1);
                    RECEIVED_CRLF_BUF[EXPECTED_RESPONSE_LENGTH - 1] = RESPONSE_BUFFER[i];
                    if (!strncmp(RECEIVED_CRLF_BUF, _Expected_Response, EXPECTED_RESPONSE_LENGTH)) {
                        TimeOut = 0;
                        Response_Status = ESP8266_RESPONSE_FINISHED;
                        return;
                    }
                }
            }
        }
        _delay_ms(1);
        TimeCount++;
    }
}

void ESP8266_Clear() {
    memset(RESPONSE_BUFFER, 0, BUF_SIZE);
    responseLen = 0;
    pointer = 0;
}

void Start_Read_Response(char *_ExpectedResponse) {
    Response_Status = ESP8266_RESPONSE_STARTING;
    do {
        Read_Response(_ExpectedResponse);
    } while (Response_Status == ESP8266_RESPONSE_WAITING);

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
bool WaitForExpectedResponse(char *ExpectedResponse) {
    Start_Read_Response(ExpectedResponse); /* read response */

    if (Response_Status != ESP8266_RESPONSE_TIMEOUT)
        return true;
    return false;
}

void sendAT(char *ATCmd) {
    ESP8266_Clear();

    /* send AT command to ESP8266 */
    USART0_print(ATCmd);
    USART0_print("\r\n");

    printf("***\r\n");
}

bool SendATandExpectResponse(char *ATCommand, char *ExpectedResponse) {
    ESP8266_Clear();

    /* send AT command to ESP8266 */
    USART0_print(ATCommand);
    USART0_print("\r\n");

    return WaitForExpectedResponse(ExpectedResponse);
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

bool ESP8266_ConnectionMode(uint8_t Mode) {
    char _atCommand[20];
    memset(_atCommand, 0, 20);
    sprintf(_atCommand, "AT+CIPMUX=%d", Mode);
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

bool ESP8266_WIFIMode(uint8_t _mode) {
    char _atCommand[20];
    memset(_atCommand, 0, 20);
    sprintf(_atCommand, "AT+CWMODE=%d", _mode);
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
        printf("%s\n", RESPONSE_BUFFER);
        if (strstr(RESPONSE_BUFFER, "+CWJAP:1"))
            return ESP8266_CONNECTION_TIMEOUT;
        else if (strstr(RESPONSE_BUFFER, "+CWJAP:2"))
            return ESP8266_WRONG_PASSWORD;
        else if (strstr(RESPONSE_BUFFER, "+CWJAP:3"))
            return ESP8266_NOT_FOUND_TARGET_AP;
        else if (strstr(RESPONSE_BUFFER, "+CWJAP:4"))
            return ESP8266_CONNECTION_FAILED;
        else
            return ESP8266_JOIN_UNKNOWN_ERROR;
    }
}

uint8_t ESP8266_connected() {
    SendATandExpectResponse("AT+CIPSTATUS", "\r\nOK\r\n");
    if (strstr(RESPONSE_BUFFER, "STATUS:2"))
        return ESP8266_CONNECTED_TO_AP;
    else if (strstr(RESPONSE_BUFFER, "STATUS:3"))
        return ESP8266_CREATED_TRANSMISSION;
    else if (strstr(RESPONSE_BUFFER, "STATUS:4"))
        return ESP8266_TRANSMISSION_DISCONNECTED;
    else if (strstr(RESPONSE_BUFFER, "STATUS:5"))
        return ESP8266_NOT_CONNECTED_TO_AP;
    else
        return ESP8266_CONNECT_UNKNOWN_ERROR;
}

uint8_t ESP8266_Start(uint8_t _ConnectionNumber, char *Domain, char *Port) {
    bool _startResponse;
    char _atCommand[100];
    memset(_atCommand, 0, 100);
    _atCommand[99] = 0;

    _delay_ms(4000);
    if (SendATandExpectResponse("AT+CIPMUX?", "CIPMUX:0"))
        sprintf(_atCommand, "AT+CIPSTART=\"TCP\",\"%s\",%s", Domain, Port);
    else
        sprintf(_atCommand, "AT+CIPSTART=\"TCP\",\"%s\",%s", Domain, Port);
//        sprintf(_atCommand, "AT+CIPSTART=\"%d\",\"TCP\",\"%s\",%s", _ConnectionNumber, Domain,
//                Port);

    _startResponse = SendATandExpectResponse(_atCommand, "CONNECT\r\n");
    if (!_startResponse) {
        if (Response_Status == ESP8266_RESPONSE_TIMEOUT)
            return ESP8266_RESPONSE_TIMEOUT;
        return ESP8266_RESPONSE_ERROR;
    }
    return ESP8266_RESPONSE_FINISHED;
}

uint8_t ESP8266_Send(char *Data) {
    char _atCommand[20];
    memset(_atCommand, 0, 20);
    sprintf(_atCommand, "AT+CIPSEND=%d", (strlen(Data) + 2));
    _atCommand[19] = 0;

    SendATandExpectResponse(_atCommand, "\r\nOK\r\n>");
    if (!SendATandExpectResponse(Data, "\r\nSEND OK\r\n")) {
        if (Response_Status == ESP8266_RESPONSE_TIMEOUT)
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
        return RESPONSE_BUFFER[pointer++];
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
    RESPONSE_BUFFER[responseLen] = UDR0;

    //printf("%c", UDR0); // print all ESP8266 responses to debug serial

    responseLen++;
    if (responseLen == BUF_SIZE) {
        responseLen = 0;
        pointer = 0;
    }
    SREG = oldsrg;
}
