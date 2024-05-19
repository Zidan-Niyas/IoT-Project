#define BLYNK_TEMPLATE_ID "TMPL3F26jiLft"
#define BLYNK_TEMPLATE_NAME "Pollution Level detection"
#define BLYNK_AUTH_TOKEN "2y4epecrnMYxWHN_bQem2Atd6F2zVejD"

#define buzzerPin 23
#define SS_PIN 5
#define RST_PIN 0
#define smokeA0 34
#define mq2Pin 35  // Add a pin for the MQ2 sensor
#define greenLedPin 25
#define redLedPin 33

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

bool setupCompleted = false;
bool co2MsgSent = false;
bool smokeMsgSent = false;

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
byte nuidPICC[4];

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Assuming 16x2 LCD

char ssid[] = "Abz";
char pass[] = "1234567890";
char auth[] = BLYNK_AUTH_TOKEN;

struct SensorReadings {
  float co2Ppm;
  float smokeConcentration;
};

float R0 = 10.0;

BlynkTimer timer;


void setup() {
  Serial.begin(9600);
  pinMode(smokeA0, INPUT);
  pinMode(mq2Pin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  Blynk.begin(auth, ssid, pass);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print("Pollution");
  lcd.setCursor(0, 1);
  lcd.print("Detection System");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing.....");
  delay(3000);

  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);

  SPI.begin();
  rfid.PCD_Init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  setupCompleted = true;
  timer.setInterval(3000L, sendSensorReadings);
}


void loop() {
  Blynk.run();
  // timer.run();
  // checkRFID();  // Call RFID detection function separately
  sendSensorReadings();
}

void sendSensorReadings() {
  if (!setupCompleted) return;
  SensorReadings readings = readSensors();
  sendNotifications(readings);
  displayReadings(readings);
  checkRFID(readings);
  delay(1000);
}

SensorReadings readSensors() {
  SensorReadings readings;
  readings.co2Ppm = readCO2Sensor();
  readings.smokeConcentration = readMQ2Sensor();
  return readings;
}

float readCO2Sensor() {
  int sensorValue = analogRead(smokeA0);
  float voltage = sensorValue * (5.0 / 1023.0);
  return pow(10, (1.32 * ((voltage / 5.0) - 0.35)))/10000;
}

float readMQ2Sensor() {
  int sensorValue = analogRead(mq2Pin);
  // Calculate smoke concentration based on sensor value
  float voltage = sensorValue * (5.0 / 1023.0);
  float smokeConcentration = voltage * 10;
  return smokeConcentration;
}

void sendNotifications(SensorReadings readings) {
  if (readings.co2Ppm > 3.5) {
    if (!co2MsgSent) {
      Blynk.logEvent("pollution_alert", "Vehicle Number: KL18H6501\nVehicle Owner : Muhammed Zidan Niyas\nCO VALUE IS TOO HIGH!\nCurrent CO value =  " + String(readings.co2Ppm) + " ppm");
      co2MsgSent = true;
    }
    tone(buzzerPin, 3000);  // Adjust frequency (in Hz) as desired
    delay(1000);
    noTone(buzzerPin);
    
  }
  if (readings.smokeConcentration > 100) {
    if (!smokeMsgSent) {
      Blynk.logEvent("pollution_alert", "Vehicle Number: KL18H6501\nVehicle Owner : Muhammed Zidan Niyas\nSMOKE CONCENTRATION IS TOO HIGH!\nCurrent smoke concentration =  " + String(readings.smokeConcentration) + "mg/m^3");
      smokeMsgSent = true;
    }
    tone(buzzerPin, 3000);  // Adjust frequency (in Hz) as desired
    delay(1000);
    noTone(buzzerPin);
    
  }
}

void displayReadings(SensorReadings readings) {
  lcd.setCursor(0, 0);
  lcd.print("CO: ");
  lcd.print(readings.co2Ppm);
  lcd.print(" ppm");
  lcd.setCursor(0, 1);
  lcd.print("Smoke:");
  lcd.print(readings.smokeConcentration);
  lcd.print("mg/m3");

  Serial.print("CO: ");
  Serial.print(readings.co2Ppm);
  Serial.print(" ppm, Smoke: ");
  Serial.print(readings.smokeConcentration);
  Serial.println(" mg/m^3");
}

void checkRFID(SensorReadings readings) {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if (isAuthorizedTag()) {
      Serial.println("Authorized RFID tag detected!");
      String message = "Authorized RFID Tag Scanned!\nVehicle Number: KL18H6501\nVehicle Owner : Muhammed Zidan Niyas\nCurrent CO: " + String(readings.co2Ppm) + " Smoke: " + String(readings.smokeConcentration);
      Blynk.logEvent("pollution_alert", message);
      lcd.setCursor(0, 0);
      lcd.clear();
      lcd.print("Authorized Tag");
      lcd.setCursor(0, 1);
      lcd.print("Detected!");
      digitalWrite(greenLedPin, HIGH);
      delay(1000);
      digitalWrite(greenLedPin, LOW);
      delay(1000);
      lcd.clear();
      
    } 
    else {
      Serial.println("Unauthorized RFID tag detected.");
      lcd.setCursor(0, 0);
      lcd.clear();
      lcd.print("Unauthorized Tag");
      lcd.setCursor(0, 1);
      lcd.print("Detected!");
      digitalWrite(redLedPin, HIGH);
      delay(1000);
      digitalWrite(redLedPin, LOW);
      delay(1000);
      lcd.clear();

      
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

bool isAuthorizedTag() {
  if (rfid.uid.uidByte[0] == 89 && rfid.uid.uidByte[1] == 86 && rfid.uid.uidByte[2] == 110 && rfid.uid.uidByte[3] == 24) {
    return true;
  }
  return false;
}
