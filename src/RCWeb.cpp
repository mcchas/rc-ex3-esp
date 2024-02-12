#include <FS.h>
#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <RCWeb.h>
#include <config.h>
#include <inttypes.h>
#include <stdlib.h>
#include <rc3serial.h>

ESP8266WebServer server(80);
DynamicJsonBuffer jsonBuffer;
JsonObject& deviceState = jsonBuffer.createObject();
uint8_t reconfigure=0;


RCWeb::RCWeb(uint16_t port) {
    // ESP8266WebServer server(port);
}

void RCWeb::configureServer(EspConfig *incfg) {


  strncpy(host_name, incfg->host_name, 20);
  strncpy(passcode, incfg->passcode, 20);
  strncpy(port_str, incfg->port_str, 6);
  strncpy(mqtt_server, incfg->mqtt_server, 40);
  strncpy(mqtt_topic, incfg->mqtt_topic, 40);
  #ifdef STATICIP
  strncpy(static_ip, incfg->static_ip, 16);
  #endif

  port = incfg->port;

 server.on("/msg", [this]() {

    int simple = 0;
    if (server.hasArg("simple")) simple = server.arg("simple").toInt();
    String signature = server.arg("auth");
    String epid = server.arg("epid");
    String mid = server.arg("mid");
    String timestamp = server.arg("time");

    if (strlen(passcode) != 0 && server.arg("pass") != passcode) {
        server.send(401, "text/plain", "Unauthorized, invalid passcode");
    } else {

      if (server.hasArg("reconfigure")) {
        server.send(200, "text/html", "Reconfigure wifi");
        reconfigure = 1;
      }

      String type = server.arg("type");

      if (type == "rcex3") {

        if (server.hasArg("speed")) {
          uint8_t speed = server.arg("speed").toInt();
          setFanSpeed(speed);
          delay(100);
          serialFlush();
        }

        if (server.hasArg("mode")) {
          uint8_t mode = server.arg("mode")[0];
          switch (mode) {
            case 'c':
            case 'C': mode=2; break;;
            case 'd':
            case 'D': mode=1; break;;
            case 'h':
            case 'H': mode=4; break;;
            case 'f':
            case 'F': mode=3; break;;
            case 'a':
            case 'A': mode=0; break;;
            default: mode=0; break;;
          }  
          setMode(mode);
          delay(100);
          serialFlush();
        }

        if (server.hasArg("temp")) {
          float temp = server.arg("temp").toFloat();
          temp = temp * 10.0;
          setTemp((int)temp);
          delay(100);
          serialFlush();
        }

        if (server.hasArg("power")) {
          uint8_t pwr = server.arg("power").toInt();
          setPowerOn(pwr);
          delay(100);
          serialFlush();
        }

        if (!server.hasArg("status")) {
          delay(250);
          // clear UART 
          serialFlush();
        }

        getStatus();
        delay(200);

        if(Serial.available()){
          size_t len = Serial.available();
          char sbuf[len+1];
          Serial.readBytes(sbuf, len);
          for (uint8_t i=1; i<len; i++) {
            if (sbuf[i] == '\x03') { 
              sbuf[i]='\0';
              break;
            }
          }
          sbuf[len+1]='\0';
          if (sbuf[6]=='1') {
            char pwr = sbuf[14];
            char mode = sbuf[18];
            char fan = sbuf[22];
            char tbuf[2];
            strncpy(tbuf, &sbuf[31], 2);
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

            server.send(200, "text/json; charset=utf-8", "{\"power\":" + String(pwr) + ",\"mode\":\"" + String(smode) + "\",\"speed\":" + String(sfan) + ",\"temp\":" + String(temp) + String(rem) + ",\"response\":\"" + String(sbuf) + "\"}\n");
          }
          else {
            server.send(200, "text/json; charset=utf-8", "{\"response\":\"" + String(sbuf) + "\"}\n");
          }
        }

        return;
      }

      if (!simple) {
        sendHomePage("Code Sent", "Success", 1); // 200
      }
    }
  });


  server.on("/", [this]() {
    sendHomePage(); // 200
  });
  
  server.begin();

}

uint8_t RCWeb::handleClient() {
  server.handleClient();
  if (reconfigure) {
    reconfigure=0;
    return 1;
  }
  return 0;
}

void RCWeb::stop() {
  server.stop();
}

void RCWeb::sendHeader() {
  sendHeader(200);
}

void RCWeb::sendHeader(int httpcode) {

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(httpcode, "text/html; charset=utf-8", "");
  server.sendContent("<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n");
  server.sendContent("<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en'>\n");
  server.sendContent("  <head>\n");
  server.sendContent("    <meta name='viewport' content='width=device-width, initial-scale=.75' />\n");
  server.sendContent("    <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css' />\n");
  server.sendContent("    <style>@media (max-width: 991px) {.nav-pills>li {float: none; margin-left: 0; margin-top: 5px; text-align: center;}}</style>\n");
  server.sendContent("    <title>Mitsubishi RC-EX3 (" + String(host_name) + ")</title>\n");
  server.sendContent("  </head>\n");
  server.sendContent("  <body>\n");
  server.sendContent("    <div class='container'>\n");
  server.sendContent("      <h1><a href='https://github.com/mcchas/rc-ex3-esp'>RC-EX3 ESP</a></h1>\n");
  server.sendContent("      <div class='row'>\n");
  server.sendContent("        <div class='col-md-12'>\n");
  server.sendContent("          <ul class='nav nav-pills'>\n");
  server.sendContent("            <li class='active'>\n");
  server.sendContent("              <a href='http://" + String(host_name) + ".local" + ":" + String(port) + "'>Hostname <span class='badge'>" + String(host_name) + ".local" + ":" + String(port) + "</span></a></li>\n");
  server.sendContent("            <li class='active'>\n");
  server.sendContent("              <a href='http://" + String(static_ip) + ":" + String(port) + "'>Local <span class='badge'>" + String(static_ip) + ":" + String(port) + "</span></a></li>\n");
  server.sendContent("            <li class='active'>\n");
  server.sendContent("              <a href='#'>MAC <span class='badge'>" + String(WiFi.macAddress()) + "</span></a></li>\n");
  server.sendContent("          </ul>\n");
  server.sendContent("        </div>\n");
  server.sendContent("      </div><hr />\n");
}

void RCWeb::sendFooter() {
  server.sendContent("      <div class='row'><div class='col-md-12'><em>" + String(millis()) + "ms uptime</em></div></div>\n");
  server.sendContent("    </div>\n");
  server.sendContent("  </body>\n");
  server.sendContent("</html>\n");
  server.sendContent(""); 
  delay(5); // give the web browser time to receive the data ...?
  server.client().stop();
}

void RCWeb::sendHomePage() {
  sendHomePage("", "");
}

void RCWeb::sendHomePage(String message, String header) {
  sendHomePage(message, header, 0);
}

void RCWeb::sendHomePage(String message, String header, int type) {
  sendHomePage(message, header, type, 200);
}

void RCWeb::sendHomePage(String message, String header, int type, int httpcode) {
  sendHeader(httpcode);
  sendFooter();
}
