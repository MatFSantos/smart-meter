<?php
    require_once "connect.php";
    $tables = ['voltages', 'currents', 'powers', 'energies', 'room_temperatures', 'plate_temperatures', 'rads'];
    foreach ($tables as $table) {
        $query = "DELETE from ".$table;
        $send = mysqli_query($connect, $query) or die(mysqli_error());
    }
    echo('Dados deletados de todas as tabelas do banco de dados "'.$db.'"');



?>