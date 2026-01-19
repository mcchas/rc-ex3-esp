#include <Arduino.h>

#define HEADER_LEN 4
#define POS_OPERATION_MODE        8
#define POS_INDOOR_AIR_TEMP       9
#define POS_TARGET_TEMP           7
#define POS_PROTECTION_CONTROL    21
#define POS_OUTDOOR_AIR_TEMP      26
#define POS_RETURN_AIR_TEMP       27
#define POS_OUTDOOR_HX_TEMP1      28
#define POS_COMPRESSOR_HZ         32
#define POS_OUTDOOR_FAN_SPEED     32
#define POS_INDOOR_HX_TEMP1       35
#define POS_DEFROST               37
#define POS_INDOOR_HX_TEMP3       40
#define POS_INDOOR_FAN_SPEED      45
#define POS_EEV_OPENING_MSB       46
#define POS_EEV_OPENING_LSB       47
#define POS_COMPRESSOR_HOURS_MSB  44
#define POS_COMPRESSOR_HOURS_LSB  45
#define POS_CURRENT               42

void serialFlush();
uint8_t checksum(char *data, uint16_t length);
void setFanSpeed(uint8_t speed);
void setFanSpeed(const char* speed);
void setPowerOn(uint8_t state);
void setTemp(uint16_t degrees);
void setMode(uint8_t mode);
void setMode(const char* mode);
void setOffTimer(uint8_t hours);
uint8_t getOffTimer();
void requestOperationalData();
void pollForOperationalData();
typedef struct
{
  uint8_t power;
  uint8_t mode;
  uint16_t degrees;
  uint8_t speed;
} Settings;

typedef struct 
{
  String power;
  String mode;
  String temp;
  String speed;
} status_string_t;


void setClimate(Settings s);

typedef struct {
    uint8_t operation_mode;
    int8_t  indoor_air_temp;
    int8_t  target_temp;
    int8_t  outdoor_air_temp;
    int8_t  return_air_temp;
    int8_t  outdoor_hx_temp1;
    int8_t  indoor_hx_temp1;
    int8_t  indoor_hx_temp3;
    uint8_t defrost;
    uint8_t indoor_fan_speed;
    uint8_t compressor_hz;
    uint8_t outdoor_fan_speed;
    uint16_t outdoor_eev_opening;
    uint16_t compressor_hours;
    int8_t  current;
    uint8_t protection_control;
} hvac_data_t;

hvac_data_t parseOperationalData(char *data, uint16_t length);
status_string_t getStatus();
uint8_t fetchOperationalData(hvac_data_t &hvac);
String statusJsonString(status_string_t status);
uint8_t modeToInt(const char* mode);
size_t readSerialAscii(char* buffer, size_t maxLength);