#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// pin definitions as wired by my lazy ass
#define TFT_DC 22
#define TFT_CS 21
#define TFT_MOSI 23
#define TFT_CLK 18
#define TFT_RST 0
#define TFT_MISO 19

#define START_BUTTON 15

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

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
#define TOTAL_TIME 10 // seconds
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

static int current_box_num; // tracking latest unlocked box
static bool show_hint;
static int box_nums[NUM_BOXES]; // randomly selected box numbers, alternating teams
static int clue_nums[NUM_BOXES][NUM_CLUES]; // randomly selected clue numbers
static int hint_nums[NUM_BOXES][NUM_HINTS]; // randomly selected hint numbers
static String chosen_clues[NUM_BOXES][NUM_CLUES];
static String chosen_hints[NUM_BOXES][NUM_HINTS];

void generate_boxes(int *arr, int length, int range);
void generate_clues(int arr[][NUM_CLUES], int length, int range);
void generate_hints(int arr[][NUM_HINTS], int length, int range);

void check_start();
void check_lose();
void start_game();
void update_display();
void clear_screen();

static unsigned long remaining_time; 
static unsigned long start_time;
static unsigned long elapsed_time;  
static bool start;

// setup code
void setup() {
  Serial.begin(9600);
  Serial.println("Hello World");

  current_box_num = 0;
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
  tft.println("Press to Start");
}

void loop() {
  check_start();

  if (start)
  {
    check_lose();
    if (TOTAL_TIME - (millis()-start_time)/1000 <= remaining_time)
    {
      remaining_time = TOTAL_TIME - (millis()-start_time)/1000;
      update_display();
    }
  }

  // at game end: elapsed_time = millis() - start_time;
}

void check_start() {
  if (digitalRead(START_BUTTON) == LOW && !start)
    start_game();
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
    setup();
  }
}

void start_game() {
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
  tft.println(NUM_BOXES/2 - current_box_num);
  tft.setCursor(4, 20);
  tft.setTextColor(WHITE, BLACK);
  tft.print("Remaining Time: ");
  if (remaining_time < 600)
    tft.print(" ");
  if (remaining_time < 120)
    tft.setTextColor(RED, BLACK);
  tft.print(remaining_time/60);
  tft.print(":");
  if (remaining_time < 10)
    tft.print("0");
  tft.println(remaining_time%60);

  tft.setTextWrap(true);
  tft.setTextColor(CYAN, BLACK);
  tft.setTextSize(2);
  tft.setCursor(4, 40);
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

void advance_stage() {
  current_box_num ++;
}