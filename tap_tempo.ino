#include "Timer.h"

// Try Using TimerOne.h -> timer with Interrupt and possibility to define a PWM signal associated
// https://github.com/DieterVDW/arduino-midi-clock/blob/master/MIDI-Clock.ino
#define interruptPin (2)  // the number of the pushbutton pin
#define ledPin (13)       // the number of the LED pin

#define DEBOUNCE_US (30000)
#define TIMEOUT_US (5000000)

typedef enum {
  stateIDLE = 0,
  // stateSTART,
  stateDEBOUNCE,
  stateTAP,
  stateTIMEOUT
} state_t;

Timer timerTimeout(MICROS);
Timer timerDebounce(MICROS);

bool ledState;
state_t state;
bool tapTrig;
uint8_t tap_idx;
uint32_t tempo;
uint32_t tempo_sum;
uint32_t timeout_us;
uint32_t previous_tap_ticks;
uint32_t tempo_start_ticks;

/*****************************************************
 * Trig beat : trigger setting or Midi at detected tempo 
 */
bool TempoTask(void) {
  ledState = !ledState;
  digitalWrite(ledPin, ledState);
  // Serial.println("tempo");
  return true;
}

void setup() {
  /* init variables */
  tapTrig=false;
  ledState = false;
  state = stateIDLE;
  tempo = 0;
  tap_idx = 0;
  tempo_sum = 0;
  previous_tap_ticks = 0;
  tempo_start_ticks = 0;
  timeout_us = TIMEOUT_US;

  /* initialize the pushbutton pin as an input: */
  pinMode(interruptPin, INPUT_PULLUP);

  /*  set init LED pin and state */
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);

  /* Init UART */
  Serial.begin(115200);
}
  
void loop()
{
  switch (state) 
  {
      case stateIDLE:
      {
          if (tapTrig) {
              state = stateTAP;
              tapTrig=false;
          }
          else if ((timerTimeout.read() >= timeout_us) && (timerTimeout.state()==RUNNING)) {
              state = stateTIMEOUT;
          }
          break;
      }
      case stateTAP:
      {
          uint32_t micro_ticks = micros();
          state = stateIDLE;
          if (tap_idx == 0) {
              /* First TAP : start tempo meas */
              TempoTask();   /* Call on Tap to sync */
              previous_tap_ticks = micro_ticks;
              tempo_start_ticks = micro_ticks; /* reset tempo start to resync on first tap */
              timeout_us = TIMEOUT_US;
              tempo_sum = 0;
              // Serial.println("start");
          } else {
              /* Other TAPs: meas avg tempo */
              uint32_t delta = micro_ticks - previous_tap_ticks;
              uint32_t previous_tempo = tempo;
              tempo_sum += delta;
              previous_tap_ticks = micro_ticks;
              tempo = tempo_sum/(tap_idx);
              timeout_us = tempo*2;
              if( previous_tempo && (delta <= previous_tempo) ) {   /* TAP occurs before tempo_tick case */ 
                  tempo_start_ticks = micro_ticks - tempo;            /* align to the current tap to be executed as soon as possible */
              } else {
                  tempo_start_ticks = micro_ticks;                    /* align to the current tap to be executed after tempo */
              }
              // Serial.println("TAP");
          }
          Serial.println(tap_idx);
          Serial.println(tempo);
          tap_idx++;

          timerTimeout.start();
          break;
      }

      case stateTIMEOUT:
      {
          timerTimeout.stop();
          Serial.println("timeout");
          state = stateIDLE;
          tap_idx = 0;
          tapTrig=0;
          break;
      }
  }

  /****************************************/
  /******* Tempo Task scheduler ***********/
  /****************************************/
  if( ((tap_idx>0) || (state==stateIDLE)) 
      && tempo
      && ((micros() - tempo_start_ticks) >= tempo ))
  {
      tempo_start_ticks += tempo;
      TempoTask();
  }

  /*************************************/
  /******* TAP Button handle ***********/
  /*************************************/
  static int previouspinState = HIGH;
  static bool start_button_debounce = false;
  int currentpinState;
  /* Falling edge must trig immediatly TAP state, then hold during debounce time to not detect false rising
   * Rising edge must first wait for debounce time then update considered pin state
   */
  /* debounce */
  if((start_button_debounce && (timerDebounce.read() >= DEBOUNCE_US)) || !start_button_debounce) {
      currentpinState =  digitalRead(interruptPin);
      start_button_debounce = false;
      timerDebounce.stop();
  }

  /* steady state transition */
  if( (currentpinState == LOW) && (previouspinState==HIGH)) { 
      tapTrig = true;
      start_button_debounce=true;
      previouspinState = LOW;
      timerDebounce.start();
  } 
  else if( (currentpinState == HIGH) && (previouspinState==LOW)) {
      start_button_debounce=true;
      previouspinState = HIGH;
      timerDebounce.start();
  }
}
