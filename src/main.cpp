#include <Arduino.h>
#include <avr/wdt.h>
#include "midi/portal_short.h"
// #include "midi/marblemachine.h"

/*
// Setup & Notes //
xxd -i yoursong.midi > midi/yoursong.h
Check out https://musicboxmaniacs.com/ for compatible midis

// Resources //
http://www.technoblogy.com/show?2E6L
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

#define DEBUG 1

#define TX        0     //PB0 (pin5)
#define RX        1     //PB1 (pin6)
#define PIN_SPKR  4     //PB4 (pin3)
#define PIN_SENSE A3    //PB3 (pin2)
// #define PIN_LED   2     //PB2 (pin7)
#define PIN_LED   1     //PB1 (pin6) // TODO: Update PCB to use this pin



#define DIST_FAR    400
#define DIST_CLOSE  300

// Synth **********************************************
const int Volume = 8; // Volume: 7 = loud, 8 = medium, 9 = quiet
const int Decay = 14; // Length of note decay: 14 = maximum, 13 = medium, 12 = staccato

unsigned int Scale[] = {10973, 11626, 12317, 13050, 13826, 14648, 15519, 16442, 17419, 18455, 19552, 20715};

const int Channels = 4;
int Chan = 0;

volatile unsigned int Acc[Channels];
volatile unsigned int Freq[Channels];
volatile unsigned int Amp[Channels];

// Generate square waves on Channels channels
ISR(TIMER0_COMPA_vect){
  static uint8_t c;
  signed char Temp, Mask, Env, Note;
  Acc[c] = Acc[c] + Freq[c];
  Amp[c] = Amp[c] - (Amp[c] != 0);
  Temp = Acc[c] >> 8;
  Temp = Temp & Temp<<1;
  Mask = Temp >> 7;
  Env = Amp[c] >> Volume;
  Note = (Env ^ Mask) + (Mask & 1);
  OCR1B = Note + 128;
  c = (c + 1) & 3;
}

// Tempo timing **********************************************
// Global tick counter
volatile unsigned int GlobalTicks = 0;

// Ticks timer
unsigned int Ticks() {
  unsigned int t;
  cli();
  t = GlobalTicks;
  sei();
  return t;
}

// Delay using Watchdog timer - 62.5 ticks a second
void Delay(unsigned int t) {
  unsigned int t0 = Ticks();
  while (Ticks() - t0 < t);
}

// Watchdog interrupt counts ticks
ISR(WDT_vect) {
  WDTCR |= 1<<WDIE;
  GlobalTicks++;
}

// MIDI interpreter **********************************************
unsigned int Ptr, EndBlock;
unsigned long Tempo = 500000;    // Default - microseconds per beat; ie 120bpm
unsigned int TempoDivisor;
const int Error = 2;  // Error LED on PB2
const long MThd = 0x4D546864;
const long MTrk = 0x4D54726B;

unsigned int loopCount = 0;

void readIgnore(int n){
  Ptr = Ptr + n;
}

unsigned long readNumber(int n){
  long result = 0;
  for (int i=0; i<n; i++) result = (result<<8) + pgm_read_byte(&Tune[Ptr++]);
  return result;
}

unsigned long readVariable(){
  long result = 0;
  uint8_t b;
  do {
    b = pgm_read_byte(&Tune[Ptr++]);
    result = (result<<7) + (b & 0x7F);
  } while (b & 0x80);
  return result;
}

// Blink LED and report error
void error(int n){
  #ifdef DEBUG
  Serial.println("1=header block, 2=track block, 3=MIDI event");
  Serial.print("Error: ");
  Serial.println(n);
  #endif
  while (1) {
    for (int i=0; i<n; i++) {
      digitalWrite(Error, HIGH);
      Delay(15);
      digitalWrite(Error, LOW);
      Delay(15);
    }
    Delay(125);
  }
}

// Play note. 60 = middle C (261.63 Hz)
void noteOn(uint8_t number){
  uint8_t octave = number/12;
  uint8_t note = number%12;
  unsigned int freq = Scale[note];
  uint8_t shift = 9-octave;
  Freq[Chan] = freq>>shift;
  Amp[Chan] = 1<<Decay;
  Chan = (Chan + 1) & 3;
}

void playMidiData(){
  Ptr = 0;                                  // Begin at start of file
  // Read header chunk
  unsigned long type = readNumber(4);
  if (type != MThd) error(1);
  unsigned long len = readNumber(4);
  unsigned int format = readNumber(2);
  unsigned int tracks = readNumber(2);
  unsigned int division = readNumber(2);    // Ticks per beat
  TempoDivisor = (long)division*16000/Tempo;

  // Read track chunks
  for (int t=0; t<tracks; t++) {
    type = readNumber(4);
    if (type != MTrk) error(2);
    len = readNumber(4);
    EndBlock = Ptr + len;

    // Parse track
    while (Ptr < EndBlock) {
      unsigned long delta = readVariable();
      uint8_t event = readNumber(1);
      uint8_t eventType = event & 0xF0;
      if (delta > 0) Delay(delta/TempoDivisor);

      // Meta event
      if (event == 0xFF) {
        uint8_t mtype = readNumber(1);
        uint8_t mlen = readNumber(1);
        // uint8_t mlen = readVariable(); // TODO TESTING!
        // Tempo
        if (mtype == 0x51) {
          Tempo = readNumber(mlen);
          TempoDivisor = (long)division*16000/Tempo;
        // Ignore other meta events
        } else readIgnore(mlen);

      // Note off - ignored
      } else if (eventType == 0x80) {
        uint8_t number = readNumber(1);
        uint8_t velocity = readNumber(1);
      // Note on
      } else if (eventType == 0x90) {
        uint8_t number = readNumber(1);
        uint8_t velocity = readNumber(1);
        noteOn(number);
      // Polyphonic key pressure
      } else if (eventType == 0xA0) readIgnore(2);
      // Controller change
      else if (eventType == 0xB0) readIgnore(2);
      // Program change
      else if (eventType == 0xC0) readIgnore(1);
      // Channel key pressure
      else if (eventType == 0xD0) readIgnore(1);
      // Pitch bend
      else if (eventType == 0xD0) readIgnore(2);
      else error(3);
    }
  }
}

// Setup **********************************************
void setupIO(){
  // Enable 64 MHz PLL and use as source for Timer1
  PLLCSR = 1<<PCKE | 1<<PLLE;

  // Set up Timer/Counter1 for PWM output
  TIMSK = 0;                     // Timer interrupts OFF
  TCCR1 = 1<<CS10;               // 1:1 prescale
  GTCCR = 1<<PWM1B | 2<<COM1B0;  // PWM B, clear on match
  OCR1B = 128;
  pinMode(PIN_SPKR, 1); // DDRB = 1<<DDB4; // Enable PWM output on pin 4

  // Set up Timer/Counter0 for 25kHz interrupt to output samples.
  TCCR0A = 3<<WGM00;             // Fast PWM
  TCCR0B = 1<<WGM02 | 2<<CS00;   // 1/8 prescale
  OCR0A = 19;                    // Divide by 100/Channels
  TIMSK = 1<<OCIE0A;             // Enable compare match, disable overflow

  // Set up Watchdog timer for 16ms interrupt for ticks timer.
  WDTCR = 1<<WDIE | 0<<WDP0;     // Interrupt every 16ms
}

void play(){
  loopCount = 0;
  setupIO();
  playMidiData();
  for(;;); // TODO; return to loop
}

void setup() {
  // Set up photoresistor & LED
  pinMode(PIN_SENSE, INPUT);
  // pinMode(PIN_LED, OUTPUT);

  #ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Initializing...");
  analogWrite(PIN_LED, 0);
  delay(500);
  // #else
  // Boot LED
  for (int i = 0; i <= 255; i++) {
    analogWrite(PIN_LED, i);
    delay(5);
  }
  for (int i = 255; i >= 0; i--) {
    analogWrite(PIN_LED, i);
    delay(5);
  }
  #endif
}

void loop() {
  int freq;
  int sensorValue = analogRead(PIN_SENSE);
  unsigned int clampValue = constrain(map(sensorValue, 40, 500, 0, 1000), 0, 1000);
  unsigned char ledValue = map(clampValue, 0, DIST_FAR, 255, 5); // scale to led & set minimum

  // Sensitivity control
  if (clampValue < DIST_FAR ) {
    // Blink LED according to brightness
    analogWrite(PIN_LED, ledValue);

    loopCount++;
    #ifdef DEBUG
    Serial.print(loopCount);
    #endif

    int d;
    int r1;
    int min = random (1, 10);
    if (clampValue < DIST_CLOSE ) {
      r1 = random(0,200);
    } else { // far
      r1 = random(400,800);
    }
    d = map(clampValue, 0, 1000, 0, r1);

    #ifdef DEBUG
    Serial.print("\tclamp: ");
    Serial.print(clampValue);
    Serial.print("\tdelay: ");
    Serial.print(min+d);
    Serial.print("\tr1: ");
    Serial.print(r1);
    Serial.print("\t\tled: ");
    Serial.println(ledValue);
    #endif

    // TODO: Make this use variables/array
    if (loopCount < 500) {
      freq = random (100,120);     // clicky
    }
    #ifndef DEBUG
    else
    if ( loopCount < 600 ) {
      freq = random (400,450);     // beepy
    } else
    if ( loopCount < 700 ) {
      freq = random (10000,10500); // chirpy
    } else
    if ( loopCount >= 700 && loopCount < 1000 ) {
      freq = random (200,6000);    // funky
    } else
    if ( loopCount > 1000 ) {
      play();
    }
    #else
    if ( loopCount > 100 ) {
      play();
    }
    #endif

    // Play short tone
    tone(PIN_SPKR, freq);
    delay(8);
    noTone(PIN_SPKR);
    if (min >= 3 ) {
      delay(min+d);
    }
    analogWrite(PIN_LED, 0);
  }
}