#include <Arduino.h>
#include <debounce.h>
#include "costable.h"
#include "DueArbitraryWaveformGeneratorV2.h"
#include <Wire.h>
#include <Adafruit_DS1841.h>

// EDIT THESE VALUES to adjust timings on sequence
// define either recording_duration OR sound_count here, and calculate the other below
// #define RECORDING_DURATION  600000   // ms duration of entire recording sequence, must be at least BOOKEND_DURATION * 2 + SOUND_DURATION for a single sound
#define BOOKEND_DURATION     60000   // ms duration of silence at beginning and end, must be less than 1/2 RECORDING_DURATION
#define GAP_DURATION         25000   // ms between sounds
#define SOUND_DURATION        5000   // ms duration of sound to play
#define COSINE_PERIOD          500   // ms duration of cosine gate function, must be less than or equal to 1/2 SOUND_DURATION
#define SOUND_COUNT             18   // total number of samples to play

//DEBUG DEBUG DEBUG
#define BOOKEND_DURATION     1000   // ms duration of silence at beginning and end, must be less than 1/2 RECORDING_DURATION
#define GAP_DURATION         2000   // ms between sounds
#define SOUND_DURATION        5000   // ms duration of sound to play
#define COSINE_PERIOD          500   // ms duration of cosine gate function, must be less than or equal to 1/2 SOUND_DURATION
#define SOUND_COUNT             18   // total number of samples to play
//DEBUG DEBUG DEBUG

// DO NOT EDIT 
// total number of sounds that will play, if defined recording_duration above
// +GAP in numerator because last tone does not need GAP included to fit
// will be 11 under default settings of 600s recording, 120s bookend, 30s gap, 5s sound
// unsigned const int SOUND_COUNT = (RECORDING_DURATION - 2*BOOKEND_DURATION + GAP_DURATION) / (SOUND_DURATION + GAP_DURATION);

// DO NOT EDIT
// total duration of entire recording sequence, if defined sound_count instead of recording_duration
const uint32_t RECORDING_DURATION = SOUND_COUNT * (SOUND_DURATION + GAP_DURATION) + 2*BOOKEND_DURATION - GAP_DURATION;

// VOLUME CALIBRATION
// These are given as amplitude ratios of the waveform, i.e. direct control on arduino
// In theory, each -10 dB is a multiplier of 0.316227766 to amplitude
// Range of usable coefficients is 1,000,000 to 490 (for minimum amplitude of 490/1,000,000 = 2/4096 for 12 bit DAC)
const uint32_t volume_noise[9]  = {500000,158115,50000,15812,5000,1581,875,515,492};
const uint32_t volume_tone4[9]  = {500000,158115,50000,15812,5000,1581,875,515,492};
const uint32_t volume_tone8[9]  = {500000,158115,50000,15812,5000,1581,875,515,492};
const uint32_t volume_tone16[9] = {500000,158115,50000,15812,5000,1581,875,515,492};
const uint32_t volume_tone32[9] = {500000,158115,50000,15812,5000,1581,875,515,492};
const uint8_t  r[SOUND_COUNT] = {5,1,7,2,3,6,0,8,4,7,3,6,8,0,2,5,4,1}; //fixed random order to play the volumes in
uint32_t volume = 0;

//A2 56 and A4 58 are available, but do not use A3, causes noise on boot, stop before start, other weirdness
//odd pins from 25 through 39 are available now
//13 is the onboard LED but otherwise unused
#define TEST_BUTTON_PIN 25
#define SEQUENCE_BUTTON_PIN 27
#define SEQUENCE_LED_PIN 29
#define TTL_OUTPUT_PIN 31
#define RELAY_PIN 33
#define STOP_BUTTON_PIN 35

#define NOISE_PIN  45
#define TONE4_PIN  47
#define TONE8_PIN  49
#define TONE16_PIN 51
#define TONE32_PIN 53

// wave shapes
#define SINUSOIDAL '0'
#define NOISE '4'
#define SILENCE '2'

uint32_t soundAmplitude[SOUND_COUNT] = {0};
unsigned long soundToStart[SOUND_COUNT] = {0};
unsigned long soundToStop[SOUND_COUNT] = {0};
unsigned long soundStartedAt = 0; //active playing sound, for convenience
unsigned long soundStopsAt = 0; //active playing sound, for convenience
unsigned long currentMillis = 0;
unsigned long sequenceToStop = 0;

char waveShape = SILENCE; // changing this here has no effect on startup value, startup is controlled by the DAWG library
int32_t frequency = 0;

Adafruit_DS1841 ds0; //logarithmic potentiometer DS1841
Adafruit_DS1841 ds1; 
int8_t potentiometerTap; //controls output of potentiometers, 0-127
int8_t potentiometerTap_old;
extern TwoWire Wire1; // use SCL1 & SDA1 for I2c to potentiometers

