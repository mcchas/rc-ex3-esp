#include <Arduino.h>
#include <rc3serial.h>

void serialFlush(){
  while(Serial.available() > 0) {
    Serial.read();
    yield();
  }
}

uint8_t checksum(char *data, uint16_t length) {
  uint8_t sum = 0;
  for (uint8_t i=0; i<length; i++) {
    sum += data[i];
  }
  return sum;
}

void setFanSpeed(uint8_t speed) {

  uint8_t val;
  switch(speed) {
      case 1: val=0;
      break;
      case 2: val=0x01;
      break;
      case 3: val=0x02;
      break;
      case 4: val=0x06;
      break;
      default: val=0x07;
  }

  char buf[100];
  uint8_t len = sprintf(buf, "RSSL12FF0001FF02FF03%.2x04FF05FF06FF0FFF43FF", val);

  Serial.print('\x02');
  Serial.print(buf);
  uint8_t sum = checksum(buf, len);
  Serial.print(sum, HEX);
  Serial.print('\x03');
}

void setPowerOn(uint8_t state) {
  char buf[100];
  uint8_t len = sprintf(buf, "RSSL12FF0001%.2x02FF03FF04FF05FF06FF0FFF43FF", (state));

  Serial.print('\x02');
  Serial.print(buf);
  uint8_t sum = checksum(buf, len);
  Serial.print(sum, HEX);
  Serial.print('\x03');
}

void setTemp(uint16_t degrees) {

  degrees = degrees / 5;
  char buf[100];
  uint8_t len = sprintf(buf, "RSSL13FF0001FF02FF03FF04FF0503%.2x06FF0FFF43FF", degrees);
  Serial.print('\x02');
  Serial.print(buf);
  uint8_t sum = checksum(buf, len);
  Serial.print(sum, HEX);
  Serial.print('\x03');
}

void setMode(uint8_t mode) {

  char buf[100];
  uint8_t len = sprintf(buf, "RSSL12FF0001FF02%.2x03FF04FF05FF06FF0FFF43FF", mode);
  Serial.print('\x02');
  Serial.print(buf);
  uint8_t sum = checksum(buf, len);
  Serial.print(sum, HEX);
  Serial.print('\x03');
}

void setClimate(Settings s) {

  if (s.degrees != 0xFFFF) s.degrees = s.degrees / 5;
  uint8_t val;
  switch(s.speed) {
      case 0xFF: val=0xFF;
      break;
      case 1: val=0;
      break;
      case 2: val=0x01;
      break;
      case 3: val=0x02;
      break;
      case 4: val=0x06;
      break;
      default: val=0x07;
  }
  char buf[100];
  uint8_t len;

  if (s.degrees == 0xFFFF) {
    len = sprintf(buf, "RSSL12FF0001%.2x02%.2x03%.2x04FF05FF06FF0FFF43FF", (s.power), s.mode, val);
  }
  else {
    len = sprintf(buf, "RSSL13FF0001%.2x02%.2x03%.2x04FF0503%.2x06FF0FFF43FF", (s.power), s.mode, val, s.degrees);
  }
  
  Serial.print('\x02');
  Serial.print(buf);
  uint8_t sum = checksum(buf, len);
  Serial.print(sum, HEX);
  Serial.print('\x03');


}

void getStatus() {
  char buf[64];
  sprintf(buf, "RSSL12FF0001FF02FF03FF04FF05FF06FF0FFF43FF25");
  Serial.print('\x02');
  Serial.print(buf);
  Serial.print('\x03');
}