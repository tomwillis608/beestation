# beestation/arduino

Arduino sketch for wireless bee monitoring station experiments

Currently embarassing hacking at simple Arduino and LAMP projects

beestation.ino - contains sketch for listening to a sensor and sending the readings back to a PHP over WiFi

You must create your own MyCommon/mynetwork.h file that defines your own network secrets in your Arduino libraries folder. This must contain these macros, with the dummy values replaced with your own.
// Mynetwork.h
#define WEB_SERVER_IP_COMMAS 192, 168, 1, 1
#define WEB_SERVER_IP_DOTS   "192.168.1.1"
#define WLAN_SSID       "Your Network SSID"
#define WLAN_PASS       "Your Network Passcode"

I'm using the Visual Micro extension for Visual Studio because I like single-step debugging but you can use the sketch without all the Visual Studio clutter.

THANKS:
Thanks to Marc-Oliver Schwartz for this excellent walkthrough of creating an Arduino weather station, with testing as you go.  I based much of this on his example. 
https://www.openhomeautomation.net/arduino-wifi-cc3000/
https://learn.adafruit.com/wifi-weather-station-arduino-cc3000/introduction


-Tom Willis
Hacking Arduino for bees
