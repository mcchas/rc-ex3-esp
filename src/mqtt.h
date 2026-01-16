#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <config.h>
#include <ESP8266WiFi.h>

#define MQTT_USER "rc3user"
#define MQTT_PASS "pass"

extern WiFiClient espClient;
extern PubSubClient mqtt;
extern uint8_t getDiagnostics;
extern EspConfig cfg;

void mqttSetup();
void mqttLoop();

#endif
