#include <FS.h>
#include <Arduino.h>
#include <WiFiManager.h> 
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <config.h>



bool shouldSaveConfig = false; 

void saveConfigCallback () {
  shouldSaveConfig = true;
}

void configModeCallback (WiFiManager *myWiFiManager) {
}

void lostWifiCallback (const WiFiEventStationModeDisconnected& evt) {
  ESP.reset();
  delay(1000);
}


bool setupWifi(EspConfig *cfg, bool resetConf) {

  WiFiManager wifiManager;

  if (resetConf)
    wifiManager.resetSettings();

  WiFi.setHostname(cfg->host_name);

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConfigPortalTimeout(180);

  WiFiManagerParameter custom_hostname("hostname", "Choose a hostname for this device", cfg->host_name, HOSTNAME_LEN);
  wifiManager.addParameter(&custom_hostname);

  WiFiManagerParameter mqtt_server("mqtt_server", "MQTT Server", cfg->mqtt_server, 40);
  wifiManager.addParameter(&mqtt_server);
  WiFiManagerParameter mqtt_topic("mqtt_topic", "MQTT Topic", cfg->mqtt_topic, 40);
  wifiManager.addParameter(&mqtt_topic);
  
  #ifdef STATICIP
  IPAddress sip, sgw, ssn;
  sip.fromString(cfg->static_ip);
  sgw.fromString(cfg->static_gw);
  ssn.fromString(cfg->static_sn);
  wifiManager.setSTAStaticIPConfig(sip, sgw, ssn);
  #endif

  byte mac[6];
  WiFi.macAddress(mac);
  char buf[32];
  sprintf(buf, "%s %0x%0x%0x", cfg->wifi_config_name, mac[2], mac[1], mac[0]);

  if (!wifiManager.autoConnect(buf)) {
    // failed to connect and hit timeout
    ESP.reset();
    delay(1000);
  }

  // connected to WiFi
  strncpy(cfg->host_name, custom_hostname.getValue(), HOSTNAME_LEN);
  strncpy(cfg->mqtt_server, mqtt_server.getValue(), 40);
  strncpy(cfg->mqtt_topic, mqtt_topic.getValue(), 40);

  // Reset device if lost wifi Connection
  WiFi.onStationModeDisconnected(&lostWifiCallback);

  // save the custom parameters to FS
  if (shouldSaveConfig) {
    #ifdef STATICIP
    strncpy(cfg->static_ip, WiFi.localIP().toString().c_str(), 16);
    strncpy(cfg->static_gw, WiFi.gatewayIP().toString().c_str(), 16);
    strncpy(cfg->static_sn, WiFi.subnetMask().toString().c_str(), 16);
    #endif
    cfg->saveConfig();
  }

  return true;
}

