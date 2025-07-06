#include <Adafruit_NeoPixel.h>

#define SPEAKER_RX 10 // connects to DFPLayer TX pin
#define SPEAKER_TX 11 // connects to DFPLayer RX pin

#define NEOPIXEL_PIN 12 // Pin for NeoPixel data line
#define NUMPIXELS 8 // Number of NeoPixels

const int connectionsInputPins[] = {A0, A1, A2, A3, A4, A5, A6, A7};
const int connectionsOutputPins[] = {2, 3, 4, 5, 6, 7, 8, 9};
const int numberOfConnections = sizeof(connectionsInputPins) / sizeof(connectionsInputPins[0]);

uint8_t connectionStates[numberOfConnections] = {0};
uint8_t previousConnectionStates[numberOfConnections] = {0};

unsigned long previousMillis = 0;
const long interval = 100; // Interval to check pins (100 milliseconds)
// Animation frame interval (adjust as needed for smoothness)
const unsigned long frameInterval = 100;

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint8_t getConnectionState(int index) {
  // write output pin to LOW, then read input pin, if the input pin is LOW, the connection is closed
  digitalWrite(connectionsOutputPins[index], LOW);
  delay(1); // Short delay to ensure the state is read correctly
  uint8_t state = analogRead(connectionsInputPins[index]) > 500; // Read the input pin and check if it's HIGH (open) or LOW (closed)
  digitalWrite(connectionsOutputPins[index], HIGH); // Set the output pin back to HIGH
  return state == 0 ? 1 : 0; // Return 1 if closed, 0 if open
}

// This function can be used to run an LED event when a connection is closed
void runLEDEventConnectionClosed(){
  // For example, you can light up the NeoPixels in a specific color
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 255, 0)); // Set to green color
  }
  pixels.show(); // Update the NeoPixels
  delay(500); // Keep the color for 500 milliseconds
  pixels.clear(); // Clear the NeoPixels
  pixels.show(); // Update the NeoPixels
}

// This function can be used to run an LED event when a connection is opened
void runLEDEventConnectionOpened(){
  // For example, you can light up the NeoPixels in a different color
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 0, 0)); // Set to red color
  }
  pixels.show(); // Update the NeoPixels
  delay(500); // Keep the color for 500 milliseconds
  pixels.clear(); // Clear the NeoPixels
  pixels.show(); // Update the NeoPixels
}

void runLEDEventFinished() {
  modeRandomFlash(5000); // Run the random flash mode for 5 seconds
  pixels.clear(); // Clear NeoPixels after the event
  pixels.show(); // Update the NeoPixels
}

