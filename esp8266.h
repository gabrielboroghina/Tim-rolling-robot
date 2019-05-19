#ifndef ESP8266_H
#define ESP8266_H

#include <avr/io.h>

#define BUF_SIZE        600
#define DEFAULT_TIMEOUT 1000

/* Connection Mode */
#define SINGLE_CONN    0
#define MULTIPLE_CONN  1

/* Application Mode */
#define NORMAL       0
#define TRANSPERANT  1

/* Application Mode */
#define WIFI_MODE_STATION      1
#define WIFI_MODE_ACCESSPOINT  2
#define WIFI_MODE_STA_AP  3


#define DOMAIN  "tim-tim.7e14.starter-us-west-2.openshiftapps.com"
#define PORT    "80"

#define SSID       "GB"
#define PASSWORD   "v866bEXJ"

enum ESP8266_RESPONSE_STATUS {
    ESP8266_RESPONSE_WAITING,
    ESP8266_RESPONSE_FINISHED,
    ESP8266_RESPONSE_TIMEOUT,
    ESP8266_RESPONSE_BUFFER_FULL,
    ESP8266_RESPONSE_ERROR
};

enum ESP8266_CONNECT_STATUS {
    ESP8266_CONNECTED_TO_AP,
    ESP8266_CREATED_TRANSMISSION,
    ESP8266_TRANSMISSION_DISCONNECTED,
    ESP8266_NOT_CONNECTED_TO_AP,
    ESP8266_CONNECT_UNKNOWN_ERROR
};

enum ESP8266_JOINAP_STATUS {
    ESP8266_WIFI_CONNECTED,
    ESP8266_CONNECTION_TIMEOUT,
    ESP8266_WRONG_PASSWORD,
    ESP8266_NOT_FOUND_TARGET_AP,
    ESP8266_CONNECTION_FAILED,
    ESP8266_JOIN_UNKNOWN_ERROR
};

void ESP8266_deepSleep();

bool ESP8266_Begin();

bool ESP8266_WIFIMode(uint8_t mode);

bool ESP8266_ApplicationMode(uint8_t mode);

bool ESP8266_ConnectionMode(uint8_t mode);

uint8_t ESP8266_JoinAccessPoint(const char *_SSID, const char *_PASSWORD);

uint8_t ESP8266_Connected();

uint8_t ESP8266_Start(uint8_t connectionNumber, const char *domain, const char *port);

uint8_t ESP8266_Send(const char *data);

uint16_t ESP8266_Read(char *buffer);

#endif // ESP8266_H
