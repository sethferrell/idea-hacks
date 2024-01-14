// Import required libraries
#include "WiFi.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#include <AsyncFsWebServer.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>

// PIN DEFINITIONS
// rfid
#define RST_PIN 4
#define SS_PIN 5

// oled
#define TFT_DC 26
#define TFT_CS 27//5
#define TFT_MOSI 13 //23
#define TFT_CLK 14 //18
#define TFT_RST 0
#define TFT_MISO 12 //19

#define START_BUTTON 15
#define HINT_BUTTON 17

// rfid initialization
// MFRC522_I2C rfid(0x28, RST_PIN);
MFRC522 rfid(SS_PIN, RST_PIN);

// oled initialization
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

// OLED DEFINITIONS + GAME PARAMETERS
// define colors
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF

// game parameters
#define TOTAL_TIME 120 // seconds
#define TOTAL_BOXES 5
#define NUM_BOXES 4 // total boxes required per team
#define TOTAL_CLUES 2 // number of clues possible per box
#define NUM_CLUES 1 // number of clues implemented in game
#define TOTAL_HINTS 1 // number of hints possible per box
#define NUM_HINTS 1 // number of hints implemented in game

// clues
static const String clues_array[TOTAL_BOXES][TOTAL_CLUES] PROGMEM = { 
  {"Box 1, Clue 1: ", "Box 1, Clue 2: "},
  {"Box 2, Clue 1: ", "Box 2, Clue 2: "},
  {"Box 3, Clue 1: ", "Box 3, Clue 2: "},
  {"Box 4, Clue 1: ", "Box 4, Clue 2: "},
  {"Box 5, Clue 1: ", "Box 5, Clue 2: "}
};

// hints
static const String hints_array[TOTAL_BOXES][TOTAL_HINTS] PROGMEM = {
  {"Box 1, Hint 1: "},
  {"Box 2, Hint 1: "},
  {"Box 3, Hint 1: "},
  {"Box 4, Hint 1: "},
  {"Box 5, Hint 1: "}
};

// game parameters, continued
static int current_box_num; // tracking latest unlocked box
static bool show_hint;
static int box_nums[NUM_BOXES]; // randomly selected box numbers, alternating teams
static int clue_nums[NUM_BOXES][NUM_CLUES]; // randomly selected clue numbers
static int hint_nums[NUM_BOXES][NUM_HINTS]; // randomly selected hint numbers
static String chosen_clues[NUM_BOXES][NUM_CLUES];
static String chosen_hints[NUM_BOXES][NUM_HINTS];

// function declarations
void generate_boxes(int *arr, int length, int range);
void generate_clues(int arr[][NUM_CLUES], int length, int range);
void generate_hints(int arr[][NUM_HINTS], int length, int range);

void setupOLED();

void check_start();
void check_lose();
void check_hint();
void start_game();
void update_display();
void clear_screen();

static unsigned long remaining_time; 
static unsigned long start_time;
static unsigned long elapsed_time;  
static bool start;
static bool scan_complete;

// NETWORKING STUFF
// Set your access point network credentials
const char* ssid = "ESP32-Access-Point-2";
const char* password = "123456789";

// other server's access points
const char* other_ssid = "ESP32-Access-Point-1";
const char* other_password = "123456789";
const char* serverName = "http://192.168.4.1/one";
const char* statusPage = "http://192.168.4.1/status";
const char* gameTime = "http://192.168.4.1/time";


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String messageToSend = "default";
String message = "default";
unsigned long mytime;

void setup(){
  // Serial port for debugging purposes
  Serial.begin(9600);
  Serial.println();

  // RFID setup
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522

  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(HINT_BUTTON, INPUT_PULLUP);

  // oled setup
  setupOLED();
  
  // Setting the ESP as an access point
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // url to send game data
  server.on("/two", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", messageToSend.c_str());
  });
  
  // Start server
  server.begin();

  // connecting to the other network:
  connectToOtherTeam();
}
 
