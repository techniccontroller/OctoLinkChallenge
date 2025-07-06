#include <Adafruit_NeoPixel.h>
#include "DFRobotDFPlayerMini.h"
#include <SoftwareSerial.h>

#define SPEAKER_RX 10 // connects to DFPLayer TX pin
#define SPEAKER_TX 11 // connects to DFPLayer RX pin

#define NEOPIXEL_PIN 12 // Pin for NeoPixel data line
#define NUMPIXELS 8 // Number of NeoPixels


#define CHECK_CONNECTION_INTERVAL 100 // Interval to check connections in milliseconds
#define EFFECT_DURATION 20000 // Duration of the final LED effect in milliseconds


// Define different mp3 files
#define MP3_FILE_FANFARE 1    // on SD card, file name: /mp3/0001.mp3
#define MP3_FILE_FAILURE 2    // on SD card, file name: /mp3/0002.mp3
#define MP3_FILE_GOODRESULT 3 // on SD card, file name: /mp3/0003.mp3
#define MP3_FILE_SUCCESS 4    // on SD card, file name: /mp3/0004.mp3
#define MP3_FILE_WINNING 5    // on SD card, file name: /mp3/0005.mp3
#define MP3_FILE_ELEVATION 6  // on SD card, file name: /mp3/0006.mp3
#define MP3_FILE_MAJESTIC 7   // on SD card, file name: /mp3/0007.mp3
#define MP3_FILE_VICTORY 8    // on SD card, file name: /mp3/0008.mp3



const int connectionsInputPins[] = {A0, A1, A2, A3, A4, A5, A6, A7};
const int connectionsOutputPins[] = {2, 3, 4, 5, 6, 7, 8, 9};
const int numberOfConnections = sizeof(connectionsInputPins) / sizeof(connectionsInputPins[0]);

uint8_t connectionStates[numberOfConnections] = {0};
uint8_t previousConnectionStates[numberOfConnections] = {0};

unsigned long previousMillis = 0;
unsigned long previousMillisLEDEffect = 0;
// Animation frame interval (adjust as needed for smoothness)
const unsigned long frameInterval = 100;
unsigned long ledEffectStart = 0;
bool finalEffectActive = false;
int animationStep = 0;
int hueShift = 0;

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

SoftwareSerial softSerial(/*rx =*/SPEAKER_RX, /*tx =*/SPEAKER_TX);
#define FPSerial softSerial
DFRobotDFPlayerMini myDFPlayer;

bool dfPlayerInitialized = false;

uint8_t initDFPlayer() {

  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  const int maxRetries = 5;
  int attempts = 0;

  while (!myDFPlayer.begin(FPSerial, /*isACK = */true, /*doReset = */true)) {
    attempts++;
    Serial.print(F("Attempt "));
    Serial.print(attempts);
    Serial.println(F(" failed to start DFPlayer."));

    if (attempts >= maxRetries) {
      Serial.println(F("Unable to begin:"));
      Serial.println(F("1. Please recheck the connection!"));
      Serial.println(F("2. Please insert the SD card!"));      
      dfPlayerInitialized = false;
      return 1; // Return 1 to indicate failure
    }

    delay(1000); // Wait 1 second before retrying
  }

  Serial.println(F("DFPlayer Mini online."));
  dfPlayerInitialized = true;
  return 0; // Return 0 to indicate success
}

void showLEDSingleColor(uint32_t color, int duration){
  // Set all NeoPixels to the specified color
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, color);
  }
  pixels.show(); // Update the NeoPixels

  delay(duration); // Keep the color for the specified duration

  // Clear the NeoPixels
  pixels.clear();
  pixels.show(); // Update the NeoPixels
}