void setup() {
  // Initialize serial communication at 9600 bits per second:
  Serial.begin(115200);

  pixels.begin(); // Initialize NeoPixels
  pixels.clear(); // Clear NeoPixels

  // Set all defined input pins as INPUT with pull-up resistors
  for (int i = 0; i < numberOfConnections; i++) {
    pinMode(connectionsInputPins[i], INPUT_PULLUP);
  }

  // Set all defined output pins as OUTPUT
  for (int i = 0; i < numberOfConnections; i++) {
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

  // Check if it's time to read the pins
  if (currentMillis - previousMillis >= interval) {
    // Save the last time you checked the pins
    previousMillis = currentMillis;

    // Read the state of each pin and store it in the connectionStates array
    for (int i = 0; i < numberOfConnections; i++) {
      //connectionStates[i] = digitalRead(connectionsInputPins[i]);
      connectionStates[i] = getConnectionState(i);
    }

    uint8_t previousNumClosedConnections = getNumClosedConnections(previousConnectionStates, numberOfConnections);
    uint8_t numClosedConnections = getNumClosedConnections(connectionStates, numberOfConnections);
    bool allConnectionsClosed = false;
    if (numClosedConnections != previousNumClosedConnections) {
      allConnectionsClosed = (numClosedConnections == numberOfConnections);
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
    if (connectionClosed) {
      playConnectionClosedMelody(); // Play melody for connection closed
      runLEDEventConnectionClosed(); // Run LED event for connection closed
    }
    if (connectionOpened) {
      playConnectionOpenedMelody(); // Play melody for connection opened
      runLEDEventConnectionOpened(); // Run LED event for connection opened
    }

    if(allConnectionsClosed) {
      playFinishedMelody(); // Play melody when all connections are closed
      runLEDEventFinished(); // Run LED event when all connections are closed
    }

    // If a connection was opened or closed, output the state of all connections
    if (connectionOpened || connectionClosed) {
      Serial.println("Connection state changed:");
    } else {
      Serial.println("No change in connection state.");
    }

    // Output the array to serial
    Serial.print("[");
    for (int i = 0; i < numberOfConnections; i++) {
      Serial.print(connectionStates[i]);
      if (i < numberOfConnections - 1) {
        Serial.print(", ");
      }
    }
    Serial.println("]");
  }
}




// --- Blocking Mode 0: Rainbow Chase ---
void modeRainbowChase(unsigned long duration) {
  unsigned long start = millis();
  int step = 0;

  while (millis() - start < duration) {
    for (int i = 0; i < NUMPIXELS; i++) {
      int hue = (i * 256 / NUMPIXELS + step) % 256;
      pixels.setPixelColor(i, pixels.ColorHSV(hue * 256));
    }
    pixels.show();
    step = (step + 15) % 256;
    delay(frameInterval);
  }
}

// --- Blocking Mode 1: Knight Rider ---
void modeKnightRider(unsigned long duration) {
  unsigned long start = millis();
  int position = 0;
  int direction = 1;
  int hueShift = 0;

  while (millis() - start < duration) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, 0);
    }

    pixels.setPixelColor(position, pixels.ColorHSV(hueShift * 256, 255, 255));
    pixels.show();

    position += direction;
    if (position == 0 || position == NUMPIXELS - 1) direction = -direction;

    hueShift = (hueShift + 3) % 256;
    delay(frameInterval);
  }
}

// --- Blocking Mode 2: Blink All ---
void modeBlinkAll(unsigned long duration) {
  unsigned long start = millis();
  int step = 0;
  int hueShift = 0;

  while (millis() - start < duration) {
    uint32_t color = (step % 2 == 0) ? pixels.ColorHSV(hueShift * 256, 255, 255) : 0;
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, color);
    }
    pixels.show();

    hueShift = (hueShift + 3) % 256;
    step++;
    delay(frameInterval);
  }
}

// --- Blocking Mode 3: Color Wipe ---
void modeColorWipe(unsigned long duration) {
  unsigned long start = millis();
  bool turningOn = true;
  int hueShift = 0;

  while (millis() - start < duration) {
    for (int i = 0; i < NUMPIXELS; i++) {
      if (turningOn) {
        pixels.setPixelColor(i, pixels.ColorHSV(hueShift * 256, 255, 255));
      } else {
        pixels.setPixelColor(i, 0);
      }
      pixels.show();
      delay(frameInterval);
    }
    turningOn = !turningOn;
    hueShift = (hueShift + 16) % 256;
  }
}

// --- Blocking Mode 4: Random Flash ---
void modeRandomFlash(unsigned long duration) {
  unsigned long start = millis();
  int hueShift = 0;

  while (millis() - start < duration) {
    for (int i = 0; i < NUMPIXELS; i++) {
      if (random(10) > 6) {
        int randomHue = (hueShift + random(64)) % 256;
        pixels.setPixelColor(i, pixels.ColorHSV(randomHue * 256, 255, 255));
      } else {
        pixels.setPixelColor(i, 0);
      }
    }
    pixels.show();
    hueShift = (hueShift + 3) % 256;
    delay(frameInterval);
  }
}

void playFinishedMelody() {
  
}

void playConnectionClosedMelody() {
  
}

void playConnectionOpenedMelody() {
  
}
