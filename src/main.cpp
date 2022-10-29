#include <Arduino.h>
#include "midiplay.h"
#include "settings.h"

unsigned int geigerCount = 0;
unsigned int cycleCount = 0;
unsigned int loopCount = 0;

void play(){
  setupIO();
  playMidiData();
  for(;;); // TODO; return to loop
}

void pulseBlink(){
  // Boot LED
  for (int i = 0; i < 3; i++) {
    #ifdef ANALOG_LED
    for (int i = 0; i <= 255; i++) {
      analogWrite(PIN_LED, i);
      delay(1);
    }
    analogWrite(PIN_LED, 0);
    delay(500);
    // for (int i = 255; i >= 0; i--) {
    //   analogWrite(PIN_LED, i);
    //   delay(1);
    // }
    #else
    digitalWrite(PIN_LED, HIGH);
    delay(500);
    digitalWrite(PIN_LED, LOW);
    #endif
    delay(100);
  }
}

int randTick(){
  return random(1000, 20000);
}

int randDelay(){
  return random(10, 120);
}
void geigerTick(int pwmValue){
    // Blink LED
    #ifdef ANALOG_LED
    analogWrite(PIN_LED, pwmValue);
      #ifndef DEBUG
      delay(5); // extra delay or not enough time for low PWM values to be visible
      #endif
    #else
    digitalWrite(PIN_LED, HIGH);
    #endif

    // Play short tone
    #ifdef USE_TONE
    tone(PIN_SPKR, freq);
    delay(8);
    noTone(PIN_SPKR);
    #else
    digitalWrite(PIN_SPKR, HIGH);
    delay(5);
    digitalWrite(PIN_SPKR, LOW);
    #endif

    // Turn off LED
    #ifdef ANALOG_LED
    analogWrite(PIN_LED, 0);
    #else
    digitalWrite(PIN_LED, LOW);
    #endif
}

void setup() {
  // Set up photoresistor & LED
  pinMode(PIN_SENSE, INPUT);

  #ifndef USE_TONE
  pinMode(PIN_SPKR, OUTPUT);
  #endif

  #ifndef ANALOG_LED
  pinMode(PIN_LED, OUTPUT);
  #endif

  #ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Initializing...");
  analogWrite(PIN_LED, 0);
  #else
  pulseBlink();
  #endif
}

unsigned int randCount = randTick();

void loop() {
  int sensorValue = analogRead(PIN_SENSE);
  unsigned int clampValue = constrain(map(sensorValue, 40, 800, 0, 1000), 0, 1000);
  #ifdef ANALOG_LED
  unsigned char ledValue = map(clampValue, 0, DIST_FAR, 255, 5); // scale to led & set minimum
  #endif

    if (loopCount == randCount){
      int cnt = random(0,2);
      for (int i = 0; i <= cnt; i++) {
        geigerTick(255);
        delay(randDelay());
      }
      loopCount = 0;
      randCount = randTick();
    }

  // Sensitivity control
  if (clampValue < DIST_FAR ) {
    geigerCount++;
    #ifdef DEBUG
    Serial.print(geigerCount);
    Serial.print(loopCount);
    #endif

    int d;
    int r1;
    int min = randDelay();

    #ifdef DEBUG
    Serial.print("\tsensor: ");
    Serial.print(sensorValue);
    Serial.print("\tclamp: ");
    Serial.print(clampValue);
    Serial.print("\tdelay: ");
    Serial.print(min+d);
    Serial.print("\tr1: ");
    Serial.print(r1);
      #ifdef ANALOG_LED
      Serial.print("\t\tled: ");
      Serial.print(ledValue);
      #endif
    Serial.println();
    #endif

    #ifdef USE_TONE
    // TODO: Make this use variables/array
    if (geigerCount < LOOP_MAX - 800 ) {
      freq = random (100,120);     // clicky
    } else
    #ifndef DEBUG
    if ( geigerCount < LOOP_MAX - 700 ) {
      freq = random (400,450);     // beepy
    } else
    if ( geigerCount < LOOP_MAX - 600 ) {
      freq = random (10000,10500); // chirpy
    } else
    if ( geigerCount < LOOP_MAX - 500 ) {
      freq = random (200,6000);    // funky
    }
    #endif
    #endif

    if ( geigerCount > LOOP_MAX ) {
      if (cycleCount > CYCLES) {
        play();
      } else {
        cycleCount++;
      }
      geigerCount = 0;
    }

    // Do the click & blink
    #ifdef ANALOG_LED
    geigerTick(ledValue);
    #else
    geigerTick(0);
    #endif

    // Extra delay.
    if (min >= 10 ) {
      if (clampValue < DIST_CLOSE ) {
        r1 = random(0,200);
      } else { // far
        r1 = random(400,800);
      }
      d = map(clampValue, 0, 1000, 0, r1);
      delay(min+d);
    } else {
      delay(min);
    }
  }
  loopCount++;
}