uint8_t getConnectionState(int index) {
  // write output pin to LOW, then read input pin, if the input pin is LOW, the connection is closed
  digitalWrite(connectionsOutputPins[index], LOW);
  delay(1); // Short delay to ensure the state is read correctly
  uint8_t state = analogRead(connectionsInputPins[index]) > 500; // Read the input pin and check if it's HIGH (open) or LOW (closed)
  digitalWrite(connectionsOutputPins[index], HIGH); // Set the output pin back to HIGH
  return state == 0 ? 1 : 0; // Return 1 if closed, 0 if open
}

void printConnectionStates() {
  Serial.print("connection states: [");
  for (int i = 0; i < numberOfConnections; i++) {
    Serial.print(connectionStates[i]);
    if (i < numberOfConnections - 1) {
      Serial.print(", ");
    }
  }
  Serial.println("]");
}

void playFinishedMelody() {
  //myDFPlayer.playMp3Folder(MP3_FILE_WINNING);
  //myDFPlayer.playMp3Folder(MP3_FILE_FANFARE);
  //myDFPlayer.playMp3Folder(MP3_FILE_MAJESTIC);
  myDFPlayer.playMp3Folder(MP3_FILE_VICTORY);
}

void playConnectionClosedMelody() {
  myDFPlayer.playMp3Folder(MP3_FILE_SUCCESS);
}

void playConnectionOpenedMelody() {
  myDFPlayer.playMp3Folder(MP3_FILE_FAILURE);
}

// This function can be used to run an LED event when a connection is closed
void runLEDEventConnectionClosed(){
  finalEffectActive = false;
  showLEDSingleColor(pixels.Color(0, 255, 0), 500); // Show green color for 500 milliseconds
}

// This function can be used to run an LED event when a connection is opened
void runLEDEventConnectionOpened(){
  finalEffectActive = false;
  showLEDSingleColor(pixels.Color(255, 0, 0), 500); // Show red color for 500 milliseconds
}

void runLEDEventFinished() {
  ledEffectStart = millis();
  finalEffectActive = true;
}

void updateLEDs() {
  if (!finalEffectActive) return;

  unsigned long now = millis();
  // trigger the led animation every 100ms
  if (now - ledEffectStart < EFFECT_DURATION) {
    if (now - previousMillisLEDEffect >= frameInterval) {
      previousMillisLEDEffect = now;
      hueShift = (hueShift + 3) % 256; // Slowly change base hue for non-rainbow modes
      // Change this to the desired LED effect mode
      //modeRainbowChase();
      //modeKnightRider();
      //modeBlinkAll();
      //modeColorWipe();
      modeRandomFlash();
      // You can also use other modes like modeRainbowChase(), modeKnightRider(), etc.
    }
  } else {
    pixels.clear(); 
    finalEffectActive = false;
    myDFPlayer.pause(); // Stop the DFPlayer when the effect ends
    Serial.println("Final LED effect completed.");
  }
  pixels.show();
}

void setup() {
  // Initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  // Initialize the DFPlayer Mini
  FPSerial.begin(9600);

  pixels.begin(); // Initialize NeoPixels
  pixels.clear(); // Clear NeoPixels

  uint8_t res = initDFPlayer();
  if (res != 0) {
    // If DFPlayer initialization failed, blink the NeoPixels red 3 times
    for (int i = 0; i < 3; i++) {
      showLEDSingleColor(pixels.Color(255, 0, 0), 100);
      delay(100);
    }
  }

  // Set all defined connections input pins to INPUT_PULLUP and output pins to OUTPUT
  for (int i = 0; i < numberOfConnections; i++) {
    pinMode(connectionsInputPins[i], INPUT_PULLUP);
    pinMode(connectionsOutputPins[i], OUTPUT);
    digitalWrite(connectionsOutputPins[i], HIGH);
  }
  delay(1000);
  Serial.println("Setup complete. Starting connection checks...");
}

uint8_t getNumClosedConnections(uint8_t *states, int size) {
  uint8_t numClosed = 0;
  for (int i = 0; i < size; i++) {
    if (states[i] == 1) { // Assuming 1 means closed
      numClosed++;
    }
  }
  return numClosed;
}

