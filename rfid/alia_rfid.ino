//
// ALIA - RFid
// Versao 1.0
// TRT13 - Por Ericson Benjamim
//
#include <Wire.h>
#include "SSD1306Wire.h"
#include <SoftwareSerial.h>
#include "DYPlayerArduino.h"

#define rfidTx D3
#define rfidRx D2

SoftwareSerial rfidSerial(rfidRx, rfidTx);

SSD1306Wire display(0x3c, D6, D5);

DY::Player player;

// leia o comando várias vezes
unsigned char readMulti[10] = {0XAA,0X00,0X27,0X00,0X03,0X22,0XFF,0XFF,0X4A,0XDD};
unsigned char stopRead[7] = {0XAA,0X00,0X28,0X00,0X00,0X28,0XDD}; // AA 00 28 00 00 28 DD 
unsigned int timeSec = 0;
unsigned int timemin = 0;
unsigned int dataAdd = 0;
unsigned int incomedate = 0;
unsigned int parState = 0;
unsigned int codeState = 0;

int inicio = 1;
int ledCodigoOk = D0;

unsigned long millisLed = millis();
unsigned long millisInicio = millis();

String codigo = "";

int audioInicio = 1;

String codigoSetor[] = {
  // Adicionar numero do audio correspondente na mesma posicao no array audioSetor
  "E2801191A52000162346","E2801191A52000162354","E2801191A52000162355","E2801191A52000162356", // SETIC
  "E2801191A52000162347", // SEGEPE
  "E2801191A52000162348", // ACS
  "E2801191A52000162351", // CSAUDE
  "E2801191A52008372A668", // ElevadorBlocoA1
  "E2801191A52000162349", // ElevadorBlocoB1
  "E2801191A52000162353", // ElevadorPanoramico1
  "E2801191A52000162350","E2801191A52000162357","E2801191A52000162358","E2801191A52000162359", // ASPROS
};

int audioSetor[] = {
  2, 2, 2, 2, // SETIC
  3, // SEGEPE
  4, // ACS
  5, // CSAUDE
  6, // ElevadorBlocoA1
  7, // ElevadorBlocoB1
  8, // ElevadorPanoramico1
  9, 9 ,9 ,9 // ASPROS
};

String nomeSetor[] = {
  "SETIC", "SEGEPE", "ACS", "CSAUDE", "ElevadorBlocoA1", "ElevadorBlocoB1", "ElevadorPanoramico1", "ASPROS"
};

int audioSetorAnterior = -1;
unsigned long millisLeituraAnterior = millis();

void setup() {
  pinMode(ledCodigoOk, OUTPUT);
  
  rfidSerial.begin(115200);
  
  rfidSerial.println("Hello world.");
  
  rfidSerial.write(readMulti, 10);

  display.init();
  display.flipScreenVertically();
  
  player.begin();
  player.setVolume(100);
}

