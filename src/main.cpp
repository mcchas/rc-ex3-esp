#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>

#include <wifi.h>
#include <web.h>
#include <config.h>
#include <time.h>
#include <rc3.h>
#include <mqtt.h>
#include <serial_server.h>

WiFiUDP Udp;
EspConfig cfg;
RCWeb ws(HTTP_PORT);
HTTPClient http;
WiFiServer localServer(1123);
WiFiClient localClient;

void otaSetup(char *host_name) {
    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(host_name);
    ArduinoOTA.onStart([]() {
        Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
}

void setup() {

    uint8_t reset = cfg.initEspConfig();

    if (!setupWifi(&cfg, reset))
        return;

    otaSetup(cfg.host_name);
    ws.configureServer(&cfg);

    Serial.begin(38400,SERIAL_8E1);

    localServer.begin();
    localServer.setNoDelay(true);

    mqttSetup();
}

void loop() {

    ArduinoOTA.handle();

    uint8_t reconfigure = ws.handleClient();
    if (reconfigure) {
        cfg.resetConfig();
        reconfigure=0;
        delay(5000);
        ESP.restart();
        delay(1000);
    }

    mqttLoop();
    
    if (!getDiagnostics) {
        handleSerialServer();
    }
    
}



