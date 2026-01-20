#include "mqtt.h"
#include "rc3.h"

WiFiClient espClient;
PubSubClient mqtt(espClient);
uint8_t getDiagnostics = 0;

void processSetClimate(byte *payload)
{

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(payload);

  Settings s = {
      .power = 0xff,
      .mode = 0xff,
      .degrees = 0xffff,
      .speed = 0xff};

  if (!root.success())
  {
    char mbuf[50];
    sprintf(mbuf, "%s/status", cfg.mqtt_topic);
    mqtt.publish(mbuf, "json_parse_fail", false);
    jsonBuffer.clear();
    return;
  }

  yield();

  if (getDiagnostics)
  {
    return;
  }

  if (root.containsKey("speed"))
  {
    s.speed = root["speed"];
  }

  if (root.containsKey("mode"))
  {
    const char *mode = root["mode"];
    s.mode = modeToInt(mode);
  }

  if (root.containsKey("temp"))
  {
    float temp = atof(root["temp"]);
    temp = temp * 10.0;
    s.degrees = (int)temp;
  }

  if (root.containsKey("power"))
  {
    s.power = root["power"];
  }

  if (!root.containsKey("status"))
  {
    serialFlush();
  }

  if (((s.speed & s.power & s.mode) != 0xFF) || s.degrees != 0xFFFF)
  {
    setClimate(s);
    delay(100);
    serialFlush();
  }

  if (root.containsKey("delayOffHours"))
  {
    uint8_t hours = root["delayOffHours"];
    setOffTimer(hours);
    delay(100);
    serialFlush();
  }

  status_string_t status = getStatus();
  uint8_t itime = getOffTimer();
  char stime[4];
  sprintf(stime, "%u", itime);

  String buffer = "{\"power\":" + status.power + ",\"mode\":\"" + status.mode + "\",\"speed\":" + status.speed + ",\"temp\":" + status.temp + ",\"delayOffHours\":" + String(stime) + "}\n";

  char mbuf[50];
  sprintf(mbuf, "%s/status", cfg.mqtt_topic);
  mqtt.publish(mbuf, buffer.c_str());

  jsonBuffer.clear();
}

#ifdef HA_DISCOVERY
void haDiscovery1()
{

  char message[1024];
  sprintf(message, R"(
{
"name": "%s",
"unique_id": "%s_climate",
"mode_command_topic": "%s/ha_mode/set",
"mode_state_topic": "%s/ha_mode",
"modes": ["off", "cool", "dry", "heat", "fan_only", "auto"],
"fan_mode_command_topic": "%s/ha_fan/set",
"fan_mode_state_topic": "%s/ha_fan",
"fan_modes": ["auto", "low", "medium", "high", "max"],
"temperature_command_topic": "%s/temperature/set",
"temperature_state_topic": "%s/temperature",
"min_temp": 16,
"max_temp": 30,
"temp_step": 0.5,
"availability_topic": "%s/status",
"payload_available": "online",
"payload_not_available": "offline"
}
)",
  cfg.host_name,
  cfg.host_name,
  cfg.mqtt_topic,
  cfg.mqtt_topic,
  cfg.mqtt_topic,
  cfg.mqtt_topic,
  cfg.mqtt_topic,
  cfg.mqtt_topic,
  cfg.mqtt_topic);
  char topic[100];
  sprintf(topic, "homeassistant/climate/%s/config", cfg.host_name);
  mqtt.publish(topic, message, true);
}

void haDiscovery2()
{
  char message[512];
  char topic[100];
  sprintf(message, R"(
{
"name": "Power Off Timer",
"unique_id": "%s_delay_off",
"command_topic": "%s/offDelay/set",
"state_topic": "%s/offDelay/get",
"min": 1,
"max": 12,
"step": 1,
"mode": "box"
}
)",
  cfg.host_name, cfg.mqtt_topic, cfg.mqtt_topic);

  sprintf(topic, "homeassistant/number/%s_delay_off/config", cfg.host_name);
  mqtt.publish(topic, message, true);
}

