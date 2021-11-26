/*Biblioteca para fazer requisições HTTP*/
#include <ESP8266HTTPClient.h>

/*Biblioteca para fazer comunicação WiFi*/
#include <ESP8266WiFi.h>

/*Biblioteca para a comunicação serial Arduino - NodeMCU*/
#include <SoftwareSerial.h>

/*Biblioteca para captura do 'Timestamp'*/
#include <TimeLib.h>

/*Bibliotecas para serviços UDP e NTP*/
#include <WiFiUdp.h>
#include <NTPClient.h>

/*Biblioteca para o display LCD*/
#include <LiquidCrystal_I2C.h>

/*Credenciais utilizadas ao longo do código, como dados do WiFi*/
#include "credenciais.h"

/*****************************************************************/
/************Objetos, variaveis, funções e constantes*************/
/*****************************************************************/
SoftwareSerial sender(D5, D6); // RX, TX

//Servidor NTP usado para o 'timestamp':
static const char ntpServerName[] = "us.pool.ntp.org";
unsigned int localPort = 8888;

//Time-zone de São Paulo, Brasil:
const int timeZone = -3;

//Objeto WiFiUDP:
WiFiUDP Udp;

//Buffer para armazenar os pacotes de entrada e saída:
byte packetBuffer[NTP_PACKET_SIZE];

//PINO DO BOTAO:
#define BUTTON D3

//VALOR DA MEDIÇÃO DIGITAL:
#define conversao 4.88758553

//SENSIBILIDADE DOS SENSORES:
#define sensibilidadeTensao 1000.0
#define sensibilidadeCorrente 185.0
#define sensibilidadeTemperatura 100.0

//DISPLAY LCD I2C:
#define endereco 0x27
#define colunas  20
#define linhas   4

//Variavel para guarda o tempo da ultima leitura (para fins de cálculo da energia):
float pastTime = 0.0;

//Variavel que acumula a energia:
float energia = 0.0;

//flag para sinalizar qual mensagem o display irá exibir:
boolean flagDisplay = true;

//rotas das requisições:
#define fileVoltage "sendVoltage.php"
#define fileCurrent "sendCurrent.php"
#define filePower   "sendPower.php"
#define fileEnergy  "sendEnergy.php"
#define fileTempA   "sendTemperatureR.php"
#define fileTempP   "sendTemperatureP.php"
#define fileRad     "sendRad.php"
String ipPc = "221.1.1.109";

//Objeto LCD:
LiquidCrystal_I2C lcd(endereco, colunas, linhas);

float tensao = 0.0, corrente = 0.0, tempA = 0.0, tempP = 0.0, potencia = 0.0, rad = 0.0;

