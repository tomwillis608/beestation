<?php
ini_set('display_errors', 'On');
error_reporting(E_ALL);
// Connect to MySQL
include("dbconnect.php");

// Prepare the SQL statement
$sensor = $_GET["serial"];
$msg = $_GET["message"];
echo "Sensor: " . $sensor . PHP_EOL;
echo "Message: " . $msg . PHP_EOL;
$SQL = "INSERT INTO `beestation` (sensor, message) VALUES ('$sensor','$msg')";
// Execute SQL statement
mysqli_query($link, $SQL);
?>
