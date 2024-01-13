/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/communication-between-two-esp32
 */

// ESP32: TCP CLIENT + A BUTTON/SWITCH
#include <WiFi.h>

#define BUTTON_PIN 21 // ESP32 pin GPIO21 connected to button


const char* ssid = "Test";     // CHANGE TO YOUR WIFI SSID
const char* password = "password"; // CHANGE TO YOUR WIFI PASSWORD
const char* serverAddress = "172.20.10.11"; // CHANGE TO ESP32#2'S IP ADDRESS
const int serverPort = 4080;

WiFiClient TCPclient;

void setup() {
  Serial.begin(9600);

  Serial.println("ESP32: TCP CLIENT + A BUTTON/SWITCH");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // connect to TCP server (Arduino #2)
  if (TCPclient.connect(serverAddress, serverPort)) {
    Serial.println("Connected to TCP server");
  } else {
    Serial.println("Failed to connect to TCP server");
  }
}

void loop() {
  if (!TCPclient.connected()) {
    Serial.println("Connection is disconnected");
    TCPclient.stop();

    // reconnect to TCP server (Arduino #2)
    if (TCPclient.connect(serverAddress, serverPort)) {
      Serial.println("Reconnected to TCP server");
    } else {
      Serial.println("Failed to reconnect to TCP server");
    }
  }

  char message = 'a';
  TCPclient.write(message);
  delay(10);
  TCPclient.flush();
  delay(10);
  Serial.println(message);
}