void haTopics(String topicStr, String payloadStr)
{
  if (topicStr.startsWith(String(cfg.mqtt_topic) + String("/ha_mode/")))
  {

    if (topicStr.endsWith("/set"))
    {

      if (payloadStr == "cool")
      {
        setMode("cool");
        delay(100);
        serialFlush();
        setPowerOn(1);
      }
      else if (payloadStr == "heat")
      {
        setMode("heat");
        delay(100);
        serialFlush();
        setPowerOn(1);
      }
      else if (payloadStr == "dry")
      {
        setMode("dry");
        delay(100);
        serialFlush();
        setPowerOn(1);
      }
      else if (payloadStr == "fan_only")
      {
        setMode("fan");
        delay(100);
        serialFlush();
        setPowerOn(1);
      }
      else if (payloadStr == "auto")
      {
        setMode("auto");
        delay(100);
        serialFlush();
        setPowerOn(1);
      }
      else if (payloadStr == "off")
      {
        setPowerOn(0);
      }

      delay(100);
      serialFlush();
    }
    status_string_t status = getStatus();
    char mbuf[50];
    sprintf(mbuf, "%s/ha_mode", cfg.mqtt_topic);
    if (status.power == "0")
    {
      mqtt.publish(mbuf, "off");
    }
    else
    {
      mqtt.publish(mbuf, status.mode.c_str());
    }
  }

  else if (topicStr.startsWith(String(cfg.mqtt_topic) + String("/ha_fan/")))
  {
    if (topicStr.endsWith("/set"))
    {
      if (payloadStr == "auto")
      {
        setFanSpeed('0');
      }
      else if (payloadStr == "low")
      {
        setFanSpeed('1');
      }
      else if (payloadStr == "medium")
      {
        setFanSpeed('2');
      }
      else if (payloadStr == "high")
      {
        setFanSpeed('3');
      }
      else if (payloadStr == "max")
      {
        setFanSpeed('4');
      }
      delay(100);
      serialFlush();
    }
    status_string_t status = getStatus();
    char mbuf[50];
    sprintf(mbuf, "%s/ha_fan", cfg.mqtt_topic);
    switch (status.speed[0])
    {
    case '0':
      mqtt.publish(mbuf, "auto");
      break;
    case '1':
      mqtt.publish(mbuf, "low");
      break;
    case '2':
      mqtt.publish(mbuf, "medium");
      break;
    case '3':
      mqtt.publish(mbuf, "high");
      break;
    case '4':
      mqtt.publish(mbuf, "max");
      break;
    }

    mqtt.publish(mbuf, status.speed.c_str());
  }
}
#endif

