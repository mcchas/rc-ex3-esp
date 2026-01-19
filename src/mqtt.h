#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#ifdef ESP8266
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif
extern uint8_t getDiagnostics;
extern EspConfig cfg;

void mqttSetup();
void mqttLoop();

extern WiFiClient espClient;
extern PubSubClient mqtt;

#endif
