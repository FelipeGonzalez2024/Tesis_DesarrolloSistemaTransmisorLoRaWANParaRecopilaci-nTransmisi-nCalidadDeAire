#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <FastCRC.h>

#define DHTPIN 8               
#define DHTPIN_2 9              
#define DHTTYPE    DHT11         
#define SensorPin A0
DHT_Unified dht1(DHTPIN, DHTTYPE);
DHT_Unified dht2(DHTPIN_2, DHTTYPE);

FastCRC8 CRC8;

SoftwareSerial ss(10, 11);      

byte payload[20]; 
byte BytesChar[4];
byte BytesFloat[4];
byte BytesInt[2];
byte ByteAux[1];
byte Code[1];

int sensorValue = 123;
int SizePayload = 0;
bool ONChn5 = false; //Activación o Desactivación del canal 5
bool ONChn6 = false; //Activación o Desactivación del canal 6
bool ONChn7 = false; //Activación o Desactivación del canal 7
bool ONChn8 = false; //Activación o Desactivación del canal 8
String payloadString = ""; 

float DHT11_1_temp = 0;
float DHT11_1_hum = 0;
float DHT11_2_temp = 0;
float DHT11_2_hum = 0;

char rxbuff[128];               
uint8_t rxbuff_index=0;

String inputString = "";         
bool stringComplete = false;    
long old_time = 0;
long new_time;
long uplink_interval = 300000;  

void Assemble_Payload(byte* arrayOriginal, int size, int inicio) {
  for (int i = inicio; i < inicio + size; i++) {
    payload[SizePayload] = arrayOriginal[i - inicio];
    SizePayload++;
  }
}

void Float_to_Byte(float numeroFlotanteOriginal) {
  byte arregloBytes[sizeof(numeroFlotanteOriginal)];
  memcpy(arregloBytes, &numeroFlotanteOriginal, sizeof(numeroFlotanteOriginal));
  for (int i = 0; i < sizeof(numeroFlotanteOriginal); i++) {
    BytesFloat[i]=arregloBytes[i];
  }
}

void Int_to_Byte(int numero) {
  BytesInt[0] = lowByte(sensorValue);
  BytesInt[1] = highByte(sensorValue);
}

void Word_to_Byte(const char* word) {
  for (int i = 0; i < 4; i++) {
      if (word[i] != '\0') {
          BytesChar[i] = (uint8_t)word[i];
      } else {
          BytesChar[i] = 0; 
      }
  }
}

void Input_Float(float valor) {
  Float_to_Byte(valor); 
  Assemble_Payload(BytesFloat, 4, SizePayload); 
}

void Input_Int(int valor) {
  ByteAux[0]=0x11; Assemble_Payload(ByteAux, 1, SizePayload); //Tipo 0x12
  Int_to_Byte(valor); 
  Assemble_Payload(BytesInt, 2, SizePayload); 
}

void Input_Char(char* word) {
  Word_to_Byte(word);
  Assemble_Payload(BytesChar, 4, SizePayload); 
}

void Send_Payload(byte* payload, int SizePayload) {
  for (int i = 0; i < SizePayload; i++) { 
    char hex[3]; 
    sprintf(hex, "%02X", payload[i]); 
    payloadString += hex; 
  }
  String Command = "AT+SENDB=";
  String NPort = "0,2,11,";

  Serial.println("Payload");
  Serial.println(payloadString);

  payloadString = Command + NPort + payloadString;
  Serial.println("Comando Enviado:");
  Serial.println(payloadString);
  ss.print(payloadString);
  ss.print('\r');
  delay(5000);
}

void setup() {
  Serial.begin(9600);        
  ss.begin(9600);             
  ss.listen();                
  
  inputString.reserve(200);    
  dht1.begin();                
  dht2.begin();
  sensor_t sensor;
  dht1.temperature().getSensor(&sensor);   
  dht1.humidity().getSensor(&sensor);   
  dht2.temperature().getSensor(&sensor);   
  dht2.humidity().getSensor(&sensor);     
  
  ss.println("ATZ");  
  delay(10000);  
}

