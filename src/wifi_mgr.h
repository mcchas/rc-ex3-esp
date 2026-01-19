#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include <WiFiManager.h>
#include "config.h"

void saveConfigCallback();
void configModeCallback(WiFiManager *myWiFiManager);
#ifdef ESP8266
void lostWifiCallback(const WiFiEventStationModeDisconnected &evt);
#elif defined(ESP32)
void lostWifiCallback(arduino_event_id_t event, arduino_event_info_t info);
#endif
bool setupWifi(EspConfig *cfg, bool resetConf);
