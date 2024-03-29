// Import required libraries
#include "WiFi.h"
#include <AsyncFsWebServer.h>
#include <HTTPClient.h>

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5   // ESP32 pin GPIO5
#define RST_PIN 4  // ESP32 pin GPIO27
MFRC522 rfid(SS_PIN, RST_PIN);

// Set your access point network credentials
const char* ssid = "ESP32-Access-Point-1";
const char* password = "123456789";

// other server's access points
const char* other_ssid = "ESP32-Access-Point-2";
const char* other_password = "123456789";
const char* serverName = "http://192.168.4.1/two";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String messageToSend = "default";
bool gameOver = false;
unsigned long mytime;

/////////////////////////////////////////////////////////// REMOVE THIS LATER WHEN MERGING W/ OLED CODE!!!!!!!!!!!!!!!!!!!!!!
unsigned long gameTime = 100;

// leaderboard stuff
struct LeaderboardEntry {
  String name;
  unsigned long time;
};

struct LeaderboardEntry leaderboard[10];
String leaderboardHTML = "";

void setup(){
  setupLeaderboard();

  // RFID setup
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522

  // Serial port for debugging purposes
  Serial.begin(9600);
  Serial.println();
  
  // Setting the ESP as an access point
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // url to send game data
  server.on("/one", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", messageToSend.c_str());
  });

  // url for leaderboard
  server.on("/leaderboard", HTTP_GET, [](AsyncWebServerRequest *request){
    //request->send_P(200, "text/plain", messageToSend.c_str());
    request->send_P(200, "text/html", leaderboardHTML.c_str());
  });
  
  // Start server
  server.begin();

  // connecting to the other network:
  connectToOtherTeam();
}
 
void loop(){
  updateLeaderboard();
  if (gameOver) {
    messageToSend = "youwon";
    Serial.println("You Won!");
    return;
  }

  // receiving message from the other esp
  String message;
  if(WiFi.status()== WL_CONNECTED){ 
    message = httpGETRequest(serverName);
    Serial.println(message);
  }
  else {
    Serial.println("Wifi disconnected");
  }

  // resetting/winning logic
  if (message == "default") {
    messageToSend = "default";
  }
  else if (message == "youwon") {
    messageToSend = "youwon";
    return;
  }

  // game over logic
  if (messageToSend == "scanned" && message == "scanned") {
    gameOver = true;
    return;
  }
  else if (message == "scanned") {
    mytime = millis();
    while (millis() <= mytime + 5000) {
      Serial.println("waiting");
      if (rfid.PICC_IsNewCardPresent()) {
        gameOver = true;
        messageToSend = "youwon";
        return;
      }
    }

    messageToSend = "default";
    return;
  }
  else if (rfid.PICC_IsNewCardPresent()) {
    messageToSend = "scanned";
    delay(1000);
  }
}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "--"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void setupLeaderboard() {
  for (int i = 0; i < 10; ++i) {
    leaderboard[i].name = "";
    leaderboard[i].time = 0;
  }
}

void updateLeaderboard() {
  leaderboardHTML = "<!DOCTYPE html><html><head><title>LEADERBOARD</title></head><body>";

  int i;
  for (i = 0; i < 10; ++i) {
    if (leaderboard[i].name == "" && leaderboard[i].time == 0) {
      break;
    }
    else if (leaderboard[i].time < gameTime) {
      leaderboardHTML += leaderboard[i].name + "   ...   " + String(leaderboard[i].time);
      continue;
    }
    else if (leaderboard[i].time > gameTime) {
      break;
    }
  }

  leaderboardHTML += "</body></html>";
}

void connectToOtherTeam() {
  WiFi.begin(other_ssid, other_password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}