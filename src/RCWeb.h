#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <config.h>


class RCWeb {
    EspConfig *cfg;
    char host_name[20];
    char passcode[20];
    char port_str[6];
    char mqtt_server[40];
    char mqtt_topic[40];
    #ifdef STATICIP
    char static_ip[16];
    #endif

    char static_ip[16];
    int port;
    public:
        explicit RCWeb(uint16_t port);
        void configureServer(EspConfig *cfg);
        void sendHeader();
        void sendHeader(int httpcode);
        void sendFooter();
        void stop();
        uint8_t handleClient();

        void sendHomePage(String message, String header, int type, int httpcode);
        void sendHomePage();
        void sendHomePage(String message, String header);
        void sendHomePage(String message, String header, int type);
};
