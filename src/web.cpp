#include <FS.h>
#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <web.h>
#include <config.h>
#include <inttypes.h>
#include <stdlib.h>
#include <rc3.h>

ESP8266WebServer server(80);
DynamicJsonBuffer jsonBuffer;
JsonObject& deviceState = jsonBuffer.createObject();
uint8_t reconfigure=0;

uint8_t getDiags=0;
uint8_t diagCounter=0;
hvac_data_t hvac_diags;

RCWeb::RCWeb(uint16_t port) {
    // ESP8266WebServer server(port);
}

void RCWeb::configureServer(EspConfig *incfg) {


  strncpy(host_name, incfg->host_name, 20);
  strncpy(mqtt_server, incfg->mqtt_server, 40);
  strncpy(mqtt_topic, incfg->mqtt_topic, 40);
  #ifdef STATICIP
  strncpy(static_ip, incfg->static_ip, 16);
  #endif

  server.on("/reboot", [this]() {
    server.send(200, "text/html", "<html><head><meta http-equiv=\"refresh\" content=\"10;URL='/'\"/></head><body><h1>Reboot. Refreshing in 10 seconds...</h1></body></html>\n");
    ESP.restart();
  });

  server.on("/config", HTTP_POST, [this,incfg]() {
    String message;
    bool save = false;
    if (server.hasArg("hostname")) {
      message = server.arg("hostname");
      snprintf(incfg->host_name, message.length()+1, "%s", message.c_str());
      save = true;
    }
    if (server.hasArg("mqtt_server")) {
      message = server.arg("mqtt_server");
      snprintf(incfg->mqtt_server, message.length()+1, "%s", message.c_str());

      save = true;
    }
    if (server.hasArg("mqtt_topic")) {
      message = server.arg("mqtt_topic");
      snprintf(incfg->mqtt_topic, message.length()+1, "%s", message.c_str());
      save = true;
    }
    if (!save) {
      server.send(404, "text/plain", "not found\n");
      return;
    }
    server.send(200, "text/html", "<html><head><meta http-equiv=\"refresh\" content=\"3;URL='setup'\"/></head><body><h1>Saved. Redirecting in 3 seconds...</h1></body></html>\n");
    incfg->saveConfig();
  });

  server.on("/config", HTTP_GET, [this,incfg]() {
    String content = "{\"hostname\": \"" + String(incfg->host_name) + "\",";
    content += "\"mqtt_server\": \"" + String(incfg->mqtt_server) + "\",";
    content += "\"mqtt_topic\": \"" + String(incfg->mqtt_topic) + "\"";
    server.send(200, "application/json", content);
  });

  server.on("/config", HTTP_DELETE, [&]() {
    incfg->resetConfig();
    server.send(200, "text/plain", "config deleted");
  });

  server.on("/diagnostics", [this]() {
    if (!getDiags) {
      getDiags = 1;
      requestOperationalData();
    }
    if (getDiags==2) {
      getDiags = 0;
      String content = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Diagnostics</title></head>";
      content += "<style>body {text-align: left;font-family: verdana, sans-serif;background: #252525}button {border: 0;border-radius: .3rem;background: #1fa4ec4b;";
      content += "color: #faffff;line-height: 2.4rem;font-size: 1.2rem;width: 100%;-webkit-transition-duration: .4s;transition-duration: .4s;cursor: pointer}";
      content += "button:hover {background: #0e70a4}</style>";
      content += "<body><div style=text-align:left;display:inline-block;color:#eaeaea;min-width:340px><h3>Operational Data</h3>";
      content += "<table border=\"1\" cellpadding=\"4\" cellspacing=\"0\" style=\"width:340px\" >";
      content += "<tr><td>Indoor Air Temp</td><td>" + String(hvac_diags.indoor_air_temp) + "</td></tr>";
      content += "<tr><td>Target Temp</td><td>" + String(hvac_diags.target_temp) + "</td></tr>";
      content += "<tr><td>Outdoor Air Temp</td><td>" + String(hvac_diags.outdoor_air_temp) + "</td></tr>";
      content += "<tr><td>Outdoor EEV Opening</td><td>" + String(hvac_diags.outdoor_eev_opening) + "</td></tr>";
      content += "<tr><td>Compressor Hours</td><td>" + String(hvac_diags.compressor_hours) + "</td></tr>";
      content += "<tr><td>Current</td><td>" + String(hvac_diags.current) + "</td></tr>";
      content += "</table><br/><form action=\"/\"><button>Home</button></form></fieldset></div></body></html></div></body></html>";
      server.send(200, "text/html", content);
      return;
    }
    server.send(200, "text/html", "<html><head><meta http-equiv=\"refresh\" content=\"1;URL='/diagnostics?c=" + String(diagCounter++) + "'\"/></head><body><h1>Waiting for diagnostic data. Refreshing in 1 seconds...</h1></body></html>\n");
  });


  server.on("/setup", [this,incfg]() {
    String content = "<!DOCTYPE html><html><head><title>RC-EX3 ESP Setup</title><style>";
    content += "div,fieldset,input,select { padding: 7px; font-size: 1em;}";
    content += "p {margin: 0.5em 0;}";
    content += "input {width: 100%; box-sizing: border-box; -webkit-box-sizing: border-box; -moz-box-sizing:";
    content += "border-box; background: #f9f7f7fc; color: #000000;}";
    content += "body {text-align: left;font-family: verdana,sans-serif;background: #252525;}";
    content += "button { border: 0; border-radius: 0.3rem; background: #1fa4ec4b; color: #faffff;";
    content += "line-height: 2.4rem; font-size: 1.2rem; width: 100%; -webkit-transition-duration: 0.4s; transition-duration: 0.4s; cursor: pointer;}";
    content += "button:hover {background: #0e70a4;}";
    content += "</style></head><body>";
    content += "<div style='text-align:left;display:inline-block;color:#eaeaea;min-width:340px;'>";
    content += "<fieldset>";
    content += "<form action=\"/config\" method=\"post\">";
    content += "<label for=\"hostname\">Hostname:</label><br>";
    content += "<input type=\"text\" id=\"hostname\" name=\"hostname\" value=\"" + String(incfg->host_name) + "\"><br><br>";
    content += "<label for=\"mqtt_server\">MQTT Server:</label><br>";
    content += "  <input type=\"text\" id=\"mqtt_server\" name=\"mqtt_server\" value=\"" + String(incfg->mqtt_server) + "\"><br><br>";
    content += "  <label for=\"mqtt_topic\">MQTT Topic:</label><br>";
    content += "  <input type=\"text\" id=\"mqtt_topic\" name=\"mqtt_topic\" value=\"" + String(incfg->mqtt_topic) + "\"><br><br>";
    content += "<button name='save' type='submit'>Save</button>";
    content += "</form><br>";
    content += "<form action=\"/reboot\" method=\"post\">";
    content += "<button>Reboot</button>";
    content += "</form><br><form action=diagnostics><button>Diagnostics</button></form><br>";
    content += "<form action=\"/\">";
    content += "<button>Home</button>";
    content += "</form>";
    content += "</fieldset></div></body></html>";
    server.send(200, "text/html", content);
  });


  server.on("/", [this]() {

    String content = "<!doctypehtml><title>Mitsubishi RC-EX3 (" + String(host_name) + ")</title><style>div{padding:5px;font-size:1em}p{margin:.5em 0}body{text-align:left;font-family:verdana,sans-serif;background:#252525}button{border:0;border-radius:.3rem;";
    content += "background:#1fa4ec4b;color:#faffff;line-height:2.4rem;font-size:1.2rem;width:100%;-webkit-transition-duration:.4s;transition-duration:.4s;cursor:pointer}.red{background:#ec711f4b}button:hover{background";
    content += ":#0e70a4}.slidecontainer{width:100%}.slider{width:100%;height:25px;outline:0}.slider:hover{opacity:1}</style><div style=text-align:left;display:inline-block;color:#eaeaea;min-width:340px><div style=text-";
    content += "align:center;color:#eaeaea> <h2>Mitsubishi RC-EX3 (" + String(host_name) + ")</h2> <h4><a href='https://github.com/mcchas/rc-ex3-esp'>rc-ex3-esp</a></h4></div><form action=msg><input type=hidden name=r><input type=hidden name=power value=1 ><button class=poweron>Power On</button></form><p><form action=msg><input type=hidden name=r><input type=hidden name=power ";
    content += "value=0 ><button class=poweroff>Power Off</button></form><p><br><form action=msg><input type=hidden name=r><input type=hidden name=mode value=cool><button class=cool>Cool</button></form><p><form action=msg><input type=hidden name=r><input type=hidden ";
    content += "name=mode value=heat><button class=heat>Heat</button></form><p><form action=msg><input type=hidden name=r><input type=hidden name=mode value=fan><button class=fan>Fan</button></form><p><form action=msg><input type=hidden name=r><input type=hidden name=mode ";
    content += "value=dry><button class=dry>Dry</button></form></p><br>";
    content += "<p><div class=slidercontainer>Temperature: <span id=target></span> <input type=range value=21 class=slider id=tempRange max=30 min=16></div><p><button id=setTempBtn>Set Temperature</button><div style=display:block></div><p>";
    content += "<p style=\"display:flex;gap:10px\"><label for=fanSpeedSelect style=\"flex-shrink:0;padding-top:8px\">Fan Speed:</label>";
    content += "<select id=fanSpeedSelect style=\"flex:1\"><option value=0>Auto</option><option value=1>Speed 1</option><option value=2>Speed 2</option><option value=3>Speed 3</option><option value=4>Speed 4</option></select>";
    content += "<button id=setFanSpeedBtn style=\"flex:1\">Set Fan Speed</button></p><br><p><form action=setup><button>Setup</button></form></div>";
    content += "<script>var slider=document.getElementById(\"tempRange\"),output=document.getElementById(\"target\");output.innerHTML=slider.value;slider.oninput=function(){output.innerHTML=this.value};document.getElementById(\"setTempBtn\").addEventListener(\"click\",function(e){e.preventDefault();fetch(\"msg?temp=\"+slider.value).then(response=>response.json()).catch(console.error)});";
    content += "document.getElementById(\"setFanSpeedBtn\").addEventListener(\"click\",function(e){e.preventDefault();var fanSpeed=document.getElementById(\"fanSpeedSelect\").value;fetch(\"msg?speed=\"+fanSpeed).then(response=>response.json()).catch(console.error)});fetch(\"msg?status=1\").then(response=>response.json()).then(data=>{console.log(data);if(data.power==1)";
    content += "document.getElementsByClassName(\"poweron\")[0].style.backgroundColor=\"grey\";else document.getElementsByClassName(\"poweroff\")[0].style.backgroundColor=\"grey\";switch(data.mode){case\"cool\":document.getElementsByClassName(\"cool\")[0].style.backgroundColor=\"grey\";break;case\"heat\":document.getElementsByClassName(\"heat\")[0].style.backgroundColor=\"grey\";break";
    content += ";case\"fan\":document.getElementsByClassName(\"fan\")[0].style.backgroundColor=\"grey\";break;case\"dry\":document.getElementsByClassName(\"dry\")[0].style.backgroundColor=\"grey\";break}document.getElementById(\"fanSpeedSelect\").value=data.speed;output.innerHTML=data.temp}).catch(console.error);</script>";

    server.send(200, "text/html", content);
  });

  server.on("/msg", [this]() {

    if (server.hasArg("reconfigure")) {
      server.send(200, "text/html", "Reconfigure wifi");
      reconfigure = 1;
    }

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

    if (server.hasArg("delayOffHours")) {
      uint8_t hours = server.arg("delayOffHours").toInt();
      setOffTimer(hours);
      delay(100);
      serialFlush();
    }

    if (!server.hasArg("status")) {
      delay(250);
      // clear UART 
      serialFlush();
    }

    if (server.hasArg("r")) {
      server.send(200, "text/html", "<html><meta http-equiv=\"refresh\" content=\"1; url=/\" /></html>\n");
    }

    status_string_t status = getStatus();

    String content = "{\"power\":" + status.power +",\"mode\":\"" + status.mode + "\",\"speed\":" + String(status.speed) + ",\"temp\":" + String(status.temp);
    if (server.hasArg("delayOffHours")) {
      content += ",\"delayOffHours\":" + String(server.arg("delayOffHours").toInt());
    }
    content += "}\n";
    server.send(200, "application/json; charset=utf-8", content);

  });

  server.begin();

}

uint8_t RCWeb::handleClient() {

  if (getDiags == 1) {
    uint8_t diag_complete = fetchOperationalData( hvac_diags );
    if (diag_complete) {
      getDiags = 2;
    }
  }

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
