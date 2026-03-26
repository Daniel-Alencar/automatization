#include <IRremote.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <string.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define EEPROM_SIZE 512
#define BYTES_LENGTH 4
#define DEFAULT_DELAY 250
#define DEVICES_QUANTITY 5

#define RECV_PIN 15
#define LED_PIN 2       
#define BUTTON_PIN 13   

#define NUMBER_BUTTON 0
#define TIMER_BUTTON 1
#define OK_BUTTON 2
#define ARROW_BUTTON 3

#define SCALE_TO_CONVERT_SECONDS_TO_COUNTER 2
Ticker timerDispositivos; 

int actualNumberOfInterruptions = 0;

#define endereco  0x27
#define colunas   16
#define linhas    2

LiquidCrystal_I2C lcd(endereco, colunas, linhas);
bool update_LCD = false;
bool isBackLight = true;

int devices_pin[] = { 14, 25, 26, 27, 32 };
bool devices[] = {
  false, false, false, false, false
};
bool devices_timer_activated[] = {
  false, false, false, false, false
};
int actual_seconds_for_devices[] = {
  0, 0, 0, 0, 0
};
int goal_seconds_for_devices[] = {
  0, 0, 0, 0, 0
};
int device_which_timer_is_activated = (0 - 1);

int delayEstado = 1000;
uint32_t lastActivation = 0; // Trocado para uint32_t (millis retorna unsigned long)

bool programmingState = false;

// TROCADO: long int para uint32_t
uint32_t controle[] = {
  0xF20DBF00, 0xFF00BF00, 0xFE01BF00, 0xFD02BF00, 0xFB04BF00, 
  0xFA05BF00, 0xF906BF00, 0xF708BF00, 0xF609BF00, 0xF50ABF00,
  0xE11EBF00,
  0xBE41BF00,
  0xBF40BF00, 0xBD42BF00, 0xBA45BF00, 0xE21DBF00,
  0xDF20BF00,
  0xFC03BF00
};
int lengthOfControle = sizeof(controle) / sizeof(controle[0]);
uint32_t memoryValue; // TROCADO

void signalToRead();
void setTimer();
void modoSoneca();
void timerEndEvent();
void IRAM_ATTR buttonActivationEvent();
void programming();

uint32_t valueFromIRWithWhile(); // TROCADO
uint32_t valueFromIRWithoutWhile(); // TROCADO
int findIndexOfCodeIntoControle(uint32_t item); // TROCADO
int funcaoControle(int index);
int convertTimeInSeconds(String time_string);
String convertSecondsInTime(int seconds);
int convertIndexToAddress(int index);
int convertSecondsToCounter(int seconds);

void setup() {
  Serial.begin(115200); 
  
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Falha ao inicializar a EEPROM");
    delay(1000);
  }
  
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

  Wire.begin();
  lcd.init();
  lcd.backlight();
}

void loop() {
  if(update_LCD) {
    if(actual_seconds_for_devices[device_which_timer_is_activated] >= 0) {

      String time = convertSecondsInTime(
        actual_seconds_for_devices[device_which_timer_is_activated]
      );

      lcd.clear();
      lcd.print(time);
      lcd.print(String(" ") + String("Device") + String(device_which_timer_is_activated + 1));
      lcd.setCursor(0, 1);
      lcd.print("Receiving...");
    }
    update_LCD = false;
  }

  programming();
  signalToRead();
  delay(DEFAULT_DELAY);
}

void signalToRead() {
  Serial.println("RECEBENDO SINAL");

  if(programmingState == false && device_which_timer_is_activated < 0) {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Receiving...");
  }

  uint32_t code = valueFromIRWithoutWhile(); // TROCADO
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

      EEPROM.get(convertIndexToAddress(17), memoryValue);
      if(code == controle[17] || code == memoryValue) {
        theCodeisNotFounded = false;
        if(isBackLight) {
          lcd.noBacklight();
          isBackLight = false;
        } else {
          lcd.backlight();
          isBackLight = true;
        }

        delay(DEFAULT_DELAY);
      }
    }
  }
}

void setTimer() {
  uint32_t code; // TROCADO
  int index;
  int device;

  int number_time;
  String time_string = "000000";
  int lengthOfTime = sizeof(time_string) / sizeof(time_string[0]);
  int index_control = 0;

  do {
    Serial.println("Selecione um dispositivo!");
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Choose a Device!");
    
    code = valueFromIRWithWhile();
    index = findIndexOfCodeIntoControle(code);

    if(index > 0 && index <= DEVICES_QUANTITY) {
      device = index - 1;

      Serial.println(String("Dispositivo selecionado!"));
      lcd.clear();
      lcd.print(String("Device") + String(device + 1));
      
      break;
    } else {
      Serial.println("Selecione um dispositivo válido!");
    }
  } while(1);

  lcd.print(String(" ") + String("00:00:00"));
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

        lcd.clear();
        lcd.print(String("Device") + String(device + 1) + String(" "));
        lcd.print(time_string_formatted);

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

  goal_seconds_for_devices[device] = seconds;
  actual_seconds_for_devices[device] = seconds;

  actualNumberOfInterruptions = 0;

  timerDispositivos.attach(0.5, timerEndEvent); 

  Serial.println("Modo timer ativado!");
}

