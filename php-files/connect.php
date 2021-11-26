<?php
    $host = "localhost";
    $user = "root";
    $password = "";
    $db = "it";

    $connect = new mysqli($host, $user, $password, $db) or die(mysqli_error());

    if($connect){
        echo'conectou';
    }
    else{
        echo 'não conectou';
    }

?>