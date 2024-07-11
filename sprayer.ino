#include <ArduinoJson.h>

#define UART2_TX_PIN 49
#define UART2_RX_PIN 50

// Multiplexer control pins
const int enPin = 31; // asw_EN
const int a0Pin = 30; // asw_A0
const int a1Pin = 29; // asw_A1
const int a2Pin = 28; // asw_A2

bool systemOn = false; // System on/off state
String sensor1StatusColor = "red"; // Initial sensor 1 status color
String sensor2StatusColor = "red"; // Initial sensor 2 status color
String sensor3StatusColor = "red"; // Initial sensor 3 status color
String sensor4StatusColor = "red"; // Initial sensor 4 status color
String sensor5StatusColor = "red"; // Initial sensor 5 status color
String sensor6StatusColor = "red"; // Initial sensor 6 status color

int sensor1distance = 0; // Initial sensor 1 distance
int sensor2distance = 0; // Initial sensor 2 distance
int sensor3distance = 0; // Initial sensor 3 distance
int sensor4distance = 0; // Initial sensor 4 distance
int sensor5distance = 0; // Initial sensor 5 distance
int sensor6distance = 0; // Initial sensor 6 distance

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);

  // delay(10000);

  pinMode(enPin, OUTPUT);
  pinMode(a0Pin, OUTPUT);
  pinMode(a1Pin, OUTPUT);
  pinMode(a2Pin, OUTPUT);
}

void loop() {
  delay(50);
  sensor1distance = checkSensorStatus(1, sensor1StatusColor);
  delay(50);
  sensor2distance = checkSensorStatus(2, sensor2StatusColor);
  delay(50);
  sensor3distance = checkSensorStatus(3, sensor3StatusColor);
  delay(50);
  sensor4distance = checkSensorStatus(4, sensor4StatusColor);
  delay(50);
  sensor5distance = checkSensorStatus(5, sensor5StatusColor);
  delay(50);
  sensor6distance = checkSensorStatus(6, sensor6StatusColor);

  display_measures();
}


void selectSensor(int sensor) {
  digitalWrite(enPin, HIGH);
  switch (sensor) {
    case 1:
      digitalWrite(a0Pin, LOW);
      digitalWrite(a1Pin, LOW);
      digitalWrite(a2Pin, LOW);
      break;
    case 2:
      digitalWrite(a0Pin, HIGH);
      digitalWrite(a1Pin, LOW);
      digitalWrite(a2Pin, LOW);
      break;
    case 3:
      digitalWrite(a0Pin, LOW);
      digitalWrite(a1Pin, HIGH);
      digitalWrite(a2Pin, LOW);
      break;
    case 4:
      digitalWrite(a0Pin, HIGH);
      digitalWrite(a1Pin, HIGH);
      digitalWrite(a2Pin, LOW);
      break;
    case 5:
      digitalWrite(a0Pin, LOW);
      digitalWrite(a1Pin, LOW);
      digitalWrite(a2Pin, HIGH);
      break;
    case 6:
      digitalWrite(a0Pin, HIGH);
      digitalWrite(a1Pin, LOW);
      digitalWrite(a2Pin, HIGH);
      break;
    default:
      Serial.println("Invalid sensor number");
      break;
  }
}

int checkSensorStatus(int sensorNumber, String &sensorStatusColor) {
  // Serial.println("Sensor: " + String(sensorNumber));
  selectSensor(sensorNumber);
  Serial2.write(0x01); //send one byte to sensor to request a measurement
  delay(100); //waiting for a reply
  int sensorDistance = 0;

  if (Serial2.available() > 0) {
      uint8_t data[4];
      int index = 0;
      while (Serial2.available() > 0 && index < 4) {
        data[index++] = Serial2.read();
      }
      if (index == 4 && data[0] == 0xFF) {
        sensorDistance = (data[1] << 8) + data[2];
        // Serial.println("Distance from sensor" + String(sensorNumber) + ": " + String(sensorDistance) + " mm");
        sensorStatusColor = "green";
      } else {
        sensorStatusColor = "orange";
      }
    } else {
      sensorStatusColor = "red";
    }
      // Serial.println("Status from sensor" + String(sensorNumber) + ": " + sensorStatusColor);
      return sensorDistance;
  }

void display_measures(){
  Serial.println("Sensor 1: " + sensor1StatusColor + " - " + String(sensor1distance) + "mm");
  Serial.println("Sensor 2: " + sensor2StatusColor + " - " + String(sensor2distance) + "mm");
  Serial.println("Sensor 3: " + sensor3StatusColor + " - " + String(sensor3distance) + "mm");
  Serial.println("Sensor 4: " + sensor4StatusColor + " - " + String(sensor4distance) + "mm");
  Serial.println("Sensor 5: " + sensor5StatusColor + " - " + String(sensor5distance) + "mm");
  Serial.println("Sensor 6: " + sensor6StatusColor + " - " + String(sensor6distance) + "mm");
}
  
