<?php
ini_set('display_errors', 'On');
error_reporting(E_ALL);
// Connect to MySQL
include("dbconnect.php");

// Prepare the SQL statement
$sensor = $_GET["serial"];
$celsius = $_GET["temperature"];
$humidity = $_GET["humidity"];
$pressure = $_GET["pressure"];
echo "Sensor: " . $sensor . PHP_EOL;
echo "Temperature: " . $celsius . PHP_EOL;
echo "Humidity: " . $humidity . PHP_EOL;
echo "Pressure: " . $pressure . PHP_EOL;	
$SQL = "INSERT INTO `dht22` (sensor,celsius,humidity,pressure) VALUES ('$sensor','$celsius','$humidity','$pressure')";

// Execute SQL statement
mysqli_query($link, $SQL);
?>
