# Add Wifi to your Mitsubish RC-EX3 Air Conditioner 

Enables MQTT and HTTP control of a Mitsubish RC-EX3 air conditioner panel.

I have installed these in three units in my home and they have run flawlessly since Jan 2020 (over 4 years). They integrate with the rest of my bespoke home automation system (using homekit) and could be easily used unmodified, with the popular platforms of today.

This only requires a few breadboard jumper wires, an ESP-12 (ESP8266) and a 3.3v step-down buck converter. A linear regular could be used but may draw too much current and produce unwanted heat. A thermocouple is in the enclosure which should be kept away from the new components.

This is easy to install without any permanent modifications or soldering to the unit. This is done by inserting breadboard jumper pins into the through-hole vias on the board. The two components fit easily into the panel, however routing some wires can be easier with minor internal modifications to the plastic enclosure.



# Serial protocol

The protocol was reversed from an Mitsubishi application which only runs on Windows XP. 

The program does not appear to be publicly available. I managed to get it from a user on forum (thanks @Jesus-whirlpool) who found it from another user on a Russian forum..

The reversing setup used a WinXP virtual machine running the mitsubish application and a USP-over-IP client.
The Mitsubish pannel was connected to a Raspberry Pi running USPIP.
Tcpdump was used to monitor the data between the client and the device.


Serial 38400 Baud, 8E1.
```
                         M                  T
                   P     O     F            E             S
                   W     D     A            M             U
                   R     E     N            P             M
    RSSL 12 FF0001 FF 02 03 03 FF 04FF05 FF 06 FF0FFF43FF FC

    RSSL 11 FF0001 10 02 13 03 12 041005 13 24 06140F11   77

    RSSL 13 FF0001 FF 02 FF 03 FF 04FF05 03 ?? 06FF0FFF43 FF     <- set temp

    RSSL 12 FF0001 FF 02 FF 03 FF 04FF05 FF 06 FF0FFF43FF 25     <- getStatus
    RSSL 11 FF0001 10 02 13 03 12 041005 13 2C 06140F11   86
```

Only the main features were reversed as it covered my requirements, however the remaining features could be added. 
Reach out if you want the original executable to reverse the remaining features like:

* Setting the device time
* Setting schedules
* Setting on/off delays


# Building the hardware.

Build and flash the ESP with the firmware.

Prepare 4 wires to connect to the panels PCB (vcc, gnd, tx & rx). Using molded round pin (not the square dupont type) jumper wires, bend the pin on a 90 degree angle and cut off the excess wire, leaving a small stub. These fit snugly into the through-hole vias on the PCB.

Solder RX and RX jumper wires to the ESP-12.

Solder VCC and GND wires from the ESP-12 to the 3.3v Regulator. Depending on your regulator, I'd recommended adding a small electrolytic capacitor here.

[<img src="images/rc3-overview.png" width=50%/>](image.png)


Solder ground and positive wires to the regultor. 

[<img src="images/buck.png" width=50%/>](image.png)


Place the regulator into the void on the side of the enclusure. 
Press the jumper wire pins into the vias as shown.
The ESP-12 can sit flat against the PCB once sheilded with shrinkwrap.

[<img src="images/rc3-regulator-placement.png" width=50%/>](image.png)


[<img src="images/rc3-ttl-uart.png" width=50%/>](image.png)

[<img src="images/rc3-regulator-power.png" width=50%/>](image.png)


# Setup

Wifi Manager will advertise an AP to connect to when uninitialised. 
Use this to set your network credentials, MQTT topic and server details (http://192.168.4.1).

Fetch the units state with the status command.

Set the state with a JSON payload.

```
{
    "power": true/false,
    "mode": cool/dry/heat/fan/auto
    "speed": 0/1/2/3/4
    "temp": 16.0-30.0 
}
```