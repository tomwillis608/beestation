# beestation/arduino/server/php

PHP scripts for wireless bee monitoring station experiments

Currently embarassing hacking at simple Arduino and LAMP projects

The add_* PHP scripts are called by the Arduino monitoring station to record data in MySQL tables

add_info.php - Add startup information about the monitoring station to the database
add_data.php - Add sensor data from the monitoring station to the database

The view_* PHP scripts are useful for viewing the data in a simple way

view_info.php - See the data about the monitoring station behavior
view_data.php - See the sensor data collected from the monitoring station

All of the four scripts above include "dbconnect.php", which contains database secrets.

You must create your own dbconnect.php file that defines your own database secrets, using the dbconnect_template.php as an example, to define your MySQL username, password, server and database name.

-Tom Willis
Hacking Arduino for bees