//'0' sinusoidal, '2' arbitrary wave (silence), '4' noise
void changeWaveHelper(char shape) {
  UserChars[1] = shape; //set serial input to mimic e.g. 'w0' ie change to waveform 0 sinusoidal
  ChangeWaveShape(true);
}

void changeFreqHelper(uint16_t freq) {
  UserInput = freq; //set serial input to mimic e.g. '4000h' ie change to 4000 Hz frequency
  SetFreqPeriod();
  CreateWaveFull(0); //the 0 specifies waveshape 0, sinusoidal
  UserInput = 0;
  frequency = freq;
}

//Expects a number between 490 and 1,000,000 used as a coefficient for amplitude
//490 is so minimum amplitude is 2/4096 since 12-bit DAC
void changeVolumeHelper(uint32_t amplitude) {
  SinAmp = amplitude/1000000.0;
  CreateWaveFull(0); //the 0 specifies waveshape 0, sinusoidal
  //TODO noiseamp
  volume = amplitude;
  Serial.print("Volume changed to "); Serial.print(volume); Serial.println("");
}

static void playSound(int i) {
  Serial.println("Sound playing");
  //digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(TTL_OUTPUT_PIN, HIGH);
  changeWaveHelper(waveShape);
  soundStartedAt = currentMillis; //schedule, for cosine fade
  soundStopsAt = soundToStop[i];
  soundToStart[i] = 0; //clear the assignment
}

static void silenceSound(int i) {
  Serial.println("Sound silenced"); 
  //digitalWrite(RELAY_PIN, LOW);
  digitalWrite(TTL_OUTPUT_PIN, LOW);
  changeWaveHelper(SILENCE); 
  NoiseAmp = 0;
  soundStartedAt = 0; //clear the indication that sound is playing
  soundStopsAt = 0;
  soundToStop[i] = 0; //clear the assignment     
}

