# https://github.com/tomwillis608/beestation/blob/master/arduino/555%20Hardware%20Reset%20bb.pngHardware watchdog for Ardunio

Somewhere in the interaction of my wireless network, which has some instabilities I have not yet sorted out, and the CC3300 WiFi on my Arduino beestation, I am getting lockups of the sketch, typically in the TCP conenct calls.  This seems to happend about four times a day, on average. The built-in AVR watch dog timer does not consistently reset the Arduino when this happens.  I decided to add a hardware watchdog to suppliment and perhaps replace the built-in AVR watchdog.  (TO DO: repurpose the AVR WTD to put the system to sleep in between measurements to conserve power.)

Thankfully, there are several useful articles on creating a hardware watchdog using a NE555 timer chip, including:
* http://www.practicalarduino.com/news/id/471
* http://forum.arduino.cc/index.php?topic=55041.0
* http://arduino.stackexchange.com/questions/7994/resetting-an-arduino-with-a-timer
* https://github.com/mattbornski/Arduino-Watchdog-Circuit
* https://www.circuitlab.com/circuit/cc9d7s/arduino-555-watchdog-circuit/
* http://www.playwitharduino.com/?p=291

I followed the cicruit design from http://www.playwitharduino.com/?p=291 and experimented with different capacitor values for C1, which changed the charge time as predicted by the forumlas provided in that article. I ended up settling with the 220uF suggested by the author since it was long enough but not as long at the 470uF provided. 

One of several 555 calculators is here: http://www.csgnetwork.com/ne555timer2calc.html

The 555 chip is new to me, so I enjoyed reading the 555 site http://www.555-timer-circuits.com/, especially the FAQ at http://www.555-timer-circuits.com/common-mistakes.html

[![](https://github.com/tomwillis608/beestation/blob/master/arduino/555%20Hardware%20Reset%20bb.png)]

[![](https://github.com/tomwillis608/beestation/blob/master/arduino/shield%20top.png)]

[![](https://github.com/tomwillis608/beestation/blob/master/arduino/shield%20bottom.png)]
