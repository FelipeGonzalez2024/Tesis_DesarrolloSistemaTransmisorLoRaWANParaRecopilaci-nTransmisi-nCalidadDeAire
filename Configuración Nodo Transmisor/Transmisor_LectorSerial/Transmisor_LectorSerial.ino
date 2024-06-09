//Código Transmisor Para Lecutas Seriales
//Su funcionamiento consiste en recibir una señal serial por un puerto "software"
//Estas señales deben tener un formato: "1.584,2.5,7.412,589.321,50.41"
//En este caso, se ha definido que: 
//El primer espacio corresponde a MP 10 con el canal 01
//El segundo espacio corresponde a MP 2,5 con el canal 02
//El tercer espacio corresponde a CO2 con el canal 03
//El cuarto espacio corresponde a SO2 con el canal 04
//El quinto espacio corresponde a NO con el canal 05
//A diferencia del otro transmisor, este transmite tan solo cuando llega una señal a su "softwareserial", de modo que la frecuencia de transmisión lo dará el medidor de calidad de aire
//Al igual que el otro transmisor, arma paquetes de 11 bytes para su envío
//Según la necesidad, se pueden agregar o quitar parámetros y canales

#include <FastCRC.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <Separador.h>

#if defined(ARDUINO_AVR_UNO) 
const int SSRxPin = 8; 
const int SSTxPin = 9; 
SoftwareSerial SoftSerial(SSRxPin, SSTxPin);
#else
HardwareSerial* SoftSerial = &Serial1;
#endif
SoftwareSerial ss(10, 11);
Separador s;
FastCRC8 CRC8;

byte payload[20]; 
byte BytesChar[4];
byte BytesFloat[4];
byte BytesInt[2];
byte ByteAux[1];
byte Code[1];
int SizePayload = 0;
String payloadString = ""; 
String MP10;
String MP25;
String CO2;
String SO2;
String NO;

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

void Input_Float(float valor) {
  Float_to_Byte(valor); 
  Assemble_Payload(BytesFloat, 4, SizePayload); 
}

void Send_Payload(byte* payload, int SizePayload) {
  for (int i = 0; i < SizePayload; i++) { 
    char hex[3]; 
    sprintf(hex, "%02X", payload[i]); 
    payloadString += hex; 
  }
  String Command = "AT+SENDB=";
  String NPort = "0,2,11,";

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
#if defined (ARDUINO_AVR_UNO) //modificar en caso de ser UNO o NANO
    SoftSerial.begin(9600);
#else
    Serial1.begin(9600, SERIAL_8E1);
#endif
  delay(5000);
}
 
void loop() {
  
  while(SoftSerial.available() > 0)
  {
    String cadena = SoftSerial.readStringUntil('\n');
    cadena = cadena.substring(0, cadena.length()-1);
    MP10 = s.separa(cadena, ',', 0);
    MP25 = s.separa(cadena, ',', 1);
    CO2 = s.separa(cadena, ',', 2);
    SO2 = s.separa(cadena, ',', 3);
    NO = s.separa(cadena, ',', 4);
    //En caso de querer agregar parámetros, se debe seguir con la secuencia, por ejemplo
    //DatoExtra = s.separa(cadena, ',', 5);
    //Asegurandose de que llegue un dato extra al serial "1.0,2.0,3.0,4.0,5.0,'-6.0-'"

    float fMP10 = MP10.toFloat();
    float fMP25 = MP25.toFloat();
    float fCO2 = CO2.toFloat();
    float fSO2 = SO2.toFloat();
    float fNO = NO.toFloat();
    //Igualmente se agrega aquí

    //Envio Material Particulado 10 y 2,5
    ByteAux[0]=0x01; Assemble_Payload(ByteAux, 1, SizePayload); //Canal 
    Input_Float(fMP10);  //Dato
    ByteAux[0]=0x02; Assemble_Payload(ByteAux, 1, SizePayload); //Canal 
    Input_Float(fMP25);  //Dato
    ByteAux[0] = CRC8.smbus(payload, SizePayload);
    Assemble_Payload(ByteAux, 1, SizePayload);
    Send_Payload(payload, SizePayload);
    delay(2000);
    SizePayload = 0;
    payloadString = "";
    memset(payload, 0, sizeof(payload)); 

    //Envio CO2 y SO2
    ByteAux[0]=0x03; Assemble_Payload(ByteAux, 1, SizePayload); //Canal 
    Input_Float(fCO2);  //Dato
    ByteAux[0]=0x04; Assemble_Payload(ByteAux, 1, SizePayload); //Canal 
    Input_Float(fSO2);  //Dato
    ByteAux[0] = CRC8.smbus(payload, SizePayload);
    Assemble_Payload(ByteAux, 1, SizePayload);
    Send_Payload(payload, SizePayload);
    delay(2000);
    SizePayload = 0;
    payloadString = "";
    memset(payload, 0, sizeof(payload)); 

    //Envio NO y dato extra a gusto
    ByteAux[0]=0x05; Assemble_Payload(ByteAux, 1, SizePayload); //Canal 
    Input_Float(fNO);  //Dato
    ByteAux[0]=0x06; Assemble_Payload(ByteAux, 1, SizePayload); //Queda espacio para una variable más
    Input_Float(0.0);  //Dato para completar, por ejemplo: DatoExtra
    ByteAux[0] = CRC8.smbus(payload, SizePayload);
    Assemble_Payload(ByteAux, 1, SizePayload);
    Send_Payload(payload, SizePayload);
    delay(2000);
    SizePayload = 0;
    payloadString = "";
    memset(payload, 0, sizeof(payload)); 

    //También puede crecer la cantidad de paquetes a enviar, copiando las estructuras superiores
    //Pero se debe cuidar la cantidad de paquetes a enviar, ya que puede significar un gasto extra de energía

    cadena = "";
  }
  
}
