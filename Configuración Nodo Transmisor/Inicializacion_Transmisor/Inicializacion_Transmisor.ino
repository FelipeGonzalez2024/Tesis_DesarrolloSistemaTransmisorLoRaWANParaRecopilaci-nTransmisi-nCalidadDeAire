#include <SoftwareSerial.h>

SoftwareSerial ss(10, 11);      

void Config_Transmisor() {
  Serial.println("Comienza Configuración");
  ss.println("AT+CHS=915800000"); delay(2000);
  ss.println("AT+NJM=0"); delay(2000);
  ss.println("AT+ADR=0"); delay(2000);
  ss.println("AT+DR=0"); delay(2000);
  ss.println("ATZ"); delay(5000);
  Serial.println("Termina Configuración");
}

void setup() {
  Serial.begin(9600);        
  ss.begin(9600);             
  ss.listen();                 
  Serial.println("Setup");
  ss.println("ATZ");  
  delay(10000);   
  Config_Transmisor();
}

void loop() {
}