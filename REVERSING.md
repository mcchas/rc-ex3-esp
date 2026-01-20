
# Serial protocol

The protocol was reversed from an Mitsubishi application which only runs on Windows XP. 

The program does not appear to be publicly available. I managed to get this from a user on forum which had been passed on from another user on another obscure forum..

The reversing setup used a Windows 7 virtual machine running the mitsubish application in XP compatability mode and a USP-over-IP client.
The Mitsubish pannel was connected to a Raspberry Pi running USPIP.
Tcpdump was used to monitor the data between the client and the device.


Serial 38400 Baud, 8E1.
```
    H      L                 M                       T
    D      E         P       O       F               E                                  S
    R      N         W       D       A               M                                  U
           ?         R       E       N       ?       P                                  M
    RSSL  12 FF00   01 FF   02 03   03 FF   04 FF   05 FF      06 FF   0F FF   43 FF   FC
    RSSL  11 FF00   01 10   02 13   03 12   04 10   05 13 24   06 14   0F 11           77
    RSSL  13 FF00   01 FF   02 FF   03 FF   04 FF   05 03 ??   06 FF   0F FF   43      FF     <- set temp
    RSSL  12 FF00   01 FF   02 FF   03 FF   04 FF   05 FF      06 FF   0F FF   43 FF   25     <- getStatus
    RSSL  11 FF00   01 10   02 13   03 12   04 10   05 13 2C   06 14   0F 11           86
    
```

Only the main features were reversed as it covers my requirements (homekit):

* Setting power
* Setting mode
* Setting temperature
* Setting fan speed
* Setting off timer
* Fetching operational data / diagnostics


Other features could be added. Reach out if you want the original executable to reverse the remaining features like:

* Synchronising the device time
* Setting schedules
* Setting on delays



# Using the orignal software

This device provides a TCP socket direct to the serial port on the RC-EX3. 

Using a virtual COM port application (using TCP port 1123) will work with the original tool called PC-Remote (PC-RemoteSetup.exe).
This is useful for reversing other functions or manging other settings.