void loop() {
  if (inicio == 1) {
    if ((millis() - millisInicio) > 500) {
      inicio = 0;

      display.clear();
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_16);
      display.drawString(63, 10, "ALIA - TRT13");
      display.display();

      player.playSpecified(audioInicio);
    }
  }

  // Comandos de leitura cíclica ocorrem após algum intervalo de tempo
  timeSec ++ ;
  
  if (timeSec >= 50000) {
    timemin ++;
	
    timeSec = 0;
	
    if (timemin >= 20) {
      timemin = 0;

      // Enviar comando de leitura cíclica
      rfidSerial.write(readMulti, 10);
    }
  }
 
  if (rfidSerial.available() > 0) // A porta serial recebe dados
  {
    /*
    Recebi uma resposta, o seguinte é um exemplo de um cartão de leitura
    AA 02 22 00 11 C7 30 00 E2 80 68 90 00 00 50 0E 88 C6 A4 A7 11 9B 29 DD 

   	AA: cabeçalho do quadro
    02: Código de comando
    22: Parâmetro de comando
    00 11: comprimento dos dados da instrução, 17 bytes
    C7: intensidade do sinal RSSI
    30 00: código do PC da etiqueta: registro de informações relacionadas à fábrica da etiqueta
    E2 80 68 90 00 00 50 0E 88 C6 A4 A7: código EPC
    E2 80 11 91 A5 02 00 00 00 16 23 46
    E2 80 11 91 A5 02 00 00 00 16 23 47
    11 9B: Verificação CRC
    29: Verificar
    DD: fim do quadro
	  */
	
    incomedate = rfidSerial.read(); // Obtenha os dados recebidos pela porta serial
	
    // Determine se é o código de instrução correspondente
    if ((incomedate == 0x02) & (parState == 0))
    {
      parState = 1;
    }
    // Determine se é o parâmetro de instrução correspondente
    else if ((parState == 1) & (incomedate == 0x22) & (codeState == 0)) {  
        codeState = 1;
        dataAdd = 3;
    }
    else if (codeState == 1) {
      dataAdd ++;
	  
      // Obter RSSI
      if (dataAdd == 6)
      {
        rfidSerial.print("RSSI:"); 
        rfidSerial.println(incomedate, HEX); 
        }
       // Obter código do PC
       else if ((dataAdd == 7) | (dataAdd == 8)) {
        if (dataAdd == 7) {
          rfidSerial.print("PC:"); 
          rfidSerial.print(incomedate, HEX);
        }
        else {
           rfidSerial.println(incomedate, HEX);
        }
       }
	     // Obtenha EPC, se precisar processar EPC, pode fazê-lo aqui Obtenha EPC, se precisar processar EPC, pode fazê-lo aqui
       else if ((dataAdd >= 9) & (dataAdd <= 20)) {
        if (dataAdd == 9) {
          rfidSerial.print("EPC:"); 
        }        

        codigo += String(incomedate, HEX);

        rfidSerial.print(incomedate, HEX);

        if (dataAdd >= 20) {
          codigo.toUpperCase();

          display.clear();

          display.setTextAlignment(TEXT_ALIGN_CENTER);

          display.setFont(ArialMT_Plain_10);

          display.drawString(63, 10, codigo);

          if (retornaAudioSetor(codigo) == audioSetorAnterior && millis() - millisLeituraAnterior < 12000) {
            codigo = "";

            digitalWrite(ledCodigoOk, HIGH);

            rfidSerial.write(stopRead, 7);
          }
          
          if (retornaAudioSetor(codigo) > 0) {
            audioSetorAnterior = retornaAudioSetor(codigo);
            codigo = "";
            millisLeituraAnterior = millis();

            rfidSerial.write(stopRead, 7);

            player.playSpecified(audioSetorAnterior);
            
            digitalWrite(ledCodigoOk, HIGH);

            millisLed = millis();

            display.drawString(63, 45, "VOCE ESTA PROXIMO A " + nomeSetor[audioSetorAnterior - 2]);
            display.display();
          }

          codigo = "";
        }
       }
       // Estouro de localização, re-receber
       else if (dataAdd >= 21) {
         rfidSerial.println(" "); 
		
         dataAdd= 0;
         parState = 0;
         codeState = 0;
        }
    }
     else {
       dataAdd= 0;
       parState = 0;
       codeState = 0;
    }
  }

  if (digitalRead(ledCodigoOk) == HIGH) {
    if ((millis() - millisLed) > 8000) {
      digitalWrite(ledCodigoOk, LOW);

      rfidSerial.write(readMulti, 10);
    }
  }
}

int retornaAudioSetor(String pCodigo) {
  int resultado = 0;

  for (int i = 0; i < sizeof(codigoSetor) / sizeof(codigoSetor[0]); i++) {
    if (pCodigo == codigoSetor[i]) {
      resultado = audioSetor[i];

      break;
    }
  }

  return resultado;
}