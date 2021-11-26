<?php
    require_once "connect.php";

    (isset($_GET['valor'])) ? $valor = $_GET['valor'] : $valor = NULL;
    (isset($_GET['data'])) ? $data = $_GET['data'] : $data = NULL;
    (isset($_GET['hora'])) ? $hora = $_GET['hora'] : $hora = NULL;

    if(
        $valor != NULL
        && $data != NULL
        && $hora != NULL
    ){
        $query = "INSERT INTO plate_temperatures(value, time, date) VALUES ('$valor', '$hora', '$data')";
        $send = mysqli_query($connect, $query) or die(mysqli_error());
        echo('Dados enviados para o banco de dados "'.$db.'" na tabela "plate_temperatures" ');
    }
    else{
        echo('Erro na captura dos dados');
    }

?>