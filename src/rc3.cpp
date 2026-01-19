#include "rc3.h"

void serialFlush()
{
  while (Serial.available() > 0)
  {
    Serial.read();
    yield();
  }
}

uint8_t checksum(char *data, uint16_t length)
{
  uint8_t sum = 0;
  for (uint8_t i = 0; i < length; i++)
  {
    sum += data[i];
  }
  return sum;
}

size_t hexToBytes(const char *hex, uint8_t *out)
{
  size_t count = 0;
  char tmp[3] = {0};

  while (hex[0] && hex[1])
  {
    if (!isxdigit(hex[0]) || !isxdigit(hex[1]))
    {
      break;
    }

    tmp[0] = hex[0];
    tmp[1] = hex[1];
    out[count++] = static_cast<uint8_t>(strtol(tmp, nullptr, 16));

    hex += 2;
  }
  return count;
}

void setFanSpeed(const char* speed)
{
  uint8_t ispeed=0;
  switch (speed[0]) {
  case 'a':
  case 'A': speed=0; break;;
  case '1': ispeed=1; break;;
  case '2': ispeed=2; break;;
  case '3': ispeed=3; break;;
  case '4': ispeed=4; break;;
  default: ispeed=0; break;;
  }
  setFanSpeed(ispeed);
}

void setFanSpeed(uint8_t speed)
{

  uint8_t val;
  switch (speed)
  {
  case 1:
    val = 0;
    break;
  case 2:
    val = 0x01;
    break;
  case 3:
    val = 0x02;
    break;
  case 4:
    val = 0x06;
    break;
  default:
    val = 0x07;
  }

  char buf[100];
  uint8_t len = sprintf(buf, "RSSL12FF0001FF02FF03%.2x04FF05FF06FF0FFF43FF", val);

  Serial.print('\x02');
  Serial.print(buf);
  uint8_t sum = checksum(buf, len);
  Serial.print(sum, HEX);
  Serial.print('\x03');
}

void setPowerOn(uint8_t state)
{
  char buf[100];
  uint8_t len = sprintf(buf, "RSSL12FF0001%.2x02FF03FF04FF05FF06FF0FFF43FF", (state));

  Serial.print('\x02');
  Serial.print(buf);
  uint8_t sum = checksum(buf, len);
  Serial.print(sum, HEX);
  Serial.print('\x03');
}

void setTemp(uint16_t degrees)
{

  degrees = degrees / 5;
  char buf[100];
  uint8_t len = sprintf(buf, "RSSL13FF0001FF02FF03FF04FF0503%.2x06FF0FFF43FF", degrees);
  Serial.print('\x02');
  Serial.print(buf);
  uint8_t sum = checksum(buf, len);
  Serial.print(sum, HEX);
  Serial.print('\x03');
}


void setMode(const char* mode)
{
  uint8_t imode;
  switch (mode[0]) {
  case 'c':
  case 'C': imode=2; break;;
  case 'd':
  case 'D': imode=1; break;;
  case 'h':
  case 'H': imode=4; break;;
  case 'f':
  case 'F': imode=3; break;;
  case 'a':
  case 'A': imode=0; break;;
  default: imode=0; break;;
  }
  setMode(imode);
}

void setMode(uint8_t mode)
{

  char buf[100];
  uint8_t len = sprintf(buf, "RSSL12FF0001FF02%.2x03FF04FF05FF06FF0FFF43FF", mode);
  Serial.print('\x02');
  Serial.print(buf);
  uint8_t sum = checksum(buf, len);
  Serial.print(sum, HEX);
  Serial.print('\x03');
}

