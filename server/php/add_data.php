<?php
ini_set('display_errors', 'On');
error_reporting(E_ALL);
// Connect to MySQL
include("dbconnect.php");

// Prepare the SQL statement
$sensor = $_GET["sn"];
$celsius = $_GET["t1"];
$humidity = $_GET["hu"];
$pressure = $_GET["pr"];
$tBox = $_GET["t2"];
$t2In =  $_GET["t3"];
$t6In =  $_GET["t4"];
$t10In =  $_GET["t5"];
echo "Sensor: " . $sensor . PHP_EOL;
$t2In2 =  $_GET["t6"];
$t6In2 =  $_GET["t7"];
$t10In2 =  $_GET["t8"];
$t2In3 =  $_GET["t9"];
$t6In3 =  $_GET["t10"];
$t10In3 =  $_GET["t11"];
echo "Temperature: " . $celsius . PHP_EOL;
echo "Humidity: " . $humidity . PHP_EOL;
echo "Pressure: " . $pressure . PHP_EOL;	
$SQL = "INSERT INTO `dht22` (sensor,celsius,humidity,pressure,box_celsius,temp_2inch,temp_6inch,temp_10inch,2temp_2inch,2temp_6inch,2temp_10inch,3temp_2inch,3temp_6inch,3temp_10inch) VALUES ('$sensor','$celsius','$humidity','$pressure','$tBox','$t2In','$t6In','$t10In','$t2In2','$t6In2','$t10In2','$t2In3','$t6In3','$t10In3')";

// Execute SQL statement
mysqli_query($link, $SQL);
?>