void loop() {
  SizePayload = 0;
  payloadString = "";
  memset(payload, 0, sizeof(payload)); 

  while ( ss.available()) {
    char inChar = (char) ss.read();     
    inputString += inChar;             
    rxbuff[rxbuff_index++]=inChar;      
    if(rxbuff_index>128) {             
      rxbuff[rxbuff_index]='\0';   
      rxbuff_index=0;                  
      break;                            
    }
    if (inChar == '\n' || inChar == '\r') {   
      stringComplete = true;                 
      rxbuff[rxbuff_index]='\0';             
      rxbuff_index=0;                         
    }
  }

  while ( Serial.available()) {
    char inChar = (char) Serial.read();  
    inputString += inChar;               
    if (inChar == '\n' || inChar == '\r') { 
      ss.print(inputString);                
      inputString = "\0";                    
    }
  }
  
  if (stringComplete) {  
    Serial.print(inputString);  
    inputString = "\0";        
    stringComplete = false;     
  }
  
  new_time = millis();   

  if(new_time-old_time>=uplink_interval){   
    old_time = new_time;     
    //ss.println("AT+SLEEP=0");      

    sensors_event_t event;

    dht1.temperature().getEvent(&event);  
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading sensor 1!"));                                
    }
    else {
      //Medicion Temperatura DHT-11 1
      ByteAux[0]=0x01; Assemble_Payload(ByteAux, 1, SizePayload); //Canal 
      DHT11_1_temp=event.temperature;
      Input_Float(DHT11_1_temp);  //Dato
      //Medicion Humedad Relativa DHT-11 1
      dht1.humidity().getEvent(&event);  
      ByteAux[0]=0x03; Assemble_Payload(ByteAux, 1, SizePayload); //Canal 
      DHT11_1_hum=event.relative_humidity;
      Input_Float(DHT11_1_hum);  //Dato
      ByteAux[0] = CRC8.smbus(payload, SizePayload);
      Assemble_Payload(ByteAux, 1, SizePayload);
      Send_Payload(payload, SizePayload);
      delay(2000);
      SizePayload = 0;
      payloadString = "";
      memset(payload, 0, sizeof(payload)); 
    }

    dht2.temperature().getEvent(&event);  
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading sensor 2!"));                                
    }
    else {
      //Medicion Temperatura DHT-11 2
      ByteAux[0]=0x02; Assemble_Payload(ByteAux, 1, SizePayload); //Canal 
      DHT11_2_temp=event.temperature;
      Input_Float(DHT11_2_temp);  //Dato
      //Medicion Humedad Relativa DHT-11 2
      dht2.humidity().getEvent(&event);  
      ByteAux[0]=0x04; Assemble_Payload(ByteAux, 1, SizePayload); //Canal 
      DHT11_2_hum=event.relative_humidity;
      Input_Float(DHT11_2_hum);  //Dato
      ByteAux[0] = CRC8.smbus(payload, SizePayload);
      Assemble_Payload(ByteAux, 1, SizePayload);
      Send_Payload(payload, SizePayload);
      delay(2000);
      SizePayload = 0;
      payloadString = "";
      memset(payload, 0, sizeof(payload)); 
    }

    sensorValue = analogRead(SensorPin); //Medición sensor análogo en caso de haber
    Code[0] = 0xAA;

    if (!ONChn5 && ONChn6) {
      Serial.println(F("Error reading Analog!"));
      ByteAux[0] = 0x06; Assemble_Payload(ByteAux, 1, SizePayload); // Canal
      Assemble_Payload(Code, 1, SizePayload);
      for (int i = 0; i < 8; i++) {
        ByteAux[0] = 0x00;
        Assemble_Payload(ByteAux, 1, SizePayload);
      }
      ByteAux[0] = CRC8.smbus(payload, SizePayload);
      Assemble_Payload(ByteAux, 1, SizePayload);
      Send_Payload(payload, SizePayload);
      delay(2000);
      SizePayload = 0;
      payloadString = "";
      memset(payload, 0, sizeof(payload));
    } else if (ONChn5 && !ONChn6) {
      Serial.println(F("Error reading Alarm!"));
      ByteAux[0] = 0x05; Assemble_Payload(ByteAux, 1, SizePayload); // Canal
      Input_Int(sensorValue);  // Dato
      for (int i = 0; i < 7; i++) {
        ByteAux[0] = 0x00;
        Assemble_Payload(ByteAux, 1, SizePayload);
      }
      ByteAux[0] = CRC8.smbus(payload, SizePayload);
      Assemble_Payload(ByteAux, 1, SizePayload);
      Send_Payload(payload, SizePayload);
      delay(2000);
      SizePayload = 0;
      payloadString = "";
      memset(payload, 0, sizeof(payload));
    } else if (ONChn5 && ONChn6) {
      ByteAux[0] = 0x05; 
      Assemble_Payload(ByteAux, 1, SizePayload); // Canal
      Input_Int(sensorValue);  // Dato
      for (int i = 0; i < 2; i++) {
        ByteAux[0] = 0x00;
        Assemble_Payload(ByteAux, 1, SizePayload);
      }
      ByteAux[0] = 0x06; 
      Assemble_Payload(ByteAux, 1, SizePayload); // Canal
      Assemble_Payload(Code, 1, SizePayload);
      for (int i = 0; i < 3; i++) {
        ByteAux[0] = 0x00;
        Assemble_Payload(ByteAux, 1, SizePayload);
      }
      ByteAux[0] = CRC8.smbus(payload, SizePayload);
      Assemble_Payload(ByteAux, 1, SizePayload);
      Send_Payload(payload, SizePayload);
      delay(2000);
      SizePayload = 0;
      payloadString = "";
      memset(payload, 0, sizeof(payload));
    } else {
      Serial.println(F("Error reading Analog and Alarm!"));
    }

    char* word = "UACh";
    if (!ONChn7 && ONChn8) {
      Serial.println(F("Error reading Text!"));
      ByteAux[0]=0x08; Assemble_Payload(ByteAux, 1, SizePayload); //Canal 
      ss.print("AT+BAT=?");
      ss.print('\r');
      delay(5000);
      if (ss.available()) { 
        String response = ss.readStringUntil('\n'); 
        int battery = response.toInt(); // Convierte la respuesta a un número entero
        float batteryLevel = battery; 
        Input_Float(batteryLevel);
      }
      for (int i = 0; i < 5; i++) {
        ByteAux[0] = 0x00;
        Assemble_Payload(ByteAux, 1, SizePayload);
      }
      ByteAux[0] = CRC8.smbus(payload, SizePayload);
      Assemble_Payload(ByteAux, 1, SizePayload);
      Send_Payload(payload, SizePayload);
      delay(2000);
      SizePayload = 0;
      payloadString = "";
      memset(payload, 0, sizeof(payload));
    } else if (ONChn7 && !ONChn8) {
      Serial.println(F("Error reading Battery!"));
      ByteAux[0]=0x07; Assemble_Payload(ByteAux, 1, SizePayload); //Canal 
      Input_Char(word);
      for (int i = 0; i < 5; i++) {
        ByteAux[0] = 0x00;
        Assemble_Payload(ByteAux, 1, SizePayload);
      }
      ByteAux[0] = CRC8.smbus(payload, SizePayload);
      Assemble_Payload(ByteAux, 1, SizePayload);
      Send_Payload(payload, SizePayload);
      delay(2000);
      SizePayload = 0;
      payloadString = "";
      memset(payload, 0, sizeof(payload));
    } else if (ONChn7 && ONChn8) {
      ByteAux[0]=0x07; Assemble_Payload(ByteAux, 1, SizePayload); //Canal 
      Input_Char(word);
      ByteAux[0]=0x08; Assemble_Payload(ByteAux, 1, SizePayload); //Canal 
      ss.print("AT+BAT=?");
      ss.print('\r');
      delay(5000);
      if (ss.available()) { 
        String response = ss.readStringUntil('\n'); 
        int battery = response.toInt(); 
        float batteryLevel = battery / 1; 
        Serial.print("Battery is: ");
        Serial.print(batteryLevel);
        Serial.println();
        Input_Float(batteryLevel);
      }
      ByteAux[0] = CRC8.smbus(payload, SizePayload);
      Assemble_Payload(ByteAux, 1, SizePayload);
      Send_Payload(payload, SizePayload);
      delay(2000);
      SizePayload = 0;
      payloadString = "";
      memset(payload, 0, sizeof(payload));
    } else {
      Serial.println(F("Error reading Text and Battery!"));
    }
    //ss.println("AT+SLEEP=1"); 
  }
}