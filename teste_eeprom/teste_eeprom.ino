#include <EEPROM.h>

long int memory = 0xFF00BF00;
int address = 0;

void setup() {
 Serial.begin(9600);
 Serial.println("--- INICIO ---");
 
 Serial.print("Valor = "); Serial.println(memory);
 EEPROM.put(address, memory);
 
 memory = 0xFF11BF00;
 Serial.print("Valor modificado = "); Serial.println(memory);
 
 EEPROM.get(address, memory);
 Serial.print("Valor na memória = "); Serial.println(memory);
 
 Serial.println("--- FIM ---");
}

void loop() {}

//void setup() {
//  Serial.begin(9600);
//}
//
//void loop() {
//  saveInMemory(memory, 0);
//  
//  Serial.println(sizeof(memory));
//  Serial.println(memory, HEX);
//}
//
//void saveInMemory(long int code, int address) {
//  EEPROM.write(address, code);
//}
//
//long int readFromMemory(int address) {
//  byte Alto, Baixo;
//  Alto = EEPROM.read(address);
//  Baixo = EEPROM.read(address + 1);
//
//  return (Alto * 256 + Baixo);
//}
//
//void EEPROMWriteInt(int address, int value)
//{
//byte two = (value & 0xFF);
//byte one = ((value >> 8) & 0xFF);
//
//EEPROM.update(address, two);
//EEPROM.update(address + 1, one);
//}
//
//int EEPROMReadInt(int address)
//{
//long two = EEPROM.read(address);
//long one = EEPROM.read(address + 1);
//
//return ((two 0) & 0xFFFFFF) + ((one 8) & 0xFFFFFFFF);
//}
