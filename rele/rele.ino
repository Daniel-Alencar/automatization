#include <IRremote.h>
#include <EEPROM.h>
#include "TimerOne.h"

#define BYTES_LENGTH 5
#define ATRASO_PADRAO 250
#define QUANTIDADE_DISPOSITIVOS 5

#define RECV_PIN 11

#define LED_PIN 8
#define BUTTON_PIN 2

#define NUMBER_BUTTON 0
#define TIMER_BUTTON 1
#define OK_BUTTON 2
#define ARROW_BUTTON 3

int devices_pin[] = { 5, 6, 7, 3, 4 };
bool devices[] = {
  false, false, false, false, false
};
bool devices_timer_activated[] = {
  false, false, false, false, false
};
int counters_for_devices[] = {
  0, 0, 0, 0, 0
};
int time_in_seconds_for_devices[] = {
  0, 0, 0, 0, 0
};
int device_witch_timer_is_activated = -1;


bool programmingState = false;
long int controle[] = {
  // Os números 0,1,2,3,4,5,6,7,8,9
  0xF20DBF00, 0xFF00BF00, 0xFE01BF00, 0xFD02BF00, 0xFB04BF00, 
  0xFA05BF00, 0xF906BF00, 0xF708BF00, 0xF609BF00, 0xF50ABF00,
  // Setar temporizador
  0xE11EBF00,
  // Botão OK
  0xBE41BF00,
  // Setas direcionais (left, right, baixo, cima)
  0xBF40BF00, 0xBD42BF00, 0xBA45BF00, 0xE21DBF00
};
int lengthOfControle = sizeof(controle) / sizeof(controle[0]);
long int memory;

int funcaoControle(int index) {
  if(index >= 0 && index <= 9) {
    return NUMBER_BUTTON;
  } else if (index == 10) {
    return TIMER_BUTTON;
  } else if (index == 11) {
    return OK_BUTTON;
  } else if (index >= 12 && index <= 15) {
    return ARROW_BUTTON;
  }
}

void programming();
int findIndexOfCodeIntoControle(long int item);
int convertIndexToAddress(int index);
void signalToRead();
long int valueFromIR();
void verifyButton();
void callback();
int convertSecondsToCounter(int seconds);
int convertTimeInSeconds(String time_string);

void setup() {
  Serial.begin(9600);
  
  pinMode(devices_pin[0], OUTPUT);
  pinMode(devices_pin[1], OUTPUT);
  pinMode(devices_pin[2], OUTPUT);
  pinMode(devices_pin[3], OUTPUT);
  pinMode(devices_pin[4], OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  digitalWrite(devices_pin[0], LOW);
  digitalWrite(devices_pin[1], LOW);
  digitalWrite(devices_pin[2], LOW);
  digitalWrite(devices_pin[3], HIGH);
  digitalWrite(devices_pin[4], HIGH);
  digitalWrite(LED_PIN, LOW);

  IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("Sensor IR habilitado!");

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), verifyButton, CHANGE);
}

void loop() {
  programming();
  signalToRead();

  delay(ATRASO_PADRAO);
}

int convertSecondsToCounter(int seconds) {
  return 2 * seconds;
}

void callback() {
  counters_for_devices[device_witch_timer_is_activated]++;
  if(
      devices_timer_activated[device_witch_timer_is_activated] && 
      counters_for_devices[device_witch_timer_is_activated] == convertSecondsToCounter(time_in_seconds_for_devices[device_witch_timer_is_activated])
    ) {

    devices[device_witch_timer_is_activated] = !devices[device_witch_timer_is_activated];
    digitalWrite(devices_pin[device_witch_timer_is_activated], devices[device_witch_timer_is_activated]);
    
    counters_for_devices[device_witch_timer_is_activated] = 0;
    devices_timer_activated[device_witch_timer_is_activated] = false;
    Timer1.detachInterrupt();
  }
}

