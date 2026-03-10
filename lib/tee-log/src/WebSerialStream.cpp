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

#include "TLog.h"
#include "WebSerialStream.h"

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)

size_t WebSerialStream::write(uint8_t c) {
  if(c=='\0')
    return 0;
  _buff[_at % sizeof(_buff)] = c;
  _at++;
  return 1;
}

size_t WebSerialStream::write(uint8_t *buffer, size_t size) {
  size_t n = 0;
  while(size--) {
    n++;
    _buff[_at % sizeof(_buff)] = *buffer++;
    _at++;
  }
  return n;
}

size_t WebSerialStream::write(const uint8_t *buffer, size_t size) {
  if (size==0)
    return 0;
  size_t n = 0;
  while(size--) {
      n++;
    _buff[_at % sizeof(_buff)] = *buffer++;
    _at++;
  }
  return n;
}

WebSerialStream::~WebSerialStream() {
}

void WebSerialStream::stop() {
}

void WebSerialStream::loop() {
}

void WebSerialStream::begin(WebSrv_t *server) {
  
  _server = server;

  // _server->on("/debug",[this]() {
  _server->on("/debug", HTTP_GET, [this]() {
    String html;
    html += "<html><head><title>log</title></head>";
    html += "<style>";
    html += "#log { font-family: 'Courier New', monospace; white-space: pre; }";
    html += "textarea { resize: vertical; width: 98%; height: 90%; padding: 5px; overflow: auto; background: #1f1f1f; color: #65c115;}";
    html += "body { text-align: center; font-family: verdana,sans-serif; background: #252525;}";
    html += "button { border: 0; border-radius: 0.3rem; background: #1fa4ec4b; color: #faffff;";
    html += "line-height: 2.4rem; font-size: 1.2rem; width: 100%; -webkit-transition-duration: 0.4s; transition-duration: 0.4s; cursor: pointer;";
    html += "button:hover {background: #0e70a4;}";
    html += "</style>";
    html += "<script language=javascript>";
    html += "var at = 0;";
    html += "function f() { fetch('log?at='+at).";
    html += "then(r => { return r.json(); }).then( j => { ";
    html += " var isAtEnd = (window.innerHeight + window.pageYOffset) >= document.body.offsetHeight - 4; ";
    html += " document.getElementById('log').innerHTML += j.buff; ";
    html += " at= j.at; ";
    html += " if (isAtEnd) window.scrollTo(0,document.body.scrollHeight); ";
    html += "}).catch( e => { console.log(e); } );";
    html += "};";
    html += "window.onLoad = setInterval(f, 500);";
    html += "</script>";
    html += "<body>";
    html += "<textarea readonly id='log' cols='340' wrap='off'></textarea>";
    html += "<div style='text-align:left;display:inline-block;color:#eaeaea;min-width:340px;'>";
    html += "<form action=\"/\"><button>Home</button></form></div>";
    html += "</body></html>";

    _server->send(200, "text/html", html);
  });


  _server->on("/log", HTTP_GET, [this]() {

    if (!_server->hasArg("at")) {
       _server->send(400, "text/plain", "Missing at argument.");
       return;
    }
    unsigned long prevAt= _server->arg("at").toInt();
    String out = "{\"at\":" + String(_at) + ",\"buff\":\"";

    // reset browsers from the future (e.g. after a reset)
    if (prevAt > _at) {
        out += "<font color=red><hr><i>.. log reset..</i></font><hr>";
        prevAt = _at;
    }
    if (_at > sizeof(_buff) && prevAt < _at - sizeof(_buff)) {
        out += "<font color=red><hr><i>.. skipping " + 
                String(_at - sizeof(_buff) - prevAt) +
                " bytes of log - no longer in buffer ..</i><hr></font>";
        prevAt = _at - sizeof(_buff);
    }
    for(;prevAt != _at; prevAt++) {
       char c = _buff[prevAt % sizeof(_buff)];
       switch(c) {
       case '<': out += "&lt;"; break;
       case '>': out += "&gt;"; break;
       case '\b': out += "\\b"; break;
       case '\n': out += "\\n"; break;
       case '\r': out += "\\r"; break;
       case '\f': out += "\\f"; break;
       case '\t': out += "\\t"; break;
       case '"' : out += "\\\""; break;
       case '\\': out += "\\\\"; break;
       default  : out += c; break;
       };
    }
    out += "\"}";
    _server->send(200, "application/json", out);
  });
};

#endif /* ARDUINO_ARCH_ESP8266 || ARDUINO_ARCH_ESP32 */
