#include <IRremote.h>
#include <EEPROM.h>

#define atraso 250

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

bool programmingState = false;
long int controle[] = {0xFF00BF00, 0xFE01BF00, 0xFD02BF00, 0xFB04BF00, 0xFA05BF00};
int lengthOfControle = sizeof(controle)/sizeof(controle[0]);
long int memory;

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
}

void loop() {

  programming();
  verifySignal();

  delay(1000);
}

void verifyButton() {
  if(!digitalRead(BUTTON_PIN)) {
    programmingState = !programmingState;
    
    digitalWrite(LED_PIN, programmingState);
    delay(atraso);
  }
}

void verifySignal() {
  Serial.println("SIGNAL");

  if(IrReceiver.decode()) {
    Serial.println("Algum codigo foi recebido do controle remoto!");
    long int code = IrReceiver.decodedIRData.decodedRawData;
    IrReceiver.resume();

    Serial.print("Código: "); Serial.println(code, HEX);
  
    if(code == controle[0] || code == EEPROM.get(convertIndexToAddress(0), memory)) {
      LED1 = !LED1;
      digitalWrite(LED1_PIN, LED1);
      delay(atraso);
    }
    if(code == controle[1] || code == EEPROM.get(convertIndexToAddress(1), memory)) {
      LED2 = !LED2;
      digitalWrite(LED2_PIN, LED2);
      delay(atraso);
    }
    if(code == controle[2] || code == EEPROM.get(convertIndexToAddress(2), memory)) {
      LED3 = !LED3;
      digitalWrite(LED3_PIN, LED3);
      delay(atraso);
    }
    
    if(code == controle[3] || code == EEPROM.get(convertIndexToAddress(3), memory)) {
      RELE1 = !RELE1;
      digitalWrite(RELE1_PIN, RELE1);
      delay(atraso);
    }
    if(code == controle[4] || code == EEPROM.get(convertIndexToAddress(4), memory)) {
      RELE2 = !RELE2;
      digitalWrite(RELE2_PIN, !RELE2);
      delay(atraso);
    }
    
  }
}

int convertIndexToAddress(int index) {
  return index * 4;
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
  Serial.println("PROGRAMMING\n");
  
  while(verifyButton(), programmingState) {
    int index = 0;
      
    if(IrReceiver.decode() && IrReceiver.decodedIRData.decodedRawData != 0) {
      Serial.println("Código 1 recebido!");
      long int actualCode = IrReceiver.decodedIRData.decodedRawData;
      IrReceiver.resume();
      
      Serial.print("Código: "); Serial.println(actualCode, HEX);
      Serial.println();
      
      index = findIndex(controle, actualCode);
      
      if(index >= 0) {
        Serial.println("Código está na programação atual!");

        while(IrReceiver.decode() == false || IrReceiver.decodedIRData.decodedRawData == 0) { IrReceiver.resume(); }

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