void loop(){
  check_start();
  Serial.println("remaining_time: " + String(remaining_time));
  if (start)
  {
    check_scan();
    check_win();
    check_lose();
    check_hint();
    if (!start) return;
    if (TOTAL_TIME - (millis()-start_time)/1000 <= remaining_time)
    {
      remaining_time = TOTAL_TIME - (millis()-start_time)/1000;
      update_display();
    }
  }
  else return;

  // if (messageToSend == "scan_complete") {
  //   Serial.println("Scan Complete");
  //   return;
  // }

  // receiving message from the other esp
  if(WiFi.status()== WL_CONNECTED){ 
    message = httpGETRequest(serverName);
    Serial.println(message);
  }
  else {
    Serial.println("Wifi disconnected");
  }

  // resetting/winning logic
  // if (message == "default") {
  //   messageToSend = "default";
  // }
  // else if (message == "scan_complete") {
  //   messageToSend = "scan_complete";
  //   return;
  // }

  // game over logic
  // if (messageToSend == "scanned" && message == "scanned") {
  //   messageToSend = "scan_complete";
  //   return;
  // }
  // else if (message == "scanned") {
  //   mytime = millis();
  //   while (millis() <= mytime + 5000) {
  //     Serial.println("waiting");
  //     if (rfid.PICC_IsNewCardPresent()) {
  //       messageToSend = "scan_complete";
  //       return;
  //     }
  //   }

  //   messageToSend = "default";
  //   return;
  // }
  // else if (rfid.PICC_IsNewCardPresent()) {
  //   messageToSend = "scanned";
  //   delay(1000);
  // }
  
  if (rfid.PICC_IsNewCardPresent()) {
    messageToSend = "scanned";
    mytime = millis();
  }
  if (messageToSend == "scanned" && millis() <= mytime + 5000) {
    Serial.println("waiting");
    if (message == "scanned") {
      scan_complete = true;
      delay(500);
      messageToSend = "default";
      message = "default";
    }
  } else if (millis() > mytime + 5000)
    messageToSend = "default";
  

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

void setupOLED() {
  scan_complete = false;
  messageToSend = "default";
  message = "default";

  current_box_num = NUM_BOXES/2;
  show_hint = false;
  remaining_time = TOTAL_TIME;
  elapsed_time = 0;
  start = false;

  tft.begin();
  tft.setRotation(1);
  clear_screen();

  generate_boxes(box_nums, NUM_BOXES, TOTAL_BOXES);   // select boxes to put in play
  generate_clues(clue_nums, NUM_BOXES, TOTAL_CLUES);
  generate_hints(hint_nums, NUM_BOXES, TOTAL_HINTS);

  for (int i = 0; i < NUM_BOXES; i++)
    for (int j = 0; j < NUM_CLUES; j++)
    {
      chosen_clues[i][j] = clues_array[i][clue_nums[i][j]];
      Serial.println(chosen_clues[i][j]);
    }

  for (int i = 0; i < NUM_BOXES; i++)
    for (int j = 0; j < NUM_HINTS; j++)
    {
      chosen_hints[i][j] = hints_array[i][hint_nums[i][j]];
      Serial.println(chosen_hints[i][j]);
    }
  
  tft.setCursor(20, 20);
  tft.setTextSize(2);
  tft.setTextColor(WHITE, BLACK);
  tft.println("Wait for Team 1 to Start");
}

void check_start() {
  message = httpGETRequest(statusPage);
  if (message == "true" && !start)
    start_game();
}

void check_scan() {
  if (scan_complete)
  {
    clear_screen();
    scan_complete = false;
    show_hint = false;
    current_box_num ++;
  }
}

void check_win() {
  if (httpGETRequest(statusPage) == "game over"){
    start = false;
    clear_screen();
    tft.setCursor(20, 20);
    tft.setTextSize(4);
    tft.setTextColor(GREEN, BLACK);
    tft.println("YOU WIN!!!");
    tft.setCursor(20, 80);
    tft.setTextSize(2);
    tft.print("Elapsed Time: ");
    tft.print(httpGETRequest(gameTime));
    delay(10000); 
    clear_screen();
    setupOLED();
    }
}

void check_lose() {
  if (remaining_time <= 0)
  {
    start = false;
    remaining_time = TOTAL_TIME;
    clear_screen();
    tft.setCursor(20, 20);
    tft.setTextSize(4);
    tft.setTextColor(RED, BLACK);
    tft.println("YOU LOSE");
    delay(10000); 
    setupOLED();
  }
}

void check_hint() {
  if (digitalRead(HINT_BUTTON) == LOW && !show_hint)
    show_hint = true;
}

void start_game() {
  clear_screen();
  start_time = millis();
  start = true;
}

// display text
void update_display() {
  tft.setCursor(4,4);
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  tft.print("Remaining Safes: ");
  tft.setTextColor(GREEN, BLACK);
  tft.println(NUM_BOXES - current_box_num);
  tft.setCursor(4, 20);
  tft.setTextColor(WHITE, BLACK);
  tft.print("Remaining Time: ");
  if (remaining_time < 600)
    tft.print(" ");
  if (remaining_time < 120)
    tft.setTextColor(RED, BLACK);
  tft.print(remaining_time/60);
  tft.print(":");
  if (remaining_time%60 < 10)
    tft.print("0");
  tft.println(remaining_time%60);

  tft.setTextWrap(true);
  tft.setTextColor(CYAN, BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 40);
  for (int i = 0; i < NUM_CLUES; i++)
    tft.println(chosen_clues[current_box_num][i]);
  if (show_hint)
    for (int i = 0; i < NUM_HINTS; i++)
      tft.println(chosen_hints[current_box_num][i]);
}

// draws full black screen
void clear_screen() {
  tft.fillScreen(BLACK);
}

// generate random array
void generate_boxes(int *arr, int length, int range) {
  for (int i = 0; i < length; i++) {
    int randomNumber;
    bool isUnique;

    do {
      isUnique = true;
      randomNumber = random(range); 
      for (int j = 0; j < i; j++) {
        if (arr[j] == randomNumber) {
          isUnique = false;
          break;
        }
      }
    } while (!isUnique);

    arr[i] = randomNumber;
  }
}

void generate_clues(int arr[][NUM_CLUES], int length, int range) {
  for (int i = 0; i < length; i++) {
    for (int j = 0; j < NUM_CLUES; j++) {
      int randomNumber;
      bool isUnique;

      do {
        isUnique = true;
        randomNumber = random(range);
        for (int k = 0; k < j; k++) {
          if (arr[i][k] == randomNumber) {
            isUnique = false;
            break;
          }
        }
      } while (!isUnique);

      arr[i][j] = randomNumber;
    }
  }
}

void generate_hints(int arr[][NUM_HINTS], int length, int range) {
  for (int i = 0; i < length; i++) {
    for (int j = 0; j < NUM_HINTS; j++) {
      int randomNumber;
      bool isUnique;

      do {
        isUnique = true;
        randomNumber = random(range);
        for (int k = 0; k < j; k++) {
          if (arr[i][k] == randomNumber) {
            isUnique = false;
            break;
          }
        }
      } while (!isUnique);

      arr[i][j] = randomNumber;
    }
  }
}