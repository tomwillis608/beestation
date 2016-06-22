<?php
    ini_set('display_errors', 'On');
    //error_reporting(E_ALL);
    // Start MySQL Connection
    include('dbconnect.php');
?>

<html>
<head>
    <title>Arduino Beestation Status Log</title>
    <style type="text/css">
        .table_titles, .table_cells_odd, .table_cells_even {
                padding-right: 20px;
                padding-left: 20px;
                color: #000;
        }
        .table_titles {
            color: #FFF;
            background-color: #666;
        }
        .table_cells_odd {
            background-color: #CCC;
        }
        .table_cells_even {
            background-color: #FAFAFA;
        }
        table {
            border: 2px solid #333;
        }
        body { font-family: "Trebuchet MS", Arial; }
    </style>
</head>

<body>
    <h1>Arduino Beestation Status Log</h1>
    <table border="0" cellspacing="0" cellpadding="4">
        <tr>
            <td class="table_titles">ID</td>
            <td class="table_titles">Date and Time</td>
            <td class="table_titles">Sensor Serial</td>
            <td class="table_titles">Message</td>
        </tr>

<?php
    // Get record count
    $sql = "SELECT count(id) from beestation";
    $result = mysqli_query($link, $sql);
    $row = mysqli_fetch_array($result, MYSQL_NUM);
    $record_count = $row[0];

    $record_limit = (isset($_GET['limit'])) ? $_GET['limit'] : 20;
    $page = (isset($_GET['page'])) ? $_GET['page'] + 1 : 0;
    $offset = $record_limit * $page ;
         
    // Retrieve all records and display them
    $left_record = $record_count - ($page * $record_limit);

    $sql = "SELECT * FROM beestation ORDER by id DESC LIMIT $offset, $record_limit";
    $result = mysqli_query($link, $sql); 

    // Used for row color toggle
    $oddrow = true;

    // process every record
    while( $row = mysqli_fetch_array($result) ) {
        if ($oddrow) {
            $css_class=' class="table_cells_odd"';
        } else {
            $css_class=' class="table_cells_even"';
        }
        $oddrow = !$oddrow;
        echo '<tr>';
		echo '  <td'.$css_class.'>'.$row["id"].'</td>';
		echo '  <td'.$css_class.'>'.$row["event"].'</td>';
		echo '  <td'.$css_class.'>'.$row["sensor"].'</td>';
		echo '  <td'.$css_class.'>'.$row["message"].'</td>';
		echo '</tr>';
    }

    if ($left_record < $record_limit ) {
        $previous = $page - 2;
        echo "<a href = \"$_SERVER[PHP_SELF]?page=$previous&limit=$record_limit\">Previous $record_limit Records</a>";
    } else if ($page > 0) {
        $previous = $page - 2;
        echo "<a href = \"$_SERVER[PHP_SELF]?page=$previous&limit=$record_limit\">Previous $record_limit Records</a> |";
        echo "<a href = \"$_SERVER[PHP_SELF]?page=$page&limit=$record_limit\">Next $record_limit Records</a>";
    } else if ($page == 0) {
        echo "<a href = \"$_SERVER[PHP_SELF]?page=$page&limit=$record_limit\">Next $record_limit Records</a>";
    }  
    // free result set
    mysqli_free_result($result);
?>
    </table>
    </body>
</html>

<php
    // close connection 
    mysqli_close($link);
?>
