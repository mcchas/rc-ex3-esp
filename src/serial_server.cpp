#include "serial_server.h"

extern WiFiServer localServer;
extern WiFiClient localClient;

void handleSerialServer()
{

  if (localServer.hasClient())
  {
    if (!localClient.connected())
    {
      if (localClient)
        localClient.stop();
      localClient = localServer.available();
    }
  }

  // check a client for data
  if (localClient && localClient.connected())
  {
    if (localClient.available())
    {
      size_t len = localClient.available();
      uint8_t sbuf[len];
      localClient.readBytes(sbuf, len);
      Serial.write(sbuf, len);
    }

    // check UART for data
    if (Serial.available())
    {
      size_t len = Serial.available();
      uint8_t sbuf[len];
      Serial.readBytes(sbuf, len);
      localClient.write(sbuf, len);
    }
  }
}