void setup(){
  Serial.begin(115200); //inicia comunicação serial com o computador
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  delay(1000);
  
  setupWiFi(user, password); //inicia comunicação com o WiFi

  //inicia o objeto Udp na portal local informada (8888):
  Udp.begin(localPort);

  //Faz a sincronização do provedor para capturar o timestemp correto:
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  //inicia o display LCD:
  lcd.init();
  lcd.clear();
  lcd.backlight();
  
  sender.begin(9600); //inicia comunicação serial com arduino

  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Informe se deseja apagar os dados, ou continuar com os dados já existentes no banco de dados");
  lcd.setCursor(0,0);
  lcd.print("Aperte ou segure");
  lcd.setCursor(0,1);
  lcd.print("o botao");
  while(digitalRead(BUTTON)){
    Serial.print(". ");
    delay(300);
  }
  int i = 0;
  while(!digitalRead(BUTTON)){
    i++;
    delay(500);
    if(i>=6)
      break;
  }
  if(i>=6){
    String response = clearDatabase();
    if(response == "200"){
      Serial.println("Dados apagados");
      lcd.setCursor(0,3);
      lcd.print("Dados apagados");  
    }
    else{
      Serial.println("Erro ao apagar dados");
      lcd.setCursor(0,3);
      lcd.print("Erro ao apagar dados");
    }
    
  }
  else{
    Serial.println("Dados mantidos.");
    lcd.setCursor(0,3);
    lcd.print("Dados mantidos");
  }

  digitalWrite(LED_BUILTIN, HIGH);
  delay(1800);
  sender.write(1); // para avisar que está tudo certo é mandado um 1 para o arduino
  printDataLCD();
  printTimeLCD();
  digitalWrite(LED_BUILTIN, LOW); 
}
void loop(){
  
  boolean wasRead = false;
  
  if(sender.available()){
    //avisa que um dado foi lido:
    wasRead = true;

    //Ler os dados:
    readData();
  }
  //Verifica se o WiFi está conectado:
  if(WiFi.status() == WL_CONNECTED){
    //verifica se algum dado foi lido recentemente:
    if(wasRead){
      //envia as requisições com os dados para o banco de dados:
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("Envio da Tensão. Resposta do servidor: " + sendRequest_(tensao, fileVoltage));
      Serial.println("Envio da Corrente. Resposta do servidor: " + sendRequest_(corrente, fileCurrent));
      Serial.println("Envio da TempA. Resposta do servidor: " + sendRequest_(tempA, fileTempA));
      Serial.println("Envio da TempP. Resposta do servidor: " + sendRequest_(tempP, fileTempP));
      Serial.println("Envio da Potência. Resposta do servidor: " + sendRequest_(potencia, filePower));
      Serial.println("Envio da Energia. Resposta do servidor: " + sendRequest_(energia, fileEnergy));
      Serial.println("Envio da Radiação. Resposta do servidor: " + sendRequest_(rad, fileRad));
      
      // para avisar que está tudo certo é mandado um 1 para o arduino
      sender.write(1);
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
  else{
    if(wasRead){
      lcd.clear();
      lcd.setCursor(2,1);
      lcd.print("Falha na conexao");
      lcd.setCursor(1,2);
      lcd.print("Dados nao enviados");
      Serial.println("WiFi não conectado. Não foi possível enviar requisições");
      int i = 0;
      while(i<10){
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        i++;
      }
    }
  }
  printDataLCD();
  printTimeLCD();
  

  if(!digitalRead(BUTTON)){
    flagDisplay = !flagDisplay;
  }
  delay(300);
}

/*
 * Faz a leitura e conversão dos valores a partir da comunicação serial
 */
void readData(){

  digitalWrite(LED_BUILTIN, LOW);
  int t = readSerial();
  delay(1);
  int c = readSerial();
  delay(1);
  int tpa = readSerial();
  delay(1);
  int tpp = readSerial();
  delay(1);
  //captura e converte as leituras de tensao, corrente, e temperaturas.:
  tensao = conversor(t, "tensao")/sensibilidadeTensao;
  corrente = conversor(c, "corrente")/sensibilidadeCorrente;
  tempA = conversor(tpa, "temperatura1")/sensibilidadeTemperatura;
  tempP = conversor(tpp, "temperatura2")/sensibilidadeTemperatura;

  //calcula a potência a partir da corrente e tensão lidas:
  potencia = tensao*corrente;
  
  //Verifica o intervalo de leitura para calcular a energia:
  float interval = 0.0;
  if(pastTime != 0.0){
   interval = (millis() - pastTime)/3600000;
  }

  //calcula a energia com o intervalo encontrado e a potência:
  energia = energia + (potencia*interval);

  //calcula a radiação:
  rad = 0.0;

  //captura o instante que a leitura foi feita:
  pastTime = millis();

  //prints para depuração pelo monitor serial:
  Serial.println("V: " + String(tensao) + " I: " +  String(corrente) + " T1: " + String(tempA));
  Serial.println(" T2: " + String(tempP) + " P: " + String(potencia) + " E: " + String(energia));
  digitalWrite(LED_BUILTIN, HIGH);
}

/*
 * Realiza a comunicação com o WiFi.
 * 
 */
void setupWiFi(const char * ssid,const char * pass){
  Serial.print("\nTentando conexão com WiFi ");
  
  WiFi.begin(ssid, pass);

  digitalWrite(LED_BUILTIN, LOW);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(". ");
  }

  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("\nConectado ao WiFi " + String(ssid));
}

/*
 * Faz a leitura de um dado da porta serial até encontrar uma vírgula.
 * Após capturar o dado o transforma em inteiro.
 * 
 * return:
 *    Retorna o dado que foi lido, já convertido em inteiro.
 */
int readSerial(){
  String val;
  val = sender.readStringUntil('\n');

  Serial.println("Valor recebido: ");
  Serial.println(val);
  return atoi(val.c_str());
}

/*
 * Converte um valor analogico ( ex: 1023) em um valor de tensão equivalente.
 * 
 * Parâmetros:
 *  analogVal -> valor analógico à ser convertido.
 *  sensor    -> nome do sentos que vai ser feito a conversão.
 * 
 * return:
 *  retorna o valor convertido em tensão.
 * 
 */
float conversor(int analogVal,String sensor){
  if(sensor == "corrente")
    return (analogVal - 512)*conversao;
  else
    return analogVal*conversao;
}

/*
 * Faz uma requisição http por meio de arquivos PHP para adicionar um dado ao banco de dados
 * 
 * Parâmetros:
 *  data -> dado que será guardado no banco de dados.
 *  file -> arquivo que contém o código php com a requisição.
 *  
 * return:
 *  retorna a resposta da requisição 'get' feita.
 *    se == 200 -> requisição bem sucedida.
 *    se != 200 -> requisição com problemas.
 */
String sendRequest_(float data, String file){
  HTTPClient http;

  //Inicia o objeto http com a rota e os dados da requisição get( o valor, a data e a hora):
  http.begin("http://"+ ipPc + "/get/" + file + "?valor=" + String(data) + "&data=" + getDate() + "&hora=" + getTime());

  //envia a requisição:
  int httpCode = http.GET();

  //encerra a comunicação:
  http.end();

  //retorna a resposta da requisição get:
  return String(httpCode);
}

/*
 * Faz uma requisição http por meio de arquivos PHP para limpar a base de dados
 * 
 * return:
 *  retorna a resposta da requisição 'get' feita.
 *    se == 200 -> requisição bem sucedida.
 *    se != 200 -> requisição com problemas.
 */
String clearDatabase(){
  HTTPClient http;

  //Inicia o objeto http com a rota para o arquivo que limpa a base de dados:
  http.begin("http://"+ ipPc +"/get/clearDB.php");

  //envia a requisição:
  int httpCode = http.GET();

  //encerra a comunicação:
  http.end();

  //retorna a resposta da requisição get:
  return String(httpCode);
}

/*
 * Captura a data atual e faz uma conversão para String, adicionando 0's onde necessário
 * 
 * return:
 *  uma string contendo a data atual
 *    ex: 30/06/2021
 */
String getDate(){
  String day_, month_;
  if(day()<10)
    day_ = "0" + String(day());
  else
    day_ = String(day());
  
  if(month()<10)
    month_ = "0" + String(month());
  else
    month_ = String(month());

  return day_ + "/" + month_ + "/" + year();
}

/*
 * Captura o horário atual e faz uma conversão para String, adicionando 0's onde necessário
 * 
 * return:
 *  uma string contendo a data atual
 *    ex: 17:50:32
 */
String getTime(){
  String hour_, minute_, second_;
  if(hour()<10)
    hour_ = "0" + String(hour());
  else
    hour_ = String(hour());
  if(minute()<10)
    minute_ = "0" + String(minute());
  else
    minute_ = String(minute());
  if(second()<10)
    second_ = "0" + String(second());
  else
    second_ = String(second());
  
  return hour_ + ":" + minute_ + ":" + second_;
}

/*
 * Faz o print dos dados no display LCD
 */
void printDataLCD(){
  lcd.clear();
  if(flagDisplay){
    //direciona o cursor pra posição [0,0] da matriz e faz o print:
    lcd.setCursor(0,0);
    lcd.print("V= " + String(tensao));
    
    //direciona o cursor pra posição [10,0] da matriz e faz o print:
    lcd.setCursor(10,0);
    lcd.print("I= " + String(corrente));
  
    //direciona o cursor pra posição [0,1] da matriz e faz o print:
    lcd.setCursor(0,1);
    lcd.print("P= " + String(potencia));
    
    //direciona o cursor pra posição [10,1] da matriz e faz o print:
    lcd.setCursor(10,1);
    lcd.print("E= " + String(energia));
  }
  else{
    //direciona o cursor pra posição [0,0] da matriz e faz o print:
    lcd.setCursor(0,0);
    lcd.print("TA= " + String(tempA));
    
    //direciona o cursor pra posição [0,3] da matriz e faz o print:
    lcd.setCursor(10,0);
    lcd.print("TP= " + String(tempP));
    
    //direciona o cursor pra posição [0,1] da matriz e faz o print:
    lcd.setCursor(0,1);
    lcd.print("RAD= " + String(rad));
  }
}

/*
 * Faz o print da data e hora no display LCD, além de mostrar se está conectado ao
 * wifi ou não.
 */
void printTimeLCD(){
  lcd.setCursor(0,2);
  lcd.print("Data: " + getDate());

  lcd.setCursor(0,3);
  lcd.print("Hora: " + getTime());

  if(WiFi.status() != WL_CONNECTED){
    lcd.setCursor(16,3);
    lcd.print("noWF");
  }
  else{
    lcd.setCursor(18,3);
    lcd.print("WF");
  }
}
/*
 * Procedimento que faz a sincronização com o provedor para captura da hora e data.
 * 
 * return:
 *    Retorna os segundos, desde 1970 até o instante foi chamada.
 *    Retorna 0 se não conseguir sincronizar.
 */
time_t getNtpTime(){
  IPAddress ntpServerIP; //enderço IP do servidor NTP

  //descartar todos os pacotes recebidos anteriormente
  while (Udp.parsePacket() > 0);
  // obtém um servidor aleatório do pool
  WiFi.hostByName(ntpServerName, ntpServerIP);

  //Envia uma solicitação NTP para o servidor obtido aleatóriamente:
  sendNTPpacket(ntpServerIP);

  //Permanece no loop por 1500 ms:
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      //ler o pacote no buffer Udp:
      Udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long secsSince1900;
      // Converte quatro bytes começando na localização 40 em um long int:
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  
  return 0; // Retorna 0 se não for possível obter a hora
}

/*
 * Envia uma solicitação NTP para o servidor de horário no endereço fornecido
 * 
 * Parâmetros:
 *    endereço IP do servidor.
 */
void sendNTPpacket(IPAddress &address){
  // Setta todos os bytes no buffer para 0:
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  
  // Inicializa os valores necessários para formar a solicitação NTP:
  packetBuffer[0] = 0b11100011;   // LI, versão, modo
  packetBuffer[1] = 0;     // estrado, ou tipo de clock
  packetBuffer[2] = 6;     // Intervalo de votação
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes de zero para Root Delay e Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  //Agora, envia um pacote solicitando um carimbo de data/hora:
  Udp.beginPacket(address, 123); // Os pedidos NTP são para a porta 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
