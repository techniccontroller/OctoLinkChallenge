#include <Adafruit_NeoPixel.h>

#define SPEAKER_RX 10 // connects to DFPLayer TX pin
#define SPEAKER_TX 11 // connects to DFPLayer RX pin
#define SPEAKER_PIN 11

#define NEOPIXEL_PIN 12 // Pin for NeoPixel data line
#define NUMPIXELS 8 // Number of NeoPixels

// Frequency definitions for common notes
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047

const int connectionsInputPins[] = {A0, A1, A2, A3, A4, A5, A6, A7};
const int connectionsOutputPins[] = {2, 3, 4, 5, 6, 7, 8, 9};
const int numberOfConnections = sizeof(connectionsInputPins) / sizeof(connectionsInputPins[0]);


int melodyFinished[] = {NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C6};
int durationsFinished[] = {4, 4, 4, 2};  // in note values (e.g. quarter = 4)



uint8_t connectionStates[numberOfConnections] = {0};
uint8_t previousConnectionStates[numberOfConnections] = {0};

unsigned long previousMillis = 0;
unsigned long previousMillisLEDEffect = 0;
const long interval = 100;
const long ledEffectInterval = 100; // Interval for LED effects

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// State machines
bool ledEffectActive = false;
bool soundEffectActive = false;
int ledEffectType = 0;
int soundEffectType = 0;
unsigned long ledEffectStart = 0;
unsigned long soundEffectStart = 0;
int soundEffectStep = 0;
int animationStep = 0;
int hueShift = 0;
int* currentMelody = nullptr;
int* currentDurations = nullptr;
int totalNotes = 0;

// LED effect types
#define LED_EFFECT_NONE 0
#define LED_EFFECT_CLOSED 1
#define LED_EFFECT_OPENED 2
#define LED_EFFECT_FINISHED 3

// Sound effect types
#define SOUND_EFFECT_NONE 0
#define SOUND_EFFECT_CLOSED 1
#define SOUND_EFFECT_OPENED 2
#define SOUND_EFFECT_FINISHED 3




uint8_t getConnectionState(int index) {
  digitalWrite(connectionsOutputPins[index], LOW);
  delay(1);
  uint8_t state = analogRead(connectionsInputPins[index]) > 500;
  digitalWrite(connectionsOutputPins[index], HIGH);
  return state == 0 ? 1 : 0;
}

void triggerEvents(int ledType, int soundType) {
  currentMelody = melodyFinished;
  currentDurations = durationsFinished;
  totalNotes = sizeof(melodyFinished) / sizeof(melodyFinished[0]);

  ledEffectType = ledType;
  soundEffectType = soundType;

  ledEffectStart = millis();
  soundEffectStart = millis();
  animationStep = 0;
  soundEffectStep = 0;

  ledEffectActive = true;
  soundEffectActive = true;
}

void updateLEDEffects() {
  if (!ledEffectActive) return;

  unsigned long now = millis();

  switch (ledEffectType) {
    case LED_EFFECT_CLOSED:
      if (now - ledEffectStart < 500) {
        for (int i = 0; i < NUMPIXELS; i++) pixels.setPixelColor(i, pixels.Color(0, 255, 0));
      } else {
        pixels.clear(); ledEffectActive = false;
      }
      break;
    case LED_EFFECT_OPENED:
      if (now - ledEffectStart < 500) {
        for (int i = 0; i < NUMPIXELS; i++) pixels.setPixelColor(i, pixels.Color(255, 0, 0));
      } else {
        pixels.clear(); ledEffectActive = false;
      }
      break;
    case LED_EFFECT_FINISHED:
      // trigger the led animation every 100ms
      if (now - ledEffectStart < 2000) {
        if (now - previousMillisLEDEffect >= ledEffectInterval) {
          previousMillisLEDEffect = now;
          hueShift = (hueShift + 3) % 256; // Slowly change base hue for non-rainbow modes
          modeRandomFlash(); // Change this to any other mode if needed
          // You can also use other modes like modeRainbowChase(), modeKnightRider(), etc.
        }
      } else {
        pixels.clear(); ledEffectActive = false;
      }
      break;
  }
  pixels.show();
}

