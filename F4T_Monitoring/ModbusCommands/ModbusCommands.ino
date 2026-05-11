// Libraries
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <ArduinoModbus.h>
#include <DHT22.h>
//#include <SdFat.h>
#include <SPI.h>

// Definitions and variable assignments
#define pinData SDA
//#define SD_CS_PIN 4

DHT22 dht22(pinData);

//SdFat SD;
//File file;

EthernetClient ethClient;

ModbusTCPClient modbusClient(ethClient);

// Ethernet shield MAC address
byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0xA7, 0xF8 };

//----------------------------Data Transmission------------------------------------
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  //array to store received data
String receivedData;                        //string to store received data
int packetSize;                             //variable to store received packet size
EthernetUDP UDP;                            //UDP object
//----------------------------------------------------------------------------------

// Thermal Chamber IP
IPAddress chamberIP(128, 146, 33, 179);

// Port number for Modbus communication (502 is standard)
const int chamberPort = 502;

bool startup = true;

void setup() {

  // Open serial communications
  Serial.begin(9600);
  
  // Open Ethernet connection
  int EthernetStatus = Ethernet.begin(mac);
  if (EthernetStatus == 0) {
    Serial.println("Failed to configure Ethernet using DHCP.");
    // Optionally assign a static IP here
  }
  else if (EthernetStatus == 1){
    Serial.println("Configured Ethernet using DHCP.");
  }

  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());
  UDP.begin(8081);

  // Open SD connection
  //SD.begin(SD_CS_PIN);

  //Initialize led as output
  pinMode(LED_BUILTIN, OUTPUT);

  // Allow the Ethernet Shield and DHT22 Sensor to initialize
  delay(5000);

  // Connect to the thermal chamber
  if (modbusClient.begin(chamberIP, chamberPort)) {
    Serial.println("Connected to chamber.");
  }
  
  else {
    Serial.println("Failed to connect to chamber.");
  }
}

void loop() {
  //If disconnected from chamber, try reconnecting every 5 seconds
  if (!modbusClient.connected()) {
    if (!modbusClient.begin(chamberIP, chamberPort)) {
      Serial.println("Failed to connect to chamber");
      delay(5000);
      return;
    }
  }

  // Set variables for entering safety protocol
  static bool SafetyEnabled = false;
  static bool DangerZone = false;

  // Open or create file for writing
  //file = SD.open("errorlog.txt", FILE_WRITE);

  // Get temperature and humidity from sensor
  String time = String(millis()/1000);
  float t = dht22.getTemperature();
  float h = dht22.getHumidity();

  // If temperature OR humidity unacceptable, we are in danger
  if (t < -45 || t > 45 || h > 41.50) {
    DangerZone = true;
  }
  else {
    DangerZone = false;
  }
  
  // If we are in danger and haven't begun Safety protocol, begin
  if (DangerZone && !(SafetyEnabled)) {
    SafetyEnabled = true;

    // Open file for logging
    //file = SD.open("errorlog.txt", FILE_WRITE);

    // Terminate profile
    modbusClient.holdingRegisterWrite(1, 0x40B6, 148);
    delay(1000);
    
    // Set temperature control mode to auto
    modbusClient.holdingRegisterWrite(1, 0x0AAA, 10);
    delay(1000);

    // Set temperature to 23C
    modbusClient.holdingRegisterWrite(1, 0x0ADE, 0x41B8);
    modbusClient.holdingRegisterWrite(1, 0x0ADF, 0x0000);
    
    // Log time and values of error
    //file.println("Time: " + String(millis()/1000)+"s");
    //file.println("Temperature: " + String(t) + "C Humidity: " + String(h)+"%");
  }

  // If we are in danger and Safety protocol already started, blink LED until out of danger
  if (DangerZone && SafetyEnabled) {
    // Flash LED every second, checking for danger every 11 seconds
    for (int i = 0; i < 1; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
    }
    Serial.println("--------------------------------------------------------------------------------------------------------------------------------\n");
    Serial.println("!!!!!!!!!!!\tTime since initialization: "+time+"s\t\tStatus: DANGER\t\tTemperature: "+String(t)+"\tHumidity: "+String(h)+"\t!!!!!!!!");
    Serial.println("\n--------------------------------------------------------------------------------------------------------------------------------");
  }
  
  // If temperature/humidity within acceptable ranges
  if (!(DangerZone)) {
    // Once back in safety, protocol no longer being enacted
    SafetyEnabled = false;

    // Check danger every 21 seconds
    Serial.println("--------------------------------------------------------------------------------------------------------------------------------\n");
    Serial.println("Time since initialization: "+time+"s\t\tStatus: Nominal\t\tTemperature: "+String(t)+"\tHumidity: "+String(h));
    Serial.println("\n--------------------------------------------------------------------------------------------------------------------------------");
    delay(5000);
  }

  //Send data to computer python program
  packetSize = UDP.parsePacket();                     //get size of received packet 
  if(packetSize > 0)                                  //data received?
  {
    //UDP.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);   //read received data via UDP
    //String receivedData(packetBuffer);                //and then convert to string

    UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());//initialize packet send

    if(startup){
      UDP.print("!");
      startup = false;
      }
    UDP.print(time);
    UDP.print(",");
    UDP.print(t);
    UDP.print(",");
    UDP.print(h);
    UDP.endPacket();                                  //end packet send
  }
  memset(packetBuffer, 0, UDP_TX_PACKET_MAX_SIZE);    //reset packet array to 0

  // File won't reopen for writing unless closed
  //file.close();

  // Giving time before opening file again after closing
  delay(500);
}