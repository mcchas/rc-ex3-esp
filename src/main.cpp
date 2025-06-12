#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>

#include <wifi.h>
#include <RCWeb.h>
#include <config.h>
#include <time.h>
#include <rc3serial.h>

#include <PubSubClient.h>
#include <ArduinoJson.h>

WiFiUDP Udp;

EspConfig cfg;


RCWeb ws(HTTP_PORT);
HTTPClient http;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET, 1800000);

WiFiServer localServer(1123);
WiFiClient localClient;


#define MQTT_USER "rc3user"
#define MQTT_PASS "pass"

WiFiClient espClient;
PubSubClient mqtt(espClient);



void ota(char *host_name) {
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

void mqttCallback(char* topic, byte* payload, unsigned int length) {

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(payload);

    Settings s = {
        .power = 0xff, 
        .mode = 0xff,
        .degrees = 0xffff,
        .speed = 0xff
    };

    if (!root.success()) {
        // JSON parsing failed
        char mbuf[50];
        sprintf(mbuf, "%s/status", cfg.mqtt_topic);
        mqtt.publish(mbuf, "parse_fail", false);
        jsonBuffer.clear();
        return;
    } 

    yield();


    if (root.containsKey("speed")) {
        s.speed = root["speed"];
    }

    if (root.containsKey("mode")) {
        const char *mode = root["mode"];
        uint8_t imode;
        switch (mode[0]) {
        case 'c':
        case 'C': imode=2; break;;
        case 'd':
        case 'D': imode=1; break;;
        case 'h':
        case 'H': imode=4; break;;
        case 'f':
        case 'F': imode=3; break;;
        case 'a':
        case 'A': imode=0; break;;
        default: imode=0; break;;
        }
        s.mode = imode;
    }

    if (root.containsKey("temp")) {
        float temp = atof(root["temp"]);
        temp = temp * 10.0;
        s.degrees = (int)temp;
    }

    if (root.containsKey("power")) {
        s.power = root["power"];
    }

    if (!root.containsKey("status")) {
        serialFlush();
    }

    if ((s.speed & s.power & s.mode) != 0xFF && s.degrees != 0xFFFF) {
        setClimate(s);
        delay(100);
        serialFlush();
    }

    if (root.containsKey("delayOffHours")) {
        uint8_t hours = root["delayOffHours"];
        setOffTimer(hours);
        delay(100);
        serialFlush();
    }

    getStatus();
    delay(200);

    if(Serial.available()){
        size_t len = Serial.available();
        char rbuf[len+1];
        Serial.readBytes(rbuf, len);
        char sbuf[len+1];
        int sbuflen=0;
        for (uint8_t i=1; i<len; i++) {
          if (sbuflen) {
            if ((uint8_t)rbuf[i] > 32 && (uint8_t)rbuf[i] < 127) { // ascii printable
            sbuf[sbuflen++]=rbuf[i];
            }
          }
          else {
            if (rbuf[i]=='R') {
              sbuf[sbuflen++]=rbuf[i];
            }
          }
        }
        sbuf[sbuflen]='\0';
        char mbuf[50];
        sprintf(mbuf, "%s/status", cfg.mqtt_topic);

        if (sbuf[4]=='1') {
            char pwr = sbuf[13];
            char mode = sbuf[17];
            char fan = sbuf[21];
            char tbuf[2];
            strncpy(tbuf, &sbuf[30], 2);
            unsigned int number = (int)strtol(tbuf, NULL, 16);
            unsigned int temp = number * 5;
            char rem[]=".0\0";
            if (temp % 10) rem[1]='5';
            temp = temp / 10;

            char sfan[2];
            switch(fan) {
                case '0': sfan[0]='1';
                break;
                case '1': sfan[0]='2';
                break;
                case '2': sfan[0]='3';
                break;
                case '6': sfan[0]='4';
                break;
                default: sfan[0]='0';
            }
            sfan[1]='\0';
            char smode[6];
            switch (mode) {
                case '2': strncpy(smode,"cool\0",5); break;;
                case '1': strncpy(smode,"dry\0",4); break;;
                case '4': strncpy(smode,"heat\0",5); break;;
                case '3': strncpy(smode,"fan\0",4); break;;
                case '0': strncpy(smode,"auto\0",5); break;;
            }

            String buffer = "{\"power\":" + String(pwr) + ",\"mode\":\"" + String(smode) + "\",\"speed\":" + String(sfan) + ",\"temp\":" + String(temp) + String(rem) + ",\"response\":\"" + String(sbuf) + "\"";
            if (root.containsKey("delayOffHours")) {
              buffer += ",\"delayOffHours\":" + String(root["delayOffHours"])  + "\"";
            }
            buffer += "}\n";
            mqtt.publish(mbuf, buffer.c_str());
        }
        else {
            String buffer = "{\"response\":\"" + String(sbuf) + "\"}";
            mqtt.publish(mbuf, buffer.c_str());
        }
    }
    
    jsonBuffer.clear();

}




void setup() {

    // EspConfig cfg;
    uint8_t reset = cfg.initEspConfig();

    if (!setupWifi(&cfg, reset))
        return;

    ota(cfg.host_name);
    timeClient.begin();
    ws.configureServer(&cfg);

    Serial.begin(38400,SERIAL_8E1);

    localServer.begin();
    localServer.setNoDelay(true);

    mqtt.setServer(cfg.mqtt_server, 1883);
    mqtt.setCallback(mqttCallback);


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

    static unsigned long t_connect=0;
    if (!mqtt.connected()) {
      unsigned long t_now=millis();
      if (t_now - t_connect > 5000) {
        // Attempting MQTT connection...
        digitalWrite(13, HIGH); 
        if (mqtt.connect(cfg.mqtt_topic, MQTT_USER, MQTT_PASS)) {
          // connected
            char mbuf[50];
            sprintf(mbuf, "%s/status", cfg.mqtt_topic);
            mqtt.publish(mbuf, "connected", false);
            mqtt.subscribe(cfg.mqtt_topic);
        } else {
          // failed
        }
        digitalWrite(13, LOW); 
        t_connect=t_now;
      }
    }
    else {
      mqtt.loop();
    }


    timeClient.update();

    if (localServer.hasClient()){
        if (!localClient.connected()){
        if(localClient) localClient.stop();
        localClient = localServer.available();
        }
    }
        
    //check a client for data
    if (localClient && localClient.connected()){
        if(localClient.available()){
        size_t len = localClient.available();
        uint8_t sbuf[len];
        localClient.readBytes(sbuf, len);
        Serial.write(sbuf, len);      
        }
    }

    //check UART for data
    if(Serial.available()){
        size_t len = Serial.available();
        uint8_t sbuf[len];
        Serial.readBytes(sbuf, len);
        if (localClient && localClient.connected()){
        localClient.write(sbuf, len);
        }
    }
    
}



