#include <Arduino.h>
#include <debounce.h>
#include <DueFlashStorage.h>
#include <efc.h>
#include <flash_efc.h>

#define SEQUENCE_BUTTON_PIN 4
#define TEST_BUTTON_PIN 3
#define TTL_OUTPUT_PIN 7

// EDIT THESE VALUES to adjust timings on existing sequence.
#define OFFSET 1000           // ms between steps in sequence
#define IMAGE_DURATION 500    // ms duration of TTL signal for imaging
#define SOUND_DURATION 5000   // ms duration of sound to play

// EDIT THIS SECTION to define sequence itself
unsigned long currentMillis = 0;
unsigned long imageStart[3] = {0};
unsigned long imageStop[3] = {0};
unsigned long soundStart = 0;
unsigned long soundStop = 0;
bool sendingTTL = false;
bool playingSound = false;

static void sequenceHandler(uint8_t btnId, uint8_t btnState) {
  if ((btnState == BTN_PRESSED) && !imageStop[2]) {
    Serial.println("Pressed sequence button");
    
    //pre image
    imageStart[0] = currentMillis + OFFSET;
    imageStop[0] = imageStart[0] + IMAGE_DURATION;

    //sound
    soundStart = imageStop[0] + OFFSET;
    soundStop = soundStart + SOUND_DURATION;

    //mid-sound image
    imageStart[1] = soundStart + (SOUND_DURATION - IMAGE_DURATION)/2;
    imageStop[1] = imageStart[1] + IMAGE_DURATION;

    //post image
    imageStart[2] = soundStop + OFFSET;
    imageStop[2] = imageStart[2] + IMAGE_DURATION;
    
  } else {
    // btnState == BTN_OPEN.
    Serial.println("Released sequence button");
  }
}

static void playSound() {
  Serial.println("Sound playing");
  //analogWrite(AUDIO_OUTPUT_PIN, 128); 
  //digitalWrite(SOUND_GATE_PIN, HIGH);
  playingSound = true;
  soundStart = 0; //clear assignment
}

static void silenceSound() {
  Serial.println("Sound silenced"); 
  // analogWrite(AUDIO_OUTPUT_PIN, 0); //set duty cycle to 0% but doesn't fully silence, so a transistor/mosfet is needed
  // digitalWrite(SOUND_GATE_PIN, LOW); 
  playingSound = false;
  soundStop = 0; //clear assignment     
}

static void startImaging(unsigned int i) {
  Serial.println("Start imaging");
  digitalWrite(TTL_OUTPUT_PIN, HIGH);
  sendingTTL = true;
  imageStart[i] = 0; //clear assignment
}

static void stopImaging(unsigned int i) {
  Serial.println("Stop imaging");
  digitalWrite(TTL_OUTPUT_PIN, LOW);
  sendingTTL = false;
  imageStop[i] = 0; //clear assignment
}

static void testHandler(uint8_t btnId, uint8_t btnState) {
  if (btnState == BTN_PRESSED) {
    Serial.println("Testing...");
    playSound();
    startImaging(0);
  } else {
    // btnState == BTN_OPEN
    Serial.println("Test stop");
    silenceSound();
    stopImaging(0);
  }
}

// Define button with a unique id (0) and handler function.
// (The ids are so one handler function can tell different buttons apart if necessary.)
static Button seqButton(0, sequenceHandler);
static Button testButton(1, testHandler);

static void pollButtons() {
  // update() will call buttonHandler() if PIN transitions to a new state and stays there
  // for multiple reads over 25+ ms.
  seqButton.update(digitalRead(SEQUENCE_BUTTON_PIN));
  testButton.update(digitalRead(TEST_BUTTON_PIN));
}

void setup() {
  analogReadResolution(12);
  analogWriteResolution(12);
  Serial.begin (115200);
  Serial.setTimeout(50);
  
  pinMode(SEQUENCE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TEST_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TTL_OUTPUT_PIN, OUTPUT);
}

void loop() { // nothing here for ongoing pink noise, all driven by ISR
  pollButtons();
  currentMillis = millis();
  
  for (unsigned int i=0; i < sizeof imageStart / sizeof imageStart[i]; i++) {
    if(!sendingTTL && imageStart[i] && (currentMillis > imageStart[i])) startImaging(i);
    if(sendingTTL && imageStop[i] && (currentMillis > imageStop[i])) stopImaging(i);
    if(!playingSound && soundStart && (currentMillis > soundStart)) playSound();
    if(playingSound && soundStop && (currentMillis > soundStop)) silenceSound();
  }

  delay(10);
}