void verifyButton() {
  if(!digitalRead(BUTTON_PIN)) {
    programmingState = !programmingState;
    
    digitalWrite(LED_PIN, programmingState);
    delay(ATRASO_PADRAO);
  }
}

long int valueFromIR() {
  while(IrReceiver.decode() == false || IrReceiver.decodedIRData.decodedRawData == 0) {
    IrReceiver.resume();
  }
  Serial.println("Algum código foi recebido do controle remoto!");
  long int code = IrReceiver.decodedIRData.decodedRawData;
  IrReceiver.resume();

  return code;
}

void signalToRead() {
  Serial.println("SIGNAL");

  if(IrReceiver.decode()) {
    Serial.println("Algum codigo foi recebido do controle remoto!");
    long int code = IrReceiver.decodedIRData.decodedRawData;
    IrReceiver.resume();

    Serial.print("Código: "); Serial.println(code, HEX);

    int i;
    for(i = 1; i < 10; i++) {
      EEPROM.get(convertIndexToAddress(i), memory);
      if(code == controle[i] || code == memory) {
        devices[i - 1] = !devices[i - 1];

        digitalWrite(devices_pin[i - 1], devices[i - 1]);
        delay(ATRASO_PADRAO);
      }
    }
  
    EEPROM.get(convertIndexToAddress(10), memory);
    if(code == controle[10] || code == memory) {
      setTimer();
    }
  }
}

void setTimer() {
  long int newCode;
  int index;
  int dispositivo;

  int number_time;
  String time_string = "000000";
  int lengthOfTime = sizeof(time_string) / sizeof(time_string[0]);
  int index_control = 0;

  // Selecionar dispositivo
  do {
    newCode = valueFromIR();
    index = findIndexOfCodeIntoControle(newCode);

    if(index > 0 && index <= QUANTIDADE_DISPOSITIVOS) {
      Serial.println(String("Dispositivo ") + String(index) + String(" selecionado!"));
      dispositivo = index - 1;
      break;
    } else {
      Serial.println("Selecione um dispositivo válido!");
    }
  } while(1);

  // Selecionar o tempo do timer
  Serial.println("00:00:00");
  do {
    newCode = valueFromIR();
    index = findIndexOfCodeIntoControle(newCode);

    if(funcaoControle(index) == OK_BUTTON) {
      break;
    }

    if(index_control < 6) {

      if(funcaoControle(index) == NUMBER_BUTTON) {
        number_time = index;

        int i;
        for(i = 1; i < lengthOfTime; i++) {
          time_string[i - 1] = time_string[i];
        }
        time_string[i - 1] = (number_time + '0');
        char time_string_formatted[] = {
          time_string[0], time_string[1], ':', 
          time_string[2], time_string[3], ':', 
          time_string[4], time_string[5], '\0'
        };
        Serial.println(time_string_formatted);

        index_control++;

      } else {
        Serial.println("Selecione uma quantidade válida!");
      }

    } else {
      Serial.println("Aperte o botão OK");
    }

  } while(1);

  device_witch_timer_is_activated = dispositivo;
  devices_timer_activated[dispositivo] = true;
  time_in_seconds_for_devices[dispositivo] = convertTimeInSeconds(time_string);

  Timer1.initialize(500000);
  Timer1.attachInterrupt(callback);

  Serial.println("Timer feito!");
}

int convertTimeInSeconds(String time_string) {
  char hour[] = { time_string[0], time_string[1] };
  char minute[] = { time_string[2], time_string[3] };
  char second[] = { time_string[4], time_string[5] };

  int hour_int = atoi(hour);
  int minute_int = atoi(minute);
  int second_int = atoi(second);

  int time_in_seconds = hour_int * 3600 + minute_int * 60 + second_int;
  return time_in_seconds;
}

int convertIndexToAddress(int index) {
  return index * BYTES_LENGTH;
}

int findIndexOfCodeIntoControle(long int item) {
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
      
      index = findIndexOfCodeIntoControle(actualCode);
      
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
