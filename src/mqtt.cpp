#include "mqtt.h"
#include <rc3.h>

WiFiClient espClient;
PubSubClient mqtt(espClient);
uint8_t getDiagnostics = 0;


void processSetClimate(byte* payload) {

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

    if (root.containsKey("diagnostics")) {
        getDiagnostics = 1;
        return;
    }

    if (root.containsKey("stopDiagnostics")) {
        getDiagnostics = 0;
        return;
    }

    if (getDiagnostics) {
        // If diagnostics in progress, ignore other commands
        return;
    }

    if (root.containsKey("speed")) {
        s.speed = root["speed"];
    }

    if (root.containsKey("mode")) {
        const char *mode = root["mode"];
        s.mode = modeToInt(mode);
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

    if (((s.speed & s.power & s.mode) != 0xFF) || s.degrees != 0xFFFF) {
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

    status_string_t status = getStatus();

    String buffer = "{\"power\":" + status.power + ",\"mode\":\"" + status.mode + "\",\"speed\":" + status.speed + ",\"temp\":" + status.temp;
    if (root.containsKey("delayOffHours")) {
        buffer += ",\"delayOffHours\":" + String(root["delayOffHours"]);
    }
    buffer += "}\n";

    char mbuf[50];
    sprintf(mbuf, "%s/status", cfg.mqtt_topic);
    mqtt.publish(mbuf, buffer.c_str());
    
    jsonBuffer.clear();

}

void mqttCallback(char* topic, byte* payload, unsigned int length) {

    String topicStr = String(topic);
    if (topicStr.startsWith(cfg.mqtt_topic)) {

        if (topicStr.endsWith(String("/state/get"))) {
            status_string_t status = getStatus();
            String buffer = "{\"power\":" + status.power + ",\"mode\":\"" + status.mode + "\",\"speed\":" + status.speed + ",\"temp\":" + status.temp;
            buffer += "}\n";
            char mbuf[50];
            sprintf(mbuf, "%s/status", cfg.mqtt_topic);
            mqtt.publish(mbuf, buffer.c_str());
        }

        else if (topicStr.equalsIgnoreCase(cfg.mqtt_topic) || topicStr.endsWith(String("/state/set"))) {
            processSetClimate(payload);
        }

        else if (topicStr.startsWith(String(cfg.mqtt_topic) + String("/mode/"))) {
            if (topicStr.endsWith("/set")) {
                setMode((const char*)payload);
                delay(100);
                serialFlush();
            }
            status_string_t status = getStatus();
            char mbuf[50];
            sprintf(mbuf, "%s/mode", cfg.mqtt_topic);
            mqtt.publish(mbuf, status.mode.c_str());
        }

        else if (topicStr.startsWith(String(cfg.mqtt_topic) + String("/fan/"))) {
            if (topicStr.endsWith("/set")) {
                setFanSpeed(payload[0]);
                delay(100);
                serialFlush();
            }
            status_string_t status = getStatus();
            char mbuf[50];
            sprintf(mbuf, "%s/fan", cfg.mqtt_topic);
            mqtt.publish(mbuf, status.speed.c_str());
        }

        else if (topicStr.startsWith(String(cfg.mqtt_topic) + String("/power/"))) {
            if (topicStr.endsWith("/set")) {
                if (payload[1] == 'n') {
                    setPowerOn(1);
                } 
                else if (payload[1] == 'f') {
                    setPowerOn(0);
                }
                delay(100);
                serialFlush();
            }
            status_string_t status = getStatus();
            char mbuf[50];
            sprintf(mbuf, "%s/power", cfg.mqtt_topic);
            mqtt.publish(mbuf, status.power.c_str());
        }

        else if (topicStr.startsWith(String(cfg.mqtt_topic) + String("/temperature/"))) {
            if (topicStr.endsWith("/set")) {
                float temp = atof((char*)payload);
                temp = temp * 10.0;
                setTemp((int)temp);
                delay(100);
                serialFlush();
            }
            status_string_t status = getStatus();
            char mbuf[55];
            sprintf(mbuf, "%s/temperature", cfg.mqtt_topic);
            mqtt.publish(mbuf, status.temp.c_str());
        }

        else if (topicStr.endsWith("/diagnostics")) {
            if (payload[1] == 'n') {
                getDiagnostics = 1;
            } 
            else if (payload[1] == 'f') {
                getDiagnostics = 0;
            }
        }
    }

}

void mqttSetup() {
    mqtt.setServer(cfg.mqtt_server, 1883);
    mqtt.setCallback(mqttCallback);
}

void mqttLoop() {
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
            char subscribePattern[50];
            sprintf(subscribePattern, "%s/#", cfg.mqtt_topic);
            mqtt.subscribe(subscribePattern);
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

    if (getDiagnostics) {

        if (getDiagnostics == 1) {
            requestOperationalData();
            char mbuf[50];
            sprintf(mbuf, "%s/debug", cfg.mqtt_topic);
            mqtt.publish(mbuf, "getting diagnostics", false);
            getDiagnostics = 2;
            delay(100);
        }
        // diagnostics();
        hvac_data_t hvac;
        uint8_t diag_complete = fetchOperationalData(hvac);
        if (diag_complete) {
            getDiagnostics = 0;
            char dbuf[55];
            sprintf(dbuf, "%s/diagnostics", cfg.mqtt_topic);
            String buffer = "{\"indoor_air_temp\":" + String(hvac.indoor_air_temp) +
                            ",\"target_temp\":" + String(hvac.target_temp) +
                            ",\"outdoor_air_temp\":" + String(hvac.outdoor_air_temp) +
                            ",\"outdoor_eev_opening\":" + String(hvac.outdoor_eev_opening) +
                            ",\"compressor_hours\":" + String(hvac.compressor_hours) +
                            ",\"current\":" + String(hvac.current) +
                            "\"}";
            mqtt.publish(dbuf, buffer.c_str());
        }
    }
}
