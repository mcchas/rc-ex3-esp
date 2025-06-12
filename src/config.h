#pragma once
#include <FS.h>
#include <Arduino.h>

#define PROJECT_NAME "Mitsubishi RC-EX3"
#define VERSION "1.0"
#define PROJECT_LOCATION "https://github.com/mcchas/rc-ex3-esp.git"

#define MQTT_SERVER "10.0.0.1"

#define HOSTNAME "rcex3_\0"
#define HOSTNAME_LEN 20

#define IPADDR "192.168.45.111\0"
#define NETMASK "255.255.255.0\0"
#define GATEWAY "192.168.45.1\0"

#define HTTP_PORT  80
#define NTP_SERVER "time.google.com"
#define NTP_OFFSET 36000


class EspConfig {
    public:
        explicit EspConfig();

        const char *wifi_config_name = "Mitsubishi RC-EX3";
        const char *serverName = "checkip.dyndns.org";
        
        char host_name[20];
        char mqtt_server[40];
        char mqtt_topic[40];
        
        #ifdef STATICIP
        char static_ip[16];
        char static_gw[16];
        char static_sn[16];
        #endif

        bool initEspConfig();
        bool resetConfig();
        bool saveConfig();
};

