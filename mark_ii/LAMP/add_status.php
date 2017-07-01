<?php
ini_set('display_errors', 'On');
error_reporting(E_ALL);
// Connect to MySQL
include("dbconnect_bees.php");

// Prepare the SQL statement
$station = $_GET["station"];
$msg = $_GET["message"];

echo "Station: " . $station . PHP_EOL;
echo "Message: " . $msg . PHP_EOL;

$SQL = "INSERT INTO `status` (station, message) VALUES ('$station','$msg')";
// Execute SQL statement
mysqli_query($link, $SQL);
?>
