<?php
ini_set('display_errors', 'On');
error_reporting(E_ALL);
// Connect to MySQL
include("dbconnect.php");

// Prepare the SQL statement
$sensor = $_GET["serial"];
$celsius = $_GET["temperature"];
$humidity = $_GET["humidity"];
	
$SQL = "INSERT INTO `dht22` (sensor,celsius,humidity) VALUES ($sensor,$celsius,$humidity)";
// Execute SQL statement
mysqli_query($link, $SQL);
?>
