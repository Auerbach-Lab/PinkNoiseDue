#include <Arduino.h>
#include <debounce.h>
#include <DueFlashStorage.h>
#include <efc.h>
#include <flash_efc.h>
#include "costable.h"
#include "DueArbitraryWaveformGeneratorV2.h"

// EDIT THESE VALUES to adjust timings on sequence
#define RECORDING_DURATION  600000   // ms duration of entire recording sequence, must be at least BOOKEND_DURATION * 2 + SOUND_DURATION for a single sound
#define BOOKEND_DURATION    120000   // ms duration of silence at beginning and end, must be less than 1/2 RECORDING_DURATION
#define GAP_DURATION         30000   // ms between sounds
#define SOUND_DURATION        5000   // ms duration of sound to play
#define COSINE_PERIOD          500   // ms duration of cosine gate function, must be less than or equal to 1/2 SOUND_DURATION

// DO NOT EDIT 
// total number of sounds that will play, +GAP in numerator because last tone does not need GAP included to fit
// will be 11 under default settings of 600s recording, 120s bookend, 30s gap, 5s sound
unsigned const int SOUND_COUNT = (RECORDING_DURATION - 2*BOOKEND_DURATION + GAP_DURATION) / (SOUND_DURATION + GAP_DURATION);

// EDIT THIS to define software volume
#define VOLUME 200 //0-2000 range, arbitrary unit
// With trim potentiometer set so that non-signal amplification noise is inaudible while off (bypassing the relay), rough decibels are:
// NoiseAmp dB   NoiseAmp dB
//       10 50         90 80
//       20 61        100 81
//       30 67        150 86 
//       40 70        200 89    Values beyond 200 causes, in essence, a much faster volume transition  
//       50 73        300 92
//       60 75        500 94
//       70 77       1000 95    
//       80 78       2000 96    Momentary artifacts (signal clipping?) happen at transitions to on or off at this amplification




//A2 56 and A4 58 are available, but do not use A3, causes noise on boot, stop before start, other weirdness
//odd pins from 25 through 39 are available now
//13 is the onboard LED but otherwise unused
#define TEST_BUTTON_PIN 25
#define SEQUENCE_BUTTON_PIN 27
#define SEQUENCE_LED_PIN 29
#define TTL_OUTPUT_PIN 31
#define RELAY_PIN 33
#define STOP_BUTTON_PIN 35


unsigned long soundToStart[SOUND_COUNT] = {0};
unsigned long soundToStop[SOUND_COUNT] = {0};
unsigned long soundStartedAt = 0; //active playing sound, for convenience
unsigned long soundStopsAt = 0; //active playing sound, for convenience
unsigned long currentMillis = 0;
unsigned long sequenceToStop = 0;

static void playSound(int i) {
  Serial.println("Sound playing");
  //NoiseAmp = 200; //90 dB @ 1m, but instead schedule for cosine fade
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(TTL_OUTPUT_PIN, HIGH);
  soundStartedAt = currentMillis; //schedule, for cosine fade
  soundStopsAt = soundToStop[i];
  soundToStart[i] = 0; //clear assignment
}

static void silenceSound(int i) {
  Serial.println("Sound silenced"); 
  //DEBUG (WAS UNCOMMENTED) NoiseAmp = 0;
  //DEBUG (WAS UNCOMMENTED) digitalWrite(RELAY_PIN, LOW);
  digitalWrite(TTL_OUTPUT_PIN, LOW);
  soundStartedAt = 0; //clear indication that sound is playing
  soundStopsAt = 0;
  soundToStop[i] = 0; //clear assignment     
}

static void startSequence() {
  Serial.println("Sequence starts"); 
  digitalWrite(SEQUENCE_LED_PIN, HIGH);
  for (unsigned int i=0; i < SOUND_COUNT; i++) {
    soundToStart[i] = currentMillis + i * (SOUND_DURATION + GAP_DURATION) + BOOKEND_DURATION;
    soundToStop[i] = soundToStart[i] + SOUND_DURATION;
  }
  sequenceToStop = currentMillis + RECORDING_DURATION;
}

static void stopSequence() {
  Serial.println("Sequence finished"); 
  digitalWrite(SEQUENCE_LED_PIN, LOW);
  for (unsigned int i=0; i < SOUND_COUNT; i++) { //ensure that no sounds remain scheduled
    soundToStart[i] = 0;
    soundToStop[i] = 0;
  }
  sequenceToStop = 0; //clear assignment
}