void mqttCallback(char *topic, byte *payload, unsigned int length)
{

  String topicStr = String(topic);
  if (topicStr.startsWith(cfg.mqtt_topic))
  {

    String payloadStr = String((const char *)payload).substring(0, length);

    if (topicStr.endsWith(String("/state/get")))
    {
      status_string_t status = getStatus();
      uint8_t itime = getOffTimer();
      char stime[4];
      sprintf(stime, "%u", itime);
      String buffer = "{\"power\":" + status.power + ",\"mode\":\"" + status.mode + "\",\"speed\":" + status.speed + ",\"temp\":" + status.temp + ",\"delayOffHours\":" + String(stime) + "}\n";

      char mbuf[50];
      sprintf(mbuf, "%s/status", cfg.mqtt_topic);
      mqtt.publish(mbuf, buffer.c_str());
    }

    else if (topicStr.equalsIgnoreCase(cfg.mqtt_topic) || topicStr.endsWith(String("/state/set")))
    {
      processSetClimate(payload);
    }

    else if (topicStr.startsWith(String(cfg.mqtt_topic) + String("/mode/")))
    {
      if (topicStr.endsWith("/set"))
      {
        setMode((const char *)payload);
        delay(100);
        serialFlush();
      }
      status_string_t status = getStatus();
      char mbuf[50];
      sprintf(mbuf, "%s/mode", cfg.mqtt_topic);
      mqtt.publish(mbuf, status.mode.c_str());
    }

    else if (topicStr.startsWith(String(cfg.mqtt_topic) + String("/fan/")))
    {
      if (topicStr.endsWith("/set"))
      {
        setFanSpeed(payload[0]);
        delay(100);
        serialFlush();
      }
      status_string_t status = getStatus();
      char mbuf[50];
      sprintf(mbuf, "%s/fan", cfg.mqtt_topic);
      mqtt.publish(mbuf, status.speed.c_str());
    }

    else if (topicStr.startsWith(String(cfg.mqtt_topic) + String("/power/")))
    {
      if (topicStr.endsWith("/set"))
      {
        if (payload[1] == 'n')
        {
          setPowerOn(1);
        }
        else if (payload[1] == 'f')
        {
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

    else if (topicStr.startsWith(String(cfg.mqtt_topic) + String("/temperature/")))
    {
      if (topicStr.endsWith("/set"))
      {
        float temp = atof((char *)payload);
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

    else if (topicStr.startsWith(String(cfg.mqtt_topic) + String("/offDelay/")))
    {
      if (topicStr.endsWith("/set"))
      {
        payload[length] = '\0';
        uint8_t hours = atoi((char *)payload);
        setOffTimer(hours);
        delay(100);
      }
      uint8_t itime = getOffTimer();
      char stime[4];
      sprintf(stime, "%u", itime);
      char mbuf[55];
      sprintf(mbuf, "%s/offDelay", cfg.mqtt_topic);
      mqtt.publish(mbuf, String(stime).c_str());
    }

    else if (topicStr.endsWith("/diagnostics"))
    {
      if (payload[1] == 'n')
      {
        getDiagnostics = 1;
      }
      else if (payload[1] == 'f')
      {
        getDiagnostics = 0;
      }
    }
#ifdef HA_DISCOVERY
    haTopics(topicStr, payloadStr);
#endif
  }
}

void mqttSetup()
{
  mqtt.setServer(cfg.mqtt_server, 1883);
  mqtt.setCallback(mqttCallback);
}

void mqttLoop()
{
  static unsigned long t_connect = 0;
  if (!mqtt.connected())
  {
    unsigned long t_now = millis();
    if (t_now - t_connect > 5000)
    {
      // Attempting MQTT connection...
      digitalWrite(13, HIGH);
      char mbuf[50];
      sprintf(mbuf, "%s/status", cfg.mqtt_topic);
      if (mqtt.connect(cfg.mqtt_topic, cfg.mqtt_user, cfg.mqtt_pass, mbuf, 0, true, "offline"))
      {
        // connected
        mqtt.publish(mbuf, "online", true);
        char subscribePattern[50];
        sprintf(subscribePattern, "%s/#", cfg.mqtt_topic);
        mqtt.subscribe(subscribePattern);
#ifdef HA_DISCOVERY
        haDiscovery1();
        haDiscovery2();
#endif
      }
      else
      {
        // failed
      }
      digitalWrite(13, LOW);
      t_connect = t_now;
    }
  }
  else
  {
    mqtt.loop();
  }

  if (getDiagnostics)
  {

    if (getDiagnostics == 1)
    {
      requestOperationalData();
      char mbuf[50];
      sprintf(mbuf, "%s/debug", cfg.mqtt_topic);
      mqtt.publish(mbuf, "getting diagnostics", false);
      getDiagnostics = 2;
      delay(100);
    }

    hvac_data_t hvac;
    uint8_t diag_complete = fetchOperationalData(hvac);
    if (diag_complete)
    {
      getDiagnostics = 0;
      char dbuf[70];
      sprintf(dbuf, "%s/diagnostics", cfg.mqtt_topic);
      String buffer = "{\"indoor_air_temp\":" + String(hvac.indoor_air_temp) +
                      ",\"target_temp\":" + String(hvac.target_temp) +
                      ",\"outdoor_air_temp\":" + String(hvac.outdoor_air_temp) +
                      ",\"outdoor_eev_opening\":" + String(hvac.outdoor_eev_opening) +
                      ",\"compressor_hours\":" + String(hvac.compressor_hours) +
                      ",\"current\":" + String(hvac.current) +
                      "\"}";
      mqtt.publish(dbuf, buffer.c_str());
      // #ifdef HA_DISCOVERY
      // sprintf(dbuf, "%s/outdoor_temperature", cfg.mqtt_topic);
      // mqtt.publish(dbuf, String(hvac.outdoor_air_temp).c_str());
      // #endif
    }
  }
}
