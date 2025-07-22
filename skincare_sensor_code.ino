#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ArduinoJson.h>

// Pin Definitions
#define DHTPIN 4
#define DHTTYPE DHT11
#define S0 21
#define S1 22
#define S2 23
#define S3 19
#define OUT 18
#define OE 5
#define MOISTURE_PIN 34
#define UV_PIN 16

DHT dht(DHTPIN, DHTTYPE);

// Variables for color sensor
int redFrequency = 0;
int greenFrequency = 0;
int blueFrequency = 0;

// Moisture sensor calibration
const int MOISTURE_DRY = 3092;
const int MOISTURE_WET = 2500;
const int MOISTURE_RANGE_LOW = 24;
const int MOISTURE_RANGE_HIGH = 39;

// Color sensor settings
const int COLOR_SAMPLES = 10;
const int COLOR_DELAY = 50;

void setup() {
  Serial.begin(115200);
  
  // Configure pins
  pinMode(MOISTURE_PIN, INPUT);
  pinMode(UV_PIN, INPUT);
  pinMode(DHTPIN, INPUT);
  
  // Color sensor setup
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(OUT, INPUT);
  pinMode(OE, OUTPUT);
  digitalWrite(OE, LOW);
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // Initialize DHT
  dht.begin();
  delay(2000);
}

int readColorFrequency(bool s2Val, bool s3Val) {
  digitalWrite(S2, s2Val);
  digitalWrite(S3, s3Val);
  delay(100);
  return pulseIn(OUT, LOW);
}

int readColorFrequencyAverage(bool s2Val, bool s3Val) {
  digitalWrite(S2, s2Val);
  digitalWrite(S3, s3Val);
  
  long sum = 0;
  for(int i = 0; i < COLOR_SAMPLES; i++) {
    delay(COLOR_DELAY);
    int reading = pulseIn(OUT, LOW);
    if(reading != 0) {
      sum += reading;
    } else {
      i--;
    }
  }
  return sum / COLOR_SAMPLES;
}

float calculateOilLevel() {
  redFrequency = readColorFrequencyAverage(LOW, LOW);
  delay(50);
  greenFrequency = readColorFrequencyAverage(HIGH, HIGH);
  delay(50);
  blueFrequency = readColorFrequencyAverage(LOW, HIGH);
  
  float total = redFrequency + greenFrequency + blueFrequency;
  float redRatio = redFrequency / total;
  float greenRatio = greenFrequency / total;
  
  float oilEstimate = (redRatio / greenRatio) * 30.0;
  return constrain(oilEstimate, 19, 38);
}

int mapMoisture(int rawValue) {
  int constrainedValue = constrain(rawValue, MOISTURE_WET, MOISTURE_DRY);
  return map(constrainedValue, MOISTURE_DRY, MOISTURE_WET, 
            MOISTURE_RANGE_LOW, MOISTURE_RANGE_HIGH);
}

void loop() {
  static unsigned long lastSendTime = 0;
  const unsigned long SEND_INTERVAL = 5000;
  
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& doc = jsonBuffer.createObject();
    
    // Read sensors
    doc["temperature"] = dht.readTemperature();
    doc["humidity"] = dht.readHumidity();
    doc["uvIndex"] = constrain(map(analogRead(UV_PIN), 0, 4095, 0, 11), 0, 11);
    doc["moisture"] = mapMoisture(analogRead(MOISTURE_PIN));
    doc["oil"] = calculateOilLevel();
    
    // Send data
    doc.printTo(Serial);
    Serial.println();
    Serial.flush();
    
    lastSendTime = millis();
  }
  
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    if (command == "READ") {
      StaticJsonBuffer<200> jsonBuffer;
      JsonObject& doc = jsonBuffer.createObject();
      
      doc["temperature"] = dht.readTemperature();
      doc["humidity"] = dht.readHumidity();
      doc["uvIndex"] = constrain(map(analogRead(UV_PIN), 0, 4095, 0, 11), 0, 11);
      doc["moisture"] = mapMoisture(analogRead(MOISTURE_PIN));
      doc["oil"] = calculateOilLevel();
      
      doc.printTo(Serial);
      Serial.println();
      Serial.flush();
    }
  }
}