static void startSequence() {
  Serial.println("Sequence starts"); 
  digitalWrite(SEQUENCE_LED_PIN, HIGH);
  for (unsigned int i=0; i < SOUND_COUNT; i++) {
    soundToStart[i] = currentMillis + i * (SOUND_DURATION + GAP_DURATION) + BOOKEND_DURATION;
    soundToStop[i] = soundToStart[i] + SOUND_DURATION;
    
    if (waveShape == NOISE) {
      soundAmplitude[i] = volume_noise[r[i]];
    } else if (frequency == 4000) {
      soundAmplitude[i] = volume_tone4[r[i]];
    } else if (frequency == 8000) {
      soundAmplitude[i] = volume_tone8[r[i]];
    } else if (frequency == 16000) {
      soundAmplitude[i] = volume_tone16[r[i]];
    } else if (frequency == 32000) {
      soundAmplitude[i] = volume_tone32[r[i]];
    }
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
  if ((btnState == BTN_PRESSED)) {
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

  //DEBUG DEBUG DEBUG
  potentiometerTap = 0; // loud
  // ds0.setWiper(potentiometerTap);
  // ds1.setWiper(potentiometerTap);
}

static void selectorHandler(uint8_t btnId, uint8_t btnState) {
  if (btnState == BTN_PRESSED) {
    if (btnId == 3) { //pink noise setting
      Serial.println("Selected pink noise");
      waveShape = NOISE; //wave shape 4 is noise
      frequency = -1;
    } else {
      Serial.print("Selected "); Serial.print(btnId); Serial.print(" kHz sinusoidal tone."); Serial.println("");
      waveShape = SINUSOIDAL; //wave shape 0 is sinusoidal
      changeFreqHelper(btnId*1000); //btnId specifies frequency in kHz
    }
    //changeWaveHelper(waveShape); // DEBUG - for normal use do not want to actually change immediately because that would change away from silence.
  }
}

// Define button with a unique id (0) and handler function.
// (The ids are so one handler function can tell different buttons apart if necessary.)
static Button seqButton(0, sequenceHandler);
static Button testButton(1, testHandler);
static Button stopButton(2, stopHandler);
static Button pinkButton(3, selectorHandler);
static Button tone4Button(4, selectorHandler);
static Button tone8Button(8, selectorHandler);
static Button tone16Button(16, selectorHandler);
static Button tone32Button(32, selectorHandler);

static void pollButtons() {
  // update() will call buttonHandler() if PIN transitions to a new state and stays there
  // for multiple reads over 25+ ms.
  seqButton.update(digitalRead(SEQUENCE_BUTTON_PIN));
  testButton.update(digitalRead(TEST_BUTTON_PIN));
  stopButton.update(digitalRead(STOP_BUTTON_PIN));
  pinkButton.update(digitalRead(NOISE_PIN));
  tone4Button.update(digitalRead(TONE4_PIN));
  tone8Button.update(digitalRead(TONE8_PIN));
  tone16Button.update(digitalRead(TONE16_PIN));
  tone32Button.update(digitalRead(TONE32_PIN));
}

void setup() { 
  Serial.begin(115200);
  analogReadResolution(12);
  analogWriteResolution(12);

  pinMode(TEST_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SEQUENCE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SEQUENCE_LED_PIN, OUTPUT);
  pinMode(TTL_OUTPUT_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(NOISE_PIN, INPUT_PULLUP);
  pinMode(TONE4_PIN, INPUT_PULLUP);
  pinMode(TONE8_PIN, INPUT_PULLUP);
  pinMode(TONE16_PIN, INPUT_PULLUP);
  pinMode(TONE32_PIN, INPUT_PULLUP);

  digitalWrite(RELAY_PIN, HIGH);
  
  // Try to initialize!
  Wire1.begin();        // join i2c bus
  delay(10);
  while (!ds0.begin(0x28, &Wire1)) {
    Serial.println("Failed to find DS1841 chip at 0x28");
    Wire1.begin(); 
    delay(100);
  }
  while (!ds1.begin(0x2A, &Wire1)) {
    Serial.println("Failed to find DS1841 chip at 0x2A");
    Wire1.begin(); 
    delay(100);
  }
  //potentiometerTap = 127; // quiet (127 is max resistance, min volume)
  potentiometerTap = 0; // loud
  ds0.setWiper(potentiometerTap);
  ds1.setWiper(potentiometerTap);
  
  Setup_DAWG(); //Due Arbitrary Waveform Generator - not my acronym haha
  //NoiseAmp=VOLUME; //DEBUG (WAS 0) -- this only controls noise (not waveform) amplitude
  if (ExactFreqMode) ToggleExactFreqMode(); //we DON'T want to be in exact mode, which has nasty harmonics at 32khz

  //TODO: May need to turn SinAmp into uint32_t and replace waveAmp for use in fastmode
  //TODO: May want to turn NoiseAmp into a uint32_t as well and work on same scale? And change divisions to shifts?
  //Need to test getting 90-30 dB of noise
}

void loop() { 
  pollButtons();
  currentMillis = millis();
  //Serial.print("Wiper: "); Serial.print(ds.getWiper()); Serial.println(" LSB");

  for (unsigned int i=0; i < SOUND_COUNT; i++) {
    if (!soundStartedAt && soundToStart[i] && (currentMillis + 1000 > soundToStart[i])) {
      if (volume != soundAmplitude[i]) {changeVolumeHelper(soundAmplitude[i]);}
    }
    if (!soundStartedAt && soundToStart[i] && (currentMillis > soundToStart[i])) {playSound(i);}
    if (soundStartedAt && soundToStop[i] && (currentMillis > soundToStop[i])) {silenceSound(i);}
    if (sequenceToStop && (currentMillis > sequenceToStop)) {stopSequence();}
  }

  float elapsed = currentMillis - soundStartedAt; //float so we get reasonable math below rather than integer math
  float remaining = soundStopsAt - currentMillis;
  if (soundStartedAt && elapsed < COSINE_PERIOD) { //in cosine gate at start, fade up
    uint16_t j = constrain((COS_TABLE_SIZE-1) * elapsed / COSINE_PERIOD, 0, COS_TABLE_SIZE-1);
    potentiometerTap = 127 * pgm_read_word_near(cosTable + j) / COS_TABLE_AMPLITUDE;
    if (potentiometerTap != potentiometerTap_old) {
      ds0.setWiper(potentiometerTap);
      ds1.setWiper(potentiometerTap);
      potentiometerTap_old = potentiometerTap;
      //Serial.print("fade up"); Serial.print(potentiometerTap); Serial.println("");
    }
  } if (soundStartedAt && soundStopsAt - currentMillis < COSINE_PERIOD) { //in cosine gate at end, fade down
    uint16_t j = constrain((COS_TABLE_SIZE-1) * remaining / COSINE_PERIOD, 0, COS_TABLE_SIZE-1);
    potentiometerTap = 127 * pgm_read_word_near(cosTable + j) / COS_TABLE_AMPLITUDE;
    if (potentiometerTap != potentiometerTap_old) {
      ds0.setWiper(potentiometerTap);
      ds1.setWiper(potentiometerTap);
      potentiometerTap_old = potentiometerTap;
      //Serial.print("fade down"); Serial.print(potentiometerTap); Serial.println("");
    } 
  }

  Loop_DAWG(); //Due Arbitrary Waveform Generator - not my acronym haha
  delay(1);
}