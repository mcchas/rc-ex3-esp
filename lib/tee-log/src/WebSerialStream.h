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

#ifndef _H_WEBSERVER_TEE_LOG
#define _H_WEBSERVER_TEE_LOG

#include <TLog.h>

/* Use the synchronous WebServer APIs per platform:
 *  - ESP8266: ESP8266WebServer
 *  - ESP32:   WebServer
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


#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)

class WebSerialStream : public TLog {
  public:
    ~WebSerialStream();
    virtual size_t write(uint8_t c);
    virtual size_t write(uint8_t *buffer, size_t size);
    virtual size_t write(const uint8_t *buffer, size_t size);
    virtual void begin(WebSrv_t *server);
    virtual void loop();
    virtual void stop();
  private:
    WebSrv_t * _server;
    uint8_t _buff[768];
    unsigned long _at = 0;
  protected:
};
#endif /* ARDUINO_ARCH_ESP8266 || ARDUINO_ARCH_ESP32 */

#endif /* _H_WEBSERVER_TEE_LOG */
