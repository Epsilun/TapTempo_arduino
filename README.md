# TapTempo_arduino
TapTempo implementation for Arduino

Single Tap resync tempo.
From 2 Taps, duration between taps is averaged and TempoTask triggered periodicly at the averaged tempo.
tempo is resync on every Tap. 

Tap using input pin with debounce feature ( press button case ).
When button is pressed, the count is immediatly started, then debounce starts before to consider the pin state steady. 
When button is released, a debounce is started before to consider the pin state steady and allowing to get a new Tap.

Timeout mechanism is implemented, which is defined by TIMEOUT_US for the first Tap,
Timeout is set to twice the averaged tempo on following Taps.

The current implementation does not use Iterrupts because I was not able to have a consistant behavior, glitch free. 

# Improvements:
  - pin as trigger ( similar to Led pin )
  - Send Midi commands to start/stop play
  - generate Midi clock ( need rework to send 24*Tempo rate messages )

  Consider rework using TimerOne.h lib which is using interrupt on a timer and allow to use a PWM pin to generate a trigger signal. 
  see https://github.com/DieterVDW/arduino-midi-clock/blob/master/MIDI-Clock.ino
