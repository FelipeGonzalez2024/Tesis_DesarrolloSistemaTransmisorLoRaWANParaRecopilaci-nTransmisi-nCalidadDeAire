#include <SoftwareSerial.h>

SoftwareSerial ss(10, 11);      

void Config_Retransmisor() {
  Serial.println("Comienza configuración");
  ss.println("AT+FRE=915.800,915.800"); delay(2000);
  ss.println("AT+BW=0,0"); delay(2000);
  ss.println("AT+SF=10,10"); delay(2000);
  ss.println("AT+CRC=1,1"); delay(2000);
  ss.println("AT+HEADER=0,0"); delay(2000);
  ss.println("AT+CR=1,1"); delay(2000);
  Serial.println("Mitad configuración");
  ss.println("AT+IQ=0,0"); delay(2000);
  ss.println("AT+PREAMBLE=8,8"); delay(2000);
  ss.println("AT+SYNCWORD=1"); delay(2000);
  ss.println("AT+RXMOD=65535,1"); delay(2000);
  ss.println("AT+WAITTIME=1000"); delay(2000);
  ss.println("AT+GROUPMOD=0,0"); delay(2000);
  ss.println("AT+POWER=20"); delay(2000);
  ss.println("ATZ"); delay(5000);
  Serial.println("Finaliza configuración");
}

void setup() {
  Serial.begin(9600);        
  ss.begin(9600);             
  ss.listen();                 
  Serial.println("Setup");
  ss.println("ATZ");  
  delay(10000);   
  Config_Retransmisor();
}

void loop() {
}