void modoSoneca() {

  device_which_timer_is_activated = 0;
  devices_timer_activated[device_which_timer_is_activated] = true;

  int seconds = 12;

  goal_seconds_for_devices[device_which_timer_is_activated] = seconds;
  actual_seconds_for_devices[device_which_timer_is_activated] = seconds;

  actualNumberOfInterruptions = 0;

  timerDispositivos.attach(0.5, timerEndEvent); 

  Serial.println("Modo soneca ativado!");
}

void timerEndEvent() {
  if(devices_timer_activated[device_which_timer_is_activated]) {

    if(actual_seconds_for_devices[device_which_timer_is_activated] == 0) {

      devices[device_which_timer_is_activated] = !devices[device_which_timer_is_activated];
      digitalWrite(devices_pin[device_which_timer_is_activated], devices[device_which_timer_is_activated]);
      
      devices_timer_activated[device_which_timer_is_activated] = false;
      goal_seconds_for_devices[device_which_timer_is_activated] = 0;
      actual_seconds_for_devices[device_which_timer_is_activated] = 0;
      device_which_timer_is_activated = (0 - 1);

      timerDispositivos.detach(); 
    }
  }

  actualNumberOfInterruptions++;

  if(actualNumberOfInterruptions % SCALE_TO_CONVERT_SECONDS_TO_COUNTER == 0) {
    actual_seconds_for_devices[device_which_timer_is_activated]--;
    update_LCD = true;
  }
}

void IRAM_ATTR buttonActivationEvent() {
  if((millis() - lastActivation) > delayEstado) {
    lastActivation = millis();

    programmingState = !programmingState;
    digitalWrite(LED_PIN, programmingState);
  }
}

void programming() {  
  if(programmingState) {
    Serial.println("MODO PROGRAMAÇÃO");
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Programming mode!");
    delay(DEFAULT_DELAY * 5);
  }
  
  while(programmingState) {
    int index;

    lcd.clear();
    lcd.print("Code1:");
      
    uint32_t actualCode = valueFromIRWithWhile(); // TROCADO

    index = findIndexOfCodeIntoControle(actualCode);
    if(index >= 0) {
      lcd.print(String(actualCode));

      lcd.setCursor(0, 1);
      lcd.print("Code2:");

      uint32_t code = valueFromIRWithWhile(); // TROCADO

      if(code != actualCode) {
        lcd.print(String(code));
        EEPROM.put(convertIndexToAddress(index), code);
        EEPROM.commit(); 

        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Recording...");
        delay(DEFAULT_DELAY * 10);
      }
    }
  }
}

// AUXILIAR FUNCTIONS

uint32_t valueFromIRWithWhile() { // TROCADO
  while(IrReceiver.decode() == false || IrReceiver.decodedIRData.decodedRawData == 0) {
    IrReceiver.resume();
  }
  uint32_t code = IrReceiver.decodedIRData.decodedRawData; // TROCADO
  Serial.print("Código recebido:");
  Serial.println(code, HEX);

  IrReceiver.resume();

  return code;
}

uint32_t valueFromIRWithoutWhile() { // TROCADO
  uint32_t code; // TROCADO
  if(IrReceiver.decode()) {
    code = IrReceiver.decodedIRData.decodedRawData;

    Serial.print("Código recebido:");
    Serial.println(code, HEX);

    IrReceiver.resume();
    return code;
  }
  return 0;
}

int findIndexOfCodeIntoControle(uint32_t item) { // TROCADO
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
  return -1; 
}

int convertTimeInSeconds(String time_string) {
  String hour = time_string.substring(0, 2);
  String minute = time_string.substring(2, 4);
  String second = time_string.substring(4, 6);

  int hour_int = atoi(hour.c_str());
  int minute_int = atoi(minute.c_str());
  int second_int = atoi(second.c_str());

  int time_in_seconds = hour_int * 3600 + minute_int * 60 + second_int;
  return time_in_seconds;
}

String convertSecondsInTime(int seconds) {
  int hour_int = seconds / 3600;
  seconds = seconds % 3600;
  int minute_int = seconds / 60;
  seconds = seconds % 60;
  int second_int = seconds;

  char hour[3];
  char minute[3];
  char second[3];
  sprintf(hour, "%02d", hour_int);
  sprintf(minute, "%02d", minute_int);
  sprintf(second, "%02d", second_int);

  char time_string_formatted[] = {
    hour[0], hour[1], ':', 
    minute[0], minute[1], ':', 
    second[0], second[1], '\0'
  };

  return String(time_string_formatted);
}

int convertIndexToAddress(int index) {
  return index * BYTES_LENGTH;
}

int convertSecondsToCounter(int seconds) {
  return SCALE_TO_CONVERT_SECONDS_TO_COUNTER * seconds;
}