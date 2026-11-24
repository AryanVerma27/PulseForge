/*
 * PulseForge — Timer_A PWM Generation (Up / Up-Down counting, output modes 2, 4, 7)
 *
 * Board : MSP-EXP432P401R LaunchPad + Educational BoosterPack MKII (LCD)
 * IDE   : Energia
 *
 * Timer_A0 is clocked from ACLK (32.768 kHz) and drives a 10 Hz, 50 % duty
 * PWM signal on P2.5 (TA0.2 / CCR2). Wire P2.5 -> P2.3 with a jumper; pressing
 * the P1.4 (S2) push-button samples P2.3 for 1 s and plots the waveform on the LCD.
 *
 * Select one of the six experiment configurations with PWM_CONFIG below.
 *
 *   PWM_CONFIG | Counting mode | Output mode (OUTMOD)   | CCR0      | CCR2
 *   -----------+---------------+------------------------+-----------+-----------
 *        1     | Up            | 2 - Toggle/Reset       | COUNT/2   | COUNT2/2
 *        2     | Up/Down       | 2 - Toggle/Reset       | COUNT/4   | COUNT2/4
 *        3     | Up            | 4 - Toggle             | COUNT/4   | COUNT2/4
 *        4     | Up/Down       | 4 - Toggle             | COUNT/4   | COUNT2/4
 *        5     | Up            | 7 - Reset/Set          | COUNT/2   | COUNT2/2
 *        6     | Up/Down       | 7 - Reset/Set          | COUNT/4   | 0
 */

#define PWM_CONFIG 5          // <-- choose 1..6

#include <SPI.h>
#include <LCD_screen.h>
#include <LCD_screen_font.h>
#include <LCD_utilities.h>
#include <Screen_HX8353E.h>
#include <Terminal12e.h>
#include <Terminal6e.h>
#include <Terminal8e.h>

#define MSP432P401R
#include "msp.h"
#include <energia.h>
#include <stdint.h>
#include <stdio.h>

Screen_HX8353E myScreen;

#define MAXDOTS 100
int pwm_data[MAXDOTS];
void LCD_plot(void);

// Timer_A0 CTL bit fields
#define UP      0x0010    // MC = 01 : Up mode
#define UPDOWN  0x0030    // MC = 11 : Up/Down mode
#define CONT    0x0020    // MC = 10 : Continuous mode
#define HALT    0x0000    // MC = 00 : Stop
#define ACLK    0x0100    // TASSEL = 01 : ACLK (32768 Hz)

// 32768 / 5 counts = 0.2 s at ACLK
#define COUNT   (32768 / 5)
#define COUNT2  (32768 / 5 / 2)

// Energia pin numbers
#define PIN_BUTTON_P1_4  74   // S2 push-button
#define PIN_SAMPLE_P2_3  34   // P2.3 sampled to draw the PWM (jumpered from P2.5)

#if   PWM_CONFIG == 1
  #define TA_MODE  UP
  #define CCR0_VAL (COUNT / 2)
  #define OUTMOD   TIMER_A_CCTLN_OUTMOD_2
  #define CCR2_VAL (COUNT2 / 2)
#elif PWM_CONFIG == 2
  #define TA_MODE  UPDOWN
  #define CCR0_VAL (COUNT / 4)
  #define OUTMOD   TIMER_A_CCTLN_OUTMOD_2
  #define CCR2_VAL (COUNT2 / 4)
#elif PWM_CONFIG == 3
  #define TA_MODE  UP
  #define CCR0_VAL (COUNT / 4)
  #define OUTMOD   TIMER_A_CCTLN_OUTMOD_4
  #define CCR2_VAL (COUNT2 / 4)
#elif PWM_CONFIG == 4
  #define TA_MODE  UPDOWN
  #define CCR0_VAL (COUNT / 4)
  #define OUTMOD   TIMER_A_CCTLN_OUTMOD_4
  #define CCR2_VAL (COUNT2 / 4)
#elif PWM_CONFIG == 5
  #define TA_MODE  UP
  #define CCR0_VAL (COUNT / 2)
  #define OUTMOD   TIMER_A_CCTLN_OUTMOD_7
  #define CCR2_VAL (COUNT2 / 2)
#elif PWM_CONFIG == 6
  #define TA_MODE  UPDOWN
  #define CCR0_VAL (COUNT / 4)
  #define OUTMOD   TIMER_A_CCTLN_OUTMOD_7
  #define CCR2_VAL 0
#else
  #error "PWM_CONFIG must be 1..6"
#endif

void setup()
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;   // stop watchdog timer

    Serial.begin(9600);
    myScreen.begin();
    myScreen.setPenSolid(true);

    pinMode(PIN_BUTTON_P1_4, INPUT_PULLUP);
    pinMode(PIN_SAMPLE_P2_3, INPUT_PULLUP);

    // Timer_A0 sourced from ACLK in the selected counting mode
    TIMER_A0->CTL = (TA_MODE | ACLK);
    TIMER_A0->CCR[0] = CCR0_VAL;                   // period

    // P2.5 as output, primary function = TA0.2
    P2->DIR   = BIT5;
    P2->SEL0 |= BIT5;
    P2->SEL1 &= ~(BIT5);

    TIMER_A0->CCTL[2] = OUTMOD;                    // output mode on CCR2
    TIMER_A0->CCR[2]  = CCR2_VAL;                  // duty-cycle threshold
}

void loop()
{
    LCD_plot();
}

// On P1.4 press: sample P2.3 every 10 ms (100 samples = 1 s) and draw it
void LCD_plot(void)
{
    if (digitalRead(PIN_BUTTON_P1_4) == 0) {
        myScreen.dRectangle(5, 30, 10 + 100, 50, blackColour);   // clear plot area

        while (digitalRead(PIN_SAMPLE_P2_3) == 1) { }            // sync to a low level

        for (int loops = 0; loops < MAXDOTS; loops++) {
            int state = digitalRead(PIN_SAMPLE_P2_3);
            pwm_data[loops] = state;
            Serial.println(state);
            delay(10);
        }
        for (int i = 0; i < MAXDOTS; i++)
            myScreen.dRectangle(10 + i, 40 + (pwm_data[i] << 5), 1, 1, greenColour);
    }
}
