#ifndef SERIAL_SERVER_H
#define SERIAL_SERVER_H

#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#endif

void handleSerialServer();