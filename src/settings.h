#ifndef __settings_h
#define __settings_h
#define TX        0     //PB0 (pin5)
#define RX        1     //PB1 (pin6)
#define PIN_SPKR  4     //PB4 (pin3)
#define PIN_SENSE A3    //PB3 (pin2)
#define PIN_LED   1     //PB1 (pin6) led pwm
// #define PIN_LED   2     //PB2 (pin7) digital non-pwm

// Options
// #define DEBUG 1
#define ANALOG_LED 1
#define USE_TONE 1
#define DIST_FAR    1200 // lower == darker
#define DIST_CLOSE  600
#define LOOP_MAX 1000
// #define LOOP_MAX 5000
#endif