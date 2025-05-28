#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

// ======= CONFIGURABLE SETTINGS ======= 
const int numSensors = 4; // Number of piezo sensors you're using
const int sensorPins[numSensors] = {A0, A1, A2, A3}; // Analog pins
const int midiNotes[numSensors] = {60, 62, 64, 65};  // Corresponding MIDI notes
const int ledPins[numSensors] = {2, 3, 4, 5};        // LEDs for visual feedback

const int threshold = 50;          // Minimum sensor reading to trigger
const int velocityMin = 30;         // Minimum velocity
const int velocityMax = 127;        // Maximum velocity
const int triggerCooldown = 100;   // Cooldown in ms between triggers
const int noteOffDelay = 50;        // Delay after Note On before sending Note Off (ms)

// ======= INTERNAL STATE ======= 
unsigned long lastTriggerTimes[numSensors];
bool notesAreOn[numSensors];

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI); // Listen to all MIDI channels

  // Setup sensor and LED pins
  for (int i = 0; i < numSensors; i++) {
    pinMode(sensorPins[i], INPUT);
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);

    lastTriggerTimes[i] = 0;
    notesAreOn[i] = false;
  }
}

void loop() {
  unsigned long currentTime = millis();

  for (int i = 0; i < numSensors; i++) {
    int sensorValue = analogRead(sensorPins[i]);

    if (sensorValue > threshold && (currentTime - lastTriggerTimes[i]) > triggerCooldown) {
      int velocity = map(sensorValue, threshold, 1023, velocityMin, velocityMax);
      velocity = constrain(velocity, velocityMin, velocityMax);

      // Send Note On
      MIDI.sendNoteOn(midiNotes[i], velocity, 1); // MIDI channel 1
      notesAreOn[i] = true;
      lastTriggerTimes[i] = currentTime;

      // Flash LED
      digitalWrite(ledPins[i], HIGH);
    }

    // Handle Note Off
    if (notesAreOn[i] && (currentTime - lastTriggerTimes[i]) > noteOffDelay) {
      MIDI.sendNoteOff(midiNotes[i], 0, 1);
      notesAreOn[i] = false;

      // Turn off LED
      digitalWrite(ledPins[i], LOW);
    }
  }
}
