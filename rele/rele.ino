#include <IRremote.h>
#include <EEPROM.h>
#include "TimerOne.h"
#include <string.h>

#define BYTES_LENGTH 5
#define DEFAULT_DELAY 250
#define DEVICES_QUANTITY 5

#define RECV_PIN 11

#define LED_PIN 8
#define BUTTON_PIN 2

#define NUMBER_BUTTON 0
#define TIMER_BUTTON 1
#define OK_BUTTON 2
#define ARROW_BUTTON 3

// A multiplicação deve ser 1000000 (que representa 1 segundo)
#define DEFAULT_MICROSECONDS_FOR_TIME 500000
#define SCALE_TO_CONVERT_SECONDS_TO_COUNTER 2


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
int device_which_timer_is_activated = (0 - 1);

int delayEstado = 1000;
long int lastActivation = 0;

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
  0xBF40BF00, 0xBD42BF00, 0xBA45BF00, 0xE21DBF00,
  // Ativar modo soneca
  0xDF20BF00
};
int lengthOfControle = sizeof(controle) / sizeof(controle[0]);
long int memoryValue;

void signalToRead();
void setTimer();
void modoSoneca();
void timerEndEvent();
void buttonActivationEvent();
void programming();

long int valueFromIRWithWhile();
long int valueFromIRWithoutWhile();
int findIndexOfCodeIntoControle(long int item);
int funcaoControle(int index);
int convertTimeInSeconds(String time_string);
int convertIndexToAddress(int index);
int convertSecondsToCounter(int seconds);

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

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonActivationEvent, CHANGE);
}

void loop() {
  programming();
  signalToRead();
  delay(DEFAULT_DELAY);
}

// PRIMARY FUNCTIONS

void signalToRead() {
  Serial.println("SIGNAL");

  long int code = valueFromIRWithoutWhile();
  bool theCodeisNotFounded = true;

  if(code != 0) {
    int i;
    for(i = 1; i < 10 && theCodeisNotFounded; i++) {
      EEPROM.get(convertIndexToAddress(i), memoryValue);
      if(code == controle[i] || code == memoryValue) {
        theCodeisNotFounded = false;

        devices[i - 1] = !devices[i - 1];
        digitalWrite(devices_pin[i - 1], devices[i - 1]);

        delay(DEFAULT_DELAY);
      }
    }

    if(theCodeisNotFounded) {
      EEPROM.get(convertIndexToAddress(10), memoryValue);
      if(code == controle[10] || code == memoryValue) {
        theCodeisNotFounded = false;
        setTimer();

        delay(DEFAULT_DELAY);
      }

      EEPROM.get(convertIndexToAddress(16), memoryValue);
      if(code == controle[16] || code == memoryValue) {
        theCodeisNotFounded = false;
        modoSoneca();

        delay(DEFAULT_DELAY);
      }
    }
  }
}

void setTimer() {
  long int code;
  int index;
  int device;

  int number_time;
  String time_string = "000000";
  int lengthOfTime = sizeof(time_string) / sizeof(time_string[0]);
  int index_control = 0;

  // Selecionar dispositivo
  do {
    code = valueFromIRWithWhile();
    index = findIndexOfCodeIntoControle(code);

    if(index > 0 && index <= DEVICES_QUANTITY) {
      Serial.println(String("Dispositivo selecionado!"));
      device = index - 1;
      break;
    } else {
      Serial.println("Selecione um dispositivo válido!");
    }
  } while(1);

  // Selecionar tempo do timer
  Serial.println("00:00:00");
  do {
    code = valueFromIRWithWhile();
    index = findIndexOfCodeIntoControle(code);

    if(funcaoControle(index) == OK_BUTTON) {
      break;
    }

    if(index_control < lengthOfTime) {
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
      Serial.println("Aperte o botão OK!");
    }

  } while(1);

  device_which_timer_is_activated = device;
  devices_timer_activated[device] = true;

  int seconds = convertTimeInSeconds(time_string);

  time_in_seconds_for_devices[device] = seconds;
  counters_for_devices[device] = 0;

  Timer1.initialize(DEFAULT_MICROSECONDS_FOR_TIME);
  Timer1.attachInterrupt(timerEndEvent);

  Serial.println("Modo timer ativado!");
}

