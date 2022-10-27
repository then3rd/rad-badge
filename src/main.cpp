#include <Arduino.h>
#include "midiplay.h"
#include "settings.h"

/*
// Flashing //
To set the correct fuses required for this project, Use Platformio's "Set Fuses" feature to enforce
the values set within platformio.ini
// ctrl+alt+t -> Set Fuses
// pio run --target fuses --environment attiny85

// Resources //
https://cyberblogspot.com/how-to-program-attiny85-with-arduino-ide/
https://riteshkhanna.com/2016/04/20/programming-attiny45attiny85-with-a-usbasp-avr-programmer/
https://hackaday.io/project/159973-z80-mbc2-a-4-ics-homebrew-z80-computer/log/150087-how-use-the-icsp-port-with-the-usbasp-programmer-under-linux-to-burn-the-bootloader
https://wolles-elektronikkiste.de/en/programming-attiny-with-arduino-code
https://quadmeup.com/attiny85-light-sensor-i2c-slave-device/
https://www.mouser.com/datasheet/2/268/Atmel_2586_AVR_8_bit_Microcontroller_ATtiny25_ATti-1315542.pdf
https://github.com/lukejenkins/minibadge
https://saintcon.org/wp-content/uploads/2021/10/MINIBADGE-Guide-2021.pdf
https://github.com/SaadOjo/DIY_Li-Fi/blob/master/transmitter/transmitter.ino
https://github.com/SaadOjo/DIY_Li-Fi/blob/master/receiver/receiver.ino
*/

unsigned int geigerCount = 0;
unsigned int cycleCount = 0;

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

int randWait() {
    return random(1000, 5000);;
}

void geigerTick(){
    // Blink LED
    #ifndef ANALOG_LED
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

unsigned int randCount = randWait();

void loop() {
  int sensorValue = analogRead(PIN_SENSE);
  unsigned int clampValue = constrain(map(sensorValue, 40, 800, 0, 1000), 0, 1000);
  #ifdef ANALOG_LED
  unsigned char ledValue = map(clampValue, 0, DIST_FAR, 255, 5); // scale to led & set minimum
  #endif

  // Sensitivity control
  if (clampValue < DIST_FAR ) {
    #ifdef ANALOG_LED
    analogWrite(PIN_LED, ledValue);
      #ifndef DEBUG
      delay(5); // extra delay or not enough time for low PWM values to be visible
      #endif
    #endif
    geigerCount++;
    #ifdef DEBUG
    Serial.print(geigerCount);
    Serial.print(loopCount);
    #endif

    int d;
    int r1;
    int min = random (5, 20);
    if (clampValue < DIST_CLOSE ) {
      r1 = random(0,200);
    } else { // far
      r1 = random(400,800);
    }
    d = map(clampValue, 0, 1000, 0, r1);

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
    if (geigerCount < LOOP_MAX - 500 ) {
      freq = random (100,120);     // clicky
    }
    #ifndef DEBUG
    else
    if ( geigerCount < LOOP_MAX - 400 ) {
      freq = random (400,450);     // beepy
    } else
    if ( geigerCount < LOOP_MAX - 300 ) {
      freq = random (10000,10500); // chirpy
    } else
    if ( geigerCount < LOOP_MAX - 200 ) {
      freq = random (200,6000);    // funky
    } else
    #endif
    #endif


    #ifndef DEBUG
    if ( geigerCount > LOOP_MAX ) {
      if (cycleCount > CYCLES) {
        play();
      } else {
        cycleCount++;
      }
      geigerCount = 0;
    }
    #else
    if ( geigerCount > 100 ) {
      play();
    }
    #endif

    // Do the click & blink
    geigerTick();

    // Extra delay.
    if (min >= 10 ) {
      delay(min+d);
    }
  }
}