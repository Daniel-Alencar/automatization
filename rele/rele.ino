#include <IRremote.h>
#include <EEPROM.h>
#include "TimerOne.h"

#define BYTES_LENGTH 5
#define atraso 250
#define QUANTIDADE_DISPOSITIVOS 4

#define RECV_PIN 11

#define LED1_PIN 5
#define LED2_PIN 6
#define LED3_PIN 7

#define RELE1_PIN 3
#define RELE2_PIN 4

#define LED_PIN 8
#define BUTTON_PIN 2

bool LED1 = false, LED2 = false, LED3 = false;
bool RELE1 = false, RELE2 = false;

bool LED1_timer = false, LED2_timer = false, LED3_timer = false;
bool RELE1_timer = false, RELE2_timer = false;

bool programmingState = false;
long int controle[] = {
  // Os números 1,2,3,4,5,6,7,8,9,0
  0xFF00BF00, 0xFE01BF00, 0xFD02BF00, 0xFB04BF00, 0xFA05BF00, 
  0xF906BF00, 0xF708BF00, 0xF609BF00, 0xF50ABF00, 0xF20DBF00,
  // Setar temporizador
  0xE11EBF00,
  // Botão OK
  0xBE41BF00,
  // Setas direcionais (left, right, baixo, cima)
  0xBF40BF00, 0xBD42BF00, 0xBA45BF00, 0xE21DBF00
};
int lengthOfControle = sizeof(controle) / sizeof(controle[0]);
long int memory;
long int counter = 0;

void programming();
int findIndex(long int *items, long int item);
int convertIndexToAddress(int index);
void signalToRead();
long int valueFromIR();
void verifyButton();
int funcaoControle(int index);
void callback();

int funcaoControle(int index) {
  if(index >= 0 && index <= 9) {
    return 0;
  } else if (index == 10) {
    return 1;
  } else if (index == 11) {
    return 2;
  } else if (index >= 12 && index <= 15) {
    return 3;
  }
}

void setup() {
  Serial.begin(9600);
  
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(RELE1_PIN, OUTPUT);
  pinMode(RELE2_PIN, OUTPUT);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
  digitalWrite(RELE1_PIN, HIGH);
  digitalWrite(RELE2_PIN, HIGH);

  digitalWrite(LED_PIN, LOW);

  IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("Sensor IR habilitado!");

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), verifyButton, CHANGE);
}

void loop() {

  programming();
  signalToRead();

  delay(atraso);
}

void callback() {
  counter++;
  if(LED1_timer && counter == 2) {

    LED1 = !LED1;
    digitalWrite(LED1_PIN, LED1);
    
    counter = 0;
    LED1_timer = false;
  }
}

void verifyButton() {
  if(!digitalRead(BUTTON_PIN)) {
    programmingState = !programmingState;
    
    digitalWrite(LED_PIN, programmingState);
    delay(atraso);
  }
}

long int valueFromIR() {
  if(IrReceiver.decode() && IrReceiver.decodedIRData.decodedRawData != 0) {
    Serial.println("Algum codigo foi recebido do controle remoto!");
    long int code = IrReceiver.decodedIRData.decodedRawData;
    IrReceiver.resume();

    return code;
  }
  return 0;
}

void signalToRead() {
  Serial.println("SIGNAL");

  if(IrReceiver.decode()) {
    Serial.println("Algum codigo foi recebido do controle remoto!");
    long int code = IrReceiver.decodedIRData.decodedRawData;
    IrReceiver.resume();

    Serial.print("Código: "); Serial.println(code, HEX);
  
    EEPROM.get(convertIndexToAddress(0), memory);
    if(code == controle[0] || code == memory) {
      LED1 = !LED1;
      digitalWrite(LED1_PIN, LED1);
      delay(atraso);
    }
    EEPROM.get(convertIndexToAddress(1), memory);
    if(code == controle[1] || code == memory) {
      LED2 = !LED2;
      digitalWrite(LED2_PIN, LED2);
      delay(atraso);
    }
    EEPROM.get(convertIndexToAddress(2), memory);
    if(code == controle[2] || code == memory) {
      LED3 = !LED3;
      digitalWrite(LED3_PIN, LED3);
      delay(atraso);
    }
    
    EEPROM.get(convertIndexToAddress(3), memory);
    if(code == controle[3] || code == memory) {
      RELE1 = !RELE1;
      digitalWrite(RELE1_PIN, RELE1);
      delay(atraso);
    }
    EEPROM.get(convertIndexToAddress(4), memory);
    if(code == controle[4] || code == memory) {
      RELE2 = !RELE2;
      digitalWrite(RELE2_PIN, !RELE2);
      delay(atraso);
    }

    EEPROM.get(convertIndexToAddress(10), memory);
    if(code == controle[10] || code == memory) {
      long int newCode;
      int index;
      int dispositivo;
      int segundos;

      // Selecionar dispositivo
      do {
        newCode = valueFromIR();
        index = findIndex(controle, newCode);

        if(index >= 0 && index < QUANTIDADE_DISPOSITIVOS) {
          Serial.println(String("Dispositivo ") + String(index + 1) + String(" selecionado!"));
          dispositivo = index + 1;
          break;
        } else {
          Serial.println("Selecione um dispositivo válido!");
        }
      } while(1);

      // Selecionar quantidade de segundos
      do {
        newCode = valueFromIR();
        index = findIndex(controle, newCode);

        if(funcaoControle(index) == 0) {
          Serial.println(String(index + 1) + String(" segundos selecionados!"));
          break;
        } else {
          Serial.println("Selecione uma quantidade válida!");
        }
      } while(1);

      // Apertar "OK"
      do {
        newCode = valueFromIR();
        index = findIndex(controle, newCode);

        if(funcaoControle(index) == 2) {
          Serial.println(String("OK"));
          break;
        } else {
          Serial.println("Selecione OK!");
        }
      } while(1);

      setTimer(dispositivo, segundos);
    }
  }
}

void setTimer(int dispositivo, int segundos) {
  Timer1.initialize(500000);
  Timer1.attachInterrupt(callback);
  LED1_timer = true;
  Serial.println("Timer feito!");
}

int convertIndexToAddress(int index) {
  return index * BYTES_LENGTH;
}

int findIndex(long int *items, long int item) {
  int i;
  for(i = 0; i < lengthOfControle; i++) {
    if(controle[i] == item) {
      return i;
    }
  }
  return -1;
}

void programming() {  
  Serial.println("PROGRAMMING");
  
  while(programmingState) {
    int index;
      
    if(IrReceiver.decode() && IrReceiver.decodedIRData.decodedRawData != 0) {
      Serial.println("Código 1 recebido!");
      long int actualCode = IrReceiver.decodedIRData.decodedRawData;
      IrReceiver.resume();
      
      Serial.print("Código: "); Serial.println(actualCode, HEX);
      Serial.println();
      
      index = findIndex(controle, actualCode);
      
      if(index >= 0) {
        Serial.println("Código está na programação atual!");

        while(IrReceiver.decode() == false || IrReceiver.decodedIRData.decodedRawData == 0) {
          IrReceiver.resume();
        }

        Serial.println("Código 2 recebido!");
        long int code = IrReceiver.decodedIRData.decodedRawData;
        IrReceiver.resume();
        
        Serial.print("Código: "); Serial.println(code, HEX);
        Serial.println();

        if(code != actualCode) {
          Serial.println("Gravando...");
          EEPROM.put(convertIndexToAddress(index), code);
        } else {
          Serial.println("Esse código é o mesmo que o anterior...");
        }
        
      }
      
    }
    IrReceiver.resume();
  }
  
}
