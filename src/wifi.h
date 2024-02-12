#include <Arduino.h>
#include <WiFiManager.h>
#include <config.h>

void saveConfigCallback();
void configModeCallback(WiFiManager *myWiFiManager);
void lostWifiCallback(const WiFiEventStationModeDisconnected& evt);
bool setupWifi(EspConfig *cfg, bool resetConf);