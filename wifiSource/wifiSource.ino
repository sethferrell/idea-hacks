/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/communication-between-two-esp32
 */

// ESP32: TCP CLIENT + A BUTTON/SWITCH
#include <WiFi.h>

#define SERVER_PORT 80

const char* ssid = "Test";     // CHANGE TO YOUR WIFI SSID
const char* password = "password"; // CHANGE TO YOUR WIFI PASSWORD
const char* serverAddress = "172.20.10.11"; // CHANGE TO ESP32#2'S IP ADDRESS
const int serverPort = 80;

WiFiServer TCPserver(SERVER_PORT);
WiFiClient TCPclient;

void setup() {
  Serial.begin(9600);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Print your local IP address:
  Serial.print("ESP32 #2: TCP Server IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("ESP32 #1: -> Please update the serverAddress in ESP32 #2 code");
  // Start listening for a TCP client (from ESP32 #1)
  TCPserver.begin();

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

  // Wait for a TCP client from ESP32 #1:
  WiFiClient client = TCPserver.available();
  if (client) {
    // Read the command from the TCP client:
    char command = client.read();
    Serial.print("ESP32 #2: - Received command: ");
    Serial.println(command);
    client.stop();
  }

  sendMessage('a');
}

void sendMessage(char message) {
  TCPclient.write(message);
  delay(10);
  TCPclient.flush();
  delay(10);
}
