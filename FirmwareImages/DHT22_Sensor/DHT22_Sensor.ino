#include "DHT.h"
#define DHT22_PIN 5
DHT dht22(DHT22_PIN, DHT22);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  dht22.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  float humidity = dht22.readHumidity();
  float tempC = dht22.readTemperature();

  if (isnan(humidity) || isnan(tempC)) {
    Serial.println("Failed to read from DHT22 sensor!");
  } else {
    Serial.println("Humidity: " + String(humidity) + " | " + "Temperature: " + String(tempC));
  }
  delay(1000);
}