void loop() {
  unsigned long currentMillis = millis();

  // Check if it's time to read the pins
  if (currentMillis - previousMillis >= CHECK_CONNECTION_INTERVAL) {
    previousMillis = currentMillis;

    // Read the state of each pin and store it in the connectionStates array
    for (int i = 0; i < numberOfConnections; i++) {
      connectionStates[i] = getConnectionState(i);
    }

    uint8_t previousNumClosedConnections = getNumClosedConnections(previousConnectionStates, numberOfConnections);
    uint8_t numClosedConnections = getNumClosedConnections(connectionStates, numberOfConnections);
    bool allConnectionsClosed = false;
    if (numClosedConnections != previousNumClosedConnections) {
      allConnectionsClosed = (numClosedConnections == 4);
    }


    // Check if a connection was closed or opened
    bool connectionOpened = false;
    bool connectionClosed = false;
    for (int i = 0; i < numberOfConnections; i++) {
      if (connectionStates[i] != previousConnectionStates[i]) {
        if (connectionStates[i] == 1) {
          connectionClosed = true; // A connection was closed
        } else {
          connectionOpened = true; // A connection was opened
        }
      }
      previousConnectionStates[i] = connectionStates[i]; // Update the previous state
    }

    // Run LED events based on the connection state changes
    if(allConnectionsClosed) {
      Serial.println("All connections are closed.");
      playFinishedMelody();
      runLEDEventFinished();
    }
    else if (connectionClosed) {
      Serial.println("A connection was closed.");
      playConnectionClosedMelody();
      runLEDEventConnectionClosed();
    }
    else if (connectionOpened) {
      Serial.println("A connection was opened.");
      playConnectionOpenedMelody();
      runLEDEventConnectionOpened();
    }


    printConnectionStates();
  }

  updateLEDs();
}




// --- Mode 0: Rainbow Chase ---
void modeRainbowChase() {
  for (int i = 0; i < NUMPIXELS; i++) {
    int hue = (i * 256 / NUMPIXELS + animationStep) % 256;
    pixels.setPixelColor(i, pixels.ColorHSV(hue * 256));
  }
  pixels.show();
  animationStep = (animationStep + 15) % 256;
}

// --- Mode 1: Knight Rider (Bouncing red dot with shifting hue) ---
void modeKnightRider() {
  static int position = 0;
  static int direction = 1;

  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, 0);
  }

  pixels.setPixelColor(position, pixels.ColorHSV(hueShift * 256, 255, 255));
  pixels.show();

  position += direction;
  if (position == 0 || position == NUMPIXELS - 1) direction = -direction;
}

// --- Mode 2: Blink All (Fades between on and off with changing color) ---
void modeBlinkAll() {
  uint32_t color = (animationStep % 2 == 0) ? pixels.ColorHSV(hueShift * 256, 255, 255) : 0;
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, color);
  }
  pixels.show();
  animationStep++;
}

// --- Mode 3: Color Wipe (LEDs light up one by one with hue shift) ---
void modeColorWipe() {
  static bool turningOn = true;

  if (turningOn) {
    pixels.setPixelColor(animationStep, pixels.ColorHSV(hueShift * 256, 255, 255));
  } else {
    pixels.setPixelColor(animationStep, 0); // Turn off
  }

  pixels.show();
  animationStep++;

  if (animationStep >= NUMPIXELS) {
    animationStep = 0;
    turningOn = !turningOn; // Switch phase
  }
}

// --- Mode 4: Random Flash (Each LED randomly on/off with random hue-shifted colors) ---
void modeRandomFlash() {
  for (int i = 0; i < NUMPIXELS; i++) {
    if (random(10) > 6) {
      int randomHue = (hueShift + random(64)) % 256;
      pixels.setPixelColor(i, pixels.ColorHSV(randomHue * 256, 255, 255));
    } else {
      pixels.setPixelColor(i, 0);
    }
  }
  pixels.show();
}
