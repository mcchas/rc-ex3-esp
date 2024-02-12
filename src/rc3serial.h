#include <Arduino.h>

void serialFlush();
uint8_t checksum(char *data, uint16_t length);
void setFanSpeed(uint8_t speed);
void setPowerOn(uint8_t state);
void setTemp(uint16_t degrees);
void setMode(uint8_t mode);
void getStatus();