void setClimate(Settings s)
{

  if (s.degrees != 0xFFFF)
    s.degrees = s.degrees / 5;
  uint8_t val;
  switch (s.speed)
  {
  case 0xFF:
    val = 0xFF;
    break;
  case 1:
    val = 0;
    break;
  case 2:
    val = 0x01;
    break;
  case 3:
    val = 0x02;
    break;
  case 4:
    val = 0x06;
    break;
  default:
    val = 0x07;
  }
  char buf[100];
  uint8_t len;

  if (s.degrees == 0xFFFF)
  {
    len = sprintf(buf, "RSSL12FF0001%.2x02%.2x03%.2x04FF05FF06FF0FFF43FF", (s.power), s.mode, val);
  }
  else
  {
    len = sprintf(buf, "RSSL13FF0001%.2x02%.2x03%.2x04FF0503%.2x06FF0FFF43FF", (s.power), s.mode, val, s.degrees);
  }
  Serial.print('\x02');
  Serial.print(buf);
  uint8_t sum = checksum(buf, len);
  Serial.print(sum, HEX);
  Serial.print('\x03');
}

void setOffTimer(uint8_t hours)
{
  if (hours < 1 || hours > 12)
    return;

  char buf[32];
  uint8_t len = sprintf(buf, "RSJ802%.2x00", hours);
  Serial.print('\x02');
  Serial.print(buf);
  uint8_t sum = checksum(buf, len);
  Serial.print(sum, HEX);
  Serial.print('\x03');
}

size_t readSerialAscii(char *buffer, size_t maxLength)
{
  char rbuf[maxLength];
  size_t len = Serial.readBytesUntil('\x03', rbuf, sizeof(rbuf) - 1);
  int sbuflen = 0;
  for (uint8_t i = 1; i < len; i++)
  {
    if (sbuflen)
    {
      if ((uint8_t)rbuf[i] > 32 && (uint8_t)rbuf[i] < 127)
      {
        buffer[sbuflen++] = rbuf[i];
      }
    }
    else
    {
      if (rbuf[i] == 'R')
      {
        buffer[sbuflen++] = rbuf[i];
      }
    }
  }
  buffer[sbuflen] = '\0';
  return len;
}


uint8_t getOffTimer()
{
  serialFlush();
  Serial.print('\x02');
  Serial.print("RSJ928");
  Serial.print('\x03');
  delay(100);

  char buf[50];
  readSerialAscii(buf, sizeof(buf) - 1);
  if (buf[3] == '9' && buf[5] == '2')
  {
    char tbuf[5] = {0};
    strncpy(tbuf, &buf[6], 4);
    tbuf[2] = '\0';
    uint8_t timer = (uint8_t)strtol(tbuf, NULL, 16);
    return timer;
  }
  return 0;
}

void requestOperationalData()
{
  char buf[64];
  sprintf(buf, "RSR10000E8");
  Serial.print('\x02');
  Serial.print(buf);
  Serial.print('\x03');
}

hvac_data_t parseOperationalData(char *input, uint16_t length)
{

  const char *hex_data = input + HEADER_LEN;
  uint8_t data[256] = {0};
  hexToBytes(hex_data, data);

  hvac_data_t hvac = {0};

  hvac.operation_mode = data[POS_OPERATION_MODE - HEADER_LEN]; // ?
  hvac.indoor_air_temp = (int8_t)data[POS_INDOOR_AIR_TEMP - HEADER_LEN]; // confirmed
  hvac.target_temp = (int8_t)data[POS_TARGET_TEMP - HEADER_LEN] / 2; // confirmed
  hvac.protection_control = data[POS_PROTECTION_CONTROL - HEADER_LEN]; // ?

  hvac.outdoor_air_temp = (int8_t)((uint8_t)data[POS_OUTDOOR_AIR_TEMP - HEADER_LEN] / 4 - 22); // confirmed
  hvac.return_air_temp = (int8_t)((uint8_t)data[POS_RETURN_AIR_TEMP - HEADER_LEN] / 4); // confirmed
  hvac.outdoor_hx_temp1 = (int8_t)data[POS_OUTDOOR_HX_TEMP1 - HEADER_LEN]; // ?

  hvac.compressor_hz = data[POS_COMPRESSOR_HZ - HEADER_LEN]; // ?
  hvac.outdoor_fan_speed = data[POS_OUTDOOR_FAN_SPEED - HEADER_LEN]; // ?

  hvac.indoor_hx_temp1 = (int8_t)data[POS_INDOOR_HX_TEMP1 - HEADER_LEN]; // ?
  hvac.defrost = data[POS_DEFROST - HEADER_LEN]; // ?
  hvac.indoor_hx_temp3 = (int8_t)data[POS_INDOOR_HX_TEMP3 - HEADER_LEN]; // ?
  hvac.indoor_fan_speed = data[POS_INDOOR_FAN_SPEED - HEADER_LEN]; // ?

  hvac.outdoor_eev_opening =
      ((uint16_t)data[POS_EEV_OPENING_MSB - HEADER_LEN] << 8) |
      (uint16_t)data[POS_EEV_OPENING_LSB - HEADER_LEN]; // confirmed

  hvac.compressor_hours =
      ((uint16_t)data[POS_COMPRESSOR_HOURS_MSB - HEADER_LEN] << 8) |
      (uint16_t)data[POS_COMPRESSOR_HOURS_LSB - HEADER_LEN] * 100; // confirmed

  hvac.current = (int8_t)data[POS_CURRENT - HEADER_LEN]; // confirmed

  return hvac;
}

