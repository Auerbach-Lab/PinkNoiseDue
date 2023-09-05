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
#define COSINE_PERIOD            1   // ms duration of cosine gate function, must be less than or equal to 1/2 SOUND_DURATION
#define SOUND_COUNT             18   // total number of samples to play

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
// Range of usable coefficients is 1,000,000 to 489 (for minimum amplitude of 489/1,000,000 = 2/4096 for 12 bit DAC)
// Coefficients for noise are up through 65535 and lowest three are sentinel values for altering volume with potentiometers
const uint32_t volume_noise[9]  = {65535,23000,7300,2300,730,200,199,198,197};
const uint32_t volume_tone4[9]  = {460000,145000,46000,14500,5200,1900,800,505,491};
const uint32_t volume_tone8[9]  = {320000,100000,32000,10250,3500,1400,580,495,489};
const uint32_t volume_tone16[9] = {700000,225000,75000,23000,8500,2800,1200,525,492};
const uint32_t volume_tone32[9] = {1000000,316228,100000,31623,10000,3500,1500,580,502};
const uint8_t  r[SOUND_COUNT] = {5,1,7,2,3,6,0,8,4,7,3,8,6,0,2,5,4,1}; //fixed random order to play the volumes in


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
#define SILENCE '2'
#define NOISE '4'

uint32_t soundAmplitude[SOUND_COUNT] = {0};
unsigned long soundToStart[SOUND_COUNT] = {0};
unsigned long soundToStop[SOUND_COUNT] = {0};
unsigned long soundStartedAt = 0; //active playing sound, for convenience
unsigned long soundStopsAt = 0; //active playing sound, for convenience
unsigned long currentMillis = 0;
unsigned long sequenceToStop = 0;

char waveShape = SILENCE; // changing this here has no effect on startup value, startup is controlled by the DAWG library
int32_t frequency = 0;
uint32_t volume = 0;

extern TwoWire Wire1; // use SCL1 & SDA1 for I2c to potentiometers
Adafruit_DS1841 ds0; //logarithmic potentiometer DS1841
Adafruit_DS1841 ds1; 
int8_t potTap = 0; //controls output of potentiometers, 0-127
int8_t potTap_old = -1;
int8_t potTap_min = 0;

void updatePots(uint8_t tap) {
  if (tap != potTap_old) {
    ds0.setWiper(tap);
    ds1.setWiper(tap);
    potTap_old = tap;
    Serial.print("fade "); Serial.print(tap); Serial.println("");
  } 
}

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

//Expects a number between 489 and 1,000,000 used as a coefficient for amplitude
//(or 10-65535 for noise)
void changeVolumeHelper(uint32_t amplitude) {
  potTap_min = 0; //reset minimum by default, regardless of shape
  if (waveShape == SINUSOIDAL) {
    SinAmp = amplitude/1000000.0; //sinamp is a float
    CreateWaveFull(0); //the 0 specifies waveshape 0, sinusoidal
  } else {
    NoiseAmp = (uint16_t) amplitude; //noiseamp is a uint32_t
    //noiseamp takes effect immediately, no need to rebuild wave

    //hacky fix for getting the lowest volumes from noise: use the potentiometers
    //lowest three volumes are amplitudes of 199,198,197
    //both pots at 75 gives -10dB, both at 109 gives -20 dB, both at 125 gives -30 dB
    if (amplitude == 199) potTap_min = 75;
    if (amplitude == 198) potTap_min = 109;
    if (amplitude == 197) potTap_min = 125;
  } 
  volume = amplitude;
  Serial.print("Volume changed to "); Serial.print(volume); Serial.println("");
}

static void playSound(int i) {
  Serial.println("Sound playing");
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(TTL_OUTPUT_PIN, HIGH);
  changeWaveHelper(waveShape);
  soundStartedAt = millis(); //schedule, for cosine fade
  soundStopsAt = soundToStop[i];
  soundToStart[i] = 0; //clear the assignment
}

