# beestation/arduino

Arduino sketch for wireless beehive monitoring station experiments

Currently embarassing hacking at simple Arduino and LAMP projects

[![](https://github.com/tomwillis608/beestation/blob/master/arduino/Beestation%20breadboard.png)]

beestation.ino - contains sketch for listening to a sensor and sending the readings back to a PHP over WiFi

beestation_breadboard.png - Frizing breadboard diagram for the kit (Note, I am actually using an Adafruit BME280 breakout, not the Sparkfun one shown in the image.

You must create your own MyCommon/mynetwork.h file that defines your own network secrets in your Arduino libraries folder. This must contain these macros, with the dummy values replaced with your own.
```C++
// Mynetwork.h
#define WEB_SERVER_IP_COMMAS 192, 168, 1, 1
#define WEB_SERVER_IP_DOTS   "192.168.1.1"
#define WLAN_SSID       "Your Network SSID"
#define WLAN_PASS       "Your Network Passcode"
```

I'm using the Visual Micro extension for Visual Studio because I like single-step debugging but you can use the sketch without all the Visual Studio clutter.

## THANKS:
Thanks to Marc-Oliver Schwartz for this excellent walkthrough of creating an Arduino weather station, with testing as you go.  I based much of this on his example. 
https://www.openhomeautomation.net/arduino-wifi-cc3000/
https://learn.adafruit.com/wifi-weather-station-arduino-cc3000/introduction

## Sensors:
* BME280 - Pressure, Humidity and Temperature. This will be measured outside the hive, at the board.
* DS2401 - Silicon serial number.  Unique ID for this kit. 

## Hardware watchdog
* NE555-based watchdog timer added to reset the Arduino if any of the code locks up
  
## To do:
* Add DS18B20 temperature sensors, in beeproof sleeves, to measure inside the hive
* Build hive sound volume level meter, in beeproof sleeve
* Harden wireless code
* Use built-in WTD watchdog for power-saving sleep
* On server end, collect local temperature and pressure from WU or other API as control values

-Tom Willis

Hacking Arduino for bees
