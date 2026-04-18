/* Copyright 2008, 2012-2022 Dirk-Willem van Gulik <dirkx(at)webweaving(dot)org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *  
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * Library that provides a fanout, or T-flow; so that output or logs do 
 * not just got to the serial port; but also to a configurable mix of a
 * telnetserver, a webserver, syslog or MQTT.
 */
#ifndef _H_LOG_TEE
#define _H_LOG_TEE

#include <Arduino.h>
#include <Print.h>
#include <stddef.h>
#include <memory>
#include <vector>
#include <functional>
/* Use platform-specific synchronous webserver: ESP8266 uses ESP8266WebServer,
 * ESP32 uses WebServer. This keeps the public API type name stable as `WebSrv_t`.
 */
#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WebServer.h>
  using WebSrv_t = ESP8266WebServer;
#elif defined(ARDUINO_ARCH_ESP32)
  #include <WebServer.h>
  using WebSrv_t = WebServer;
#else
  class WebSrv_t;
#endif

#define IDENTIFIER_GENERATOR "unknown"

#define REPORT_INTERVAL (10*60*1000)

class LOGBase : public Print {
public:
    LOGBase(String identifier = IDENTIFIER_GENERATOR) : _identifier(identifier) {};
    String identifier() { return _identifier; };
    void setIdentifier(String identifier) { _identifier = identifier; };
    virtual void begin() { return; };
    virtual void begin(WebSrv_t *server) { return; };
    virtual void reconnect() { return; };
    virtual void loop() { return; };
    virtual void stop() { return; };

protected:
    String _identifier;
};

class TLog : public LOGBase
{
  public:
    void disableSerial(bool onoff) { _disableSerial = onoff; };
    //void addPrintStream2(const LOGBase * _handler) { addPrintStream(std::make_shared<LOGBase>(_handler)); }
    void addPrintStream(const std::shared_ptr<LOGBase> &_handler) {
      auto it = find(handlers.begin(), handlers.end(), _handler);
      if ( handlers.end() == it)
        // we're not using push_back; that copies; but use a reference. 
        // As it can see reuse.
        handlers.emplace_back(_handler); 
    };
    virtual void begin(WebSrv_t *server) {
      for (auto it = handlers.begin(); it != handlers.end(); ++it) {
        (*it)->begin(server);
      }
    };
    virtual void begin() {
      for (auto it = handlers.begin(); it != handlers.end(); ++it) {
        (*it)->begin();
      }
    };
    virtual void loop() {
        for (auto it = handlers.begin(); it != handlers.end(); ++it) {
          (*it)->loop();
      }
    };
    virtual void stop() {
      for (auto it = handlers.begin(); it != handlers.end(); ++it) {
        (*it)->stop();
      }
    };
    size_t write(uint8_t a) {
      if(a=='\0')
        return 0;
      for (auto it = handlers.begin(); it != handlers.end(); ++it) {
        (*it)->write(a);
      }
      if (_disableSerial)
          return 1;
      return Serial.write(a);
    };
    size_t write(const uint8_t *buffer, size_t size) {
      for (auto it = handlers.begin(); it != handlers.end(); ++it) {
        (*it)->write(buffer, size);
      }
      if (_disableSerial)
          return size;
      return Serial.write(buffer, size);
    }
  private:
    std::vector<std::shared_ptr<LOGBase>> handlers;
    bool _disableSerial = false;
};

extern TLog Log;
extern TLog Debug;
#endif