static void testHandler(uint8_t btnId, uint8_t btnState) {
  if (btnState == BTN_PRESSED) {
    Serial.println("Pressed test button, testing...");
    playSound(0);
  } else {
    // btnState == BTN_OPEN
    Serial.print("Released test button, test to stop in ");
    Serial.print(COSINE_PERIOD);
    Serial.println(" ms.");
    soundToStop[0] = max(currentMillis, soundStartedAt+COSINE_PERIOD) + COSINE_PERIOD;    //silenceSound();
    soundStopsAt = soundToStop[0];
  }
}

static void sequenceHandler(uint8_t btnId, uint8_t btnState) {
  if ((btnState == BTN_PRESSED) && !soundToStop[SOUND_COUNT]) {
    Serial.println("Pressed sequence button");
    if(sequenceToStop) {
      Serial.println("Sequence already active");
    } else {
      startSequence();
    }
  } else { // btnState == BTN_OPEN.
    Serial.println("Released sequence button");
  }
}

static void stopHandler(uint8_t btnId, uint8_t btnState) {
  if (btnState == BTN_PRESSED) {
    Serial.println("Pressed stop button");
    silenceSound(0); // stop active sound, if any
    stopSequence(); // unschedule all sounds
  } else { // btnState == BTN_OPEN
    Serial.println("Released stop button");
  }
}

// Define button with a unique id (0) and handler function.
// (The ids are so one handler function can tell different buttons apart if necessary.)
static Button seqButton(0, sequenceHandler);
static Button testButton(1, testHandler);
static Button stopButton(2, stopHandler);

static void pollButtons() {
  // update() will call buttonHandler() if PIN transitions to a new state and stays there
  // for multiple reads over 25+ ms.
  seqButton.update(digitalRead(SEQUENCE_BUTTON_PIN));
  testButton.update(digitalRead(TEST_BUTTON_PIN));
  stopButton.update(digitalRead(STOP_BUTTON_PIN));
}

void setup() { 
  pinMode(TEST_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SEQUENCE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SEQUENCE_LED_PIN, OUTPUT);
  pinMode(TTL_OUTPUT_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); //DEBUG (WAS LOW))

  Setup_DAWG(); //Due Arbitrary Waveform Generator - not my acronym haha
  NoiseAmp=VOLUME; //DEBUG (WAS 0) -- this only controls noise (not waveform) amplitude

  // TONES
  UserChars[1] = '0'; //set serial input to mimic 'w0' ie change to waveform 0 ie sinusoidal
  ChangeWaveShape(true);
  
  UserInput = 4000; //set serial input to mimic '4000h' ie change to 4000 Hz frequency
  SetFreqPeriod();

  //if (!ExactFreqMode) ToggleExactFreqMode(); //exact freq mode is needed for software volume control, otherwise we copy entire buffer at a time in

  //SinAmp=0.10; //change volume and recalculate wave
  WaveAmp=1048576; //1,048,576 = 100% volume. TODO: May need to turn NoiseAmp into a uint32_t as well
  CreateNewWave();
}

void loop() { 
  pollButtons();
  currentMillis = millis();

  for (unsigned int i=0; i < SOUND_COUNT; i++) {
    if (!soundStartedAt && soundToStart[i] && (currentMillis > soundToStart[i])) {playSound(i);}
    if (soundStartedAt && soundToStop[i] && (currentMillis > soundToStop[i])) {silenceSound(i);}
    if (sequenceToStop && (currentMillis > sequenceToStop)) {stopSequence();}
  }

  float elapsed = currentMillis - soundStartedAt; //float so we get reasonable math below rather than integer math
  float remaining = soundStopsAt - currentMillis;
  if (soundStartedAt && elapsed < COSINE_PERIOD) { //in cosine gate at start, fade up
    uint16_t j = constrain((COS_TABLE_SIZE-1) * (COSINE_PERIOD - elapsed) / COSINE_PERIOD, 0, COS_TABLE_SIZE-1);
    NoiseAmp = VOLUME * pgm_read_word_near(cosTable + j) / COS_TABLE_AMPLITUDE;
    //Serial.println(NoiseAmp);
  } if (soundStartedAt && soundStopsAt - currentMillis < COSINE_PERIOD) { //in cosine gate at end, fade down
    uint16_t j = constrain((COS_TABLE_SIZE-1) * (COSINE_PERIOD - remaining) / COSINE_PERIOD, 0, COS_TABLE_SIZE-1);
    NoiseAmp = VOLUME * pgm_read_word_near(cosTable + j) / COS_TABLE_AMPLITUDE;
    //Serial.println(NoiseAmp);
  }

  Loop_DAWG(); //Due Arbitrary Waveform Generator - not my acronym haha
  delay(1);
}