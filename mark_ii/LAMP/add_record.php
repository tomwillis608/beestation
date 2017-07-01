<?php
ini_set('display_errors', 'On');
error_reporting(E_ALL);
// Connect to MySQL
include("dbconnect_bees.php");

// Prepare the SQL statement
$probe = $_GET["s"];
$result = $_GET["r"];
$type = $_GET["t"];
$seq = $_GET["n"];

echo "Probe: " . $probe . PHP_EOL;
echo "Result: " . $result . PHP_EOL;
echo "Type: " . $type . PHP_EOL;
echo "Sequence: " . $seq . PHP_EOL;

$SQL = "INSERT INTO `records` (probe,result,type,sequence) VALUES ('$probe','$result','$type','$seq')";

// Execute SQL statement
mysqli_query($link, $SQL);
?>
