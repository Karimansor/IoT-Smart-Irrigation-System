// Smart Irrigation System with Correct Relay Handling

#define BLYNK_TEMPLATE_ID "TMPL2TgB9ZVtW"
#define BLYNK_TEMPLATE_NAME "Smart Irrigation System"
#define BLYNK_AUTH_TOKEN " "

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

// WiFi credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "iotlab";
char pass[] = "hostiotlab";

// Blynk timer
BlynkTimer timer;

// DHT Sensor settings
#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Pin definitions
#define sensor A0
#define waterPump D3   // Relay control pin
#define ledPin D4      // LED for pump status

// Virtual Pins
#define VPIN_SOIL V0
#define VPIN_PUMP V1
#define VPIN_TEMP V2
#define VPIN_HUMIDITY V3

bool manualPump = false;
bool pumpRunning = false;

void setup() {
  Serial.begin(9600);
  
  pinMode(waterPump, OUTPUT);
  pinMode(ledPin, OUTPUT);
  
  // Initial states
  digitalWrite(waterPump, LOW); // Pump OFF initially (Active LOW relay)
  digitalWrite(ledPin, LOW);    // LED OFF initially

  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  dht.begin();

  timer.setInterval(1000L, soilMoistureSensor);  // Read soil moisture every 1 sec
  timer.setInterval(3000L, readTempHumidity);    // Read DHT11 every 3 sec
}

// Manual control from Blynk Button
BLYNK_WRITE(VPIN_PUMP) {
  int buttonState = param.asInt();
  
  if (buttonState == 1) {
    manualPump = true;
    digitalWrite(waterPump, HIGH); // ✅ Pump ON (Active LOW)
    digitalWrite(ledPin, HIGH);    // ✅ LED ON
    pumpRunning = true;
    Blynk.logEvent("watering_done", "Pump started manually");
  } else {
    manualPump = false;
    digitalWrite(waterPump, LOW); // ✅ Pump OFF
    digitalWrite(ledPin, LOW);    // ✅ LED OFF
    pumpRunning = false;
  }
}

// Read soil moisture and auto control pump
void soilMoistureSensor() {
  int value = analogRead(sensor);
  
  // Map the value to 0-100% moisture
  value = map(value, 0, 1024, 0, 100);
  value = (value - 100) * -1; // Reverse

  Blynk.virtualWrite(VPIN_SOIL, value);

  // Only auto-control if not manually forced ON
  if (!manualPump) {
    if (value < 30 && !pumpRunning) {
      // Soil dry -> Start watering
      digitalWrite(waterPump, HIGH); // ✅ Pump ON
      digitalWrite(ledPin, HIGH);    // ✅ LED ON
      pumpRunning = true;
      Blynk.virtualWrite(VPIN_PUMP, 1);
      Blynk.logEvent("watering_done", "Soil dry. Auto watering...");
      delay(5000);  // Water for 5 seconds
      digitalWrite(waterPump, LOW);  // ✅ Pump OFF
      digitalWrite(ledPin, LOW);     // ✅ LED OFF
      pumpRunning = false;
      Blynk.virtualWrite(VPIN_PUMP, 0);
    }
    else if (value >= 30 && pumpRunning) {
      // Soil wet -> Ensure pump is OFF
      digitalWrite(waterPump, LOW);  // ✅ Pump OFF
      digitalWrite(ledPin, LOW);     // ✅ LED OFF
      pumpRunning = false;
      Blynk.virtualWrite(VPIN_PUMP, 0);
    }
  }
}

// Read DHT11 sensor and send to Blynk
void readTempHumidity() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (!isnan(temperature)) {
    Blynk.virtualWrite(VPIN_TEMP, temperature);
  }

  if (!isnan(humidity)) {
    Blynk.virtualWrite(VPIN_HUMIDITY, humidity);
  }
}

void loop() {
  Blynk.run();
  timer.run();
}