void modoSoneca() {
  device_which_timer_is_activated = 0;
  int seconds = 1200;

  devices_timer_activated[device_which_timer_is_activated] = true;
  counters_for_devices[device_which_timer_is_activated] = 0;
  time_in_seconds_for_devices[device_which_timer_is_activated] = seconds;

  Timer1.initialize(DEFAULT_MICROSECONDS_FOR_TIME);
  Timer1.attachInterrupt(timerEndEvent);

  Serial.println("Modo soneca ativado!");
}

void timerEndEvent() {
  if(devices_timer_activated[device_which_timer_is_activated]) {

    int goalCounter = convertSecondsToCounter(
      time_in_seconds_for_devices[device_which_timer_is_activated]
    );

    if(counters_for_devices[device_which_timer_is_activated] == goalCounter) {

      devices[device_which_timer_is_activated] = !devices[device_which_timer_is_activated];
      digitalWrite(devices_pin[device_which_timer_is_activated], devices[device_which_timer_is_activated]);
      
      devices_timer_activated[device_which_timer_is_activated] = false;
      time_in_seconds_for_devices[device_which_timer_is_activated] = 0;
      counters_for_devices[device_which_timer_is_activated] = 0;
      device_which_timer_is_activated = (0 - 1);

      Timer1.detachInterrupt();
    }
  }
  counters_for_devices[device_which_timer_is_activated]++;
}

void buttonActivationEvent() {
  if((millis() - lastActivation) > delayEstado) {
    lastActivation = millis();

    programmingState = !programmingState;
    digitalWrite(LED_PIN, programmingState);
    delay(DEFAULT_DELAY);
  }
}

void programming() {  
  Serial.println("PROGRAMMING");
  
  while(programmingState) {
    int index;
      
    long int actualCode = valueFromIRWithWhile();
    Serial.println("Código 1 recebido!");

    index = findIndexOfCodeIntoControle(actualCode);
    if(index >= 0) {
      Serial.println("Código está no controle primário!");

      long int code = valueFromIRWithWhile();
      Serial.println("Código 2 recebido!");

      if(code != actualCode) {
        Serial.println("Gravando...");
        EEPROM.put(convertIndexToAddress(index), code);
      } else {
        Serial.println("Esse código é o mesmo que o anterior...");
      }
    }
  }
}

// AUXILIAR FUNCTIONS

long int valueFromIRWithWhile() {
  while(IrReceiver.decode() == false || IrReceiver.decodedIRData.decodedRawData == 0) {
    IrReceiver.resume();
  }
  long int code = IrReceiver.decodedIRData.decodedRawData;
  Serial.print("Código recebido:");
  Serial.println(code, HEX);

  IrReceiver.resume();

  return code;
}

long int valueFromIRWithoutWhile() {
  long int code;
  if(IrReceiver.decode()) {
    code = IrReceiver.decodedIRData.decodedRawData;

    Serial.print("Código recebido:");
    Serial.println(code, HEX);

    IrReceiver.resume();
    return code;
  }
  return 0;
}

int findIndexOfCodeIntoControle(long int item) {
  int i;
  for(i = 0; i < lengthOfControle; i++) {
    if(controle[i] == item) {
      return i;
    }
  }
  return (0 - 1);
}

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

int convertTimeInSeconds(String time_string) {
  String hour = time_string.substring(0, 2);
  String minute = time_string.substring(2, 4);
  String second = time_string.substring(4, 6);

  int hour_int = atoi(hour.c_str());
  int minute_int = atoi(minute.c_str());
  int second_int = atoi(second.c_str());

  int time_in_seconds = hour_int * 60 * 60 + minute_int * 60 + second_int;
  return time_in_seconds;
}

int convertIndexToAddress(int index) {
  return index * BYTES_LENGTH;
}

int convertSecondsToCounter(int seconds) {
  return SCALE_TO_CONVERT_SECONDS_TO_COUNTER * seconds;
}
