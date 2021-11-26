/*Biblioteca para a comunicação serial Arduino - NodeMCU*/
#include <SoftwareSerial.h>

//Pinos dos sensores:
#define TENSAO A0
#define CORRENTE A1
#define TEMPA A2
#define TEMPP A3

//Variaveis para acumular os valores analógicos dos sensores:
int analogTensao = 0;
int analogCorrente = 0;
int analogTempA = 0;
int analogTempP = 0;

//Variavel acumuladora:
int acumulador = 0;


boolean ok = false;

//Inicia o objeto serial para comunicação:
SoftwareSerial receiver(A5, A4); // RX, TX
void setup(){
  Serial.begin(9600);
  pinMode(TENSAO, INPUT);
  pinMode(CORRENTE, INPUT);
  pinMode(TEMPA, INPUT);
  pinMode(TEMPP, INPUT);

  delay(1000);

  Serial.println("Inicio da captura de dados...");

  receiver.begin(9600);
  
  Serial.println("Aguardando NodeMCU");
  while(!receiver.available()){
    Serial.print(" . ");
    delay(500);
  }
  receiver.read();
  ok = true;
  Serial.println("\nComeçando captura de dados...");
  
}

void loop(){
  if(ok){
    Serial.read();
    analogTensao = analogTensao +  analogRead(TENSAO);
    analogCorrente = analogCorrente +  analogRead(CORRENTE);
    analogTempA = analogTempA +  analogRead(TEMPA);
    analogTempP = analogTempP +  analogRead(TEMPP);
    
    Serial.println("Tensao: " + String(analogRead(TENSAO)));
    Serial.println("Corrente: " + String(analogRead(CORRENTE)));
    Serial.println("TempA: " + String(analogRead(TEMPA)));
    Serial.println("TempP: " + String(analogRead(TEMPP)));
    
    acumulador++;
    Serial.println("Acumulador:" + String(acumulador));

    if(acumulador == 30){
      int mediaTensao = analogTensao/acumulador;
      int mediaCorrente = analogCorrente/acumulador;
      int mediaTempA = analogTempA/acumulador;
      int mediaTempP = analogTempP/acumulador;

      Serial.println("Tensao media: " + String(mediaTensao));
      Serial.println("Corrente media: " + String(mediaCorrente));
      Serial.println("TempA media: " + String(mediaTempA));
      Serial.println("TempP media: " + String(mediaTempP));
      
      receiver.println(mediaTensao);
      receiver.println(mediaCorrente);
      receiver.println(mediaTempA);
      receiver.println(mediaTempP);
      Serial.println("dados enviados, esperando resposta...");
      ok = false;
      acumulador = 0;
      analogTensao = 0;
      analogCorrente = 0;
      analogTempA = 0;
      analogTempP = 0;
    }
  }
  else{
    Serial.println("Aguardando NodeMCU");
    while(!receiver.available()){
      Serial.print(" . ");
      delay(500);
    }
    receiver.read();
    ok = true;
    Serial.println("\nComeçando captura de dados...");
  }
  delay(500);
}