String statusJsonString(status_string_t status) {
  String buffer = "{\"power\":" + status.power + ",\"mode\":\"" + status.mode + "\",\"speed\":" + status.speed + ",\"temp\":" + status.temp + "}\n";
  return buffer;
}

uint8_t modeToInt(const char* mode)
{
  uint8_t imode;
  switch (mode[0]) {
  case 'c':
  case 'C': imode=2; break;;
  case 'd':
  case 'D': imode=1; break;;
  case 'h':
  case 'H': imode=4; break;;
  case 'f':
  case 'F': imode=3; break;;
  case 'a':
  case 'A': imode=0; break;;
  default: imode=0; break;;
  }
  return imode;
}


status_string_t getStatus()
{

  char buf[64];
  sprintf(buf, "RSSL12FF0001FF02FF03FF04FF05FF06FF0FFF43FF25");
  Serial.print('\x02');
  Serial.print(buf);
  Serial.print('\x03');

  status_string_t status;


  char sbuf[256];
  readSerialAscii(sbuf, sizeof(sbuf) - 1);
  
  if (sbuf[4] == '1')
  {
    char pwr = sbuf[13];
    char mode = sbuf[17];
    char fan = sbuf[21];
    char tbuf[2];
    strncpy(tbuf, &sbuf[30], 2);
    unsigned int number = (int)strtol(tbuf, NULL, 16);
    unsigned int temp = number * 5;
    char rem[] = ".0\0";
    if (temp % 10)
      rem[1] = '5';
    temp = temp / 10;

    char sfan[2];
    switch (fan)
    {
    case '0':
      sfan[0] = '1';
      break;
    case '1':
      sfan[0] = '2';
      break;
    case '2':
      sfan[0] = '3';
      break;
    case '6':
      sfan[0] = '4';
      break;
    default:
      sfan[0] = '0';
    }
    sfan[1] = '\0';
    char smode[6];
    switch (mode)
    {
    case '2':
      strncpy(smode, "cool\0", 5);
      break;
      ;
    case '1':
      strncpy(smode, "dry\0", 4);
      break;
      ;
    case '4':
      strncpy(smode, "heat\0", 5);
      break;
      ;
    case '3':
      strncpy(smode, "fan\0", 4);
      break;
      ;
    case '0':
      strncpy(smode, "auto\0", 5);
      break;
      ;
    }

    status.power = pwr;
    status.mode = smode;
    status.speed = sfan;
    status.temp = String(temp) + String(rem);
  }

  return status;
}

uint8_t fetchOperationalData(hvac_data_t &hvac)
{
  char sbuf[256];
  size_t sbuflen = readSerialAscii(sbuf, sizeof(sbuf) - 1);

  if (sbuf[2] == 'R' && sbuf[3] == '2')
  {
    delay(500);
    char buf[64];
    sprintf(buf, "RSR20000E9");
    Serial.print('\x02');
    Serial.print(buf);
    Serial.print('\x03');
  }

  if (sbuf[2] == 'R' && sbuf[3] == '1')
  {
    hvac = parseOperationalData(sbuf, sbuflen);
    return true;
  }
  
  return false;
}
