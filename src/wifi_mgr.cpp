#include "wifi_mgr.h"

bool shouldSaveConfig = false; 

void saveConfigCallback () {
  shouldSaveConfig = true;
}


void configModeCallback (WiFiManager *myWiFiManager) {
}

#ifdef ESP8266
void lostWifiCallback (const WiFiEventStationModeDisconnected& evt) {
  ESP.reset();
  delay(1000);
}
#elif defined(ESP32)
void lostWifiCallback(arduino_event_id_t event, arduino_event_info_t info) {
  ESP.restart();
  delay(1000);
}
#endif

bool setupWifi(EspConfig *cfg, bool resetConf) {

  WiFiManager wifiManager;

  if (resetConf)
    wifiManager.resetSettings();

  WiFi.setHostname(cfg->host_name);

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setConnectTimeout(30);

  WiFiManagerParameter custom_hostname("hostname", "Choose a hostname for this device", cfg->host_name, HOSTNAME_LEN);
  wifiManager.addParameter(&custom_hostname);

  WiFiManagerParameter mqtt_server("mqtt_server", "MQTT Server", cfg->mqtt_server, 40);
  wifiManager.addParameter(&mqtt_server);
  WiFiManagerParameter mqtt_topic("mqtt_topic", "MQTT Topic", cfg->mqtt_topic, 40);
  wifiManager.addParameter(&mqtt_topic);

  WiFiManagerParameter mqtt_user("mqtt_user", "MQTT Username", cfg->mqtt_user, 40);
  wifiManager.addParameter(&mqtt_user);

  WiFiManagerParameter mqtt_pass("mqtt_pass", "MQTT Password", cfg->mqtt_pass, 40);
  wifiManager.addParameter(&mqtt_pass);
  
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

  #ifdef WM_WPA_PASS
  if (!wifiManager.autoConnect(buf, WM_WPA_PASS)) {
  #else
  if (!wifiManager.autoConnect(buf)) {
  #endif
    // failed to connect and hit timeout
    #ifdef ESP8266
    ESP.reset();
    #elif defined(ESP32)
    ESP.restart();
    #endif
    delay(1000);
  }

  // connected to WiFi
  strncpy(cfg->host_name, custom_hostname.getValue(), HOSTNAME_LEN);
  strncpy(cfg->mqtt_server, mqtt_server.getValue(), 40);
  strncpy(cfg->mqtt_topic, mqtt_topic.getValue(), 40);
  strncpy(cfg->mqtt_user, mqtt_user.getValue(), 40);
  strncpy(cfg->mqtt_pass, mqtt_pass.getValue(), 40);

  // Reset device if lost wifi Connection
  #ifdef ESP8266
  WiFi.onStationModeDisconnected(&lostWifiCallback);
  #elif defined(ESP32)
  WiFi.onEvent(lostWifiCallback, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  #endif

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

