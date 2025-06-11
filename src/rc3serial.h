#include <Arduino.h>

void serialFlush();
uint8_t checksum(char *data, uint16_t length);
void setFanSpeed(uint8_t speed);
void setPowerOn(uint8_t state);
void setTemp(uint16_t degrees);
void setMode(uint8_t mode);
void setOffTimer(uint8_t hours);
void getStatus();
typedef struct
{
  uint8_t power;
  uint8_t mode;
  uint16_t degrees;
  uint8_t speed;
} Settings;
void setClimate(Settings s);
