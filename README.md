# beestation
Repo for wireless bee monitoring station experiments.

Currently embarassing hacking at simple Arduino and LAMP projects.  My first Arduino project.

ardunio/ - contains sketch for listening to sensors and sending the readings back to a PHP over WiFi.
server/ - contains php and mysql to use on your LAMP server

Done:
 * Add hardware watchdog circuit based on 555 chip. Something between my wireless network and the TCP connect calls locks up 5x a day.

To Do:
 * Add multiple One-Wire sensors
 * Add power-saving features

-Tom Willis
Hacking Arduino for bees
