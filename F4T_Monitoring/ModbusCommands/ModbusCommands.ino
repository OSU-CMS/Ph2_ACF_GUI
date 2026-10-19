// Libraries
#include <Ethernet_Generic.h>
#include <ArduinoModbus.h>
#include <DHT22.h>
#include <SdFat.h>
#include <SPI.h>

// Definitions and variable assignments
#define pinData SDA           // Pin 20 on the Mega 2560
#define SD_CS_PIN 4
#define W5100_CS 10           // Explicitly define Ethernet CS pin

DHT22 dht22(pinData);

SdFat SD;
File file;

EthernetClient ethClient;
ModbusTCPClient modbusClient(ethClient);

// Ethernet shield MAC address
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Data Transmission
unsigned int localPort = 8081;                        // Local port to listen on
unsigned int remotePort = 3000;                       // Port on the receiving PC/Device
char packetBuffer[ETHERNET_UDP_TX_PACKET_MAX_SIZE];   //array to store recieved data
String receivedData;                                  //string to store received data
int packetSize;                                       //variable to store received packet size
EthernetUDP UDP;                                      //UDP object

IPAddress remoteIP(128,146,32,122);

// Thermal Chamber IP
IPAddress chamberIP(128, 146, 32, 200);
const int chamberPort = 502;

// Timing Variables for Non-Blocking Loop
unsigned long previousSensorMillis = 0;
const long sensorInterval = 5000; // Check sensor every 5 seconds

// State Variables
bool SafetyEnabled = false;
bool DangerZone = false;

bool startup = true;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("--- Initializing System ---");

  // Force Mega to stay in SPI Master mode
  pinMode(53, OUTPUT); 
  digitalWrite(53, HIGH);
  
  // Disable Ethernet and SD Card initially to prevent SPI collisions
  pinMode(W5100_CS, OUTPUT); 
  digitalWrite(W5100_CS, HIGH); 
  pinMode(SD_CS_PIN, OUTPUT);  
  digitalWrite(SD_CS_PIN, HIGH);

  // Initialize Ethernet using DHCP
  Ethernet.init(W5100_CS);
  Serial.println("Requesting IP address via DHCP...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Error: Failed to configure Ethernet using DHCP");
    // Halt if no network
    while(true) { delay(1); } 
  } else {
    Serial.print("Ethernet connected. IP: ");
    Serial.println(Ethernet.localIP());
    // Start UDP
    UDP.begin(localPort);
    Serial.println("UDP started.");
  }

  // Initialize SD Card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Error: SD initialization failed!");
  } else {
    Serial.println("SD initialized.");
  }

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("Waiting 5s for sensors to stabilize...");
  delay(5000);

  // Initial Modbus Connection Attempt
  Serial.println("Attempting initial Modbus connection...");

  if (modbusClient.begin(chamberIP, chamberPort)) {
    Serial.println("Connected to thermal chamber via Modbus TCP.");
  } else {
    Serial.println("Failed initial connection to thermal chamber.");
  }
}

void loop() {    
  
  static unsigned long lastReconnectAttempt = 0;

  if (!modbusClient.connected()) {
    Serial.println("Modbus connection lost. Attempting to reconnect...");
    modbusClient.begin(chamberIP, chamberPort);
    delay(1000);
  }

  int packetSize = UDP.parsePacket();
  if (packetSize) {
    UDP.read(packetBuffer, ETHERNET_UDP_TX_PACKET_MAX_SIZE);
        // Check if the request is "h" (as sent by your Python script)
    if (packetBuffer[0] == 'h') {
      float t = dht22.getTemperature();
      float h = dht22.getHumidity();
      unsigned long seconds = millis() / 1000;

      // Construct the response: "time,temp,humidity"
      String response = String(seconds) + "," + String(t) + "," + String(h);

      // REPLY to the sender (Python)
      UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
      UDP.print(response);
      UDP.endPacket();
      
     // Serial.println("Sent response to Python: " + response);
    }
  }


  // 2. NON-BLOCKING SENSOR TIMER (Executes every 5 seconds)

  unsigned long currentMillis = millis();

  if (currentMillis - previousSensorMillis >= sensorInterval) {
    previousSensorMillis = currentMillis; // Reset the stopwatch

    // Get live temperature and humidity
    float t = dht22.getTemperature();
    float h = dht22.getHumidity();

    // Check danger conditions
    DangerZone = (t < 0 || t > 45 || h > 60.0);

    if (DangerZone) {
      // ENACT SAFETY PROTOCOL
      if (!SafetyEnabled) {
        SafetyEnabled = true;
        Serial.println("\n*** DANGER DETECTED: INITIATING SAFETY PROTOCOL ***");

        // Execute Modbus Shutdown Commands
        if (modbusClient.connected()) {
          // Terminate profile
          modbusClient.holdingRegisterWrite(1, 0x40B6, 148);
          delay(1000);
    
          // Turn off the power to event 1
          modbusClient.holdingRegisterWrite(1, 0x40D2, 62);

          Serial.println("Modbus safety commands transmitted.");
        } else {
          Serial.println("ERROR: Cannot send Modbus commands. Not connected");
        }

      }

      // Log to SD Card
      file = SD.open("temperaturelog.txt", FILE_WRITE);
      if (file) {
        file.println("Time: " + String(currentMillis / 1000) + "s");
        file.println("Temperature: " + String(t) + "C Humidity: " + String(h) + "%");
        file.close();
        Serial.println("Temperature logged to SD card.");
      }

      // Flash LED (Briefly blocks for 1 second, acceptable in an emergency state)
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);

      // Print Live DANGER Update
      Serial.println("-----------------------------------------------------------------------------------------");
      Serial.println("STATUS: DANGER \t Time: " + String(currentMillis / 1000) + "s \t Temp: " + String(t) + "C \t Hum: " + String(h) + "%");
      Serial.println("-----------------------------------------------------------------------------------------");

    } else {
      // NOMINAL OPERATION
      if (SafetyEnabled) {
        Serial.println("\n*** CONDITIONS RETURNED TO SAFE RANGES. SAFETY PROTOCOL LIFTED. ***");
      }
      SafetyEnabled = false;

      // Print Live NOMINAL Update
      Serial.println("STATUS: Nominal \t Time: " + String(currentMillis / 1000) + "s \t Temp: " + String(t) + "C \t Hum: " + String(h) + "%");
    
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
      UDP.print(t);
      UDP.print(",");
      UDP.print(t);
      UDP.print(",");
      UDP.print(h);
      UDP.endPacket();                                  //end packet send
    }
    memset(packetBuffer, 0, ETHERNET_UDP_TX_PACKET_MAX_SIZE);    //reset packet array to 0

    // File won't reopen for writing unless closed
    //file.close();

    // Giving time before opening file again after closing
   
    delay(500);
    }
  }
}
