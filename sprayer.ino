#include "WiFiC3.h"
#include "WiFiClient.h"
#include "WiFiServer.h"
#include <ArduinoJson.h>

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = "SECRET_SSID";    // your network SSID (name)
char pass[] = "SECRET_PASS";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;               // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

WiFiServer server(80);

#define UART2_TX_PIN 49
#define UART2_RX_PIN 50

// Multiplexer control pins
const int enPin = 31; // asw_EN
const int a0Pin = 30; // asw_A0
const int a1Pin = 29; // asw_A1
const int a2Pin = 28; // asw_A2

bool systemOn = false; // System on/off state
String sensor1StatusColor = "red"; // Initial sensor 1 status color
int sensor1distance = 0; // Initial sensor 1 distance

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Access Point Web Server");

  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);

  if (strlen(pass) < 8) {
    Serial.println("Creating access point failed");
    Serial.println("The Wi-Fi password must be at least 8 characters long");
    while (true);
  }

  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed, status: " + String(status));
    while (true);
  }

  delay(10000);

  server.begin();
  printWiFiStatus();

  pinMode(enPin, OUTPUT);
  pinMode(a0Pin, OUTPUT);
  pinMode(a1Pin, OUTPUT);
  pinMode(a2Pin, OUTPUT);
}

void loop() {
  if (status != WiFi.status()) {
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      Serial.println("Device connected to AP");
    } else {
      Serial.println("Device disconnected from AP");
    }
  }

  WiFiClient client = server.available();

  if (client) {
    Serial.println("New client connected");
    String currentLine = "";
    bool isGetDataRequest = false;
    bool isToggleSystemRequest = false;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);

        if (c == '\n') {
          if (currentLine.length() == 0) {
            if (isGetDataRequest) {
              sendSensorData(client);
            } else if (isToggleSystemRequest) {
              systemOn = !systemOn;
              digitalWrite(LEDR, systemOn ? HIGH : LOW);
              digitalWrite(LEDG, systemOn ? LOW : HIGH);
              digitalWrite(LEDB, HIGH);
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/plain");
              client.println("Connection: keep-alive");
              client.println();
              client.print("System toggled");
            } else {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: keep-alive");
              client.println("Keep-Alive: timeout=30000, max=100000");
              client.println();
              client.print("<html><head>");
              client.print("<style>* { font-family: sans-serif;} body { padding: 2em; font-size: 2em; text-align: center;} ");
              client.print("a { -webkit-appearance: button;-moz-appearance: button;appearance: button;text-decoration: none;color: initial; padding: 25px;} ");
              client.print("#onOffButton{color:" + String(systemOn ? "red" : "green") + ";}");
              client.print("</style>");
              client.print("<script>");
              client.print("function toggleSystem() { fetch('/toggleSystem').then(response => response.text()).then(data => { console.log(data); updateStatus(); }); }");
              client.print("function updateStatus() { fetch('/data').then(response => response.json()).then(data => { ");
              client.print("document.getElementById('sensorButton1').innerText = 'Sensor 1 Status: ' + data.sensor1StatusColor + ' - Distance: ' + data.sensor1distance + 'mm';");
              client.print("}); }");
              client.print("setInterval(updateStatus, 1000);");
              client.print("</script>");
              client.print("</head><body><h1> SYSTEM CONTROLS </h1>");
              client.print("<button onclick='toggleSystem()' id='onOffButton'>" + String(systemOn ? "Turn OFF" : "Turn ON") + "</button>");
              client.print("<div style='display: flex; flex-direction: column; margin: 1em;'>");
              client.print("<span id='sensorButton1'>Sensor 1 Status: " + sensor1StatusColor + " - Distance: " + sensor1distance + "mm</span>");
              client.print("</div></body></html>");
              client.println();
            }
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        if (currentLine.endsWith("GET /data")) {
          isGetDataRequest = true;
        }
        if (currentLine.endsWith("GET /toggleSystem")) {
          isToggleSystemRequest = true;
        }
      }
    }
    client.stop();
    Serial.println("Client disconnected");
  }

  if (systemOn) {
    checkSensorStatus(1, sensor1StatusColor, sensor1distance);
  }
}

void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
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
    default:
      Serial.println("Invalid sensor number");
      break;
  }
}

void checkSensorStatus(int sensorNumber, String &sensorStatusColor, int &sensorDistance) {
  selectSensor(sensorNumber);
  Serial2.write(0x01); //send one byte to sensor to request a measurement
  delay(50); //waiting for a reply

  if (Serial2.available() > 0) {
    uint8_t data[4];
    int index = 0;
    while (Serial2.available() > 0 && index < 4) {
      data[index++] = Serial2.read();
    }
    if (index == 4 && data[0] == 0xFF) {
      uint16_t distance = (data[1] << 8) + data[2];
      Serial.print("Distance from sensor: ");
      Serial.print(distance);
      Serial.println(" mm");
      sensorStatusColor = "green";
      sensorDistance = distance;
      Serial.print("Status: ");
      Serial.println(sensorStatusColor);
    } else {
      sensorStatusColor = "orange";
      Serial.print("Status: ");
      Serial.println(sensorStatusColor);
    }
  } else {
    sensorStatusColor = "red"; 
    Serial.print("Status: ");
    Serial.println(sensorStatusColor);
  }
}

void sendSensorData(WiFiClient client) {
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["sensor1StatusColor"] = sensor1StatusColor;
  jsonDoc["sensor1distance"] = sensor1distance;
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: keep-alive");
  client.println();
  client.print(jsonString);
}