static void silenceSound(int i) {
  Serial.println("Sound silenced"); 
  changeWaveHelper(SILENCE); 
  digitalWrite(TTL_OUTPUT_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
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
    soundToStop[0] = max(currentMillis, soundStartedAt+COSINE_PERIOD) + COSINE_PERIOD;
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
    Serial.println("Pressed abort button");
    silenceSound(0); // stop active sound, if any
    stopSequence(); // unschedule all sounds
  } else { // btnState == BTN_OPEN
    Serial.println("Released abort button");
  }
}

static void selectorHandler(uint8_t btnId, uint8_t btnState) {
  //First, process an abort since scheduled volumes will be wrong
  silenceSound(0); // stop active sound, if any
  stopSequence(); // unschedule all sounds

  //Then..
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
    if (waveShape == NOISE) {
      changeVolumeHelper(volume_noise[0]);
    } else if (frequency == 4000) {
      changeVolumeHelper(volume_tone4[0]);
    } else if (frequency == 8000) {
      changeVolumeHelper(volume_tone8[0]);
    } else if (frequency == 16000) {
      changeVolumeHelper(volume_tone16[0]);
    } else if (frequency == 32000) {
      changeVolumeHelper(volume_tone32[0]);
    }
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
  
  //digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(RELAY_PIN, LOW);
  potTap = 127; // quiet (max resistance) | 0 is loud (min resistance)
  updatePots(potTap);
  Setup_DAWG(); //Due Arbitrary Waveform Generator - not my acronym haha  
  if (ExactFreqMode) ToggleExactFreqMode(); //we DON'T want to be in exact mode, which has nasty harmonics at 32khz
  NoiseAmp = 0;
}

void loop() { 
  pollButtons();
  currentMillis = millis();
  static unsigned long elapsed;
  static unsigned long remaining;
  
  for (unsigned int i=0; i < SOUND_COUNT; i++) {
    //specify volume for next sound shortly (1s) before it plays
    if (!soundStartedAt && soundToStart[i] && (currentMillis + 1000 > soundToStart[i])) {
      if (volume != soundAmplitude[i]) {changeVolumeHelper(soundAmplitude[i]);}
    }
    //play sound
    if (!soundStartedAt && soundToStart[i] && (currentMillis > soundToStart[i])) {playSound(i);}
  }

  currentMillis = millis();
  elapsed = currentMillis - soundStartedAt; //float so we get reasonable math below rather than integer math
  remaining = soundStopsAt - currentMillis;

  //play sound, fading up or down as needed
  static uint16_t j;
  if (soundStartedAt && remaining == 0) { //min volume
    potTap = 127;
    //Serial.print("off ");
    updatePots(potTap);
  } else if (soundStartedAt && remaining <= COSINE_PERIOD) { //in cosine gate at end, fade down
    j = constrain((COS_TABLE_SIZE-1) * remaining / COSINE_PERIOD, 0, COS_TABLE_SIZE-1);
    potTap = potTap_min + (127-potTap_min) * pgm_read_word_near(cosTable + j) / COS_TABLE_AMPLITUDE;
    //Serial.print("down ");
    updatePots(potTap);
  } else if (soundStartedAt && elapsed <= COSINE_PERIOD) { //in cosine gate at start, fade up
    j = constrain((COS_TABLE_SIZE-1) * elapsed / COSINE_PERIOD, 0, COS_TABLE_SIZE-1);
    potTap = potTap_min + (127-potTap_min) * pgm_read_word_near(cosTable + j) / COS_TABLE_AMPLITUDE;
    //Serial.print("up ");
    updatePots(potTap);
  } else if (soundStartedAt && elapsed > COSINE_PERIOD && elapsed < remaining) { //full volume
    potTap = potTap_min;
    //Serial.print("full ");
    updatePots(potTap);
  } 

  for (unsigned int i=0; i < SOUND_COUNT; i++) {
    //silence sound
    if (soundStartedAt && soundToStop[i] && (currentMillis >= soundToStop[i])) {silenceSound(i);}
    if (sequenceToStop && (currentMillis >= sequenceToStop)) {stopSequence();}
  }
  

  Loop_DAWG(); //Due Arbitrary Waveform Generator - not my acronym haha
  //Serial.print(foo); Serial.print("   "); Serial.print(bar); Serial.print("   "); Serial.print(baz);Serial.println("");
  delay(0); //sound production itself is interrupt-driven, so this just spends less time in the keypad processing and fading volumes
}