void updateSoundEffects() {
  if (!soundEffectActive) return;
  unsigned long now = millis();

  switch (soundEffectType) {
    case SOUND_EFFECT_CLOSED:
      switch (soundEffectStep) {
        case 0: tone(SPEAKER_PIN, NOTE_E5, 100); soundEffectStart = now; soundEffectStep++; break;
        case 1: if (now - soundEffectStart > 120) { tone(SPEAKER_PIN, NOTE_G5, 100); soundEffectStart = now; soundEffectStep++; } break;
        case 2: if (now - soundEffectStart > 120) { noTone(SPEAKER_PIN); soundEffectActive = false; } break;
      }
      break;
    case SOUND_EFFECT_OPENED:
      switch (soundEffectStep) {
        case 0: tone(SPEAKER_PIN, NOTE_G5, 100); soundEffectStart = now; soundEffectStep++; break;
        case 1: if (now - soundEffectStart > 120) { tone(SPEAKER_PIN, NOTE_E5, 100); soundEffectStart = now; soundEffectStep++; } break;
        case 2: if (now - soundEffectStart > 120) { noTone(SPEAKER_PIN); soundEffectActive = false; } break;
      }
      break;
    case SOUND_EFFECT_FINISHED:
      if (soundEffectStep < totalNotes) {
        if (soundEffectStep == 0 || ((now - soundEffectStart) > uint16_t(1000 / currentDurations[soundEffectStep - 1] + 20))) {
          int duration = 1000 / currentDurations[soundEffectStep];
          tone(SPEAKER_PIN, currentMelody[soundEffectStep], duration);
          soundEffectStart = now;
          soundEffectStep++;
        }
      } else {
        if (now - soundEffectStart > 100) { // short delay after final note
          noTone(SPEAKER_PIN);
          soundEffectActive = false;
        }
      }
      break;
  }
}

void setup() {
  // Initialize serial communication at 9600 bits per second:
  Serial.begin(115200);

  pixels.begin(); // Initialize NeoPixels
  pixels.clear(); // Clear NeoPixels

  // Set all defined input pins as INPUT with pull-up resistors
  for (int i = 0; i < numberOfConnections; i++) {
    pinMode(connectionsInputPins[i], INPUT_PULLUP);
    pinMode(connectionsOutputPins[i], OUTPUT);
    digitalWrite(connectionsOutputPins[i], HIGH);
  }
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

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    for (int i = 0; i < numberOfConnections; i++) {
      connectionStates[i] = getConnectionState(i);
    }

    uint8_t previousNumClosed = getNumClosedConnections(previousConnectionStates, numberOfConnections);
    uint8_t currentNumClosed = getNumClosedConnections(connectionStates, numberOfConnections);

    bool connectionOpened = false;
    bool connectionClosed = false;

    for (int i = 0; i < numberOfConnections; i++) {
      if (connectionStates[i] != previousConnectionStates[i]) {
        if (connectionStates[i] == 1) connectionClosed = true;
        else connectionOpened = true;
      }
      previousConnectionStates[i] = connectionStates[i];
    }

    if (connectionClosed) triggerEvents(LED_EFFECT_CLOSED, SOUND_EFFECT_CLOSED);
    if (connectionOpened) triggerEvents(LED_EFFECT_OPENED, SOUND_EFFECT_OPENED);
    if (currentNumClosed == 4 && currentNumClosed != previousNumClosed) {
      triggerEvents(LED_EFFECT_FINISHED, SOUND_EFFECT_FINISHED);
    }
  }

  updateLEDEffects();
  updateSoundEffects();
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