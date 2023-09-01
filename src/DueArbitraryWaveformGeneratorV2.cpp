#include <Arduino.h>
#include <debounce.h>
#include <DueFlashStorage.h>
#include <efc.h>
#include <flash_efc.h>
#include "DueArbitraryWaveformGeneratorV2.h"


// The Due Arbitrary Waveform Generator was created by Bruce Evans. Version 1 was written in 2017. Some code (specifically some of the "Direct port manipulation" code found mostly at the end of this file) was adapted from Kerry D. Wong, ard_newie, Mark T, MartinL, the Magician and possibly others. Many thanks! Version 2 was developed with some inspiration from mszoke01, chhckm, gagarinui and others who commented on the create.arduino website listed below.
//
// This is Version 2, introduced in 2022:
// Version 2 has the ability to tag more arbitrary waves onto the end of any you've already uploaded from your P.C. (via the accomanying controller GUI [General User Interface] mentioned below which can be found on the website/s also below,)
// Also added is the ability to create random 'arbitrary' waves.
// Also added is the ability for the GUI to reconnect to the Arduino without restarting if USB contact is lost, automatically restoring Arduino settings.
// Also added is the ability to control parameters such as amplitude (volume) and many other parameters, enabling the creation of new wave shapes.
// Also added is the ability to save and reload settings, and to create new default start-up settings.
// Also added is the ability to generate noise using the Arduino Due's unique TRNG feature. White, pink or brown noise, or anything in between, can be selected.
// Also added is the ability to play music, and even compose it with the new Music window in the GUI.
//
// See:  https://create.arduino.cc/projecthub/BruceEvans/arduino-due-arbitrary-waveform-generator-a9d180?f=1 and https://github.com/Bruce-Evans/ArduinoDueArbitraryWaveformGeneratorAndController/releases
//
// To use:
// 1: Upload this sketch to the Arduino Due. DueFlashStorage library is needed for saving settings etc to flash memory.
//
// 2: Connect pins 3 & 7 with a link, or preferably a resistor (approx 47 to 100 ohms). Then take the output from pin 7 if a resistor is used, rather than pin 3 to improve square wave rise and fall times at very high frequency (several megahertz).
//    (The synchronised square wave output is always produced by pin 3. The UNsyncronised square wave output is produced by pin 7, except below 163Hz when it's produced by pin 3 to maximize wave specifications. One pin is disabled when the other is enabled)
//
// 3: The analogue wave output is taken from DAC0. (Can be changed to DAC1 by following the instructions in functions "void dac_setup()" & "void dac_setup2()" at the very end of this script.)
//
// 4: If this AWG is connected to random external equipment, it's strongly recommended that the easily damaged pins 3, 7 and DAC0 be connected via a protection circuit. Details of a suitable protection circuit can be found on the above Arduino create website.
//
// 5: Run the accompanying "DueAWGController" General User Interface program (GUI mentioned above) to control the Arduino from your PC. No need to install it. See its help file (in its data folder, or via the help button in the GUI) for more info.
//
//  or alternatively...
//  If you want to use 2 potentiometers to control frequency & duty cycle, use any value pots between 1k & 50k, and connect them between 0V & 3.3V (NOT 5V!) with their wipers connected to A0 & A1.
//  If you want to use switches to control operations: (No need to connect all switches, LEDs, etc; just the ones you want to use.)
//    Connect a momentary switch between the following pins: (no pull-up resistors required)
//      pin 22 & ground - enable / disable pots, switches & LEDs - 1st press enables switches, 2nd press enables pots, 3rd press enables both, 4th press disables both again.
//      pin 24 & ground - toggle Pot's control of: Unsync'ed square wave: pot FREQ or PERIOD
//      pin 26 & ground - cycle thru: Unsync'ed square wave: Pot's freq / period RANGE
//      pin 28 & ground - toggle Pot's control of: Sync'ed waves: pot FREQ or PERIOD
//      pin 30 & ground - cycle thru: Sync'ed waves: Pot's freq / period RANGE
//      pin 32 & ground - toggle Pot's control of: Unsync'ed square wave: pot DUTY CYCLE or PULSE WIDTH
//      pin 34 & ground - cycle thru: Unsync'ed square wave: Pot's pulse width RANGE (duty cycle range is fixed at 0 - 100%)
//      pin 36 & ground - toggle Pot's control of: Sync'ed waves: pot DUTY CYCLE or PULSE WIDTH
//      pin 38 & ground - cycle thru: Sync'ed waves: Pot's pulse width RANGE (duty cycle range is fixed at 0 - 100%)
//      pin 40 & ground - cycle thru: WAVE SHAPES
//      pin 42 & ground - toggle: Exact Freq Mode ON / OFF
//      pin 44 & ground - toggle: Square Wave Sync ON / OFF
//      pin 46 & ground - cycle thru: Which wave/s to control (unsynch'ed square wave, analogue wave, or both)
//      pin 20 & ground - toggle: Modulation Mode ON / OFF
//      pin  2 & ground - toggle: Freq Sweep ON / OFF
//      pin  5 & ground - toggle: Timer ON / OFF
//      pin 21 & ground - start / stop Sweep or Timer
//      pin  8 & ground - clear keypad switch - press before typing numbers with keypad if old data needs clearing. Also stops tune playing, even if switches are disabled. All other switches are disabled while playing
//      pin  9 & ground - 0 keypad switch
//      pin 10 & ground - 1 keypad switch
//      pin 11 & ground - 2 keypad switch
//      pin 12 & ground - 3 keypad switch
//      pin 14 & ground - 4 keypad switch // pin 13 (with onboard LED) not used
//      pin 15 & ground - 5 keypad switch
//      pin 16 & ground - 6 keypad switch
//      pin 17 & ground - 7 keypad switch
//      pin 18 & ground - 8 keypad switch
//      pin 19 & ground - 9 keypad switch
//      pin 43 & ground - to Save current settings as Start-up Default
//      pin 52 & ground - to Save Preset - press after entering Preset number with keypad. Press twice to replace existing Preset data (LED on pin 50 will light up if data exists) or press clear to cancel
//      pin 53 & ground - to Load Preset - press after entering Preset number with keypad  
//      pin 59 (A5)  & ground - toggle: Music Mode ON / OFF (analogue pins A5 to A11 are setup to be used as digital pins)
//      pin 60 (A6)  & ground - to Save Startup Tune - press after entering tune number with keypad - add 100 to tune number to stay in music mode after playing. To cancel playing at startup, make tune number zero
//      pin 61 (A7)  & ground - to Play a Tune - press after entering number with keypad - will stay in music mode after playing. Press clear while playing to stop
//      pin 62 (A8)  & ground - to set FREQ        - press after entering value in Hz with keypad           - or Sweep MIN FREQ  - or Timer DAYS
//      pin 63 (A9)  & ground - to set PERIOD      - press after entering value in milliSeconds with keypad - or Sweep MAX FREQ  - or Timer HOURS
//      pin 64 (A10) & ground - to set DUTY CYCLE  - press after entering value in percent with keypad      - or Sweep RISE TIME - or Timer MINS
//      pin 65 (A11) & ground - to set PULSE WIDTH - press after entering value in microSeconds with keypad - or Sweep FALL TIME - or Timer SECS
//  If you want to use LEDs to display operation, modes, etc: (not range or wave-shape)
//    Connect a single LED or a dual colour LED or 2 back to back LEDs (in parallel) via a single resistor (approx 560 ohms) between the following pins:
//      pins 51 & ground - indicates: switches enabled (use a single LED only)
//      pins 23 & ground - indicates: pots enabled (use a single LED only)
//      pins  4 & ground - indicates: Freq Sweep ON (use a single LED only)
//      pins  6 & ground - indicates: Timer ON (use a single LED only)
//      pins 25 & 27     - indicates: Unsync'ed square wave: Pot controls FREQ or PERIOD
//      pins 29 & 31     - indicates: Sync'ed wave: Pot controls FREQ or PERIOD
//      pins 33 & 35     - indicates: Unsync'ed square wave: Pot controls DUTY CYCLE or PULSE WIDTH
//      pins 37 & 39     - indicates: Sync'ed wave: Pot controls DUTY CYCLE or PULSE WIDTH
//      pins 41 & ground - indicates: Exact Freq Mode ON (use a single LED only) (pin 43 is low current, so not used here)
//      pins 45 & ground - indicates: Square Wave Sync ON (use a single LED only)
//      pins 47 & ground - indicates: Analogue wave is being controlled (use a single LED only)
//      pins 49 & ground - indicates: Unsynch'ed Square wave is being controlled (use a single LED only)
//      pins 48 & ground - indicates: lights up when keypad etc pressed (use a single LED only)
//      pins 50 & ground - indicates: lights up if confirmation needed to replace existing Preset data when saving Preset (use a single LED only)
//  Note that when pots or switches are enabled, the Arduino will not only accept commands from them, but also from the PC via USB when a signal is received.
//
// or alternatively...
// TO CONTROL FROM THE ARDUINO IDE's SERIAL MONITOR:
// Set your serial monitor to 115200 baud.
// Type the following then press enter (or Send)
// Type:   ?   (or any unused character) to display the help screen. (shortened version of this below)
// Type:   a   to create a new Arbitrary wave or view the menu. Enter the value (between 0 and 4095) for each waypoint. Up to 4096 points. Stepped points can also be easily entered. On-screen instructions and examples will be given. The points will be evenly spread over the period of the wave.
// Type:   X   to delete any existing uploaded Arbitrary wave from memory
// Type:   r   to view a menu of commands for creating a Random arbitrary wave (Pots must be disabled).
// Type:   s   to view a menu of Setup commands for each wave shape.
// Type:   n   to view a menu of Noise commands for the TRNG True Random Noise Generator.
// Type:   w   to cycle through the analogue Wave types (including noise)
// Type:   wr  to cycle through the analogue Wave types in Reverse order
// Type:   wx  to switch directly to a Wave shape, where x is the wave shape number. For example: w0 will go directly to wave 0, the sine wave
// Type:   v   to toggle between Viewing synchronized or unsynchronized square wave                       } to toggle view and control simultaneously:
// Type:  ' '  [spacebar] to toggle between controlling synchronized waves or unsynchronized square wave  } both 'v' & ' ' can be typed (followed by enter)
// Type:   b   to control Both synchronized waves and unsynchronized square wave simultaneously
// Type:   h   to set frequency of wave/s, type required frequency in Hz (not kHz etc) followed by h. so the freq range is: synchronized waves limit: 0.00005h - 100000h (100kHz). unsynchronized square wave limit: 0.000093136h - 42000000h (42MHz) via serial cconnection, or slightly less range using pots
// Type:   m   to set period of wave/s, type required period in Milliseconds followed by m. so the period range is: synchronized waves limit: 20000000m - 0.01m (5hrs 33mins 20secs or 20,0000 Secs - 10 MicroSecs). unsynchronized square wave limit: 10737000m - 0.0000239m (10,737Secs - 23.9 nanoSecs)
// Type:   d   to set Duty-cycle type required percentage duty-cycle (0 - 100) followed by d.
// Type:   u   to set pulse width. Type required pulse width in µ seconds followed by u. PULSE WIDTH WILL REMAIN FIXED until duty-cycle (above) is set instead.
// Type:   e   to toggle Exact Freq Mode on/off (synchronized waves only) eliminating freq steps, but has lower sample rate, & dithering on synchronized sq. wave & sharp edges (so view on oscilliscope with HF filter on)
// Type:   S   to enter the frequency Sweep mode. Follow on-screen instructions.
// Type:   T   to enter the Timer mode. Follow on-screen instructions.
// Type:   P   once to enable switches only, or twice for Pots. 3 times enables both. (4 times returns to disabled)
// Type:   f   to toggle between pot controlling Freq of wave, or period of wave. Synchronized and unsynchronized waves can be set independently by using the ' ' or 'b' command shown above
// Type:   p   to toggle between pot controlling duty-cycle Percent, or Pulse width of wave. Synchronized and unsynchronized waves can be set independently by using the ' ' or 'b' command shown above
// Type:   r   to toggle frequency/period Range of the pots: x 1, x10, x100, x1000 & x 10000 (synchronized & unsynchronized ranges are adjustable independently by using the ' ' or 'b' command shown above)
// Type:   R   to toggle pulse width Range of the pot. (does not affect duty-cycle percent): x1, x10, x100, x1000 & x10000."); (synchronized & unsynchronized ranges are adjustable independently or together by using the ' ' or 'b' command shown above)
// Type:  SD   to Save current settings as Start-up Default settings.
// Type:  LD   to Load start-up Default settings.
// Type:  FD   to load Factory start-up Default settings.
// Type:  SPx  to Save current settings to a Preset. The x is the Preset number: 1 to 50.
// Type:  LPx  to Load Preset settings. The x is the Preset number: 1 to 50.
// Type:  CPx  to Clear Preset settings. The x is the Preset number: 1 to 50.
// Type:  SNxn to Save a Name to a preset. The x is the Preset number and must be 2 digits: 01 to 50. The n is the Name up to 22 characters of any kind, and can even begin with a number.
// Type:  lp   to view a List of Presets with their names.
// Type:  m1   to enter Modulation mode. Use analogue input A2 to amplitude modulate the composite wave, which can be set to any mix of waveshapes.
// Type:  m2   or m3 to enter Music mode. The normal waveform stops, and then you can play previously saved tunes. See PTx below.
// Type:  m0   to enter normal waveform mode with no Modulation. Only this mode works at full sample rate.
// Type:  lt   to view a List of Tunes with their names.
// Type: PTx   to Play a Tune you saved in flash memory using GUI. The x is the Tune number: 1 to 50. (will automatically enter music mode if not already enabled)
// Type: PTxl  as above, but first Load the preset you Linked to the tune, if not already loaded, so it plays with the chosen waveshape. (see LTxPx below)
// Type: PTxr  as above, but first Reload the preset you linked to the tune, even if it's already loaded. (see LTxPx below)
// Type: LTxPx to Link a Tune to a Preset, so it plays with the chosen waveshape. (the tune's instrument must be 'wave')
// Type: STNxn to Save Tune Name. The x must be 2 digits: 01 to 50. The n is the Name up to 28 chars.
// Type: SUTx  to select a Start Up Tune to play when Arduino starts. The x is the Tune number: 1 to 50.
//             (add 100 to Start Up Tune number to not load default settings after playing, but keep the tune's wave settings and stay in music mode. ie: SUT105 for tune 5)

/********************************************************/
// Do not adjust if working okay: Type number before delimiter (This is for testing and if altered, will return to default unless variables: MinMaxDuty, Delay1, 2 & 3 are altered accordingly)
// Type:   M   for setup only. Min/Max duty-cycle percentage, expressed in samples (when synchronized & above 1kHz with Exact Freq Mode off) - will return to default (4) when switching from unsynchronized to synchronized square wave (by typing 'v') [settings below 4 cause polarity reversal due to DMA timing]
// Type:   H   for setup only. delay of square wave for best phase synchronization at High sample rate. ie: 10kHz, 20kHz, 40khz & 100kHz [delay (high or low) can't be adjusted below 1.01kHz] - default is 10
// Type:   L   for setup only. delay of square wave for best phase synchronization at Low sample rate. ie: 1.1kHz, 11kHz, 21kHz, & 41kHz (adj high sample rate delay 1st) - default is 36 - 55
// Type:   D   for setup only. delay of square wave for best phase synchronization at Low duty-cycle, but not 0%. - default is 110

/********************************************************/
// Global variables start with a Capital letter and local variables with a lower case letter. Constants have ALL capitals. Although unconventional, I've personally found this more helpful.
// Common to all Waveforms:
byte PotsEnabled = 0;      //  0 = pots, switches & LEDs disabled. 1 = switches & LEDs enabled. 2 = pots & LEDs enabled. 3 = pots, switches & LEDs enabled
bool PotPulseWidth[3];     // enable / disable pulse width input from pot instead of duty-cycle % {unsynchronized wave, synchronized waves}
// Range of pots:
int   Range[]  = {1, 100, 1, 1}; // {unsynchronized wave freq/period, synchronized waves freq/period, unsynchronized wave Pulse Width Range, synchronized waves Pulse Width Range}
// Unsynchronized ^    ^   // if PotPeriodMode[0] = 1: Range x1 = low period (24.0 nS - 40.9 µS), x10 = per. x 10, x100 = per. x 100, x1000 = per. x 1000, x10000 = per. x 10000 (up to 409.6 mS)
// Unsynchronized ^    ^   // if PotPeriodMode[0] = 0: Range x1 = low freq   (1.26 Hz - 4.0 kHz), x10 = freq x 10, x100 = freq x 100, x1000 = freq x 1000, x10000 = freq x 10000 (up to 42 MHz)
// Sine/triangle wave  ^   // if PotPeriodMode[1] = 0: Range x1 = low freq   (10 mHz - 40.9  Hz), x10 = freq x 10, x100 = freq x 100, x1000 = freq x 1000, x10000 = freq x 10000 (up to 100 kHz)
// Sine/triangle wave  ^   // if PotPeriodMode[1] = 1: Range x1 = low period  (10 µS - 409.6 µS), x10 = per. x 10, x100 = per. x 100, x1000 = per. x 1000, x10000 = per. x 10000 (up to 4.096 Secs)
bool UsingGUI;             //  1 = using the Arbitrary Waveform Generator Controller GUI on the PC
volatile boolean SquareWaveSync = LOW; // HIGH;
byte  Control  = 2;        // control of synchronized waves or unsynchronized square wave: 0 = unsync'ed, 1 = analogue / sync'ed, 2 = both
int   Pot0     = 1000;     // reading from freq pot
int   Pot1     = 2000;     // reading from duty-cycle pot
float DutyReading[] = {50, 50};  // {unsynchronized wave, synchronized waves} duty-cycle reading from the pot
float OldReading[3];             // {unsynchronized wave, synchronized waves} freq / period reading from the pot
bool  PotPeriodMode[3];          // {unsynchronized wave, synchronized waves} 0 = pot adjusts freq of wave, 1 = pot adjusts period of wave
unsigned long SwitchPressedTime; // for debouncing pot switches
unsigned long LEDupdateTime;     // update pot function LED indicators every 300 mSecs (not every cycle of the loop)
double UserInput       = 0;      // Numbers read from serial connection
char   UserChars[5]    = ">   "; // serial characters following UserInput number (above). 1st one read into UserChars[0]. If that character is 's' 'n' or 'r' and more serial characters are available they are read into rest of array
unsigned long TouchedTime = 0;   // detects when enter pressed twice within 500 mSecs for triggering Status message
byte     SweepMode     = 0;      // 0 = Sweep off, 1 =  Set SweepMinFreq, 2 = Set SweepMaxFreq, 3 = Set SweepRiseTime, 4 = Set SweepFallTime, 5 = Run
float    SweepMinFreq  = 20;
float    SweepMaxFreq  = 20000;
uint16_t SweepRiseTime = 20;
uint16_t SweepFallTime = 20;
byte     InterruptMode = 0; // 0 = normal waveform mode. 1 = Modulation mode at half sample rate. 2 = Music mode with Wave instrument at half sample rate. 3 = Music mode with minisoundfont instruments at very low sample rate. 10 = calculating / updating waveforms at quarter sample rate
/********************************************************/
// For the Unsynchronized Square Wave:
//uint16_t ClockDivisor2 = 210;       // for TC_setup3() divides 42MHz (CPU timer 1) by 105 (42 MHz / 210) - clocks at 200kHz - no longer used
uint32_t ClkAFreq = 42000000ul;     // system clock 1
//uint32_t PWMfreq  = 42000000ul;   // enabled later in program if freq is 1300Hz min (lowest poss is 1281)  - min (duty-cycle) pulse width:  12nSec
uint32_t PWMfreq  = 10500000ul;     // enabled later in program if freq is  650Hz min                (~641)  - min (duty-cycle) pulse width:  24nSec
//uint32_t PWMfreq  = 2;            // enabled later in program if freq is  325Hz min                (~321)  - min (duty-cycle) pulse width:  48nSec
//uint32_t PWMfreq  = 4;            // enabled later in program if freq is  163Hz min                (~161)  - min (duty-cycle) pulse width:  96nSec
volatile uint32_t PulsePeriod[] = {1010, 2020}; // {Pulse, Period}  -  if freq is < 163Hz, used by interupt handler - min (duty-cycle) pulse width: 96nSec
bool     PeriodHalf;                // identifies which half -  if freq is < 163Hz, used by interupt handler
int      MicroPeriodMultiplier = 2; // for calculating microSec period at different pwm freqs
double   Period       = 42000;      // period in 'timer counts' of 24 nSecs per count, so 42,000 = 1 mSec or 1 kHz
double   Pulse        = 21000;      // pulse width in 'timer counts' of 24 nSecs per count
int      FreqReading  = 1000;       // unsynchronized square wave freq (reading from pot)
double   TargetFreq   = 1000;       // unsynchronized square wave target freq    <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< DETERMINES START-UP FREQUENCY for Unsynchronized Square Wave
float    TargetPeriod = 0;          // only used for saving to flash and sending to GUI (0 if not set)
float    TargetDuty   = 50;        // unsynchronized square wave target duty-cycle  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< DETERMINES START-UP DUTY-CYCLE for Unsynchronized Square Wave
double   ActualFreq   = 1000;     // unsynchronized square wave calculated actual freq
float    ActualDuty   = 50;      // unsynchronized square wave actual duty-cycle
bool     MinOrMaxDuty;          // if duty-cycle is 0% or 100% only 1 half of unsynchronized square wave is displayed - if freq is < 163Hz, used by interupt handler
float    TargetPulseWidth = 0; // unsynchronized square wave target pulse width in microsecs (0 if not set)
float    MicroPulseWidth;     // pulse width calculated in microSeconds
float    MicroPeriod;        // period calculated in microSeconds
byte     TimerMode;         // time period can be entered - in days, hours, minutes, seconds (max period is 4294967296 days, or 11,767,033.6 years!) - if freq is < 163Hz, used by interupt handler
bool     TimerInvert;      // sets timer ouput neg instead of pos
bool     TimerRun;        // reset or start timer running
bool     TimeUp;         // output goes positive when time entered has passed
uint32_t TimeIncrement; // 200kHz, 5 microseconds clock rate - if freq is < 163Hz - used by interupt handler
uint32_t OldTime;       // previous time increment or wavebit - used with timer
byte     OldSec;        // previous sec - used with timer
byte     TimerSecs;     // time that has passed since resetting timer
byte     TimerMins;     // as above
byte     TimerHours;    // as above
uint32_t TimerDays;     // as above
byte     PeriodS = 10;  // seconds - Target time period for timer
byte     PeriodM;       // minutes
byte     PeriodH;       // hours
uint32_t PeriodD;       // days
bool     SecChanged;    // indicates when second changed
byte     LowFreqDisplay = 0; // if freq below 0.5Hz. 1 = info ready to display, 2 = display info (time since start of cycle)
/********************************************************/
// For Sine, Triangle, Arbitrary and Composite (Analogue) Waveforms: (can be accompanied by synchronized square wave)
#define  WAVERESOL-1 4095           // resolution of waves - 1 (12 bit)
#define  WAVERESOL   4096           // resolution of waves (12 bit)
#define  HALFRESOL   2048           // half resolution of waves
#define  NWAVEFULL   4096           // number of points (size) in Full wave-table for slow mode
#define  NWAVETABLE   160           // max number of points (size) in wave-table for fast mode
#define  NARBWAVE    4096           // max number of waypoints in arbitrary wave
int      SamplesPerCycle[] = {160, 80, 40, 16}; // {FastMode0, FastMode1, FastMode2, FastMode3}
volatile boolean WaveHalf = LOW;    // identifies current 1/2 cycle
//      1/2 cycle ID    ______{0,  1,  2, 3}   {0,  1,  2, 3} <-- FastMode ID
//                 |   |
//                 V   V
volatile int  Duty[ ] [4] = {{80, 40, 20, 8}, {80, 40, 20, 8}}; // Samples per 1/2 cycle - changes with duty cycle
// here we are storing 12 bit samples in 16 bit ints arrays:
int16_t  WaveSin[NWAVEFULL + 1];     // Stores Sine wave half 1 in 16 bit signed int array to allow wave manipulation before sending to WaveFull for reading. 1st half of cycle
int16_t  WaveSin2[NWAVEFULL + 1];    // Stores Sine wave half 2 in 16 bit signed int array to allow wave manipulation before sending to WaveFull for reading. 2nd half of cycle
int16_t  WaveTri[NWAVEFULL + 1];     // Stores Triangle wave half 1 in 16 bit signed int array to allow wave manipulation before sending to WaveFull for reading. 1st half of cycle
int16_t  WaveTri2[NWAVEFULL + 1];    // Stores Triangle wave half 2 in 16 bit signed int array to allow wave manipulation before sending to WaveFull for reading. 2nd half of cycle
int16_t  WaveArb[NWAVEFULL + 1];     // Stores Arbitrary wave in 16 bit signed int array to allow wave manipulation before sending to WaveFull for reading.
int16_t  WaveCom[NWAVEFULL + 1];     // Stores Composite wave half 1 in 16 bit signed int array to allow wave manipulation before sending to WaveFull for reading. 1st half of cycle
int16_t  WaveCom2[NWAVEFULL + 1];    // Stores Composite wave half 2 in 16 bit signed int array to allow wave manipulation before sending to WaveFull for reading. 2nd half of cycle
int16_t  WaveFull[NWAVEFULL + 1];    // 1st wave half wave-table for slow mode used up to 1kHz, and in ExactFreqMode at all freq's (4096 bits)
int16_t  WaveFull2[NWAVEFULL + 1];   // 2nd wave half wave-table for slow mode used up to 1kHz, and in ExactFreqMode at all freq's (4096 bits) for 2nd wave half
int16_t  WaveTable[NWAVETABLE + 1];  // 1st wave half wave-table for copying into Wave0 to Wave3 for fast mode use (160 points)
int16_t  WaveTable2[NWAVETABLE + 1]; // 2nd wave half wave-table for copying into Wave0 to Wave3 for fast mode use (160 points)
int16_t  Wave0[2][NWAVETABLE];       // wave used for direct DMA access up to 10kHz  - FastMode0 (160 points)
int16_t  Wave1[2][NWAVETABLE /  2];  // wave used for direct DMA access up to 20kHz  - FastMode1 (80 points)
int16_t  Wave2[2][NWAVETABLE /  4];  // wave used for direct DMA access up to 40kHz  - FastMode2 (40 points)
int16_t  Wave3[2][NWAVETABLE / 10];  // wave used for direct DMA access up to 100kHz - FastMode3 (16 points)
int      ClockDivisor1    = 105;     // divides 42MHz (CPU timer 1) by 105 (42 MHz / 105) - clocks DAC at 400kHz
int      TimerCounts      = 26;      // for TC_setup2() divides 42MHz (CPU timer 1) by 26 counts to produce 1.61538 MHz max for DMA handler - varies with TargetWaveFreq & FastMode
int      Delay1           = 10;      // square wave sync delay set at high sample rate
float    Delay2           = 0.55;    // square wave sync delay set at low sample rate (set high 1st)
float    Delay3           = 110;     // square wave sync delay set at low duty
volatile int SyncDelay    = (TimerCounts - Delay1) * Delay2; // delay of square wave for synchronization with peaks of analogue wave when analogue wave phase shift is set to 0.5
volatile bool MinOrMaxWaveDuty;     // if duty-cycle is 0% or 100% only 1 half of wave is displayed
volatile int FastMode     = -1;     // -1 = up to 1kHz (slow mode), 0 = up to 10kHz, 1 = up to 20kHz, 2 = up to 40khz, 3 = up to 100kHz
int      OldFastMode      = -1;     // indicates when FastMode changes.
bool     OldSquareWaveSync;         // indicates previous mode when exiting noise selection
byte     MinMaxDuty       = 1;      // min & max duty-cycle limit for waves (in samples) due to DMA - will be set to 4 when viewing synchronized square wave to prevent polarity reversal due to DMA timing. (set to 1 with unsynchronized square wave as polarity reversal irrelevant)
bool     PotAdjFreq[]     = {1, 1}; // {unsynchronized wave, synchronized waves}: toggles freq adjustment by pot(1) or serial(0)
bool     PotAdjDuty[]     = {1, 1}; // {unsynchronized wave, synchronized waves}: toggles duty-cycle adjustment by pot(1) or serial(0)
float    WaveReading      = 1000;   // Target freq / period (reading from pot)
byte     WaveShape        = 4;      // 0 = Sinewave, 1 = Triangle / Sawtooth, 2 = Arbitrary, 3 = Composite, 4 = TRNG Noise
double   TargetWaveFreq   = 1000;   // synchronized waves Target freq     <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< DETERMINES START-UP FREQUENCY for Analogue Wave
float    TargetWavePeriod = 0;      // only used for saving to flash and sending to GUI (0 if not set)
float    TargetWaveDuty   = 50;     // synchronized waves Target duty-cycle  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< DETERMINES START-UP DUTY-CYCLE for Analogue Wave
double   ActualWaveFreq;            // synchronized waves Actual freq
float    ActualWaveDuty;            // synchronized waves Actual duty-cycle
float    TargetWavePulseWidth;      // synchronized waves Target pulse width in microsecs
float    AnaPulseWidth;             // Actual Analogue Pulse Width in mSeconds
float    LastAllowedWaveDuty = 50;  // At high freq duty cycle is limited - this is the previous allowed duty
volatile uint32_t WaveBit;          // the bit/sample of the wave curently being processed
double   FreqIncrement = 21475000;  // the number of wave-table bits to skip in slow mode for required freq (multiplied, so we can use an int (below) instead of a float, which is faster)
double   FreqIncrmt[3] = {21475000, 21475000}; // as above {1st half of wave, 2nd half of wave} (21475000 = half wave for 1kHz) used for calculating
volatile uint32_t Increment[]  = {21475000, 21475000}; // as above {1st half of wave, 2nd half of wave} (21475000 = half wave for 1kHz)
double   IncrProportion[] = {1.00, 1.00};   // faster way used to calculate Increment[] when freq changed but duty-cycle not changed
int16_t  DitherPoint;                       // used at very low freq's to dither the Increment to produce more accurate intermediate freq's (between freq steps)
unsigned long DitherTime;                   // indicates time to dither Increment
int      ArbUpload = 0;                     // 1 = arbitrary wave uploading
int16_t  ArbitraryPointNumber;              // total number of waypoints entered
int16_t  ArbitraryWave[NARBWAVE + 1];       // variable to store arbitrary wave point data - all points stored (but not the extra data for stepped points)
int16_t  ArbitraryWaveStep[NARBWAVE + 1];   // variable to store arbitrary wave point data - extra data for stepped points stored here
volatile boolean ExactFreqMode = LOW;       // high indicates Exact freq mode is on. Exact at 50% duty-cycle only, causes dithering synchronized square wave
volatile boolean ExactFreqDutyNot50 = LOW;  // high indicates when in Exact freq mode and Not at 50% duty-cycle (and not 0% or 100%) - slightly reduces the workload for the interupt handler when not at 50% duty-cycle
double   ExactFreqModeAccuracy = 0.9999925; // ADJUST ACCURACY of Exact Freq Mode <<<<<<<<<<<<<<<<<<<<<<<< can be tweaked here <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
int      DutyMultiplier[3];                 // used when in ExactFreqMode if not at 50% duty-cycle (& not at 0 or 100%), to TRY to maintain freq
/***********************************************************************************************/
// For Noise: (Analogue) also see WaveShape 4 below
int16_t  TrngNum;           // used with main low pass filter - stores TRNG random number for Noise generation
int16_t  TrngFast;          // used with high freq filter
int16_t  TrngSlo;           // used with low freq filter
uint8_t  TrngCount;         // sets timing for low freq filter
uint16_t NoiseCol   = 3500; // Noise colour with altered linearity: 3500 = Pink noise
uint16_t NoiseLFB   = 0;    // Low freq boost - balance
uint16_t NoiseFil   = 100;  // Main low pass filter - balance
uint16_t NoiseHFB   = 0;    // High freq boost - balance
uint16_t NoiseLFC   = 15;   // Low freq boost - cut-off freq
/********************************************************/
uint32_t WaveAmp     = 65536;  // WaveAmp multiplier used in exact-freq mode for 'live' software volume control
// For Setup parameters:
byte     NumWS       = 4;    // Number of WaveShapes including Noise (5 counting wave 0)
// WaveShape 0 - Sine Wave:
float    SinAmp      = 1.0;  // Amplitude
float    SinVshift   = 0.5;  // Vertical shift
float    SinPhase    = 0.5;  // Phase shift
float    SinFreq2    = 8;    // Sinewave 2 (2nd sinewave) Frequency Multiple. (X times Sinewave 1)
float    SinAddMix   = 0;    // Sinewave 2 percentage Mix in Add Waves mode
float    SinMulMix   = 0;    // Sinewave 2 percentage Mix in Multiply Waves mode
// WaveShape 1 - Triangle Wave:
float    TriAmp      = 1.0;  // Amplitude / slope
float    TriVshift   = 0.5;  // Vertical shift
float    TriPhase    = 0.5;  // Phase shift
byte     TriNumS     = 0;    // Number of Steps per half wave (0 = off)
// WaveShape 2 - Arbitrary Wave:
float    ArbAmp      = 1.0;  // Amplitude
float    ArbVshift   = 0.5;  // Vertical shift
float    ArbHzoom    = 1.0;  // Horizontal zoom
float    ArbHshift   = 0.5;  // Horizontal shift
bool     ArbMirror   = 0;    // half cycle Mirror effect (0 = off)
// WaveShape 3 - Composite wave:
float    ComSinAmp   = 0.5;  // Sine Wave mix
float    ComTriAmp   = 0.5;  // Triangle Wave mix
float    ComArbAmp   = 0.5;  // Arbitrary Wave mix
// WaveShape 4 - TRNG Noise:
uint16_t NoiseAmp    = 0;  // Amplitude
uint16_t NoiseColour = 500; // Noise colour: 500 = Pink noise
/********************************************************/
// For Modulation & Music:
uint16_t Modulation;       // loudness of music notes 0 to 4095. 12 bits to suit DAC ouput
uint16_t ModulationCalc;   // equals Modulation * 16 - Calculating with higher numbers allows better (slower) fading out when level becomes low
//;;; byte     TimeSig = 4;      // time signature (notes per beat) affects Tempo if greater than 6 (if x/8 instead of x/4) // 2 = 2/4, 3 = 3/4, 4 = 4/4, 5 = 5/4, 6 = 6/4, 7 = 6/8, 8 = 8/8, 9 = 9/8
int16_t  LastSample = 0; // previous sample - used for filtering when Legato enabled
uint16_t PeakLevel = 65535; // 65536 = 4096 * 16 - Calculating with higher numbers allows better (slower) log fading out when level becomes low
//  7 instruments v possible  WAVE defaults    PIANO defaults    GUITAR defaults   MARIMBA defaults  TRUMPET defaults   SAX defaults     Violin defaults 
byte     Envelope[7][5] = { {45, 4, 5, 0, 20},{45, 4, 5, 0, 20},{45, 4, 5, 0, 20},{45, 4, 5, 0, 20},{45, 4, 5, 0, 20},{45, 4, 5, 0, 20},{45, 4, 5, 0, 20} }; // {attack rate, decay delay time, decay rate, sustain level, release rate}
byte     EnvSet; //  ^ 5 envelope settings - ie: attack rate, decay delay time, decay rate, sustain level, release rate
uint16_t AttackRate[7]; // calculated from Envelope[x][0] later
uint16_t DecayDelay = 0; // calculated from Envelope[x][1] later
byte     DecayRate = Envelope[0][2];
bool     Play; // 1 = tune playing
uint8_t  ClearPreset = 255;
uint8_t  ClearTune = 255;
uint8_t  LinkedPreset = 0;
uint8_t  LoadedPreset = 0;
uint8_t  LoadedTune = 0;
bool     StartupTune = 0; // 1 = enabled
// Create the structure of the configuration for saving defaults to flash memory (Due has no EEPROM)
#include <DueFlashStorage.h>
DueFlashStorage dueFlashStorage;
struct Configuration // The structure of the configuration for saving settings to flash memory
{
  double   TargetFreq;
  double   TargetWaveFreq;
  double   TargetPeriod;
  double   TargetWavePeriod;
  float    TargetDuty;
  float    TargetWaveDuty;
  float    TargetPulseWidth;
  float    TargetWavePulseWidth;
  float    SweepMinFreq;  // = 20;
  float    SweepMaxFreq;  // = 20000;
  uint16_t SweepRiseTime; // = 20;
  uint16_t SweepFallTime; // = 20;
  uint32_t PeriodD;
  byte     PeriodH;
  byte     PeriodM;
  byte     PeriodS;   // = 10;  // seconds - Target time period for timer
  byte     TimerMode;
  byte     SweepMode; // = 0;
  byte     WaveShape;
  bool     ExactFreqMode;
  bool     SquareWaveSync;
  bool     TimerInvert;
  byte     PotsEnabled;
  bool     PotPulseWidth0; // [0]
  bool     PotPulseWidth1; // [1]
  bool     PotPeriodMode0; // [0]
  bool     PotPeriodMode1; // [1]
  int      Range0;      // [0]
  int      Range1;      // [1]
  int      Range2;      // [2]
  int      Range3;      // [3]
  byte     Control;     // = 2;
  float    SinAmp;      // = 1.0; // Sinewave Amplitude 1.0 = 100%
  float    SinVshift;   // = 0.5; // Sinewave Vertical shift
  float    SinPhase;    // = 0.5; // Sinewave Phase shift
  byte     SinFreq2;    // = 8;   // Sinewave 2 (2nd sinewave) Frequency Multiple. (X times Sinewave 1)
  float    SinAddMix;   // = 0;   // Sinewave 2 percentage Mix in Add Waves mode
  float    SinMulMix;   // = 0;   // Sinewave 2 percentage Mix in Multiply Waves mode
  float    TriAmp;      // = 1.0; // Triangle wave Amplitude 1.0 = 100%
  float    TriVshift;   // = 0.5; // Triangle Vertical shift
  float    TriPhase;    // = 0.5; // Triangle Phase shift
  byte     TriNumS;     // = 0;   // Triangle Number of Steps per half wave (0 = off)
  float    ArbAmp;      // = 1.0; // Arbitrary wave Amplitude 1.0 = 100%
  float    ArbVshift;   // = 0.5; // Arbitrary wave Vertical shift
  float    ArbHzoom;    // = 1.0; // Arbitrary wave horizontal Zoom
  float    ArbHshift;   // = 0.5; // Arbitrary wave Horizontal shift
  bool     ArbMirror;   // = 0;   // Arbitrary wave half cycle Mirror effect (0 = off)
  float    ComSinAmp;   // = 0.5; // Composite Sinewave mix 0.5 = 50%
  float    ComTriAmp;   // = 0.5; // Composite Triangle Wave mix 0.5 = 50%
  float    ComArbAmp;   // = 0.5; // Composite Arbitrary Wave mix 0.5 = 50%
  uint16_t NoiseAmp;    // = 100; // Noise Amplitude percentage 100 = 100%
  uint16_t NoiseColour; // = 500; // Noise colour: 500 = Pink noise
};
Configuration Cfg; // initialize structure
/***********************************************************************************************/

void Setup_DAWG()
{
  analogReadResolution(12);
  analogWriteResolution(12);
  Serial.begin (115200);
  Serial.setTimeout(50);
  Serial.println("\n   ************** Due Arbitrary Waveform Generator **************\n\n");
  pinMode(2, INPUT_PULLUP);  // sweep on / off - pin 21 (sweep / timer run) has a fixed pull-up resistor
  pinMode(5, INPUT_PULLUP);  // timer on / off
  pinMode(8, INPUT_PULLUP);  // clear keypad switch - press before typing numbers with keypad if old data needs clearing. Also stops tune playing, even if switches are disabled. All other switches are disabled while playing
  pinMode(9, INPUT_PULLUP);  // 0 keypad switch
  pinMode(10, INPUT_PULLUP); // 1 keypad switch
  pinMode(11, INPUT_PULLUP); // 2 keypad switch
  pinMode(12, INPUT_PULLUP); // 3 keypad switch
  pinMode(14, INPUT_PULLUP); // 4 keypad switch
  pinMode(15, INPUT_PULLUP); // 5 keypad switch
  pinMode(16, INPUT_PULLUP); // 6 keypad switch
  pinMode(17, INPUT_PULLUP); // 7 keypad switch
  pinMode(18, INPUT_PULLUP); // 8 keypad switch
  pinMode(19, INPUT_PULLUP); // 9 keypad switch
  //pinMode(43, INPUT_PULLUP); // Save current settings as Start-up Default switch
  pinMode(52, INPUT_PULLUP); // Save Preset switch - press after entering number with keypad - Press twice to replace existing Preset data (LED will light up if it exists) or press clear to cancel
  pinMode(53, INPUT_PULLUP); // Load Preset switch - press after entering number with keypad  
  pinMode(62, INPUT_PULLUP); // enter frequency switch - press after entering number with keypad
  pinMode(63, INPUT_PULLUP); // enter period switch - press after entering number with keypad
  pinMode(64, INPUT_PULLUP); // enter duty cycle switch - press after entering number with keypad
  pinMode(65, INPUT_PULLUP); // enter pulse width switch - press after entering number with keypad
  pinMode(22, INPUT_PULLUP); // enable / disable pots, switches & LEDs
  pinMode(24, INPUT_PULLUP); // toggle Pot's control of: Unsync'ed square wave: pot FREQ or PERIOD
  pinMode(26, INPUT_PULLUP); // cycle thru: Unsync'ed square wave: Pot's freq / period RANGE
  pinMode(28, INPUT_PULLUP); // toggle Pot's control of: Sync'ed waves: pot FREQ or PERIOD
  pinMode(30, INPUT_PULLUP); // cycle thru: Sync'ed waves: Pot's freq / period RANGE
  pinMode(32, INPUT_PULLUP); // toggle Pot's control of: Unsync'ed square wave: pot DUTY CYCLE or PULSE WIDTH
  pinMode(34, INPUT_PULLUP); // cycle thru: Unsync'ed square wave: Pot's pulse width RANGE (duty cycle range is fixed at 0 - 100%)
  pinMode(36, INPUT_PULLUP); // toggle Pot's control of: Sync'ed waves: pot DUTY CYCLE or PULSE WIDTH
  pinMode(38, INPUT_PULLUP); // cycle thru: Sync'ed waves: Pot's pulse width RANGE (duty cycle range is fixed at 0 - 100%)
  pinMode(40, INPUT_PULLUP); // cycle thru: WAVE SHAPES
  pinMode(42, INPUT_PULLUP); // toggle: Exact Freq Mode ON / OFF
  pinMode(44, INPUT_PULLUP); // toggle: Square Wave Sync ON / OFF
  pinMode(46, INPUT_PULLUP); // cycle thru: Which wave/s to control (unsynch'ed square wave, analogue wave, or both)
  pinMode(4, OUTPUT);  // indicates: Freq Sweep ON
  pinMode(6, OUTPUT);  // indicates: Timer ON
  pinMode(23, OUTPUT); // indicates: pots enabled
  //pinMode(25, OUTPUT); // indicates: Unsync'ed square wave: Pot controls FREQ or PERIOD
  //pinMode(27, OUTPUT); // indicates: Unsync'ed square wave: Pot controls FREQ or PERIOD
  //pinMode(29, OUTPUT); // indicates: Sync'ed wave: Pot controls FREQ or PERIOD
  //pinMode(31, OUTPUT); // indicates: Sync'ed wave: Pot controls FREQ or PERIOD
  //pinMode(33, OUTPUT); // indicates: Unsync'ed square wave: Pot controls DUTY CYCLE or PULSE WIDTH
  //pinMode(35, OUTPUT); // indicates: Unsync'ed square wave: Pot controls DUTY CYCLE or PULSE WIDTH
  //pinMode(37, OUTPUT); // indicates: Sync'ed wave: Pot controls DUTY CYCLE or PULSE WIDTH
  //pinMode(39, OUTPUT); // indicates: Sync'ed wave: Pot controls DUTY CYCLE or PULSE WIDTH
  //pinMode(41, OUTPUT); // indicates: Exact Freq Mode ON 
  //pinMode(45, OUTPUT); // indicates: Square Wave Sync ON
  //pinMode(47, OUTPUT); // indicates: Analogue wave is being controlled
  //pinMode(49, OUTPUT); // indicates: Unsynch'ed Square wave is being controlled
  pinMode(48, OUTPUT); // indicates: key pressed
  pinMode(50, OUTPUT); // lights up if confirmation needed to replace existing Preset data when saving Preset
  //pinMode(51, OUTPUT); // indicates: switches enabled
  pmc_enable_periph_clk(ID_TRNG);
  trng_enable(TRNG);
  dac_setup();  // set up fast mode for dac
  TimerCounts = freqToTc(TargetWaveFreq); // for TC_setup()
  if (dueFlashStorage.read(226749) >= 1 && dueFlashStorage.read(226749) <= 50)
  {
    StartupTune = 1;
    dac_setup2(); // set up slow mode for dac
    DACC->DACC_CDR = HALFRESOL;
    delay(200); // as "StartupTune" disables interrupts to produce silence, allow time for GUI to send handshake signal
  }
  else dac_setup2(); // set up slow mode for dac
  TC_setup();   // set up fast mode timing
  TC_setup2();  // set up slow mode timing
  TC_setup4();  // set up sync'ed sq wave settings
  TC_setup5();  // timer clock set-up for timer - when operated from the serial monitor
  Settings(0, 0, 0); // set custom start-up default settings (and Preset settings etc later)
  if (TargetFreq < 163 || SquareWaveSync) PIO_Configure(PIOC, PIO_PERIPH_B, PIO_PC28B_TIOA7, PIO_DEFAULT); // enable pin 3
  else pinMode(7, OUTPUT); // Square wave PWM output
  randomSeed(analogRead(3)); // for arbitrary random wave only (not noise) - A0 & A1 used for pots. A2 used for modulation
  
  //noise setup
  if (TimerMode == 2) OldSquareWaveSync = 1;
  else OldSquareWaveSync = SquareWaveSync;
  if (SquareWaveSync) ToggleSquareWaveSync(0); // change to Unsychronized Square Wave if sychronized
  NVIC_DisableIRQ(TC0_IRQn); // disable TC_setup2() SlowMode IRQ before setting TC_setup1()
  TC_setup1();
  dac_setup2();

  Setup2();
}

void Setup2()
{
  NoiseFilterSetup();
  if (WaveShape != 4) // don't set analogue wave if noise selected
  {
    SetWaveFreq(1);
    CalculateWaveDuty(1);
  }
  if (TimerMode == 0) // don't set Unsync'ed Sq.Wave if Timer is on
  {
    SetFreqAndDuty(1, 1);
    Serial.print("   Unsync'ed Sq.Wave Freq: ");
    PrintUnsyncedSqWaveFreq(); Serial.print(", Target: "); Serial.print(TargetFreq, 3);
    Serial.print(" Hz\n   Unsync'ed Sq.Wave Period: ");
    PrintUnsyncedSqWavePeriod();
    Serial.print("   Unsync'ed Sq.Wave Duty-cycle: ");
    Serial.print(ActualDuty);
    Serial.println(" %\n");
  }
  DACC->DACC_CDR = HALFRESOL;
  CreateWaveFull(10);
}

void Settings(byte defaultMode, int preset, boolean sendToGUI) // defaultMode: 0 = just started up. 1 = loading defaults or preset. 2 = loading factory defaults. preset = number to load if > 0
{
  // Flash is erased every time new code is uploaded. Write the "factory" default configuration to flash (as start-up defaults) if just uploded
  if (dueFlashStorage.read(0) > 0 || defaultMode == 2) // if just uploaded (flash bytes will be 255 at first run) read Factory Defaults OR read Factory Defaults to do a restoration if requested
  {
    Cfg.TargetFreq       = 1000;
    Cfg.TargetWaveFreq   = 1000;
    Cfg.TargetPeriod     = 0; // 0 = not set
    Cfg.TargetWavePeriod = 0; // 0 = not set
    Cfg.TargetDuty       = 50;
    Cfg.TargetWaveDuty   = 50;
    Cfg.TargetPulseWidth     = 0; // 0 = not set
    Cfg.TargetWavePulseWidth = 0; // 0 = not set
    Cfg.SweepMinFreq   = 20;
    Cfg.SweepMaxFreq   = 20000;
    Cfg.SweepRiseTime  = 20;
    Cfg.SweepFallTime  = 20;
    Cfg.PeriodD        = 0;
    Cfg.PeriodH        = 0;
    Cfg.PeriodM        = 0;
    Cfg.PeriodS        = 10;  // seconds - Target time period for timer
    Cfg.TimerMode      = 0;
    Cfg.SweepMode      = 0;
    Cfg.WaveShape      = 4;
    Cfg.ExactFreqMode  = 0;
    Cfg.SquareWaveSync = 0;
    Cfg.TimerInvert    = 0;
    Cfg.PotsEnabled    = 0;
    Cfg.PotPulseWidth0 = 0; // [0]
    Cfg.PotPulseWidth1 = 0; // [1]
    Cfg.PotPeriodMode0 = 0; // [0]
    Cfg.PotPeriodMode1 = 0; // [1]
    Cfg.Range0         = 1; // [0]
    Cfg.Range1         = 100; // [1]
    Cfg.Range2         = 1; // [2]
    Cfg.Range3         = 1; // [3]
    Cfg.Control        = 2; // = 2;
    Cfg.SinAmp         = 1.0;  // Amplitude
    Cfg.SinVshift      = 0.5;  // Vertical shift
    Cfg.SinPhase       = 0.5;  // Phase shift
    Cfg.SinFreq2       = 8;    // Sinewave 2 (2nd sinewave) Frequency Multiple. (X times Sinewave 1)
    Cfg.SinAddMix      = 0;    // Sinewave 2 percentage Mix in Add Waves mode
    Cfg.SinMulMix      = 0;    // Sinewave 2 percentage Mix in Multiply Waves mode
    Cfg.TriAmp         = 1.0;  // Amplitude / slope
    Cfg.TriVshift      = 0.5;  // Vertical shift
    Cfg.TriPhase       = 0.5;  // Phase shift
    Cfg.TriNumS        = 0;    // Number of Steps per half wave (0 = off)
    Cfg.ArbAmp         = 1.0;  // Amplitude
    Cfg.ArbVshift      = 0.5;  // Vertical shift
    Cfg.ArbHzoom       = 1.0;  // horizontal Zoom
    Cfg.ArbHshift      = 0.5;  // Horizontal shift
    Cfg.ArbMirror      = 0;    // half cycle Mirror effect (0 = off)
    Cfg.ComSinAmp      = 0.5;  // Sine Wave mix
    Cfg.ComTriAmp      = 0.5;  // Triangle Wave mix
    Cfg.ComArbAmp      = 0.5;  // Arbitrary Wave mix
    Cfg.NoiseAmp       = 0;  // Amplitude
    Cfg.NoiseColour    = 500;  // Noise colour: 500 = Pink noise
    byte bArray[sizeof(Configuration)]; // create byte array to store the structure
    memcpy(bArray, &Cfg, sizeof(Configuration)); // copy the struct to the byte array
    if (dueFlashStorage.read(0) > 0) // if just uploaded
    {
      SaveSliderDefaults();
      dueFlashStorage.write(44, bArray, sizeof(Configuration)); // if justUploaded write byte array (as start-up Defaults) to flash at address 44
      for (byte i = 1; i <= 50; i++) // set line feeds so GUI can count Preset Name Numbers. i = Preset Number
      {
        dueFlashStorage.write((i * 240) + 221, '\n');
      }      
      for (byte i = 0; i < 50; i++) // set line feeds so GUI can count Tune Name Numbers. i = Tune - 1
      {
        dueFlashStorage.write((i * 29) + 226900, '\n');
      }      
    }
  }
  if (dueFlashStorage.read(0) == 0) // if NOT just uploaded
  {
    int interruptMode = dueFlashStorage.read((preset * 240) + 220);
    if (interruptMode == 255) interruptMode = 0;
  //  Serial.print("   interruptMode "); Serial.println(interruptMode);
    byte timerMode;
    byte sweepMode;
    bool squareWaveSync = 0;
    byte waveShape = 0;
    Configuration cfg; // create a temporary structure
    if (defaultMode == 2) // Factory Default restoration: copy factory defaults from Cfg Configuration above to cfg configuration below
    {
      memcpy(&cfg, &Cfg,  sizeof(Configuration)); // copy Factory Defaults to temporary structure
      if (!UsingGUI) Serial.println("   Factory Defaults loading...\n");
    }
    else // checked in Serial read section:  if (preset == 0 || dueFlashStorage.read((preset * 240) + 3) <= 1) // Load Defaults from flash at start-up, or if requested - OR Load Preset if not empty
    {
      byte* bArrayRead = dueFlashStorage.readAddress((preset * 240) + 44); // byte array which is read from flash at address 44 (must be a multiple of 4) 160 bytes used
      memcpy(&cfg, bArrayRead, sizeof(Configuration)); // copy byte array to temporary structure
      if (defaultMode == 1)
      {
        if (preset == 0) Serial.println("   Loading Defaults...\n");
        else           { Serial.print("   Loading Preset "); Serial.print(preset); Serial.println("...\n"); }
      }
    }
    
    // copy from flash cfg. to variables: // checked in Serial read section:  else if (preset > 0 && dueFlashStorage.read((preset * 240) + 3) > 0) return;
    TargetFreq       = cfg.TargetFreq; //     = 1000;
    TargetWaveFreq   = cfg.TargetWaveFreq; // = 1000;
    TargetPeriod     = cfg.TargetPeriod; //   = 0; // 0 = not set
    TargetWavePeriod = cfg.TargetWavePeriod; // = 0; // 0 = not set
    TargetDuty       = cfg.TargetDuty; //     = 50;
    TargetWaveDuty   = cfg.TargetWaveDuty; // = 50;
    TargetPulseWidth     = cfg.TargetPulseWidth; //     = 0; // 0 = not set
    TargetWavePulseWidth = cfg.TargetWavePulseWidth; // = 0; // 0 = not set
    SweepMinFreq     = cfg.SweepMinFreq; //   = 20;
    SweepMaxFreq     = cfg.SweepMaxFreq; //   = 20000;
    SweepRiseTime    = cfg.SweepRiseTime; //  = 20;
    SweepFallTime    = cfg.SweepFallTime; //  = 20;
    PeriodD          = cfg.PeriodD; //        = 0;
    PeriodH          = cfg.PeriodH; //        = 0;
    PeriodM          = cfg.PeriodM; //        = 0;
    PeriodS          = cfg.PeriodS; //        = 10;  // seconds - Target time period for timer
    timerMode        = cfg.TimerMode; //      = 0;
    sweepMode        = cfg.SweepMode; //      = 0;
    waveShape        = cfg.WaveShape; //      = 0;
    ExactFreqMode    = cfg.ExactFreqMode; //  = 0;
    squareWaveSync   = cfg.SquareWaveSync; // = 0; // Sq Wave Sync always starts switched off, then is enabled later if required.
    TimerInvert      = cfg.TimerInvert; //    = 0;
    if (InterruptMode == 0) PotsEnabled = cfg.PotsEnabled; // this setting should not change if playing a tune or if in modulation mode
    PotPulseWidth[0] = cfg.PotPulseWidth0; // = 0; // [0]
    PotPulseWidth[1] = cfg.PotPulseWidth1; // = 0; // [1]
    PotPeriodMode[0] = cfg.PotPeriodMode0; // = 0; // [0]
    PotPeriodMode[1] = cfg.PotPeriodMode1; // = 0; // [1]
    Range[0]         = cfg.Range0; //         = 1; // [0]
    Range[1]         = cfg.Range1; //         = 100; // [1]
    Range[2]         = cfg.Range2; //         = 1; // [2]
    Range[3]         = cfg.Range3; //         = 1; // [3]
    Control          = cfg.Control; //        = 2; // = 2;
    SinAmp           = cfg.SinAmp; //         = 1.0;  // Amplitude
    SinVshift        = cfg.SinVshift; //      = 0.5;  // Vertical shift
    SinPhase         = cfg.SinPhase; //       = 0.5;  // Phase shift
    SinFreq2         = cfg.SinFreq2; //       = 8;    // Sinewave 2 (2nd sinewave) Frequency Multiple. (X times Sinewave 1)
    SinAddMix        = cfg.SinAddMix; //      = 0;    // Sinewave 2 percentage Mix in Add Waves mode
    SinMulMix        = cfg.SinMulMix; //      = 0;    // Sinewave 2 percentage Mix in Multiply Waves mode
    TriAmp           = cfg.TriAmp; //         = 1.0;  // Amplitude / slope
    TriVshift        = cfg.TriVshift; //      = 0.5;  // Vertical shift
    TriPhase         = cfg.TriPhase; //       = 0.5;  // Phase shift
    TriNumS          = cfg.TriNumS; //        = 0;    // Number of Steps per half wave (0 = off)
    ArbAmp           = cfg.ArbAmp; //         = 1.0;  // Amplitude
    ArbVshift        = cfg.ArbVshift; //      = 0.5;  // Vertical shift
    ArbHzoom         = cfg.ArbHzoom; //       = 1.0;  // horizontal Zoom
    ArbHshift        = cfg.ArbHshift; //      = 0.5;  // Horizontal shift
    ArbMirror        = cfg.ArbMirror; //      = 0;    // half cycle Mirror effect (0 = off)
    ComSinAmp        = cfg.ComSinAmp; //      = 0.5;  // Sine Wave mix
    ComTriAmp        = cfg.ComTriAmp; //      = 0.5;  // Triangle Wave mix
    ComArbAmp        = cfg.ComArbAmp; //      = 0.5;  // Arbitrary Wave mix
    NoiseAmp         = cfg.NoiseAmp; //       = 100;  // Amplitude
    NoiseColour      = cfg.NoiseColour; //    = 500;  // Noise colour: 500 = Pink noise
  // THE FOLLOWING CODE RUNS WHEN CHANGING FROM ONE SETTINGS CONFIGURATION TO ANOTHER. SUCH AS LOADING DEFAULTS OR A PRESET:
  // The code ensures the correct order of settings changes, so as to prevent lock-ups or omitted changes.
    if (StartupTune)
    {
      if (dueFlashStorage.read(226748) == 100) // if stay in music mode after playing startup tune
      {
        waveShape = 3; // necessary if minisoundfont
        ExactFreqMode = 1; // necessary if minisoundfont
      }
    }
    if (waveShape == 4 && WaveShape != 4) // if proposed waveshape is noise and present waveshape is not noise 
    {
      if (defaultMode == 0) CreateWaveFull(10); // perform before entering noise, instead of in Setup2()
      else // if (defaultMode > 0) // if not start-up 
      {
        SetWaveFreq(0);       // must be set before leaving noise by wave-change command, so set when entering
        CalculateWaveDuty(0); // must be set before leaving noise by wave-change command, so set when entering
        OldSquareWaveSync = squareWaveSync;
      }
    }
    if (((waveShape == 4 && squareWaveSync == 1 && TimerMode == 1) || (WaveShape == 4 && OldSquareWaveSync == 1 && timerMode == 1) 
      || (waveShape == 4 && squareWaveSync == 0 && TimerMode == 2) || (WaveShape == 4 && OldSquareWaveSync == 0 && timerMode == 2)) && defaultMode > 0) // if not at start-up & sync needs to change, exit noise & timer first
    {
      if (squareWaveSync == 1) OldSquareWaveSync = 1; // OldSquareWaveSync indicates state sync was before entering noise, so should be when leaving noise (WaveShape 4) later
      UserChars[1] = waveShape + 48; // 48 in ASCII = '0' // if leaving noise selection
      ChangeWaveShape(1);
      ExitTimerMode();
      ToggleSquareWaveSync(0);
    }
    if      (TimerMode > 0 && TimerMode != timerMode) ExitTimerMode(); // if exiting or changing Timer mode - makes TimerMode = 0 - could go back into Timer mode below if Sync (when originally entering timer) changed, ie: TimerMode 1 or 2
    else if (SweepMode > 0 && sweepMode == 0) ExitSweepMode(); // if leaving sweep mode - makes SweepMode = 0
    if (waveShape == 4 && WaveShape == 4) OldSquareWaveSync = squareWaveSync; // OldSquareWaveSync indicates state sync should be when leaving WaveShape 4 later
    bool sqWaveSync;
    if (waveShape == 4) sqWaveSync = OldSquareWaveSync; // OldSquareWaveSync is state of SquareWaveSync before entering WaveShape 4
    else
    {
      if (timerMode == 1) sqWaveSync = 0;
      else if (timerMode == 2) sqWaveSync = 1;
    }
    if (SquareWaveSync != sqWaveSync) ToggleSquareWaveSync(0);
    if (TimerMode == 0 && timerMode > 0) // if entering timer mode
    {
      if ((timerMode == 1 && sqWaveSync) || (timerMode == 2 && !sqWaveSync)) ToggleSquareWaveSync(0); // will make EnterTimerMode() on next line set TimerMode correctly to either 1 or 2 (2 means Sync was on when timer entered)
      EnterTimerMode();
    }
    else if (sweepMode > 0) EnterSweepMode();
    if (defaultMode == 0 && squareWaveSync == 1) ToggleSquareWaveSync(0); // Sq Wave Sync always starts switched off, then is enabled here if required.
    if (waveShape != WaveShape) // if changing wave shape
    {
      UserChars[1] = waveShape + 48; // 48 in ASCII = '0'
      ChangeWaveShape(1);
    }
    if (defaultMode > 0 && squareWaveSync != SquareWaveSync) // if not at start-up & sync needs changing
    {
      if (waveShape != 4 && WaveShape == 4) ToggleSquareWaveSync(1); // 1 = exiting noise selection
      else ToggleSquareWaveSync(0);
    }
    if (defaultMode < 2) // if not loading factory defaults
    {
      if (UsingGUI && sendToGUI) SendSettings(preset); // to GUI
      if (dueFlashStorage.read((preset * 240) + 3) == 1 || dueFlashStorage.read((preset * 240) + 3) == 11) // if Arbitrary wave is included in Preset or Default
      {
        int16_t fi = 0; // flash memory index number
        int16_t temp = 0;
        int     startPos = 102400; // memory start position. 1st 11 presets (counting default at preset 0) end at 102396 = 12240 + 90156 (11 * 8196) then 102400 + 124320 (40 * 3108) = 226720
        byte    presetNum = preset - 11; // preset 11 is at start position above, so call it presetNum 0
        int16_t arbWavSp = 3108;  // arbitrary wave spacing (ArbitraryPointNumber is just before each wave)
        if (preset < 11)
        {
          presetNum = preset;
          startPos = 12240;
          arbWavSp = 8196;
        }
        ArbitraryPointNumber = word(dueFlashStorage.read((presetNum * arbWavSp) + startPos - 2), dueFlashStorage.read((presetNum * arbWavSp) + startPos - 1)); // reconstruct value from 2 bytes read from flash
        Serial.print(" ArbitraryPointNumber = "); Serial.println(ArbitraryPointNumber);
        for(int ai = 0; ai <= ArbitraryPointNumber; ai++) // ai is arbitrary wave index number
        {
          temp = word(dueFlashStorage.read((presetNum * arbWavSp) + startPos + fi), dueFlashStorage.read((presetNum * arbWavSp) + startPos + 1 + fi)); // reconstruct value from 2 bytes read from flash
          if (temp >= 5000) // if 1st part of step point
          {
            ArbitraryWaveStep[ai] = temp - 5000; // subtract 5000, which was added when saving 1st part of step point
            fi += 2;
            ArbitraryWave[ai] = word(dueFlashStorage.read((presetNum * arbWavSp) + startPos + fi), dueFlashStorage.read((presetNum * arbWavSp) + startPos + 1 + fi)); // 2nd part of step point - reconstruct value from 2 bytes read from flash
          }
          else // if not a step point
          {
            ArbitraryWaveStep[ai] = -1; // indicates not a step point
            ArbitraryWave[ai] = temp; // normal point
          }
          fi += 2;
 //         Serial.print(" ArbWavStep[ai] = "); Serial.print(ArbitraryWaveStep[ai]); Serial.print(" ArbWave[ai] = "); Serial.println(ArbitraryWave[ai]);
        }
        ArbUpload = 1;
        if (UsingGUI && sendToGUI) SendArbitraryWave(); // to GUI
        if (WaveShape != 4) CreateWaveFull(2);
      }
      else if (UsingGUI && sendToGUI) Serial.print(">"); // inform GUI to skip receiving Arbitrary wave
      
    }
  }
  else dueFlashStorage.write(0, 0); // if (dueFlashStorage.read(0) > 0) // if just uploaded write 0 to address 0 to indicate it's NOT just uploaded, so in future the factory defaults won't be read & saved at start-up. This occurs only once when just uploaded
  if (defaultMode > 0) Setup2(); // otherwise if defaultMode == 0 (at start-up) return to normal setup()
}

void SaveToFlash(int preset) // save preset
{
  if (UsingGUI)
  {
    for (int i = 0; i < 40; i++)
    {
      dueFlashStorage.write((preset * 240) + 4 + i, Serial.read()); // GUI Setup Slider limits
    }
  }
  Cfg.TargetFreq       = TargetFreq;     // = 1000;
  Cfg.TargetWaveFreq   = TargetWaveFreq; // = 1000;
  Cfg.TargetPeriod     = TargetPeriod;     // = 0; // 0 = not set
  Cfg.TargetWavePeriod = TargetWavePeriod; // = 0; // 0 = not set
  Cfg.TargetDuty       = TargetDuty;     // = 50;
  Cfg.TargetWaveDuty   = TargetWaveDuty; // = 50;
  Cfg.TargetPulseWidth     = TargetPulseWidth;     // = 0; // 0 = not set
  Cfg.TargetWavePulseWidth = TargetWavePulseWidth; // = 0; // 0 = not set
  Cfg.SweepMinFreq   = SweepMinFreq; //   = 20;
  Cfg.SweepMaxFreq   = SweepMaxFreq; //   = 20000;
  Cfg.SweepRiseTime  = SweepRiseTime; //  = 20;
  Cfg.SweepFallTime  = SweepFallTime; //  = 20;
  Cfg.PeriodD        = PeriodD; //        = 0;
  Cfg.PeriodH        = PeriodH; //        = 0;
  Cfg.PeriodM        = PeriodM; //        = 0;
  Cfg.PeriodS        = PeriodS; //        = 10;  // seconds - Target time period for timer
  Cfg.TimerMode      = TimerMode; //      = 0;
  Cfg.SweepMode      = SweepMode; //      = 0;
  Cfg.WaveShape      = WaveShape; //      = 0;
  Cfg.ExactFreqMode  = ExactFreqMode; //  = 0;
  if (WaveShape == 4) Cfg.SquareWaveSync = OldSquareWaveSync; // save the state sync was in before entering noise
  else Cfg.SquareWaveSync = SquareWaveSync; // = 0;
  Cfg.TimerInvert    = TimerInvert; //    = 0;
  Cfg.PotsEnabled    = PotsEnabled; //    = 0;
  Cfg.PotPulseWidth0 = PotPulseWidth[0]; // = 0; // [0]
  Cfg.PotPulseWidth1 = PotPulseWidth[1]; // = 0; // [1]
  Cfg.PotPeriodMode0 = PotPeriodMode[0]; // = 0; // [0]
  Cfg.PotPeriodMode1 = PotPeriodMode[1]; // = 0; // [1]
  Cfg.Range0         = Range[0]; //         = 1; // [0]
  Cfg.Range1         = Range[1]; //         = 100; // [1]
  Cfg.Range2         = Range[2]; //         = 1; // [2]
  Cfg.Range3         = Range[3]; //         = 1; // [3]
  Cfg.Control        = Control; //        = 2; // = 2;
  Cfg.SinAmp         = SinAmp; //         = 1.0;  // Amplitude
  Cfg.SinVshift      = SinVshift; //      = 0.5;  // Vertical shift
  Cfg.SinPhase       = SinPhase; //       = 0.5;  // Phase shift
  Cfg.SinFreq2       = SinFreq2; //       = 8;    // Sinewave 2 (2nd sinewave) Frequency Multiple. (X times Sinewave 1)
  Cfg.SinAddMix      = SinAddMix; //      = 0;    // Sinewave 2 percentage Mix in Add Waves mode
  Cfg.SinMulMix      = SinMulMix; //      = 0;    // Sinewave 2 percentage Mix in Multiply Waves mode
  Cfg.TriAmp         = TriAmp; //         = 1.0;  // Amplitude / slope
  Cfg.TriVshift      = TriVshift; //      = 0.5;  // Vertical shift
  Cfg.TriPhase       = TriPhase; //       = 0.5;  // Phase shift
  Cfg.TriNumS        = TriNumS; //        = 0;    // Number of Steps per half wave (0 = off)
  Cfg.ArbAmp         = ArbAmp; //         = 1.0;  // Amplitude
  Cfg.ArbVshift      = ArbVshift; //      = 0.5;  // Vertical shift
  Cfg.ArbHzoom       = ArbHzoom; //       = 1.0;  // horizontal Zoom
  Cfg.ArbHshift      = ArbHshift; //      = 0.5;  // Horizontal shift
  Cfg.ArbMirror      = ArbMirror; //      = 0;    // half cycle Mirror effect (0 = off)
  Cfg.ComSinAmp      = ComSinAmp; //      = 0.5;  // Sine Wave mix
  Cfg.ComTriAmp      = ComTriAmp; //      = 0.5;  // Triangle Wave mix
  Cfg.ComArbAmp      = ComArbAmp; //      = 0.5;  // Arbitrary Wave mix
  Cfg.NoiseAmp       = NoiseAmp; //       = 100;  // Amplitude
  Cfg.NoiseColour    = NoiseColour; //    = 500;  // Noise colour: 500 = Pink noise
  byte bArray[sizeof(Configuration)]; // create byte array to store the structure
  memcpy(bArray, &Cfg, sizeof(Configuration)); // copy the struct to the byte array
// slider limits are at (preset * 240) + 4
  dueFlashStorage.write((preset * 240) + 44, bArray, sizeof(Configuration)); // write byte array to flash at address 44 // GUI Setup Slider limits saved before address 44 // Start-up Default settings saved before address 240
  dueFlashStorage.write((preset * 240) + 220, InterruptMode); // save music mode
//  preset names are at (preset * 240) + 221
  int rep = 0;
  int16_t steps = 0;
  int     startPos = 102400; // memory start position. 1st 11 presets (counting default at preset 0) end at 102396 = 12240 + 90156 (11 * 8196) then 102400 + 124320 (40 * 3108) = 226720
  byte    presetNum = preset - 11; // preset 11 is at start position above, so call it presetNum 0
  int16_t arbWavSp = 3108;  // arbitrary wave spacing (ArbitraryPointNumber is just before each wave)
  int16_t maxWavNum = 3104; // max arbitrary wave size - number of bytes. 1552 16 bit int wave samples
  int16_t chunk = 1552; // tempArb saved in chunks (2 x 1552 = 3104 bytes)
  if (preset < 11)
  {
    presetNum = preset;
    startPos = 12240;
    arbWavSp = 8196;
    maxWavNum = 8192; // number of bytes. 4096 16 bit int wave samples
    chunk = 1024;
  }
  uint16_t arbitraryPointNumber = min(maxWavNum / 2, ArbitraryPointNumber);
  byte tempArb[chunk];
  if (arbitraryPointNumber > 0) // if Arbitrary wave has been uploaded and is present)
  {
    if (UsingGUI) dueFlashStorage.write((preset * 240) + 3, 1); // write 1 to address before start of Preset config to indicate it's been written to and arbitrary wave is included - up to 25 arbitrary waves can be saved to flash
    else       dueFlashStorage.write((preset * 240) + 3, 11); // write 1 + 10 to address before start of Preset config to indicate it's been written to and arbitrary wave is included, but without SetupSliderLimit info
    int16_t fi = 0; // flash memory index number
//    Serial.print(" ArbPointNum = "); Serial.print(ArbitraryPointNumber); Serial.print("  arbPointNum = "); Serial.println(arbitraryPointNumber);
    for (int ai = 0; ai <= arbitraryPointNumber; ai++) // ai is arbitrary wave index number
    {
      if (ArbitraryWaveStep[ai] >= 0 && ai < arbitraryPointNumber) // if stepped point & not past the last point
      {
        tempArb[fi    ] = highByte(ArbitraryWaveStep[ai] + 5000); // the added 5000 indicates 1st part of step point in wave (to be subtracted when read) - save to flash memory as 2 bytes
        tempArb[fi + 1] =  lowByte(ArbitraryWaveStep[ai] + 5000); // the added 5000 indicates 1st part of step point in wave (to be subtracted when read) - save to flash memory as 2 bytes
        fi += 2;
        steps++;
      }
      if (fi >= chunk || ai == arbitraryPointNumber)
      {
  //      Serial.print(" ai = "); Serial.print(ai); Serial.print(" rep + fi = "); Serial.print(rep + fi); Serial.print(" st = "); Serial.print(ArbitraryWaveStep[ai]); Serial.print(" aw = "); Serial.println(ArbitraryWave[ai]);
        dueFlashStorage.write((presetNum * arbWavSp) +  startPos + rep, tempArb, chunk); // min(2048, 4096 - rep - fi)); // write byte array to flash at address (preset * 8200) + 12240
        rep += chunk;
        if (rep >= maxWavNum || ai >= arbitraryPointNumber) break;
        fi = 0;
      }
      tempArb[fi    ] =  highByte(ArbitraryWave[ai]); // usual point, or 2nd part of step point - save to flash memory as 2 bytes
      tempArb[fi + 1] =   lowByte(ArbitraryWave[ai]); // usual point, or 2nd part of step point - save to flash memory as 2 bytes
      fi += 2;
      if (fi >= chunk || ai == arbitraryPointNumber)
      {
 //       Serial.print(" ai2 = "); Serial.print(ai); Serial.print(" rep + fi = "); Serial.print(rep + fi); Serial.print(" st = "); Serial.print(ArbitraryWaveStep[ai]); Serial.print(" aw = "); Serial.println(ArbitraryWave[ai]);
        dueFlashStorage.write((presetNum * arbWavSp) +  startPos + rep, tempArb, chunk); // min(2048, 4096 - rep - fi)); // write byte array to flash at address (preset * 8200) + 12240
        rep += chunk;
        if (rep >= maxWavNum) break;
        fi = 0;
      }
 //     if (ai >= arbitraryPointNumber - 1) {Serial.print(" ai3 = "); Serial.print(ai); Serial.print(" rep + fi = "); Serial.print(rep + fi); Serial.print(" st = "); Serial.print(ArbitraryWaveStep[ai]); Serial.print(" aw = "); Serial.println(ArbitraryWave[ai]);}
    }
    if (ArbitraryPointNumber + steps > maxWavNum / 2) arbitraryPointNumber -= ArbitraryPointNumber + steps - (maxWavNum / 2);
//    Serial.print(" ArbPointNum = "); Serial.print(ArbitraryPointNumber); Serial.print("  arbPointNum = "); Serial.println(arbitraryPointNumber);
    dueFlashStorage.write((presetNum * arbWavSp) + startPos - 2, highByte(arbitraryPointNumber)); // save to flash memory as 2 bytes - saved just before start of Arbitrary wave
    dueFlashStorage.write((presetNum * arbWavSp) + startPos - 1,  lowByte(arbitraryPointNumber)); // save to flash memory as 2 bytes - saved just before start of Arbitrary wave
  }
  else
  {
    if (UsingGUI) dueFlashStorage.write((preset * 240) + 3, 0); // write 0 to address before start of Preset config to indicate it's been written to without arbitrary wave
    else        dueFlashStorage.write((preset * 240) + 3, 10); // write 0 + 10 to address before start of Preset config to indicate it's been written to without arbitrary wave, and without SetupSliderLimit info
  }
}

void CreateWaveFull(byte setupSelection) // WaveFull: for low freq use; prevents 'sample' noise at very low audio freq's (sample-skipping used without DMA)
{
   //       Serial.print("cws InterruptMode = "); Serial.println(InterruptMode);
  bool loweredSampleRate = 0; // 1 = sample rate lowered during wave calculation to speed it up, as DueStorage library uses too much of the little remaining CPU time!
  volatile uint32_t increment[] = {Increment[0], Increment[1]}; // remember setting - used to return sample size to normal after reduced sample rate
  if (FastMode < 0 && WaveShape != 4)
  {
    loweredSampleRate = 1; // 1 = sample rate lowered during wave calculation, as DueStorage library uses too much of the little remaining CPU time!
    if (InterruptMode > 0 || TargetFreq < 163 || (WaveShape == 3 && OldFastMode < 0 && ComArbAmp != 0 && ArbMirror == 0)) // if low freq with interrupt instead of PWM, OR while calculating WaveShape 3 with ComArbAmp != 0 in slow mode with mirror effect OFF (as constraining needed in interrupt handler which uses more CPU time)
    {
      TC_setup2b(); // set up analogue slow mode timing at quarter of normal sample rate (during calculating, to speed it up)
      if (InterruptMode > 0) // if Modulation or Music mode
      {
        Increment[0] = Increment[0] * 2; // increase step size to compensate for reduced sample rate (during calculating, to speed it up)
        Increment[1] = Increment[1] * 2;
        //  Serial.println("here 2 ");
      }
      else // if normal mode
      {
        Increment[0] = Increment[0] * 4; // increase step size to compensate for reduced sample rate (during calculating, to speed it up)
        Increment[1] = Increment[1] * 4;
        //  Serial.println("here 4 ");
      }
    }
    else
    {
      TC_setup2a(); // set up analogue slow mode timing at half of normal sample rate (during calculating, to speed it up)
      Increment[0] = Increment[0] * 2; // increase step size to compensate for reduced sample rate (during calculating, to speed it up)
      Increment[1] = Increment[1] * 2;
       //   Serial.println("else 2 ");
    }
  }
  // Serial.print(" WaveShape = "); Serial.print(WaveShape); Serial.print("  setupSelection = "); Serial.println(setupSelection);
  if (WaveShape == 2 || WaveShape == 3) // Arbitrary wave or Composite wave
  {
    if (ArbUpload == 1) // if JUST UPLOADED arbitrary wave Or loading arbitrary wave from Preset or Defaults
    {
      //    Serial.print("ArbitraryPointNumber = "); Serial.println(ArbitraryPointNumber);
      float pointSpacing = NWAVEFULL / float(ArbitraryPointNumber); // ArbitraryPointNumber = 300 if touch screen used
      int16_t nextPointLocation = 0;
      //   Serial.print("pointSpacing = "); Serial.println(pointSpacing);
      for (int16_t point = 0; point < ArbitraryPointNumber; point++) // changes for each defined point
      {
        int16_t nextPointValue;
        int16_t lastPointValue;
        float level = ArbitraryWave[point];
        if (point < ArbitraryPointNumber - 1) // if not the last point:
        {
          if (ArbitraryWaveStep[point + 1] > -1) nextPointValue = ArbitraryWaveStep[point + 1]; // if next point is a wave-step, read level from 'step' variable
          else nextPointValue = ArbitraryWave[point + 1];                                      // otherwise it's a normal point, so read level from usual variable
        }
        else // if this is the last point:
        {
          if (ArbitraryWaveStep[0] > -1) nextPointValue = ArbitraryWaveStep[0]; // if 1st point is a wave-step, read level from 'step' variable
          else nextPointValue = ArbitraryWave[0];                              // otherwise it's a normal point, so read level from usual variable
        }
        if (ArbitraryWaveStep[point] > -1 && point != 0) lastPointValue = ArbitraryWaveStep[point]; // if this point is a wave-step & not 1st point, read level from 'step' variable
        else lastPointValue = ArbitraryWave[max(0, point - 1)];                                    // otherwise read level from previous point's usual variable
        byte waveStepPeak = 0; // if sharp wave peak detected (if direction changed sharply) at start of wave step value will depend on freq
        if (ArbitraryWaveStep[point] > -1 && UserChars[2] != '!') // if this point is a wave-step & not loading Preset or Defaults
        {
          if (abs(constrain((ArbitraryWaveStep[point] - ArbitraryWave[max(0, point - 1)]) / 100, -2, 2) - constrain((ArbitraryWave[point] - ArbitraryWaveStep[point]) / 100, -2, 2)) > 1) waveStepPeak = min(int(TargetWaveFreq / 100), min(12, int(pointSpacing))); // if sharp wave peak detected (if direction changed sharply) at start of wave step
        }
        byte wavePeak = 0; // if sharp wave peak detected (if direction changed) value will depend on freq:
        if (abs(constrain((ArbitraryWave[point] - lastPointValue) / 100, -2, 2) - constrain((nextPointValue - ArbitraryWave[point]) / 100, -2, 2)) > 1) wavePeak = min(int(TargetWaveFreq / 100), min(12, int(pointSpacing))); // if sharp wave peak detected (if direction changed sharply)
        float stepValue = float(nextPointValue - ArbitraryWave[point]) / (pointSpacing - min(1, ArbitraryWaveStep[point] + 1) - wavePeak); // calculate size of steps needed to join defined points - less steps needed if sharp wave peak detected (if direction changed sharply)
        if (waveStepPeak > 0 && wavePeak > 0)
        {
          waveStepPeak /= 2;
          wavePeak -= waveStepPeak;
        }
        int16_t currentPointLocation = nextPointLocation; // equals the previous "nextPointLocation"    // location of current defined waypoint in full wave
        nextPointLocation = min(round((float(point + 1) / ArbitraryPointNumber) * NWAVEFULL), NWAVEFULL); // location of next defined waypoint in full wave
        for (int i = currentPointLocation; i < nextPointLocation; i++) // changes for each bit, joining defined points
        {
          if (i == currentPointLocation)
          {
            if (ArbitraryWaveStep[point] > -1 && point != 0)
            {
              if (waveStepPeak > 0) // if sharp wave peak detected at start of wave step (& freq is high now), repeat peak value (create a plateau) to make it more visible:
              {
                int16_t cl = i; // currentPointLocation
                for (i = i; i < min(cl + waveStepPeak, NWAVEFULL - 1); i++)
                {
                  WaveArb[i] = ArbitraryWaveStep[point] - HALFRESOL; // HALFRESOL centres wave around zero
                }
              }
              WaveArb[i] = ArbitraryWaveStep[point] - HALFRESOL; // HALFRESOL centres wave around zero
              i++; // move to next wave bit to write 2nd half of step
            }
            if (wavePeak > 0) // if sharp wave peak detected (& freq is high now), repeat peak value (create a plateau) to make it more visible:
            {
              int16_t cl = i; // currentPointLocation
              for (i = i; i < min(cl + wavePeak, nextPointLocation - 1); i++)
              {
                WaveArb[i] = round(level) - HALFRESOL;
              }
            }
          }
          WaveArb[i] = round(level) - HALFRESOL;
          level += stepValue;
    //      if (point >= ArbitraryPointNumber - 1) {Serial.print("level = "); Serial.println(level);}
        }
 //       if (point >= ArbitraryPointNumber - 1)  {Serial.print("point = "); Serial.print(point); Serial.print("  ArbitraryWaveStep[point] = "); Serial.print(ArbitraryWaveStep[point]); Serial.print("  ArbitraryWave[point] = "); Serial.println(ArbitraryWave[point]);}
      }
      ArbUpload = 0; // exit arbitrary upload mode when wave finished uploading
    }
    for (int index = 0; index < NWAVEFULL; index++)
    {
      if (ArbMirror == 0) // if half cycle Mirror effect OFF - 4096 WaveArb points of data will be spread over 4096x2 WaveFull points
      {
        if (index < NWAVEFULL / 2) // 1st half of wave cycle
        {
          for (byte i = 0; i < 2; i++) // process each WaveArb point twice
          {
            if (i == 0) WaveFull[(index * 2)]                      = constrain(((0.5 + (ArbAmp * (0.5 - ArbVshift))) * WAVERESOL) + (ArbAmp * WaveArb[constrain(int(ArbHshift * NWAVEFULL) + int(ArbHzoom * (index - (NWAVEFULL / 2))), 0, NWAVEFULL - 1)]), 0, WAVERESOL-1); // constrain calculations between min & max values & within start & end of cycle
            else WaveFull[(index * 2) + 1] = WaveFull[index * 2]; // copy same data into next WaveFull point
          }
        }
        else // if (index >= NWAVEFULL / 2) // 2nd half of wave cycle - must be UN-reversed
        {
          for (byte i = 0; i < 2; i++) // process each WaveArb point twice
          {
            if (i == 0) WaveFull2[((index - (NWAVEFULL / 2)) * 2)] = constrain(((0.5 + (ArbAmp * (0.5 - ArbVshift))) * WAVERESOL) + (ArbAmp * WaveArb[constrain(int(ArbHshift * NWAVEFULL) + int(ArbHzoom * (index - (NWAVEFULL / 2))), 0, NWAVEFULL - 1)]), 0, WAVERESOL-1); // constrain calculations between min & max values & within start & end of cycle
            else WaveFull2[((index - (NWAVEFULL / 2)) * 2) + 1] = WaveFull2[(index - (NWAVEFULL / 2)) * 2]; // copy same data into next WaveFull point
          }
        }
      }
      else // if half cycle Mirror effect ON
      {
        WaveFull[NWAVEFULL - 1 - index]                            = constrain(((0.5 + (ArbAmp * (0.5 - ArbVshift))) * WAVERESOL) + (ArbAmp * WaveArb[constrain(int(ArbHshift * NWAVEFULL) + int(ArbHzoom * (index - (NWAVEFULL / 2))), 0, NWAVEFULL - 1)]), 0, WAVERESOL-1); // constrain calculations between min & max values & within start & end of cycle
        WaveFull2[index] = WaveFull[NWAVEFULL - 1 - index]; // 2nd wave half always read from here
      }
    }
  }
  if (WaveShape != 2 || setupSelection <= 1 || setupSelection == 10) // if NOT WaveShape 2 or setupSelection is 0, 1 or 10
  {
    int8_t posNeg = -1; // converts positive number to negative and vise versa when value is -1. Value may also be 1 to have no effect - for Triangle wave
    int calc = int(floor(TriPhase + 0.5));// helps to calculate other variables (saves calculating multiple times) - for Triangle wave
    if (calc % 2 == 0) posNeg = 1;   // calculate direction the wave starts moving (at index 0), up or down, with any given phase shift. posNeg equals 1 if TriPhase (phase shift) equals either -0.5, or -1.501 to -2.5, or -3.501 to -4.5, etc. or -0.499 to 0.499, or 1.5 to 2.499, or 3.5 to 4.499, etc. // for Triangle wave
    float phStartLevel = posNeg * (TriPhase - calc); // calculate start level resulting from phase shift (before cosidering bias or amplitude) [Range: -0.5 to 0.5] - for Triangle wave
    int8_t startDir = -posNeg;    // set start direction - won't change during cycle, unlike "posNeg" // - Triangle wave
    int16_t half2StartIndx = int((((startDir * phStartLevel) + 0.5) * NWAVEFULL)); // calculate index at start of 2nd half of cycle // - Triangle wave
    int16_t half2 = 0; // - Triangle wave
    int16_t stepTime = 0;    // step time index - indicates when to take a step (steps will always include extreme top & bottom of cycle at all phase shifts) - Triangle wave
    int vShift0;             // vertical shift of wave 0
    int vShift1;           // vertical shift of wave 1
    int32_t waveTemp;     // needed only in wave 0
    int32_t waveTemp1;     // needed only in wave 1
    uint16_t halfResol = (WAVERESOL - 1) / 2; // - Sine & Triangle waves
    int stepNum = 0; // - Triangle wave
    int32_t stepVolts; // - Triangle wave
    if (WaveShape == 0 || setupSelection == 0 || setupSelection == 10) vShift0 = (int) ((SinAmp * (0.5 - SinVshift)) * WAVERESOL); // ((SinVshift - 0.5) * WAVERESOL); // setupSelection = 10 if restoring settings after USB disconnection and reconnection or loading settings (in GUI)
    if (WaveShape == 1 || WaveShape == 3 || setupSelection == 1 || setupSelection == 10) // calculate wave 1's input to wave 3 // setupSelection = 10 also at start-up to pre-calculate wave 0 & wave 3)
    {
      vShift1 = (0.5 + (TriAmp * (0.5 - TriVshift))) * WAVERESOL; // WS1bias * WAVERESOL; // bias1 = HALFRESOL + (int)((WS1bias - 0.5) * WAVERESOL);
      if (TriNumS > 0) // if Staircase effect enabled
      {
        for (int i = 0; i <= (TriNumS + 1); i++) // check how many spaces between steps will fit in before the next wave peak (either neg or pos peak)
        {
          if ((float(NWAVEFULL) / (TriNumS + 1)) * ((TriNumS + 1) - i) < half2StartIndx) // if time (index points) required for number of steps is less than time available before the next wave peak, either neg or pos
          {
            stepTime = half2StartIndx - ((float(NWAVEFULL) / (TriNumS + 1)) * ((TriNumS + 1) - i)); // calculate the next step time (index)
            break;
          }
          else if (stepNum <= TriNumS) stepVolts = HALFRESOL + (-startDir * HALFRESOL) + startDir * (((float(NWAVEFULL) / TriNumS) * (TriNumS - i) / NWAVEFULL) * WAVERESOL); // calculate the starting step voltage
          //        Serial.print("  stepNum = "); Serial.print(stepNum); Serial.print("  stepVolts = "); Serial.print(stepVolts); Serial.print("  phStartLevel = "); Serial.print(phStartLevel); Serial.print("  startDir = "); Serial.print(startDir); Serial.print("  Step points = "); Serial.print((float(NWAVEFULL)  / (TriNumS + 1)) * ((TriNumS + 1) - i)); Serial.print("  half2StartIndx = "); Serial.println(half2StartIndx);
          stepNum++;
        }
        if (stepNum > TriNumS) stepNum = TriNumS;
        waveTemp1 = vShift1 + TriAmp * (stepVolts - HALFRESOL);
      }
    }
    if (WaveShape == 3 && OldFastMode < 0 && ComArbAmp != 0 && ArbMirror == 0 && InterruptMode == 0) InterruptMode = 10; // constrained in the interrupt handler, but only during calculating of WaveShape 3 in slow mode if mirror effect is OFF as constraining here causes clipping and incorrect mixing of high amplitude waves. This makes the wave look good during calculating, although it slows the calculating process a little.
    for (int index = 0; index < NWAVEFULL; index++) // create the individual samples for the wave - (1st half cycle: 12 bit range, 4096 steps.  2nd half cycle: 12 bit range, 4096 steps)
    {
      if (WaveShape == 0 || setupSelection == 0 || setupSelection == 10) // Sine wave - 1st wave half saved into full wave table - 2nd wave half saved into 2nd full wave table, inverted around it's centre only if single (main) sine wave displayed. Or calculate 2nd wave half (without inverting) for 2 waves
      {
        float sin1AddAmp = (100 - SinAddMix) / 100; // addition mix = UserInput;
        float sin1MulAmp = min(1, (100 - SinMulMix) / 50); // multiplication mix
        float sin2MulAmp = min(1, SinMulMix / 50);
        float sin2AddAmp = 1 - sin1AddAmp;
        float sin1MulBias = 1 - sin1MulAmp;
        float sin2MulBias = 1 - sin2MulAmp;
        if (setupSelection == 0 || setupSelection == 10) // setupSelection = 10 at starup to pre-calculate wave 0 & wave 1
        {
          if      (sin2AddAmp == 0 && sin2MulAmp == 0) waveTemp = (int32_t)  ((SinAmp       *   (sin((PI / NWAVEFULL) * (index + (SinPhase * NWAVEFULL))) * sin1MulAmp) * halfResol) + vShift0); // save temporarily into a 32 bit signed int to allow large minus calculations.
          else if (sin2AddAmp > 0  && sin2MulAmp > 0)  waveTemp = (int32_t) ((((SinAmp / 2) *  ((sin((PI / NWAVEFULL) * (index + (SinPhase * NWAVEFULL))) * sin1AddAmp)                  +  (sin(((SinFreq2 * PI) / NWAVEFULL) * index) * sin2AddAmp) + (((sin((PI / NWAVEFULL) * (index + (SinPhase * NWAVEFULL))) * sin1MulAmp) + sin1MulBias) * ((sin(((SinFreq2 * PI) / NWAVEFULL) * index) * sin2MulAmp) + sin2MulBias)))) * halfResol) + vShift0);
          else if (sin2AddAmp > 0)                     waveTemp = (int32_t) (((SinAmp       *  ((sin((PI / NWAVEFULL) * (index + (SinPhase * NWAVEFULL))) * sin1AddAmp)                  +  (sin(((SinFreq2 * PI) / NWAVEFULL) * index) * sin2AddAmp)))                  * halfResol) + vShift0);
          else if (sin2MulAmp > 0)                     waveTemp = (int32_t) (((SinAmp       * (((sin((PI / NWAVEFULL) * (index + (SinPhase * NWAVEFULL))) * sin1MulAmp) + sin1MulBias)   * ((sin(((SinFreq2 * PI) / NWAVEFULL) * index) * sin2MulAmp) + sin2MulBias))) * halfResol) + vShift0);
          WaveSin[index] = constrain(waveTemp, -HALFRESOL, halfResol); // save 1st wave half into 1st half sine table. WaveSin[] stores data to enable quick copying into WaveFull[] when changing wave shape
        }
        //    Serial.print(index); Serial.print(" W = "); Serial.println(waveTemp);
        if (WaveShape == 0) WaveFull[index] = constrain(WaveSin[index] + HALFRESOL, 0, WAVERESOL-1); // save 1st wave half into full wave table, constrained between min & max values.
        if (setupSelection == 0 || setupSelection == 10) // setupSelection = 10 at starup to pre-calculate wave 0 & wave 1. 2nd wave half saved into 2nd full wave table, inverted around it's centre:
        {
          if      (sin2AddAmp == 0 && sin2MulAmp == 0) WaveSin2[index] = constrain(vShift0 - waveTemp + vShift0, -HALFRESOL, halfResol); // if single (main) 1st wave with no addition or multiplication save 2nd half cycle into 2nd half cycle sine table, inverted around it's centre
          else if (sin2AddAmp > 0  && sin2MulAmp > 0)  waveTemp = (int32_t) ((((SinAmp / 2) * ((sin((PI / NWAVEFULL) * (index + NWAVEFULL + (SinPhase * NWAVEFULL))) * sin1AddAmp)                  +  (sin(((SinFreq2 * PI) / NWAVEFULL) * (index + NWAVEFULL )) * sin2AddAmp) + (((sin((PI / NWAVEFULL) * (index + NWAVEFULL + (SinPhase * NWAVEFULL))) * sin1MulAmp) + sin1MulBias) * ((sin(((SinFreq2 * PI) / NWAVEFULL) * (index + NWAVEFULL )) * sin2MulAmp) + sin2MulBias)))) * halfResol) + vShift0); // if both 2nd sine wave addition and multiplication amplitude are greater than zero, calculate 2nd wave half (without inverting or reversing) for both waves
          else if (sin2AddAmp > 0)                     waveTemp = (int32_t) (((SinAmp       * ((sin((PI / NWAVEFULL) * (index + NWAVEFULL + (SinPhase * NWAVEFULL))) * sin1AddAmp)                  +  (sin(((SinFreq2 * PI) / NWAVEFULL) * (index + NWAVEFULL )) * sin2AddAmp))) * halfResol) + vShift0); // if 2nd sine wave addition amplitude greater than zero, calculate 2nd wave half (without inverting or reversing) for both waves
          else if (sin2MulAmp > 0)                     waveTemp = (int32_t) (((SinAmp      * (((sin((PI / NWAVEFULL) * (index + NWAVEFULL + (SinPhase * NWAVEFULL))) * sin1MulAmp) + sin1MulBias)   * ((sin(((SinFreq2 * PI) / NWAVEFULL) * (index + NWAVEFULL )) * sin2MulAmp) + sin2MulBias))) * halfResol) + vShift0); // if 2nd sine wave multiplication amplitude greater than zero, calculate 2nd wave half (without inverting or reversing) for both waves
          if      (sin2AddAmp > 0 || sin2MulAmp > 0)   WaveSin2[index] = constrain(waveTemp, -HALFRESOL, halfResol); // save 2nd wave half into full wave table, constrained between min & max values
        }
        //    Serial.print(index); Serial.print("\t  W2 = "); Serial.println(WaveSin2[index]);
        if (WaveShape == 0) WaveFull2[index] = constrain(WaveSin2[index] + HALFRESOL, 0, WAVERESOL-1); // Copy into final wavetable
      }
      if (WaveShape == 1 || WaveShape == 3 || setupSelection == 1) // calculate wave 1's input to wave 3 // || setupSelection == 10) // Triangle wave - 1st wave half saved into full wave table - 2nd wave half saved into 2nd full wave table inverted, constrained between min & max values
      {
      //;  if (WaveShape == 1 || WaveShape == 3 || setupSelection == 1) // || setupSelection == 10) // setupSelection = 10 at starup to pre-calculate wave 0 & wave 1)
      //;  {
          if (index == half2StartIndx + min(1, TriNumS)) // [half2StartIndx + 1 if TriNumS > 0] if at start of 2nd half cycle (even if index still equals 0)
          {
            stepNum = 0;
            posNeg = posNeg * -1;
            half2 = -posNeg * int(((startDir * phStartLevel) + 0.5) * (NWAVEFULL * 2) * TriAmp);
          }
          if (TriNumS > 0) // Staircase effect
          {
            if (index == stepTime) // if time to take a step
            {
              stepTime += (float(NWAVEFULL)  / (TriNumS + 1)); // calculate next step time index
              if (stepNum <= TriNumS) stepVolts += posNeg * round(float(NWAVEFULL) / TriNumS); // calculate next step
              stepVolts = constrain(stepVolts , 0, WAVERESOL-1);
              waveTemp1 = vShift1 + TriAmp * (stepVolts - HALFRESOL); // apply bias and amplitude, and hold voltage until next step time
              stepNum++; // count steps
            }
          }
          else waveTemp1 = (int32_t) (vShift1 + ((phStartLevel * TriAmp) * (NWAVEFULL - 1))) + ((posNeg * (TriAmp * index)) + half2); // NWAVEFULL - 1 - index; // save temporarily into a 32 bit signed int to allow large minus calculations.
    //;    }
        if (WaveShape == 1)
        {
          WaveFull[index] = constrain(waveTemp1, 0, WAVERESOL-1); // Centre wave around zero for easy mixing. Save 1st wave half into full wave table, constrained between min & max values
          WaveFull2[index] = constrain(vShift1 - waveTemp1 + vShift1, 0, WAVERESOL-1); // Save 2nd wave half into 2nd full wave table, inverted around it's centre, constrained between min & max values
        }
      }
      if (WaveShape == 3 || setupSelection == 1) // Composite wave - 1st wave half saved into full wave table - 2nd wave half saved into 2nd full wave table, neither reversed or inverted - Calculate Arbitrary wave first, then add other waves to it - wave is centred around zero, for easier mixing
      {       
        if (ComArbAmp != 0) // if arbitrary wave amplitide (set in composite wave setup) is not zero
        {
          if (ArbMirror == 0) // if half cycle Mirror effect is OFF spread the arb wave (4096 points) across the full cycle (8192 points)
          {
            if (index < NWAVEFULL / 2) for (byte i = 0; i < 2; i++)
              {
                WaveCom[(index * 2) + i]  = (((1 - ArbVshift) * WAVERESOL) + (ComArbAmp * ArbAmp * WaveArb[constrain(int(ArbHshift * NWAVEFULL) + int(ArbHzoom * (index - (NWAVEFULL / 2))), 0, NWAVEFULL - 1)])); // "1 - ArbVshift" (not 0.5) coupled with "- HALFRESOL" below also prevent clippimg during calculation
                WaveCom2[(index * 2) + i] = (((1 - ArbVshift) * WAVERESOL) + (ComArbAmp * ArbAmp * WaveArb[constrain(int(ArbHshift * NWAVEFULL) + int(ArbHzoom *  index), 0, NWAVEFULL - 1)])); // "1 - ArbVshift" (not 0.5) coupled with "- HALFRESOL" below also prevent clippimg during calculation
              }
            WaveCom[index]  = WaveCom[index] - HALFRESOL; // "- HALFRESOL" coupled with "1 - ArbVshift" above help prevent clippimg during calculation
            WaveCom2[index] = WaveCom2[index] - HALFRESOL; // "- HALFRESOL" coupled with "1 - ArbVshift" above help prevent clippimg during calculation
          }
          else // if half cycle Mirror effect is ON
          {
            WaveCom[index]  =             (((0.5 - ArbVshift) * WAVERESOL) + (ComArbAmp * ArbAmp * WaveArb[constrain(int(ArbHshift * NWAVEFULL) + int(ArbHzoom * (NWAVEFULL - 1 - index - (NWAVEFULL / 2))), 0, NWAVEFULL - 1)])); // constrain calculations within start & end of cycle // 1st half cycle mirrored
            WaveCom2[index] =             (((0.5 - ArbVshift) * WAVERESOL) + (ComArbAmp * ArbAmp * WaveArb[constrain(int(ArbHshift * NWAVEFULL) + int(ArbHzoom * (index - (NWAVEFULL / 2))), 0, NWAVEFULL - 1)])); // constrain calculations within start & end of cycle // 1st half cycle mirrored // = WaveFull[indx];
          }
        }
        else // if arbitrary wave amplitude (set in composite wave setup) is zero
        {
          WaveCom[index]  = 0; // speeds up waveshape change by avoiding above calculations when not needed
          WaveCom2[index] = 0;
        }
        if (ComSinAmp != 0) // if sine wave amplitide (set in composite wave setup) is not zero
        {
          WaveCom[index]  += ComSinAmp * WaveSin[index]; // add sine wave - 1st wave half
          WaveCom2[index] += ComSinAmp * WaveSin2[index]; // add sine wave - 2nd wave half was fully calculated 2nd half
        }
        if (ComTriAmp != 0) // if triangle wave amplitide (set in composite wave setup) is not zero
        {
          WaveCom[index]  += ComTriAmp * (waveTemp1 - HALFRESOL); // add triangle wave - 1st wave half // Centre wave around zero for easy mixing. Save Triangle 1st wave half into Composite 1st wave table
          WaveCom2[index] += ComTriAmp * (vShift1 - waveTemp1 + vShift1 - HALFRESOL); // add triangle wave - 2nd wave half was originally 1st half inverted with bias added // Save Triangle 2nd wave half into Composite 2nd wave table, inverted around it's centre
        }
        if (WaveShape == 3 && (InterruptMode == 0 || InterruptMode == 10)) // if composite wave, and in normal mode, send composite output to WaveFull
        {
          WaveFull[index] = constrain(WaveCom[index] + HALFRESOL, 0, WAVERESOL - 1); // 1st wave half - return wave from centred around zero (for mixing) to 0 - 4095
          WaveFull2[index] = constrain(WaveCom2[index] + HALFRESOL, 0, WAVERESOL - 1); // 2nd wave half always read from here - return wave from centred around zero (for mixing) to 0 - 4095.
        }
        else if (WaveShape == 3 && InterruptMode == 1) // WaveFull is left centred around zero and manipulated on the fly while reading WaveFull. if InterruptMode > 1 && < 10 output can be read from WaveCom[] array whilst WaveFull is not updated
        {
          WaveFull[index]  = WaveCom[index]; // 1st wave half - return wave from centred around zero (for mixing) to 0 - 4095
          WaveFull2[index] = WaveCom2[index]; // 2nd wave half always read from here - return wave from centred around zero (for mixing) to 0 - 4095.
        }
      }
      // Serial.print(index); Serial.print(" WF = "); Serial.println(waveTemp);
    }
     //     Serial.print("cwb4e InterruptMode = "); Serial.println(InterruptMode);
    if (WaveShape == 3 && ArbMirror == 0 && InterruptMode == 10) InterruptMode = 0;
  }
  if (loweredSampleRate) // return sample rate to usual rate, as wave calculation is finished
  {
    Increment[0] = increment[0]; // return to normal increment size (from increased increment size during calculating, to speed it up)
    Increment[1] = increment[1];
    if (InterruptMode > 0) TC_setup2a(); // return analogue slow mode timing to normal Compose sample rate (from reduced rate during calculating, to speed it up)
    else TC_setup2(); // return analogue slow mode timing to normal sample rate (from reduced rate during calculating, to speed it up)
  }
      //    Serial.print("cwe InterruptMode = "); Serial.println(InterruptMode);
  CreateWaveTable();
  CreateNewWave();
}

void CreateWaveTable() // WaveTable: a smaller wave for higher freq use; faster to copy from smaller table into NewWave (below) [It takes 50% longer to copy from full wave above!]
{
  float reduce = NWAVEFULL / 160.0;
  for (int index = 0; index < NWAVETABLE; index++) // - copy from WaveFull with 4096 points, to this WaveTable with 160 points. 4096 / 160 = 25.6
  {
    WaveTable[index]  = (uint16_t) WaveFull[round(index * reduce)]; // 1st wave half
    WaveTable2[index] = (uint16_t) WaveFull2[round(index * reduce)]; // 2nd wave half
    // Serial.print(index); Serial.print(" WT = "); Serial.println(WaveTable[index]);
  }
}

// NewWave: For analogue wave frequencies above 1kHz. Samples from WaveTable to be copied here with selected duty cycle, for copying by DMA
void CreateNewWave() // create the individual samples for each FastMode wave whenever the duty-cycle is changed (copy from WaveTable)
{
  boolean wh = !WaveHalf; // idendifies which half of wave is CURRENTLY being written by DMA - (LOW = 1st half of wave [positive going half])
  byte fm = max(0, FastMode); // capture the current FastMode
  for (int cycle = 0; cycle < 4; cycle++) // update all 4 FastMode waves, current one first
  {
    byte dv = 0;
    if (TargetWaveDuty == 0 || TargetWaveDuty == 100) dv++; // if displaying one half of wave only divide by 1 less (next lines) to maintain full amplitude as both 0 & 4095 will be included in same half
    float inc0 = float(NWAVETABLE - dv) / (Duty[0][fm] - dv); // calculate increment needed for sampling 1st half wave from wavetable the required number of steps (depending on duty-cycle)
    float inc1 = float(NWAVETABLE) / (Duty[1][fm] - dv); // calculate increment needed for sampling 2nd half wave from wavetable the required number of steps (depending on duty-cycle)
    if (wh == HIGH)
    {
      if (TargetWaveDuty >   0) Create1stHalfNewWave(fm, inc0); // update next half wave cycle first (causes less disturbance to wave especially at high freq's)
      if (TargetWaveDuty < 100) Create2ndHalfNewWave(fm, inc1); // update current half wave cycle afterwards
    }
    else // if (wh ==  LOW)
    {
      if (TargetWaveDuty < 100) Create2ndHalfNewWave(fm, inc1); // update next half wave cycle first (causes less disturbance to wave especially at high freq's)
      if (TargetWaveDuty >   0) Create1stHalfNewWave(fm, inc0); // update current half wave cycle afterwards
    }
    if (fm < 3) fm++;
    else fm = 0;
  }
}
void Create1stHalfNewWave(byte fm, float inc0)
{
  //  Serial.println("Half 1");
  float  x = 0;
  for (int index = 0; index < Duty[0][fm]; index++) // 1st half of cycle
  { // separate files used (Wave0 - 3) instead of arrays to reduce memory useage: [since Wave3 is much smaller than Wave0]
    if ((TargetWaveDuty > 0 && TargetWaveDuty < 100) || index != 0) x = min(NWAVETABLE - 1, x + inc0);
    if      (fm == 0) Wave0[0][index] = WaveTable[round(x)];
    else if (fm == 1) Wave1[0][index] = WaveTable[round(x)];
    else if (fm == 2) Wave2[0][index] = WaveTable[round(x)];
    else if (fm == 3) Wave3[0][index] = WaveTable[round(x)];
    //    if      (fm == 3) {Serial.print(index); Serial.print("ArbUpload = "); Serial.print(round(x)); Serial.print(" val = "); Serial.println(Wave3[0][index]);}
  }
}
void Create2ndHalfNewWave(int fm, float inc1)
{
  //  Serial.println("Half 2");
  float  x = 0;
  for (int index = 0; index < Duty[1][fm]; index++) // 2nd half of cycle
  { // separate files used (Wave0 - 3) instead of arrays to reduce memory useage: [since Wave3 is much smaller than Wave0]
    if ((TargetWaveDuty > 0 && TargetWaveDuty < 100) || index != 0) x = min(NWAVETABLE - 1, x + inc1); // if displaying one half of wave only start from full amplitude
    if      (fm == 0) Wave0[1][index] = WaveTable2[round(x)];
    else if (fm == 1) Wave1[1][index] = WaveTable2[round(x)];
    else if (fm == 2) Wave2[1][index] = WaveTable2[round(x)];
    else if (fm == 3) Wave3[1][index] = WaveTable2[round(x)];
    //    if      (fm == 3) {Serial.print(index); Serial.print("b = "); Serial.print(round(x)); Serial.print(" val = "); Serial.println(Wave3[1][index]);}
  }
}

void Loop_DAWG()
{
  if (millis() > SwitchPressedTime + 500) // check state of pot enable switch:
  {
    bool keyPressed = 0;
    if (!digitalRead(22)) // enable / disable pots & switches (& LEDs)
    {
      if (PotsEnabled < 3) PotsEnabled++;
      else PotsEnabled = 0;
      Serial.print("   PotsEnabled = "); Serial.println(PotsEnabled);
      //if (PotsEnabled == 1 || PotsEnabled == 3) digitalWrite(51, HIGH); // Switches enabled LED
      //else digitalWrite(51, LOW);
      if (PotsEnabled >= 2) digitalWrite(23, HIGH); // Pots enabled LED
      else digitalWrite(23, LOW);
      if (PotsEnabled == 0)
      {
        digitalWrite(4, LOW);
        digitalWrite(6, LOW);
        // digitalWrite(25, LOW);
        // digitalWrite(27, LOW);
        // digitalWrite(29, LOW);
        // digitalWrite(31, LOW);
        // digitalWrite(33, LOW);
        // digitalWrite(35, LOW);
        // digitalWrite(37, LOW);
        // digitalWrite(39, LOW);
        // digitalWrite(41, LOW);
        // digitalWrite(45, LOW);
        // digitalWrite(47, LOW);
        // digitalWrite(49, LOW);        
        digitalWrite(48, LOW);
        digitalWrite(50, LOW);
        keyPressed = 1; // causes LEDs to be turned off
      }
      SwitchPressedTime = millis(); // debouncing
    }
    if (PotsEnabled == 1 || PotsEnabled == 3) // check other switches:
    {
      int8_t keyedInput = -1;
      if (!digitalRead(8)) // clear keypad switch - press before typing numbers with keypad if old data needs clearing. Also stops tune playing, even if switches are disabled. All other switches are disabled while playing
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        UserInput  = 0;
        ClearPreset = 255; // reset
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(9))  keyedInput = 0; // keypad 0
      else if (!digitalRead(10)) keyedInput = 1; // keypad 1
      else if (!digitalRead(11)) keyedInput = 2; // keypad 2
      else if (!digitalRead(12)) keyedInput = 3; // keypad 3
      else if (!digitalRead(14)) keyedInput = 4; // keypad 4 // pin 13 (with onboard LED) not used
      else if (!digitalRead(15)) keyedInput = 5; // keypad 5
      else if (!digitalRead(16)) keyedInput = 6; // keypad 6
      else if (!digitalRead(17)) keyedInput = 7; // keypad 7
      else if (!digitalRead(18)) keyedInput = 8; // keypad 8
      else if (!digitalRead(19)) keyedInput = 9; // keypad 9
      if (keyedInput >= 0) // if any 0 - 9 key pressed
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        UserInput = (UserInput * 10) + keyedInput;    
        Serial.print("   UserInput = "); Serial.println(UserInput, 0);
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(53)) // Load Preset
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        if (UserInput < 1 || UserInput > 50)
        {
          Serial.print("   Preset "); Serial.print(UserInput, 0); Serial.println(" does not exist!\n");
        }
        else if (dueFlashStorage.read((UserInput * 240) + 3) <= 11) // if Preset not empty
        {
          UserChars[2] = '!'; // indicates loading Defaults or Preset when changing WaveShape
          Settings(1, UserInput, UsingGUI); // if Preset not empty, read it from flash in Settings() // send to GUI if Using GUI
          UserChars[2] = ' '; // return to default
          Serial.print("   Preset "); Serial.print(UserInput, 0); Serial.print(" loaded");
          if (!UsingGUI)
          {
            if (UserInput < 30 && (dueFlashStorage.read((UserInput * 240) + 3) == 1 || dueFlashStorage.read((UserInput * 240) + 3) == 11)) Serial.print(" - including Arbitrary wave!"); // if Arbitrary wave included
            else                                                                                                                            Serial.print(" - without Arbitrary wave!"); // if Arbitrary wave not included
          }
          Serial.println("\n");
        }
        else { Serial.print("   Preset "); Serial.print(UserInput, 0); Serial.println(" is empty!\n"); }
        UserInput = 0;
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(52)) // Save Preset
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        if (UserInput < 1 || UserInput > 50)
        {
          Serial.print("   Preset "); Serial.print(UserInput, 0); Serial.println(" does not exist!\n");
        }
        else if (dueFlashStorage.read((UserInput * 240) + 3) > 11 || ClearPreset <= 50) // if Preset is empty OR received cofirmation to replace it
        {
          SaveToFlash(UserInput);
          if (UsingGUI) { Serial.print("Preset "); Serial.print(UserInput, 0); Serial.println(" saved"); }
          else
          {
            Serial.print("   Current Settings have been saved as Preset "); Serial.print(UserInput, 0);
            if (dueFlashStorage.read((UserInput * 240) + 3) == 11 && UserInput < 30)  Serial.println(" - including Arbitrary wave!\n"); // if Arbitrary wave included
            else                                                                    Serial.println(" - without Arbitrary wave!\n"); // if Arbitrary wave not included
          }
          UserInput = 0;
          ClearPreset = 255; // reset
          digitalWrite(50, LOW);
        }
        else if (dueFlashStorage.read((UserInput * 240) + 3) <= 11) // if Preset not empty
        {
          if (!UsingGUI) { Serial.print("   Preset "); Serial.print(UserInput, 0); Serial.println(" is not empty!\n   Do you want to replace it?  Type Y or N  (the N must be upper case)\n"); }
          else Serial.println("Preset In Use");
          ClearPreset = UserInput;
          digitalWrite(50, HIGH);
        }
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(62)) // set FREQ - or Sweep MIN FREQ - or Timer DAYS
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        if (SweepMode > 0)
        {
          SweepMinFreq = int(constrain(UserInput, 0.001, 990000));
          if (!UsingGUI) { Serial.print("   You typed:\n\tHighest freq = "); Serial.print(SweepMaxFreq); Serial.print(" Hz  \tRise time = "); Serial.print(SweepRiseTime); Serial.print(" Sec\n  \tLowest freq  = "); Serial.print(SweepMinFreq); Serial.print(" Hz  \tFall time = "); Serial.print(SweepFallTime); Serial.println(" Sec\n"); }
        }
        else if (TimerMode > 0)
        {
          PeriodD = int(max(0, UserInput));
          if (!UsingGUI) { Serial.print("   You typed: "); Serial.print(PeriodD); Serial.print(" days, "); Serial.print(PeriodH); Serial.print(" hours, "); Serial.print(PeriodM); Serial.print(" mins, "); Serial.print(PeriodS); Serial.println(" secs.\n"); }
        }
        else SetFreqPeriod(); // FREQ or PERIOD ADJUSTMENT: in Hertz or Milliseconds
        UserInput = 0;
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(63)) // set PERIOD - or Sweep MAX FREQ - or Timer HOURS
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        if (SweepMode > 0)
        {
          SweepMaxFreq = int(constrain(UserInput, 0.0011, 100000));
          if (!UsingGUI) { Serial.print("   You typed:\n\tHighest freq = "); Serial.print(SweepMaxFreq); Serial.print(" Hz  \tRise time = "); Serial.print(SweepRiseTime); Serial.print(" Sec\n  \tLowest freq  = "); Serial.print(SweepMinFreq); Serial.print(" Hz  \tFall time = "); Serial.print(SweepFallTime); Serial.println(" Sec\n"); }
        }
        else if (TimerMode > 0)
        {
          PeriodH = int(constrain(UserInput, 0, 23));
          if (!UsingGUI) { Serial.print("   You typed: "); Serial.print(PeriodD); Serial.print(" days, "); Serial.print(PeriodH); Serial.print(" hours, "); Serial.print(PeriodM); Serial.print(" mins, "); Serial.print(PeriodS); Serial.println(" secs.\n"); }
        }
        else
        {
          UserChars[0] = 'm';
          SetFreqPeriod(); // FREQ or PERIOD ADJUSTMENT: in Hertz or Milliseconds
          UserChars[0] = '>';
        }
        UserInput = 0;
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(64)) // set DUTY CYCLE - or Sweep RISE TIME - or Timer MINS
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        if (SweepMode > 0)
        {
          SweepRiseTime = int(max(0, UserInput));
          if (!UsingGUI) { Serial.print("   You typed:\n\tHighest freq = "); Serial.print(SweepMaxFreq); Serial.print(" Hz  \tRise time = "); Serial.print(SweepRiseTime); Serial.print(" Sec\n  \tLowest freq  = "); Serial.print(SweepMinFreq); Serial.print(" Hz  \tFall time = "); Serial.print(SweepFallTime); Serial.println(" Sec\n"); }
        }
        else if (TimerMode > 0)
        {
          PeriodM = int(constrain(UserInput, 0, 59));
          if (!UsingGUI) { Serial.print("   You typed: "); Serial.print(PeriodD); Serial.print(" days, "); Serial.print(PeriodH); Serial.print(" hours, "); Serial.print(PeriodM); Serial.print(" mins, "); Serial.print(PeriodS); Serial.println(" secs.\n"); }
        }
        else SetDutyPulse(); // DUTY CYCLE or PULSE WIDTH ADJUSTMENT: in % or microseconds
        UserInput = 0;
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(65)) // set PULSE WIDTH - or Sweep FALL TIME - or Timer SECS
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        if (SweepMode > 0)
        {
          SweepFallTime = int(max(0, UserInput));
          if (!UsingGUI) { Serial.print("   You typed:\n\tHighest freq = "); Serial.print(SweepMaxFreq); Serial.print(" Hz  \tRise time = "); Serial.print(SweepRiseTime); Serial.print(" Sec\n  \tLowest freq  = "); Serial.print(SweepMinFreq); Serial.print(" Hz  \tFall time = "); Serial.print(SweepFallTime); Serial.println(" Sec\n"); }
        }
        else if (TimerMode > 0)
        {
          PeriodS = int(constrain(UserInput, 0, 59));
          if (!UsingGUI) { Serial.print("   You typed: "); Serial.print(PeriodD); Serial.print(" days, "); Serial.print(PeriodH); Serial.print(" hours, "); Serial.print(PeriodM); Serial.print(" mins, "); Serial.print(PeriodS); Serial.println(" secs.\n"); }
        }
        else
        {
          UserChars[0] = 'u';
          SetDutyPulse(); // DUTY CYCLE or PULSE WIDTH ADJUSTMENT: in % or microseconds
          UserChars[0] = '>';
        }
        UserInput = 0;
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(40)) // Change Wave Shape
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        ChangeWaveShape(0);
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(42) && WaveShape != 4) // toggle ExactFreqMode if Noise not selected
      {
        ToggleExactFreqMode();
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(44)) // toggle Square Wave Sync
      {
        ToggleSquareWaveSync(0);
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(46)) // Change which wave/s to control
      {
        if (Control < 2) Control++;
        else Control = 0;
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(2)) // Freq Sweep on / off
      {
        if (TimerMode == 0) // if Timer off
        {
          if (SweepMode == 0) EnterSweepMode();
          else ExitSweepMode();
        }
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(5)) // Timer on / off
      {
        if (SweepMode == 0) // if Sweep off
        {
          if (TimerMode == 0) EnterTimerMode();
          else ExitTimerMode();
        }
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(21)) // run Sweep or Timer
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        if (TimerMode > 0) timerRun(); // if Timer on, start or stop timer
        else if (SweepMode > 0) // if Freq Sweep on
        {
          if (SweepMinFreq > 0 && SweepMinFreq < SweepMaxFreq && SweepRiseTime + SweepFallTime > 0)
          {
            while (!digitalRead(8)) delay(10); // wait for key to be released
            digitalWrite(48, HIGH); // keyPressed LED on
            delay(400); // wait for 'debouncing' of the clear key
            digitalWrite(48, LOW); // keyPressed LED off
            SweepMode = 2;
            SweepFreq(); // start Running freq sweep
          }
          else Serial.println("   The settings are incorrect. The sweep cannot run! ");
        }
        SwitchPressedTime = millis();
      }      
    }
    if (PotsEnabled >= 2) // check Pot switches:
    {
      if (!digitalRead(24)) // toggle Pot's control of unsync'ed wave freq / period
      {
        PotPeriodMode[0] = !PotPeriodMode[0];
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(26)) // change unsync'ed wave freq / period Pot's range
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        if (Range[0] < 10000) Range[0] *= 10;
        else Range[0] = 1;
        Serial.print("   Unsync'ed Sq.Wave Pot Freq Range: x "); Serial.println(Range[0]);
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(28)) // toggle Pot's control of sync'ed waves freq / period
      {
        PotPeriodMode[1] = !PotPeriodMode[1];
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(30)) // change unsync'ed wave freq / period Pot's range
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        if (Range[0] < 10000) Range[0] *= 10;
        else Range[0] = 1;
        Serial.print("   Unsync'ed Sq.Wave Pot Freq Range: x "); Serial.println(Range[0]);
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(32)) // toggle Pot's control of unsync'ed waves Duty Cycle / Pulse Width
      {
        PotPulseWidth[0] = !PotPulseWidth[0];
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(34)) // change unsync'ed wave Pulse Width Pot's Range
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        if (Range[2] < 10000) Range[2] *= 10;
        else Range[2] = 1;
        Serial.print("   Unsync'ed Sq.Wave Pot Pulse Width Range: x "); Serial.println(Range[2]);
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(36)) // toggle Pot's control of unsync'ed waves Duty Cycle / Pulse Width
      {
        PotPulseWidth[1] = !PotPulseWidth[1];
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(38)) // change sync'ed wave Pulse Width Pot's Range
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        if (Range[3] < 10000) Range[3] *= 10;
        else Range[3] = 1;
        Serial.print("   Sync'ed Sq.Wave Pot Pulse Width Range: x "); Serial.println(Range[3]);
        SwitchPressedTime = millis();
      }
    }
    if (PotsEnabled > 0 || keyPressed == 1) // if either pots or switches are enabled OR PotsEnabled just switched off
    {
      if (millis() > LEDupdateTime + 300 && PotsEnabled > 0) // if time to set LEDs AND PotsEnabled not just switched off
      {
        if (PotsEnabled >= 2) // pot related switches:
        {
          // digitalWrite(25, PotPeriodMode[0]);
          // digitalWrite(27, !PotPeriodMode[0]);
          // digitalWrite(29, PotPeriodMode[1]);
          // digitalWrite(31, !PotPeriodMode[1]);
          // digitalWrite(33, !PotPulseWidth[0]);
          // digitalWrite(35, PotPulseWidth[0]);
          // digitalWrite(37, !PotPulseWidth[1]);
          // digitalWrite(39, PotPulseWidth[1]);
        }
        if (PotsEnabled != 2) // non - pot related switches:
        {
          digitalWrite(4, min(1, SweepMode));
          digitalWrite(6, min(1, TimerMode));
          //digitalWrite(41, ExactFreqMode);
          //digitalWrite(45, SquareWaveSync);
          //if (Control > 0) digitalWrite(47, HIGH);
          //else digitalWrite(47, LOW);
          //if (Control != 1) digitalWrite(49, HIGH);
          //else digitalWrite(49, LOW);   
        }
        LEDupdateTime = millis();
      }
      else if (PotsEnabled == 0) // if PotsEnabled just switched off
      {
        digitalWrite(4, LOW);
        digitalWrite(6, LOW);
        // digitalWrite(25, LOW);
        // digitalWrite(27, LOW);
        // digitalWrite(29, LOW);
        // digitalWrite(31, LOW);
        // digitalWrite(33, LOW);
        // digitalWrite(35, LOW);
        // digitalWrite(37, LOW);
        // digitalWrite(39, LOW);
        // digitalWrite(41, LOW);
        // digitalWrite(45, LOW);
        // digitalWrite(47, LOW);
        // digitalWrite(49, LOW);
      }
      if (keyPressed == 1 && (PotsEnabled == 1 || PotsEnabled == 3)) digitalWrite(48, HIGH); // if key pressed AND switches enabled
      else digitalWrite(48, LOW);
    }
  }
  if (PotsEnabled >= 2)
  {
    Pot0 = analogRead(A0);
    Pot1 = analogRead(A1);
    /***********************************************************************************************/
    // UNSYNC'ED SQ.WAVE FREQ / PERIOD ADJUSTMENT:
    float newReading =  Pot0 * Range[0];
    if (PotAdjFreq[0] == 1 || Control == 1)
    {
      OldReading[0] += (newReading - OldReading[0]) * constrain(0.02 * abs(newReading - OldReading[0]), 0.03, 1); // Responsive smoothing
      if (PotPeriodMode[0] == 1) FreqReading = min(84000000 / OldReading[0], 42000000); // if adjusting period convert to freq - max freq = 42MHz
      else FreqReading = min(OldReading[0]  , 42000000); // if adjusting freq - max freq = 42MHz
    }
    if ((abs(FreqReading - TargetFreq) > TargetFreq / 40 && PotAdjFreq[0] == 1) || (abs(newReading - OldReading[0]) > OldReading[0] / 20 && PotAdjFreq[0] == 0))
    {
      if (Control != 1)
      {
        PotAdjFreq[0] = 1;
        if (PotPulseWidth[0] && TargetPulseWidth > 0) TargetDuty = min(100, 100 * (TargetPulseWidth / (1000000 / min(FreqReading, 42000000)))); // convert pulse width input into %
        else TargetPulseWidth = 0;

        TargetFreq = FreqReading;
        if (TargetFreq >= 100) Period = 84000000 / TargetFreq;
        else Period = 200000 / TargetFreq;
        SetFreqAndDuty(1, 1);
        Serial.print("   Unsync'ed Sq.Wave Freq: ");
        PrintUnsyncedSqWaveFreq();
        Serial.print(", Unsync'ed Sq.Wave Duty Cycle: "); Serial.print(ActualDuty);
        Serial.print(" %\n   Unsync'ed Sq.Wave Period: ");
        PrintUnsyncedSqWavePeriod();
        Serial.println("");
      }
    }
    // PULSE DUTY CYCLE ADJUSTMENT:
    float newRead;
    if (PotPulseWidth[0]) newRead = (Pot1 / 40) * Range[2];
    else newRead = Pot1 / 40;
    if (PotAdjDuty[0] == 1 || Control == 1) DutyReading[0] += (newRead - DutyReading[0]) * constrain(0.04 * abs(newRead - DutyReading[0]), 0.05, 1); // Responsive smoothing
    if ((abs(DutyReading[0] - TargetDuty) > 0.5 && PotAdjDuty[0] == 1) || (abs(newRead - DutyReading[0]) > 1 && PotAdjDuty[0] == 0))
    {
      if (Control != 1)
      {
        PotAdjDuty[0] = 1;
        if (PotPulseWidth[0])
        {
          TargetPulseWidth = round(DutyReading[0]);
          TargetDuty = 100 * (round(DutyReading[0]) / (1000000 / ActualFreq)); // convert pulse width input into %
        }
        else TargetDuty = round(DutyReading[0]);
        SetFreqAndDuty(0, 1);
        Serial.print("   Unsync'ed Sq.Wave Duty-cycle: "); Serial.print(ActualDuty); Serial.print(" %,  Actual: "); Serial.print(TargetDuty); Serial.println(" %\n");
      }
    }
    /***********************************************************************************************/
    // SYNC'ED WAVES FREQ ADJUSTMENT:
    if (WaveShape != 4)
    {
      float newWaveReading = max(Pot0 * Range[1] * 0.01, 0.01);
      if (PotAdjFreq[1] == 1 || Control == 0)
      {
        OldReading[1] += (newWaveReading - OldReading[1]) * constrain(0.02 * abs(newWaveReading - OldReading[1]), 0.03, 1); // Responsive smoothing
        if (PotPeriodMode[1] == 1) WaveReading = min(100000 / OldReading[1], 100000); // if adjusting period convert to freq
        else WaveReading = min(OldReading[1], 100000); // if adjust freq
      }
      if ((abs(TargetWaveFreq - WaveReading) > TargetWaveFreq / 40 && PotAdjFreq[1] == 1) || (abs(newWaveReading - OldReading[1]) > OldReading[1] / 20 && PotAdjFreq[1] == 0))
      {
        if (Control > 0)
        {
          PotAdjFreq[1] = 1;
          if (PotPulseWidth[1] && TargetWavePulseWidth > 0) TargetWaveDuty = min(100, 100 * (TargetWavePulseWidth / (1000000 / min(WaveReading, 100961)))); // convert pulse width input into %
          else TargetWavePulseWidth = 0;
          TargetWaveFreq = min(WaveReading, 100961);
  
          FreqIncrement = WaveReading * 21475;
          //      TargetWaveFreq = WaveReading;
          SetWaveFreq(0);
          Serial.print("   Analogue Wave Freq: ");
          PrintSyncedWaveFreq();
          Serial.print(", Analogue Wave Duty Cycle: "); Serial.print(ActualWaveDuty);
          Serial.print(" %\n   Analogue Wave Period: ");
          PrintSyncedWavePeriod();
          Serial.println("");
        }
      }
      // SYNC'ED WAVES DUTY CYCLE ADJUSTMENT:
      if (PotPulseWidth[1]) newRead = (Pot1 / 40) * Range[3];
      else newRead = Pot1 / 40;
      if (PotAdjDuty[1] == 1 || Control == 0) DutyReading[1] += (newRead - DutyReading[1]) * constrain(0.04 * abs(newRead - DutyReading[1]), 0.05, 1); // Responsive smoothing
      if ((abs(DutyReading[1] - TargetWaveDuty) > 0.5 && PotAdjDuty[1] == 1) || (abs(newRead - DutyReading[1]) > 1 && PotAdjDuty[1] == 0))
      {
        if (Control > 0)
        {
          PotAdjDuty[1] = 1;
          if (PotPulseWidth[1])
          {
            TargetWavePulseWidth = round(DutyReading[1]);
            TargetWaveDuty = 100 * (round(DutyReading[1]) / (1000000 / ActualWaveFreq)); // convert pulse width input into %
          }
          else TargetWaveDuty = round(DutyReading[1]);
          CalculateWaveDuty(0);
          Serial.print("   Analogue Wave Duty-cycle: "); Serial.print(ActualWaveDuty); Serial.print(" %,  Target: "); Serial.print(TargetWaveDuty); Serial.println(" %\n");
          CreateNewWave();
        }
      }
    }
  }
  if (InterruptMode == 1) Modulation = analogRead(A2);
  /***********************************************************************************************/
  if (Serial.available())
  {
    //   Serial.print("   start: "); Serial.print(UserInput, 5); Serial.print("  "); Serial.print(UserChars[0]); Serial.print(UserChars[1]); Serial.print(UserChars[2]); Serial.println(char(Serial.peek()));
    int8_t minus = 1;
    int8_t numDecimalPlaces = -2; // -2 = no number has been received this iteration. -1 = number has been received. 0 or more = decimal point been received and this is the number of decimal places counted
    uint8_t maxNum = 1;
    while (Serial.peek() >= '-' && Serial.peek() <= '9' && Serial.peek() != '/') // read Double related chars ('0' to '9' '-' and '.') into UserInput until other char appears, then stop. "parseDouble" not availible for Arduino!!! "parseFloat" has only 32 bit precision and "toDouble" only gives 2 decimal places. Double precision needed at ultra low freq when pulse width (expressed in microseconds) is set above 214 seconds.
    {
      delayMicroseconds(100);
      if (Serial.peek() == '-') // if next char to be received is '-'
      {
        if (numDecimalPlaces == -2) // if no number has been received yet
        {
          minus = -1; // to be applied to following number (if a number is received next)
          Serial.read(); // clear next to be received '-' char, so the following chars can be received
        }
        else break; // if a number has been received followed next by a '-' (this happens when receiving a stepped point in an arbitrary wave) don't clear next to be received '-' char, it must be processed later
      }
      else if (Serial.peek() >= '0' && Serial.peek() <= '9') // if the next char to be received is a number
      {
        if (numDecimalPlaces == -2) numDecimalPlaces = -1; // if this is the first number to be received, remember that a number has been received
        UserInput = (UserInput * 10) + (Serial.read() - '0'); // the char '0' is ASCII 48, so '0'(48) minus '0'(48) = int 0 (and '9'(57) minus '0'(48) = int 9 etc)
        if (numDecimalPlaces >= 0) numDecimalPlaces++; // if one of the previous chars was '.' count the number of decimal places received
        //      if (Serial.peek() >= '-'  minus = 1;
      }
      else if (Serial.read() == '.') numDecimalPlaces = 0; // if the next char is a '.' read it and prepare to start counting decimal places received
    }
    UserInput = minus * UserInput * pow(10, min(0, -numDecimalPlaces)); // multiply input by 10 to the power of minus the number of decimal places
    if (Serial.peek() == ',' || Serial.peek() == '-' || Serial.peek() == ';') // If currently receiving arbitrary wave. Serial.peek() will be ',' or '-' or ';'
    {
      if (ArbUpload == 0) // only occurs when starting to add more points to an existing arbitrary wave
      {
        ArbUpload = 1;
        if (!UsingGUI) Serial.print("   "); // indent start of numbers in serial monitor if starting to add more points to an existing arbitrary wave
      }
      while(Serial.available()) // receiving arbitrary wave
      {
        if (Serial.peek() == '-') // stepped point separator
        {
          UserChars[0] = Serial.read();
          ArbitraryWaveStep[ArbitraryPointNumber] = UserInput; // save Wave Step data
          if (!UsingGUI)
          {
            if (ArbitraryPointNumber == 0) Serial.print("   You entered:\n   ");
            Serial.print(ArbitraryWaveStep[ArbitraryPointNumber]); Serial.print("-");
          }
        }
        if ((ArbitraryPointNumber > 0 && Serial.peek() >= '0' && Serial.peek() <= '9') || UserChars[0] == '-') // 1st point value read above - remaining points read here, including 2nd half of 1st point if it was stepped
        {
          UserInput = 0;
          UserInput = Serial.parseInt();
          unsigned long del = millis();
          while (Serial.available() == 0 && millis() - 2 < del) delayMicroseconds(10); // if next serial char not arrived yet, wait up to 2 mSecs for serial to become available. if still no serial, continue in case it was the last char sent
        }
        if (Serial.peek() == ',') // point separator / counter
        {
          UserChars[0] = Serial.read();
          if (ArbitraryPointNumber < NARBWAVE - 1)
          {
            ArbitraryWave[ArbitraryPointNumber] = UserInput; // save Wave point data
            if (!UsingGUI && ArbitraryPointNumber == 0 && ArbitraryWaveStep[ArbitraryPointNumber] == -1) Serial.print("   You entered:\n   ");
            if (!UsingGUI)
            {
              Serial.print(ArbitraryWave[ArbitraryPointNumber]);
              Serial.print(",");
            }
            if (ArbitraryPointNumber < NARBWAVE - 1) ArbitraryPointNumber++;
            if (!UsingGUI && ArbitraryPointNumber > 0 && ArbitraryPointNumber % 10 == 0) Serial.print("\n   "); // start a new line after every 10 points entered
          }
        }
        else if (Serial.peek() == ';') // last point detector / signal to create wave
        {
          UserChars[0] = Serial.read();
          ArbitraryWave[ArbitraryPointNumber] = UserInput;
          if (!UsingGUI)
          {
            Serial.print(ArbitraryWave[ArbitraryPointNumber]);
            Serial.println(";");
          }
          if (ArbitraryPointNumber < NARBWAVE) ArbitraryPointNumber++;
          Serial.print("   ..... a total of "); Serial.print(ArbitraryPointNumber);
          if (ArbitraryPointNumber < NARBWAVE - 1) Serial.println(" points.\n");
          else Serial.println(" points. THIS IS THE MAXIMUM LIMIT\n");
          if (!UsingGUI)
          {
            if (millis() < 180000) Serial.println("   A half cycle mirrored effect can be created by typing 1s2m uu");
            Serial.println("   Type 'a' again to enter a new arbitrary wave and clear the current wave from memory");
            if (ArbitraryPointNumber < 3700) Serial.println("   Or you can just add more points by not typing 'a' first\n   ");
            else Serial.println("\n   ");
          }
          if (WaveShape != 2 && InterruptMode < 2) // if not set to arbitrary wave and if not in music mode
          {
            UserChars[1] = '2'; // the wave shape to change to
            ChangeWaveShape(1);
          }
          CreateWaveFull(WaveShape);
          ArbUpload = 0; // exit arbitrary mode
          //       Serial.println("                    ***** Exited Arbitrary Creation Mode *****\n\n");
          break;
        }
        if (!UsingGUI && Serial.peek() == '\n') Serial.read(); // clear and allow loop to be exited - if received unfinished typing from serial monitor
  //      Serial.print("   total of "); Serial.print(ArbitraryPointNumber);
        unsigned long del = millis();
        while (Serial.available() == 0 && millis() - 2 < del) delayMicroseconds(10); // if next serial char not arrived yet, wait up to 2 mSecs for serial to become available. if still no serial, continue in case it was the last char sent
      }
      UserInput = 0; // reset
      UserChars[0] = '>'; // Change 1st character back to one not used.
      TouchedTime = 0; // allows repeatedly adding of more points quickly without Status being displayed
    }
    else // if not currently receiving Arbitrary wave
    {
      if (ArbUpload == 1) // if a 
      {
        ArbUpload = 0;
        if (!UsingGUI) Serial.println("                    ***** Exited Arbitrary Creation Mode *****\n\n");
      }
      if (Serial.peek() == 's') maxNum = 3; // look ahead to see what the next char will be - if it's 's' maxNum is 3 for Setup parameters
      else if (Serial.peek() == 'n' || Serial.peek() == 'r') maxNum = 2; // look ahead to see what the next character will be - if it's 'n' or 'r' maxNum is 2 for Noise or Arbitrary Random Wave. Otherwise it's 1.
      Serial.readBytesUntil('\n', UserChars, maxNum); // read all characters (including numbers) after initial (float) numbers up to maxNum of characters (only 1 character if next character is not 's' or 'n' or 'r')
    }
    if (UserChars[0] == 'r' && UserChars[1] != ' ') // If creating random wave. UserChars[0] will be 'r' & UserChars[1] will be s, n or m (default char if nothing received is ' ')
    {
      ArbUpload = 1;
      if (UserChars[0] == 'r') // create random wave
      {
        byte randomMode = 2;
        if      (UserChars[1] == 'n') randomMode = 0; // no steps
        else if (UserChars[1] == 's') randomMode = 1; // stepped
        else if (UserChars[1] == 'm') randomMode = 2; // mixed
        ArbitraryPointNumber = 300;
        for (int i = 0; i <= ArbitraryPointNumber; i++) // reset points 0 - 300
        {
          ArbitraryWave[i] = -1; // make points = -1 to indicate they're not set
          ArbitraryWaveStep[i] = -1;
        }
        ArbitraryWave[0] = int(random(0, 4096)); // random start
        int temp2 = ArbitraryWave[0];
        int i = int(random(0, ArbitraryPointNumber)); // average start point
        i = int(random(0, i)); // earlier average start point
        //        i = int(random(0, i)); // even earlier average start point
        while (i < ArbitraryPointNumber)
        {
          int mixed = 0;
          if (randomMode == 2) mixed = int(random(0, 2)); // if mixed - generate random  0 or 1
          if (mixed == 0 && randomMode > 0) ArbitraryWaveStep[i] = temp2; // if stepped
          temp2 = int(random(0, 4096));
          ArbitraryWave[i] = temp2;
          int divisor = int(random(1, 15));
          i = min(ArbitraryPointNumber, int(random(i + 1, i + (ArbitraryPointNumber / divisor))));
          if (i >= ArbitraryPointNumber)
          {
            if (mixed == 0 && randomMode > 0) ArbitraryWaveStep[ArbitraryPointNumber] = temp2; // if stepped
            ArbitraryWave[ArbitraryPointNumber] = ArbitraryWave[0];
          }
        }
        int lastFilledPointValue = ArbitraryWave[0];
        int unfilledPoints = 0;
        float stepValue = 0;
        for (int point = 1; point <= ArbitraryPointNumber; point++) // start from 2nd point, 1 (1st point is pre-filled)
        {
          if (ArbitraryWave[point] == -1) unfilledPoints++; // if current point not filled, count unfilled points
          else                                             // if current point filled, check whether previous point is filled
          {
            if (ArbitraryWave[point - 1] == -1)          // if previous point not filled, fill all previous unfilled points
            {
              if (ArbitraryWaveStep[point] > -1) stepValue = float(ArbitraryWaveStep[point] - lastFilledPointValue) / (unfilledPoints + 1); // if point is a wave-step, read level from 'step' variable
              else stepValue = float(ArbitraryWave[point] - lastFilledPointValue) / (unfilledPoints + 1);                                  // otherwise it's a normal point so read level from usual variable
              float level = lastFilledPointValue;
              for (int i = point - unfilledPoints; i < point; i++)
              {
                level += stepValue;
                ArbitraryWave[i] = round(level);
              }
              unfilledPoints = 0;
            }
            lastFilledPointValue = ArbitraryWave[point];
          }
        }
        Serial.println("                              Arbitrary Random Wave Created!");
        Serial.println("                 To replace it with another one, re-send the last command");
        Serial.println("                       To quit the Random Wave Creation Mode type q\n");
        UserInput = ArbitraryWave[ArbitraryPointNumber];
        if (WaveShape == 4) // if exiting noise selection
        {
          NVIC_DisableIRQ(TC2_IRQn); // disable noise IRQ
          if (FastMode >= 0)
          {
            TC_setup();
            dac_setup();
          }
          else TC_setup2();
          if (OldSquareWaveSync) ToggleSquareWaveSync(1); // restore Sychronized Square Wave if necessary
        }
        WaveShape = 2;
        CreateWaveFull(WaveShape);
        UserChars[0] = '>';
        ArbUpload = 0; // exit arbitrary mode
      }
    }
    else if (ArbUpload == 0) // If NOT currently receiving arbitrary wave or creating random wave
    {
      delay(1); // short delay to ensure UserChars[0] is ready to be read correctly next
      if ((ClearTune <= 100 || ClearPreset <= 100) && UserChars[0] != 'y' && UserChars[0] != 'Y' && UserChars[0] != 'N') ClearTune = ClearPreset = 255; // if question below (in case 'C' and 'S') not answered correctly, cancel pending action
      switch (UserChars[0])
      {
        case '~':
          delay(1); // short delay to ensure next char is ready to be read correctly by Serial.peek()
          if (Serial.peek() == 'p') Serial.read(); // if received "~p"
          Serial.println("Ping received"); // if testing USB connection
          break;
        case 'X': // deletes uploaded Arbitrary wave
        case 'a': // Arbitrary wave creation - deletes old wave data, ready for new wave data
          ArbUpload = 1;
          for (int16_t i = 0; i <= NWAVEFULL; i++) 
          {
            if (UserChars[0] == 'X') ArbitraryWave[i] = 2047;
            ArbitraryWaveStep[i] = -1;  // resets any stepped points to 'unstepped' state
          }
          if (UserChars[0] == 'X')
          {
            if (WaveShape != 2 && InterruptMode < 2) // if not set to arbitrary wave and if not in music mode
            {
              UserChars[1] = '2'; // the wave shape to change to
              ChangeWaveShape(1);
            }
            ArbitraryPointNumber = 300; // can't be zero:
            CreateWaveFull(2);
            ArbUpload = 0;
            ArbitraryPointNumber = 0;
            break;
          }
          ArbitraryPointNumber = 0;
          if (UsingGUI) Serial.println("NEW ARBITRARY WAVE CREATION");
          else
          {
            Serial.println("\n         ************************* NEW ARBITRARY WAVE CREATION *************************\n");
            Serial.println("   Please type the value of each point you wish to define - must be 0 to 4095. (Any old data has been deleted.)");
            Serial.println("   Separate each value with a comma. Use no spaces. Finish with a semi-colon.\n");
            Serial.println("   For example:\n   2047,2150,3800,4095,3800,400,200,400,2510,2700,2510,1800,1700,1800,2040,2150,2050,1980,1960,2000;\n");
            Serial.println("   You can create steps in the wave by dividing points into two values.\n   For example, an 'M' wave:  0,0-4095,1600,4095-0;\n");
            Serial.println("   The semi-colon at the end triggers wave creation.");
            Serial.println("   Points can also be added later (without typing 'a' first), up to 4096 points total.");
            Serial.println("   (Although 4096 points can be uploaded, stepped points count as two)\n");
          }
          break;
        case 'h': // if (UserChars[0] == 'h' || UserChars[0] == 'm') // FREQ or PERIOD ADJUSTMENT:  in Hertz or Milliseconds
        case 'm':
          SetFreqPeriod(); // FREQ or PERIOD ADJUSTMENT: in Hertz or Milliseconds
          break;
        case 'd': // if (UserChars[0] == 'd' || UserChars[0] == 'u') // DUTY CYCLE or PULSE WIDTH ADJUSTMENT: in % or microseconds OR Update command
        case 'u':
          delay(1); // short delay to ensure next char is ready to be read correctly by Serial.peek()
          if (UserChars[0] == 'u' && Serial.peek() == 'u') // if received "uu" - Update command
          {
            UserChars[1] = Serial.read(); // read UserChar[1] if 2nd 'u' - Update command
            if (Serial.peek() >= '0' && Serial.peek() <= '9')
            {
              UserChars[2] = Serial.read(); // read UserChar[2]
              if (UserChars[2] == '1' && Serial.peek() == '0')
              {
                UserChars[3] = Serial.read(); // read UserChar[3]
                if       (UsingGUI) { Serial.print("   You sent: "); Serial.print(UserInput); Serial.print(UserChars[0]); Serial.print(UserChars[1]); Serial.print(UserChars[2]); Serial.println(UserChars[3]); }
                else if (!UsingGUI) { Serial.print("   Waves \""); Serial.print(UserChars[2]); Serial.print(UserChars[3]); Serial.print("\" re-calculating...   "); }
                CreateWaveFull(10); // if restoring setting after USB disconnection and reconnection OR after Loading Settings
              }
              else
              {
                if (UsingGUI) { Serial.print("   You sent: "); Serial.print(UserInput); Serial.print(UserChars[0]); Serial.print(UserChars[1]); Serial.println(UserChars[2]); }
                if (!UsingGUI) { Serial.print("   Wave "); Serial.print(UserChars[2]); Serial.print(" re-calculating...   "); }
                CreateWaveFull(UserChars[2] - '0'); // if UserChars[2] == '0' (which is ASCII 48) then '0' minus '0' (ASCII 48) = int 0
              }
              if (UsingGUI) Serial.println("loaded");
              else Serial.println(" done!\n");
            }
            else
            {
              Serial.read(); // remove '\n'
              if (!UsingGUI) { Serial.print("   Wave "); Serial.print(UserChars[2]); Serial.print(" re-calculating...   "); }
              CreateWaveFull(WaveShape);
              if (!UsingGUI) Serial.println(" done!\n");
            }
            Serial.read(); // remove 2nd 'u' so Pulse Width command won't be detected
            if (!UsingGUI) Serial.println("   Noise doesn't need updating.\n");
          }
          else if ((UserChars[0] == 'd' && UserInput >= 0 && UserInput <= 100) || UserChars[0] == 'u') SetDutyPulse(); // DUTY CYCLE or PULSE WIDTH ADJUSTMENT: in % or microseconds
          break;
        case 's':
          if (UsingGUI) {
            Serial.print("   You sent: ");
            Serial.print(UserInput);
            Serial.print(UserChars[0]);
            Serial.print(UserChars[1]);
            Serial.println(UserChars[2]);
          }
          switch (UserChars[1])
          {
            case '0': // if received 0 - Wave Shape 0 Commands:
              switch (UserChars[2])
              {
                case 'a': // if received s0a
                  SinAmp = UserInput / 1000000.0;
                  if (!UsingGUI) {
                    Serial.print("   Sine Wave Amplitude is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                  case 'w': // if received s0w
                  WaveAmp = UserInput;
                  if (!UsingGUI) {
                    Serial.print("   Wave Amplification is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                case 'v': // if received s0v
                  SinVshift = UserInput / 100.0;
                  if (!UsingGUI) {
                    Serial.print("   Sine Wave Vertical Shift is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                case 'p': // if received s0p
                  SinPhase = UserInput;
                  if (!UsingGUI) {
                    Serial.print("   Sine Wave Phase Shift is ");
                    Serial.println(SinPhase);
                    Serial.println("");
                  }
                  break;
                case 'f': // if received s0f
                  SinFreq2 = abs(constrain(round(UserInput), 1, 100));
                  if (!UsingGUI) {
                    Serial.print("   2nd Sine Wave Frequency Multiple is ");
                    Serial.println(SinFreq2, 0);
                    Serial.println("");
                  }
                  break;
                case '+': // if received s0+
                  SinAddMix = abs(constrain(UserInput, 0, 100));
                  if (!UsingGUI) {
                    Serial.print("   Sine Wave Addition Mix is ");
                    Serial.println(SinAddMix);
                    Serial.println("");
                  }
                  break;
                case '*': // if received s0*
                  SinMulMix = constrain(UserInput, 0, 100);
                  if (!UsingGUI) {
                    Serial.print("   Sine Wave Multiplication Mix is ");
                    Serial.println(SinMulMix);
                    Serial.println("");
                  }
                  break;
                default: // if received s0 with no more valid char's after it - Wave Shape 0 Help:
                  if (!UsingGUI)
                  {
                    Serial.println("\n   Sine Wave Commands: (Wave Shape 0)");
                    Serial.println(  "   Type a number followed by:");
                    Serial.println(  "   s0a - for Amplitude  (eg: 1 000 000 means 100%, is default = 1000000s0a)");
                    Serial.println(  "   s0w - for WaveAmp               (eg: 65536 means 100%, which is default)");
                    Serial.println(  "   s0v - for Vertical shift                              (default =  50s0v)");
                    Serial.println(  "   s0p - for Phase shift relative to sync'ed square wave (default = 0.5s0p)");
                    Serial.println(  "   s0f - for 2nd sine wave Frequency mulptile              (default = 8s0f)");
                    Serial.println(  "   s0+ - to Add waves      - mix: 0 to 100     (50 = both) (default = 0s0+)");
                    Serial.println(  "   s0* - to Multiply waves - mix: 0 to 100     (50 = both) (default = 0s0*)");
                    Serial.println(  "   Hint: 50s0* = ring modulation. 76s0* = amplitude mod. 100s0* = 2nd wave");
                    Serial.println(  "   Current values: ");
                    Serial.print(    "   Amplitude = "); Serial.print(SinAmp * 1000000, 0); Serial.print(    "   WaveAmp = "); Serial.print(WaveAmp); Serial.print(", Bias = "); Serial.print(SinVshift * 100, 0); Serial.print(", Phase = "); Serial.println(SinPhase);
                    Serial.print(    "   Freq multiple = "); Serial.print(SinFreq2, 0); Serial.print(", Add waves Mix = "); Serial.print(SinAddMix, 0); Serial.print(", Multiply waves Mix = "); Serial.println(SinMulMix, 0); Serial.println("\n");
                  }
                  //  break;
              }
              break;
            case '1': // if received 1 - Wave Shape 1 Commands:
              switch (UserChars[2])
              {
                case 'a': // if received s1a
                  TriAmp = UserInput / 100.0;
                  if (!UsingGUI) {
                    Serial.print("   Triangle Amplitude / Slope is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                case 'v': // if received s1v
                  TriVshift = UserInput / 100.0;
                  if (!UsingGUI) {
                    Serial.print("   Triangle Vertical Shift is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                case 'p': // if received s1p
                  TriPhase = UserInput;
                  if (!UsingGUI) {
                    Serial.print("   Triangle Phase Shift is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                case 's': // if received s1s - Staircase effect
                  TriNumS = ceil(abs(constrain(UserInput, 0, 100)));
                  if (!UsingGUI) {
                    Serial.print("   Number of Steps is ");
                    Serial.println(TriNumS);
                    Serial.println("");
                  }
                  break;
                default: // if received s1 with no more valid char's after it - Wave Shape 1 Help:
                  if (!UsingGUI)
                  {
                    Serial.println("\n   Triangle Wave Commands: (Wave Shape 1)");
                    Serial.println(  "   Type a number followed by:");
                    Serial.println(  "   s1a - for Amplitude / slope    (eg: 100 means 100%, which is default = 100s1a)");
                    Serial.println(  "   s1v - for Vertical shift                                    (default =  50s1v)");
                    Serial.println(  "   s1p - for Phase shift - relative to sync'ed square wave     (default = 0.5s1p)");
                    Serial.println(  "   s1s - for Staircase effect - number of steps                (default =   0s1s)");
                    Serial.println(  "   Hint: -100s1a = inverted wave. 200s1a & 75s1b = trapezoid wave. 1s1s = square wave");
                    Serial.println(  "   Current values: ");
                    Serial.print(    "   Amplitude = "); Serial.print(TriAmp * 100, 0); Serial.print(", Bias = "); Serial.print(TriVshift * 100, 0); Serial.print(", Phase = "); Serial.println(TriPhase);
                    Serial.print(    "   Number of Steps = "); Serial.println(TriNumS); Serial.println("\n");
                  }
                  //  break;
              }
              break;
            case '2': // if received 2 - Wave Shape 2 Commands:
              switch (UserChars[2])
              {
                case 'a': // if received s2a
                  ArbAmp = UserInput / 100.0;
                  if (!UsingGUI) {
                    Serial.print("   Arbitrary Amplitude is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                case 'v': // if received s2v
                  ArbVshift = UserInput / 100.0;
                  if (!UsingGUI) {
                    Serial.print("   Arbitrary Vertical Shift is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                case 'z': // if received s2z
                  if (UserInput != 0) ArbHzoom = 100.0 / UserInput;
                  else ArbHzoom = 100;
                  if (!UsingGUI) {
                    Serial.print("   Arbitrary Horizontal Zoom is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                case 'h': // if received s2h
                  ArbHshift = UserInput / 100.0;
                  if (!UsingGUI) {
                    Serial.print("   Arbitrary Horizontal Shift is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                case 'm': // if received s2m
                  ArbMirror = UserInput;
                  if (!UsingGUI) {
                    Serial.print("   Arbitrary Half Cycle Mirror Effect is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                default: // if received s2 with no more valid char's after it - Wave Shape 2 Help:
                  if (!UsingGUI)
                  {
                    Serial.println("\n   Arbitrary Wave Commands: (Wave Shape 2)");
                    Serial.println(  "   Type a number followed by:");
                    Serial.println(  "   s2a - for Amplitude            (eg: 100 means 100%, which is default = 100s2a)");
                    Serial.println(  "   s2v - for Vertical shift                                    (default =  50s2v)");
                    Serial.println(  "   s2z - for horizontal Zoom - a minus value reverses the wave (default =  50s2z)");
                    Serial.println(  "   s2h - for Horizontal shift                                  (default =  50s2h)");
                    Serial.println(  "   s2m - for half cycle Mirrored effect     (0 = off, 1 = on)  (default =   0s2m)");
                    Serial.println(  "   Current values: ");
                    Serial.print(    "   Amplitude = "); Serial.print(ArbAmp * 100, 0); Serial.print(", Bias = "); Serial.print(ArbVshift * 100.0, 0); Serial.print(", H. Zoom = "); Serial.print(100.0 / ArbHzoom, 0); Serial.print(", Pan = "); Serial.println(ArbHshift * 100, 0); Serial.println("\n");
                  }
                  // break;
              }
              break;
            case '3': // if received 3 - Wave Shape 3 Commands:
              switch (UserChars[2])
              {
                case 's': // if received s3s
                  ComSinAmp = UserInput / 100.0;
                  if (!UsingGUI) {
                    Serial.print("   Sine Amplitude is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                case 't': // if received s3t
                  ComTriAmp = UserInput / 100.0;
                  if (!UsingGUI) {
                    Serial.print("   Triangle Amplitude is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                case 'a': // if received s3a
                  ComArbAmp = UserInput / 100.0;
                  if (!UsingGUI) {
                    Serial.print("   Arbitrary Amplitude is ");
                    Serial.println(UserInput);
                    Serial.println("");
                  }
                  break;
                default: // if received s3 with no more valid char's after it - Wave Shape 3 Help:
                  if (!UsingGUI)
                  {
                    Serial.println("\n   Composite Wave Commands: (Wave Shape 3)\n");
                    Serial.println(  "   Type a number followed by:");
                    Serial.println(  "   s3s - Sine wave amplitude (eg: 50 means 50%, which is default = 50s3s)");
                    Serial.println(  "   s3t - Triangle wave amplitude                        (default = 50s3t)");
                    Serial.println(  "   s3a - Arbitrary wave amplitude                       (default = 50s3a)");
                    Serial.println(  "   Current values: ");
                    Serial.print(    "   Sine amplitude = "); Serial.print(ComSinAmp * 100, 0); Serial.print(", Triangle amp = "); Serial.print(ComTriAmp * 100.0, 0); Serial.print(", Arbitrary amp = "); Serial.println(ComArbAmp * 100, 0); Serial.println("\n");
                  }
                  // break;
              }
              break;
            default: // if received s followed by any invalid, unused characters or \n
              if (!UsingGUI)
              {
                Serial.println("\n   Setup Commands Menu:");
                Serial.println(  "   Type the following, then press enter:");
                Serial.println(  "   Type:   s0  to list commands for sine wave      Shape 0");
                Serial.println(  "   Type:   s1  to list commands for triangle wave  Shape 1");
                Serial.println(  "   Type:   s2  to list commands for arbitrary wave Shape 2");
                Serial.println(  "   Type:   s3  to list commands for composite wave Shape 3");
                Serial.println(  "   Type:   n   to list commands for TRNG Noise generation ");
                Serial.println(  "   Type:   uu  to Update setting changes of displayed wave");
                Serial.println(  "   Note:   uu  can be followed by a number, to recalculate\n               a different wave from the one displayed.\n               Useful when displaying composite wave\n\n");
              }
              break;
          } // end of switch case chars[1] statements
          break; // end of case 's'
        case 'n': // if received n - Noise
          if (UsingGUI && (UserChars[0] == 'n'))
          {
            Serial.print("   You sent: ");
            Serial.print(UserInput);
            Serial.print(UserChars[0]);
            Serial.println(UserChars[1]);
          }
          if (UserChars[1] == 'a')       // if received na
          {
            NoiseAmp = constrain(UserInput, 0, 2000);
            Serial.print("   Noise Amplitude is "); Serial.println(UserInput); Serial.println("");
          }
          else if (UserChars[1] == 'c')
          {
            NoiseColour = UserInput;
            NoiseFilterSetup();
          }
          else if (WaveShape == 4 && (UserChars[1] == 'w' || UserChars[1] == 'p' || UserChars[1] == 'b'))
          {
            if      (UserChars[1] == 'w') NoiseColour = 1000; // white -  if received nw
            else if (UserChars[1] == 'p') NoiseColour = 500;  // pink  -  if received np
            else if (UserChars[1] == 'b') NoiseColour = 30;   // brown -  if received nb
            NoiseFilterSetup();
          }
          else if (!UsingGUI) // if received n with no more valid char's after it - Noise Help (WaveShape 4):
          {
            Serial.println("\n   True Random Noise Generator Commands:       (\"Wave Shape\" 4)");
            Serial.println(  "   Type a number followed by:");
            Serial.println(  "   na - noise Amplitude - range: 0 to 2000        (default = 100)");
            Serial.println(  "   nc - noise Colour    - range: 0 to 1000 (default = 500 - pink)\n");
            Serial.println(  "   Preset Noise Colours:      (only when noise is displayed)");
            Serial.println(  "   nw - sets noise colour to White (1000)");
            Serial.println(  "   np - sets noise colour to Pink  (500)");
            Serial.println(  "   nb - sets noise colour to Brown (30)");
            Serial.println(  "   Current Settings: ");
            Serial.print(    "   Amplitude is "); Serial.print(int(NoiseAmp)); Serial.print(  " & Colour is "); Serial.println(int(NoiseColour)); Serial.println("\n");
          }
          break;
        case 'G': // followed by 's' if GUI is at start-up
          UsingGUI = 1;
          Serial.println("Hello GUI");
          if (Serial.peek() == 's') // if 'G' is followed by 's' then GUI is starting up (not Reconnecting)
          {
            Serial.read(); // clear 's'
            uint8_t sIMM = dueFlashStorage.read(226748); // 100 = Stay In Music Mode after playing startup tune, and keep tune's wave settings
            if (sIMM == 255) sIMM = 0;                  // 0 = load startup default settings after playing startup tune, may require exiting Music Mode
            Serial.print("@"); // indicates sending start up tune data next
            uint8_t startupTuneNum = dueFlashStorage.read(226749);
            Serial.write(startupTuneNum + sIMM); // Start up Tune number, plus 100 if Staying In Music Mode after playing - send to GUI
            uint8_t linkedPreset = dueFlashStorage.read((startupTuneNum % 100) - 1 + 226750); // if linked Preset number
            if (dueFlashStorage.read((linkedPreset * 240) + 3) > 11) linkedPreset = 0; // if linked Preset is empty don't use it
            if (StartupTune + sIMM <= 100 || linkedPreset == 0) // load default startup settings if no startup tune or if unlinked startup tune or if linked preset is empty
            {
              SendSettings(-1); // send default settings to GUI
              if (dueFlashStorage.read(3) == 1) SendArbitraryWave(); // if Arbitrary wave is included in Default settings
              else Serial.print(">"); // tells GUI to skip receiving Arbitrary wave
            }
          }
          else StartupTune = 0; // if GUI is not at start-up
          break;
        case 'w': // Change Wave Shape
          ChangeWaveShape(1);
          break;
        case 'e': // toggle ExactFreqMode
          if (WaveShape != 4) ToggleExactFreqMode(); // toggle ExactFreqMode if Noise not selected
          else Serial.print("   Cannot set Exact Freq Mode while Noise is enabled");
          break;
        case 'U':
          if (TimerMode > 0) UserChars[3] = '>'; // continue searching below
          else // unlink tune
          {
            int tune = Serial.parseInt();
            if (tune < 1 || tune > 50)
            {
              Serial.print("   Tune "); Serial.print(tune); Serial.println(" does not exist!\n");
              break;
            }
            dueFlashStorage.write(tune - 1 + 226750, 0); // 0 = indicates tune not empty & not linked to preset // 255 = empty
            Serial.print("   Tune "); Serial.print(tune); Serial.println(" has been unlinked\n");
          }
          break;
        case 'P':
          delay(1); // short delay to ensure next char is ready to be read correctly by Serial.peek()
          
          if (PotsEnabled < 3) PotsEnabled++;
          else PotsEnabled = 0;
          // if (PotsEnabled == 1 || PotsEnabled == 3) digitalWrite(51, HIGH); // Switches enabled LED
          // else digitalWrite(51, LOW);
          if (PotsEnabled >= 2) digitalWrite(23, HIGH); // Pots enabled LED
          else digitalWrite(23, LOW);
          if      (PotsEnabled == 0) Serial.println("   Pots & Switches Disabled\n");
          else if (PotsEnabled == 1) Serial.println("   Switches Only Enabled\n");
          else if (PotsEnabled == 2) Serial.println("   Pots Only Enabled\n");
          else if (PotsEnabled == 3) Serial.println("   Pots & Switches Enabled\n");
          
          break;
        case 'p':
          if (Control > 0) // synchronized waves range:
          {
            PotPulseWidth[1] = !PotPulseWidth[1]; // enable / disable pulse width input from pot instead of duty-cycle %
          }
          if (Control != 1) // unsynchronized waves range:
          {
            PotPulseWidth[0] = !PotPulseWidth[0]; // enable / disable pulse width input from pot instead of duty-cycle %
          }
          SwitchPressedTime = millis();
          break;
        case '?': 
            Serial.println("\n   STATUS:");
            if (TimerMode > 0) 
            {
              Serial.print("   Timer period is set to: "); Serial.print(PeriodD); Serial.print(" days, "); Serial.print(PeriodH); Serial.print(" hours, "); Serial.print(PeriodM); Serial.print(" mins, "); Serial.print(PeriodS); Serial.println(" secs.");
            }
            else if (SweepMode > 0)
            {
              Serial.print("   Freq Sweep: Min freq = "); Serial.print(SweepMinFreq); Serial.print(" Hz. Max freq = "); Serial.print(SweepMaxFreq); Serial.print(" Hz. Rise time = "); Serial.print(SweepRiseTime); Serial.print(" Sec. Fall time = "); Serial.print(SweepFallTime); Serial.println(" Sec");
            }
            if  (ExactFreqMode) Serial.print("   Exact Freq Mode is ON ");
            else                Serial.print("   Exact Freq Mode is OFF");
            if (SquareWaveSync) Serial.println("  Square Wave is Synchronized with Analogue Wave");
            else                Serial.println("  Square Wave is Unsynchronized");
            if (Control > 0 && TimerMode == 0) Serial.print(">> Analogue Wave Freq: ");
            else  Serial.print("   Analogue Wave Freq: ");
            PrintSyncedWaveFreq();
            Serial.print(", Analogue Wave Duty-cycle: "); Serial.print(ActualWaveDuty); Serial.println(" %");
            Serial.print("   Analogue Wave Period: ");
            PrintSyncedWavePeriod();
            if (Control != 1 && TimerMode == 0) Serial.print(">> Unsync'ed Sq.Wave Freq: ");
            else  Serial.print("   Unsync'ed Sq.Wave Freq: ");
            PrintUnsyncedSqWaveFreq();
            Serial.print(", Unsync'ed Sq.Wave Duty-cycle: "); Serial.print(ActualDuty); Serial.println(" %");
            Serial.print("   Unsync'ed Sq.Wave Period: ");
            PrintUnsyncedSqWavePeriod();
            Serial.print("   Noise Colour is ");
            Serial.print(NoiseColour);
            if      (NoiseColour >= 820) Serial.println(" - White");
            else if (NoiseColour >= 480 && NoiseColour <= 520) Serial.println(" - Pink");
            else if (NoiseColour >= 20 && NoiseColour <= 40) Serial.println(" - Brown");
            else Serial.println("");
            if (PotsEnabled == 1) Serial.println("   Switches Only Enabled");
            else if (PotsEnabled >= 2)
            {
              if (PotsEnabled == 2) Serial.print("   Pots Only Enabled - Analog Wave: ");
              else if (PotsEnabled == 3) Serial.print("   Pots & Switches Enabled - Analog Wave: ");
              if      (PotPeriodMode[1] == 0) Serial.print("Freq ");
              else if (PotPeriodMode[1] == 1) Serial.print("Period");
              Serial.print("  Unsync'ed Sq.Wave: ");
              if      (PotPeriodMode[0] == 0) Serial.println("Freq");
              else if (PotPeriodMode[0] == 1) Serial.println("Period");
              Serial.print("   Freq Pot Range - Analog Wave: x "); Serial.print(Range[1]); Serial.print("  Unsync'ed Sq.Wave: x "); Serial.println(Range[0]);
              if (PotPulseWidth) Serial.print("   Pulse Width Range - Analog Wave: x "); Serial.print(Range[3]); Serial.print("  Unsync'ed Sq.Wave: x "); Serial.println(Range[2]);
            }
            else if (PotsEnabled == 0 && millis() < 180000) Serial.println("   Type 'P' to enable / disable pots & / or switches"); // only shown for 1st 3 minutes
            Serial.println();
            TouchedTime = 0;
          break;
        case 'l':
          Serial.println();
          delay(1); // short delay to ensure next char is ready to be read correctly by Serial.peek()
          if (Serial.peek() == 'p') // if received "lp" - list preset usage command
          {
            for (byte i = 1; i <= 50; i++) // send List of Preset usage info. i = Preset Number
            {
              Serial.print("   Preset "); Serial.print(i);
              byte usage = dueFlashStorage.read((i * 240) + 3); // read Preset Usage info from flash starting at address: (Preset address) + 3
              if (usage > 11) Serial.print(" is empty");
              else // read name, etc
              {
                Serial.print(" ");
                for (byte ii = 0; ii < 22 ; ii++) // Read Name from flash memory, 1 character at a time
                {
                  char ch = dueFlashStorage.read((i * 240) + 221 + ii); // read Name from flash starting at address: (Preset address) + 220
                  if (ch == '\n')
                  {
                    if (ii == 0) Serial.print("has no name");
                    break;
                  }
                  Serial.print(ch);
                }
                if (usage == 1 || usage == 11) Serial.print(" - Arbitrary wave included");
              }
              Serial.println();
            }
          }
          Serial.read(); // clear 'p' or 't'
          Serial.println();
          break;
        case 'C':
          delay(1); // short delay to ensure next char is ready to be read correctly by Serial.peek()
          if (Serial.peek() == 'P') // if received "CP" - Clear Preset command
          {
            Serial.read(); // clear 'P'
            int preset = Serial.parseInt();
            if (preset < 1 || preset > 50)
            {
              Serial.print("   Preset "); Serial.print(preset); Serial.println(" does not exist!\n");
              break;
            }
            else if (dueFlashStorage.read((preset * 240) + 3) > 11) { Serial.print("   Preset "); Serial.print(preset); Serial.println(" is already empty!\n"); } // if Preset is empty
            else
            {
              if (!UsingGUI) { Serial.print("   Are you sure you want to clear Preset "); Serial.print(preset); Serial.println("?  Type Y or N  (the N must be upper case)\n"); }
              ClearPreset = preset + 50; // + 50 means clear instead of replace // prevents "cleared" message being shown
            }
          }
          break;
        case 'y': // y = (yes) answer to question - can be upper or lower case
        case 'Y': // Y = (Yes) answer to question - can be upper or lower case
          if (ClearPreset <= 100)
          {
            if (ClearPreset <= 50) // if replacing Preset or start-up defaults
            {
              SaveToFlash(ClearPreset);
              if (UsingGUI) { Serial.print("Preset "); Serial.print(ClearPreset); Serial.println(" saved"); }
              else
              {
                if (ClearPreset > 0) { Serial.print("   Current Settings have been saved as Preset "); Serial.print(ClearPreset); }
                else Serial.print("   Current Settings have been saved as Start-up Default ");
                if (dueFlashStorage.read((ClearPreset * 240) + 3) == 11 && ClearPreset < 30)  Serial.println(" - including Arbitrary wave!\n"); // if Arbitrary wave included
                else Serial.println(" - without Arbitrary wave!\n"); // if Arbitrary wave not included
              }
            }
            else // if clearing Preset
            {
              ClearPreset -= 50;
              dueFlashStorage.write((ClearPreset * 240) + 3, 255); // 255 means empty
              dueFlashStorage.write((ClearPreset * 240) + 220, 255); // InterruptMode // 255 means empty
              dueFlashStorage.write((ClearPreset * 240) + 221, '\n'); // set to '\n' so GUI can detect and count each Preset Name
              Serial.print("   Preset "); Serial.print(ClearPreset); Serial.println(" cleared!\n"); // if question prompted by Clear Preset command
            }
            ClearPreset = 255; // 255 means empty
          }
          break;
        case 'N': // N = (No) answer to question - must be upper case (lower case used for noise commands)
          if (ClearPreset <= 100 || ClearTune <= 100) Serial.println("   Cancelled!\n");
          ClearPreset = 255;
          ClearTune = 255;
          break;
        case 'F':
        case 'L':
        case 'S':
          delay(1); // short delay to ensure next char is ready to be read correctly by Serial.peek()
          if (UserChars[0] == 'F' && Serial.peek() == 'D') // if received "FD" - load Factory Defaults command
          {
            Serial.read(); // clear 'D'
            if (UsingGUI) Serial.println("Factory Defaults");
            UserChars[2] = '!'; // indicates loading Defaults or Preset when changing WaveShape
            Settings(2, 0, 0);
            UserChars[2] = ' ';
            LoadedPreset = 0;
            if (!UsingGUI)
            {
              Serial.println("   Factory Default settings loaded, but your previous defaults are not lost - yet!");
              Serial.println("   To keep these factory settings, type SD to Save as the Start-up Default settings");
              Serial.println("   Otherwise your previous default settings will be loaded at next start-up");
              Serial.println("   (Or you can Load your previous Start-up Default settings by typing LD)\n");
            }
            break;
          }
          else if (UserChars[0] == 'L' && (Serial.peek() == 'D' || Serial.peek() == 'P' || Serial.peek() == 'T')) // if received "LD" "LP" or "LT" - Load Defaults or Load Preset or Link Tune to preset command (for load Tune see Play Tune)
          {
            if (Serial.peek() == 'D')
            {
              Serial.read(); // clear 'D'
              UserChars[2] = '!'; // indicates loading Defaults or Preset when changing WaveShape
              Settings(1, 0, UsingGUI); // send to GUI if using GUI
              UserChars[2] = ' '; // return to default
              LoadedPreset = 0;
              if (UsingGUI) Serial.println("Defaults loaded");
              else
              {
                Serial.print("   Start-up Default settings loaded");
                if (dueFlashStorage.read(3) == 1 || dueFlashStorage.read(3) == 11) Serial.println(" - including Arbitrary wave!\n"); // if Arbitrary wave included
                else                                                               Serial.println(" - without Arbitrary wave!\n"); // if Arbitrary wave not included
              }
            }
            else if (Serial.peek() == 'P') // Load Preset:
            {
              Serial.read(); // clear 'P'
              int preset = Serial.parseInt();
              if (preset < 1 || preset > 50)
              {
                Serial.print("   Preset "); Serial.print(preset); Serial.println(" does not exist!\n");
                break;
              }
              if (dueFlashStorage.read((preset * 240) + 3) <= 11) // if Preset not empty
              {
                UserChars[2] = '!'; // indicates loading Defaults or Preset when changing WaveShape
                NVIC_DisableIRQ(TC0_IRQn); // stop interrupts running if in music mode (to speed up loading settings)
                Settings(1, preset, UsingGUI); // if Preset not empty, read it from flash in Settings() // send to GUI if using GUI
                if (InterruptMode > 0) NVIC_EnableIRQ(TC0_IRQn); // start interrupts running if in modulation mode or music mode            
                UserChars[2] = ' '; // return to default
                LoadedPreset = preset;
                Serial.print("   Preset "); Serial.print(preset); Serial.print(" loaded");
                if (!UsingGUI)
                {
                  if (preset < 26 && (dueFlashStorage.read((preset * 240) + 3) == 1 || dueFlashStorage.read((preset * 240) + 3) == 11)) Serial.print(" - including Arbitrary wave!"); // if Arbitrary wave included
                  else                                                                                                                  Serial.print(" - without Arbitrary wave!"); // if Arbitrary wave not included
                }
                Serial.println("\n");
              }
              else { Serial.print("   Preset "); Serial.print(preset); Serial.println(" is empty!\n"); }
            }            
            break;
          }
          default:
        UserChars[3] = '>'; // if none of the above matched continue to search below
      }
      if (TimerMode == 0 && UserChars[3] == '>') // if NOT in timer mode and nothing above matched
      {
        if (PotsEnabled == 0 && SweepMode == 0 && UserChars[3] == '>') // if pots NOT enabled and NOT in sweep mode and nothing above matched
        {
          UserChars[3] = ' '; // reset
          switch (UserChars[0])
          {
            case 'M': // Min/Max duty-cycle percentage, expressed in samples (when synchronized & above 1kHz with Exact Freq Mode off) - will return to default (4) when switching from unsynchronized to synchronized square wave (by typing 'v') [settings below 4 cause polarity reversal due to DMA timing]
              if (UserInput >= 1 && UserInput <= 7) MinMaxDuty = UserInput;
              else MinMaxDuty = 4;
              Serial.print("   You have temporarily set MinMaxDuty to "); Serial.print(MinMaxDuty); Serial.println("  It will be reset to 4 by typing 'M' or 'v')\n");
              break;
            case 'H': // square wave sync delay at High sample rate. ie: 10kHz, 20kHz, 40khz & 100kHz [delay (high or low) can't be adjusted below 1.01kHz] - default is 10
              if (UserInput >= 1) Delay1 = min(UserInput, 25);
              else Delay1 = 10; // 10;
              SyncDelay = (TimerCounts - Delay1) * Delay2 * max((Delay3 / (abs(ActualWaveDuty - 50.0) + Delay3)), int(MinOrMaxWaveDuty));
              Serial.print("   You have set Delay1 to "); Serial.print(Delay1); Serial.println("  It should be set to 10 (Type 'H')\n");
              break;
            case 'L': // square wave sync delay at Low sample rate. ie: 1.1kHz, 11kHz, 21kHz, & 41kHz (adj high sample rate delay 1st) - default is 36 - 55
              if (UserInput >= 1) Delay2 = min(UserInput * 0.01, 50);
              else Delay2 = 0.55; // 0.36;
              SyncDelay = (TimerCounts - Delay1) * Delay2 * max((Delay3 / (abs(ActualWaveDuty - 50.0) + Delay3)), int(MinOrMaxWaveDuty));
              Serial.print("   You have set Delay2 to "); Serial.print(Delay2 * 100, 0); Serial.println("  It should be set to 55 (Type 'L')\n");
              break;
            case 'D': // square wave sync Delay at low duty cycle, but not 0%. - default is 110
              if (UserInput >= 50) Delay3 = min(UserInput, 200);
              else Delay3 = 110;
              SyncDelay = (TimerCounts - Delay1) * Delay2 * max((Delay3 / (abs(ActualWaveDuty - 50.0) + Delay3)), int(MinOrMaxWaveDuty));
              Serial.print("   You have set Delay3 to "); Serial.print(Delay3, 0); Serial.println("  It should be set to 110 (Type 'D')\n");
              break;
            default:
              UserChars[3] = '>'; // if none of the above matched continue to search below (L or H may be found below in sweep mode, or H or M or D if in timer mode, or R if pots enabled)
              // break;
          }
        }
        if (UserChars[3] == '>') // if nothing above matched
        {
          switch (UserChars[0])
          {
            case 'T':  // if entering Timer Mode:
              EnterTimerMode();
              break;
            case 'v': // toggle between Viewing sync'ed square wave or unsync'ed square wave
              if (WaveShape == 4) Serial.println("   There is no wave to synchronize with, only noise!\n");
              else ToggleSquareWaveSync(0);
              break;
            case ' ': // space-bar - toggle control between sync'ed waves & unsync'ed sq.wave
              if (!UsingGUI)
              {
                if (Control == 1)
                {
                  Control = 0;
                  Serial.println("   CONTROL >> Unsync'ed Sq.Wave\n");
                }
                else
                {
                  Control = 1;
                  Serial.println("   CONTROL >> Analogue Wave\n");
                }
              }
              break;
            case 'b': // control both analogue wave & Unsync'ed sq.wave together
              if (Serial.peek() == ' ') // if b is followed by a space
              {
                Serial.read(); // clear 1st space
                if (Serial.peek() == ' ') // if 2 spaces after b
                {
                  Control = 0;
                  Serial.read(); // clear 2nd space so it won't be read again
                }
                else Control = 1; // if only one space after b
              }
              else Control = 2; // if no space after 'b'
              if      (Control == 0) Serial.println("   CONTROL >> Unsync'ed Sq.Wave\n");
              else if (Control == 1) Serial.println("   CONTROL >> Analogue Wave\n");
              else                   Serial.println("   CONTROL >> Both Waves\n");
              break;
            case 'S': // Sweep freq mode
              EnterSweepMode();
              break;
            case 'q':
              ExitSweepMode();
              break;
            case 'L':
            case 'H':
            case 'R':
            case 'F':
              if (SweepMode > 0)
              {
                if      (UserChars[0] == 'L') SweepMinFreq = int(max(0, UserInput));
                else if (UserChars[0] == 'H') SweepMaxFreq = int(max(0, UserInput));
                else if (UserChars[0] == 'R') SweepRiseTime = int(max(0, UserInput));
                else if (UserChars[0] == 'F') SweepFallTime = int(max(0, UserInput));
                if (!UsingGUI)
                {
                  Serial.print("   You typed:\n\tHighest freq = "); Serial.print(SweepMaxFreq); Serial.print(" Hz  \tRise time = "); Serial.print(SweepRiseTime); Serial.print(" Sec\n  \tLowest freq  = "); Serial.print(SweepMinFreq); Serial.print(" Hz  \tFall time = "); Serial.print(SweepFallTime); Serial.println(" Sec");
                  //              Serial.println(String("   You typed:\n\tHighest freq = ") + SweepMaxFreq + String(" Hz  \tRise time = ") + SweepRiseTime + String(" Sec\n  \tLowest freq  = ") + SweepMinFreq + String(" Hz  \tFall time = ") + SweepFallTime + String(" Sec"));
                  if (SweepMinFreq == 0 || SweepMinFreq >= SweepMaxFreq || SweepRiseTime + SweepFallTime == 0)
                  {
                    Serial.println("   Sorry, but that is invalid. Please try again.");
                    if (SweepMinFreq <= 0 ) Serial.println("   The Lowest freq must not be set to zero!");
                    if (SweepMinFreq >= SweepMaxFreq) Serial.println("   The Highest freq must be higher than the Lowest freq!");
                    if (SweepRiseTime + SweepFallTime == 0) Serial.println("   Only one of the Rise and Fall times can be set to zero!");
                  }
                  else Serial.println("\n   If this is correct type r to start the sweep running. \n\n       *****  Type r again to stop the sweep!  *****");
                  Serial.println("\n            (Type q to quit the Freq Sweep Mode)\n");
                }
              }
              else if (UserChars[0] == 'R' && PotsEnabled >= 2) // if NOT in sweep mode: Range of Duty-cycle / Pulse Width Pots
              {
                if (Control > 0) // synchronized waves range:
                {
                  if (Range[3] < 10000) Range[3] *= 10;
                  else Range[3] = 1;
                  Serial.print("   Synchronized Waves Pot Pulse Width Range: x "); Serial.println(Range[3]);
                }
                if (Control != 1) // unsynchronized sq.wave range:
                {
                  if (Range[2] < 10000) Range[2] *= 10;
                  else Range[2] = 1;
                  Serial.print("   Unsync'ed Sq.Wave Pot Pulse Width Range: x "); Serial.println(Range[2]);
                }
                SwitchPressedTime = millis();
              }
              break;
            case 'r': // start Running freq sweep OR Range of Freq / Period Pots
              if (SweepMode > 0)
              {
                if (SweepMinFreq > 0 && SweepMinFreq < SweepMaxFreq && SweepRiseTime + SweepFallTime > 0)
                {
                  SweepMode = 2;
                  SweepFreq(); // start Running freq sweep
                }
                else Serial.println("   The settings are incorrect. The sweep cannot run! ");
              }
              else if (PotsEnabled >= 2) // Range of Freq / Period Pots
              {
                if (Control > 0) // synchronized waves range:
                {
                  if (Range[1] < 10000) Range[1] *= 10;
                  else Range[1] = 1;
                  Serial.print("   Synchronized Waves Pot Freq Range: x "); Serial.println(Range[1]);
                }
                if (Control != 1) // unsynchronized sq.wave range:
                {
                  if (Range[0] < 10000) Range[0] *= 10;
                  else Range[0] = 1;
                  Serial.print("   Unsync'ed Sq.Wave Pot Freq Range: x "); Serial.println(Range[0]);
                }
                SwitchPressedTime = millis();
              }
              else if (!UsingGUI)
              {
                Serial.println("\n        ***** RANDOM ARBITRARY WAVE CREATION *****\n");
                Serial.println("   Type: 'rs' to create a Random Stepped arbitrary wave.");
                Serial.println("   Type: 'rn' to create a Random arbitrary wave with No steps.");
                Serial.println("   Type: 'rm' to create an arbitrary wave with randomly Mixed steps.\n\n");
              }
              break;
            case 'f': // toggle between pot controlling Freq of wave, or period of wave
              if (Control > 0)
              {
                PotPeriodMode[1] = !PotPeriodMode[1]; // toggle PotPeriodMode for synch'ed waves
                Serial.print("   Synchronized Waves Pot Mode is ");
                if      (PotPeriodMode[1] == 0) Serial.println("Freq");
                else if (PotPeriodMode[1] == 1) Serial.println("Period");
              }
              if (Control != 1)
              {
                PotPeriodMode[0] = !PotPeriodMode[0]; // toggle PotPeriodMode for unsynch'ed wave
                Serial.print("   Unsynchronized Wave Pot Mode is ");
                if      (PotPeriodMode[0] == 0) Serial.println("Freq");
                else if (PotPeriodMode[0] == 1) Serial.println("Period");
              }
              Serial.println("");
              SwitchPressedTime = millis();
              break;
            default:
              UserChars[3] = '>'; // if none of the above matched continue to search below
              if (!UsingGUI)
              {
                if (SweepMode > 0)
                {
                  Serial.println("\n   Sweep Frequency Commands:\n");
                  Serial.println("   S - enter the freq Sweep mode");
                  Serial.println("   q - Quit the freq sweep mode");
                  Serial.println("   L - Lowest frequency in hz  (eg: 20L)");
                  Serial.println("   H - Highest frequency in hz (eg: 80H)");
                  Serial.println("   R - Rise time in seconds    (eg: 20R)");
                  Serial.println("   F - Fall time in seconds    (eg: 10F)");
                  Serial.println("   r - start or stop the sweep Running\n");
                }
                else if (SweepMode == 0) // if NOT in sweep mode
                {
                  Serial.println("\n   HELP:  Type the following, then press enter:");
                  Serial.println(  "   Type:   a   to create a new Arbitrary wave or view the menu - follow on-screen instructions.");
                  Serial.println(  "   Type:   X   to delete any existing uploaded Arbitrary wave");
                  Serial.println(  "   Type:   r   to view a menu of commands for creating a Random arbitrary wave (Pots must be disabled).");
                  Serial.println(  "   Type:   s   to view a menu of Setup commands for each wave shape.");
                  Serial.println(  "   Type:   n   to view a menu of Noise commands for the TRNG True Random Noise Generator.");
                  Serial.println(  "   Type:   w   to cycle through the analogue Wave shapes (includig noise).");
                  Serial.println(  "   Type:   wr  to cycle through the analogue Wave shapes in Reverse order.");
                  Serial.println(  "   Type:   wx  to switch directly to a Wave shape, where x is the wave shape number: 0 to 4");
                  Serial.println(  "   Type:   v   to toggle between Viewing synchronized or unsynchronized square wave.");
                  Serial.println(  "   Type:  ' '  [spacebar] to toggle between controlling analogue wave or unsync'ed square wave.");
                  Serial.println(  "   Type:   b   to control Both analogue and unsynchronized waves simultaneously.");
                  Serial.println(  "   Type:   h   to set frequency of wave/s, type required frequency in Hz followed by h.");
                  Serial.println(  "   Type:   m   to set period of wave/s, type required period in Milliseconds followed by m.");
                  Serial.println(  "   Type:   d   to set Duty-cycle, type required percentage duty-cycle (0 - 100) followed by d.");
                  Serial.println(  "   Type:   u   to set pulse width, type required pulse width in microseconds followed by u.");
                  Serial.println(  "   Type:   e   to toggle on/off Exact freq mode for analogue wave, eliminating freq steps.");
                  Serial.println(  "   Type:   S   to enter the frequency Sweep mode - follow on-screen instructions.");
                  Serial.println(  "   Type:   T   to enter the Timer mode - follow on-screen instructions.");
                  Serial.println(  "   Type:   P   once to enable switches only, or twice for Pots. 3 times enables both.");
                  Serial.println(  "   Type:   f   to toggle between pot controlling Freq of wave, or period of wave.");
                  Serial.println(  "   Type:   p   to toggle between pot controlling duty-cycle Percent, or Pulse width of wave.");
                  Serial.println(  "   Type:   r   to cycle through the Range of the frequency/period pot: x1, x10, x100, x1000 & x10000.");
                  Serial.println(  "   Type:   R   to cycle through the Range of the pulse width pot: x1, x10, x100, x1000 & x10000.");
                  Serial.println(  "   Type:  LD   to Load start-up Default settings.");
                  Serial.println(  "   Type:  FD   to load Factory start-up Default settings.");
                  Serial.println(  "   Type:  LPx  to Load Preset settings. The x is the Preset number: 1 to 50.");
                  Serial.println(  "   Type:  CPx  to Clear Preset settings. The x is the Preset number: 1 to 50.");
                  Serial.println(  "   Type:  lp   to view a List of Presets with their names.");
                  Serial.println(  "   Type:   ?   to display the current status.\n\n");
                  Serial.readString(); // ensures this help menu is not displayed multiple times if multiple chars are sent!
                }
              }
             // break;
          }
        }
      } // end of if NOT in timer mode and nothing above matched
      else if (TimerMode > 0 && InterruptMode == 0 && UserChars[3] == '>') // If in Timer mode: // not in music compose mode
      {
        switch (UserChars[0])
        {
          case 'q': // if exiting Timer Mode
            ExitTimerMode();
            break;
          case 'D':
          case 'H':
          case 'M':
          case 'S':
            if (UsingGUI && TimerRun)
            {
              if      (UserChars[0] == 'D') TimerDays = int(UserInput);
              else if (UserChars[0] == 'H') TimerHours = int(UserInput);
              else if (UserChars[0] == 'M') TimerMins = int(UserInput);
              else if (UserChars[0] == 'S') TimerSecs = int(UserInput);
            }
            else // if (!UsingGUI || !TimerRun)
            {
              if      (UserChars[0] == 'D') PeriodD = int(max(0, UserInput));
              else if (UserChars[0] == 'H') PeriodH = int(max(0, UserInput));
              else if (UserChars[0] == 'M') PeriodM = int(max(0, UserInput));
              else if (UserChars[0] == 'S') PeriodS = int(max(0, UserInput));
            }
            if (!UsingGUI)
            {
              //       Serial.println(String("   You typed: ") + PeriodD + String(" days, ") + PeriodH + String(" hours, ") + PeriodM + String(" mins, ") + PeriodS + String(" secs."));
              Serial.print("   You typed: "); Serial.print(PeriodD); Serial.print(" days, "); Serial.print(PeriodH); Serial.print(" hours, "); Serial.print(PeriodM); Serial.print(" mins, "); Serial.print(PeriodS); Serial.println(" secs.");
              if (PeriodH > 23 || PeriodM > 59 || PeriodS > 59 || PeriodD + PeriodH + PeriodM + PeriodS == 0 ) Serial.println("   Sorry but that is invalid. Please try again.");
              else Serial.println("\n   If this is correct type r to start the timer. \n\n    *****  Type r again to stop the timer!  *****");
              Serial.println("\n           (Type q to quit the Timer Mode)\n");
            }
            break;
          case 'U':
            TimeUp = 1;
            Serial.println("   Time Up!");
            if (TimerInvert) digitalWrite(7, LOW);
            else digitalWrite(7, HIGH);
            break;
          case 'R': // start timer running if using GUI
            TimerRun = 1;
            Serial.println("   Timer Running...");
            break;
          case 'r': // toggle timer running if not using GUI - or stop / reset only if using GUI
            timerRun();
            break;
          case 'i':
            TimerInvert = false;
            digitalWrite(7, LOW);
            Serial.println("   Positive Timer Mode\n");
            break;
          case 'I':
            TimerInvert = true;
            digitalWrite(7, HIGH);
            Serial.println("   Negative Timer Mode\n");
            break;
          default:
            Serial.println("\n   Timer Commands:\n");
            Serial.println("   T - enter the Timer mode");
            Serial.println("   q - Quit the timer mode");
            Serial.println("   D - number of Days    (eg: 15D)");
            Serial.println("   H - number of Hours   (eg: 23H)");
            Serial.println("   M - number of Minutes (eg: 59M)");
            Serial.println("   S - number of Seconds (eg: 59S)");
            Serial.println("   I - Invert output, negative mode");
            Serial.println("   i - don't Invert output, positive mode");
            Serial.println("   r - start or stop the timer Running\n");
        }
      }
    }
    UserInput = 0; // reset to 0 ready for the next sequence of digits
    UserChars[0] = '>'; // Change 1st character to one not used. ' ' is used as 1st char.
    for (byte i = 1; i < 4; i++) UserChars[i] = ' '; // Change to characters not used. A space will go unnoticed so will look better in the serial monitor
    if (Serial.peek() == '\n') Serial.read(); // remove \n to ensure a clean start for next loop
  }
  //************************************************************************
  if (TimerRun == 1 || (SweepMode == 0 && TimerMode == 0 && ((!SquareWaveSync && TargetFreq < 0.5) || (SquareWaveSync && TargetWaveFreq < 0.5)))) // if timer running or if sq.wave is less than 0.5Hz
  {
    bool waveHalfStart = 0;
    if ((SquareWaveSync && WaveBit < OldTime) || (!SquareWaveSync && TimeIncrement < OldTime)) waveHalfStart = 1;
    byte sec = TC1->TC_CHANNEL[1].TC_CV / 656250; // elapsed seconds not in timer mode. 656250(Hz) * 60(seconds) = 39375000 clocks per minute
    //    Serial.print(", WaveBit: "); Serial.print(WaveBit); Serial.print(", OldTime: "); Serial.print(OldTime); Serial.print(", TimeIncrement: "); Serial.println(TimeIncrement);
    //    Serial.print(", sec: "); Serial.print(sec); Serial.print(", OldSec: "); Serial.print(OldSec); Serial.print(", waveHalfStart: "); Serial.println(waveHalfStart);
    if ((sec != OldSec) || waveHalfStart) // if remaining seconds have changed or at start of wavehalf then update
    {
      if ((sec == OldSec + 1 || (sec == 0 && OldSec == 59)) && !waveHalfStart && LowFreqDisplay == 1)
      {
        SecChanged = 1;
        if (TimerSecs == 59)
        {
          if (TimerMins < 59) TimerMins++;
          else
          {
            TimerMins = 0;
            if (TimerHours < 23) TimerHours++;
            else
            {
              TimerHours = 0;
              TimerDays++;
            }
          }
        }
        LowFreqDisplay = 2;
      }
      byte oldTimerSecs = TimerSecs;
      if (waveHalfStart)
      {
        TimerSecs = 0;
        OldSec = 0;
        sec = 0;
      }
      else TimerSecs = sec;
      if (TimerMode > 0 && !UsingGUI) // if in timer mode & not using GUI
      {
        if (TimerDays >= PeriodD && TimerHours >= PeriodH && TimerMins >= PeriodM && TimerSecs >= PeriodS && PeriodD + PeriodH + PeriodM + PeriodS > 0) // if time elapsed & if not set to zero
        {
          if (TimerInvert) digitalWrite(7, LOW);
          else digitalWrite(7, HIGH);
          TimeUp = 1; // keeps output high (or low) after time is up in timer mode
          Serial.print("   *** Time Is Up! ***");
        }
      }
      else if (((!SquareWaveSync && TargetFreq < 0.5) || (SquareWaveSync && TargetWaveFreq < 0.5)) && TimerMode == 0 && ArbUpload == 0) // ArbUpload = 1 if arbitrary wave being received
      {
        if (oldTimerSecs + TimerMins + TimerHours + TimerDays > 0 && waveHalfStart) // if at start of wave half
        {
          if ((SquareWaveSync && (WaveHalf || AnaPulseWidth < 1000)) || !SquareWaveSync) // if at start of period
          {
            TimerSecs  = 0; // Reset time at start of each cycle
            TimerMins  = 0;
            TimerHours = 0;
            TimerDays  = 0;
            LowFreqDisplay = 2;
            delay(10); // slightly late sync of timer ensures period resets reliably next time
            TC_Start(TC1, 1); // Reset timer
          }
        }
      }
      if            (UsingGUI && TimerMode == 0 && LowFreqDisplay == 2 && ArbUpload == 0) Serial.print("INFO>");
      //    if       (!UsingGUI && TimerMode > 0) Serial.println(String("   Time Elapsed: ") + TimerDays + String(" days, ") + TimerHours + String(" hours, ") + TimerMins + String(" mins, ") + TimerSecs + String(" secs\n"));
      if       (!UsingGUI && TimerMode > 0)
      {
        Serial.print("   Time Elapsed: ");
        Serial.print(TimerDays);
        Serial.print(" days, ");
        Serial.print(TimerHours);
        Serial.print(" hours, ");
        Serial.print(TimerMins);
        Serial.print(" mins, ");
        Serial.print(TimerSecs);
        Serial.println(" secs\n");
      }
      else if (SweepMode == 0 && TimerMode == 0 && LowFreqDisplay == 2 && ArbUpload == 0)
      {
        Serial.print("   ");
        Serial.print(TimerHours);
        Serial.print(" hours, ");
        Serial.print(TimerMins);
        Serial.print(" mins, ");
        Serial.print(TimerSecs);
        Serial.println(" secs from start of period\n");
      }
      //    else if (SweepMode == 0 && TimerMode == 0 && LowFreqDisplay == 2 && ArbUpload == 0) Serial.println(String("   ") + TimerHours + String(" hours, ") + TimerMins + String(" mins, ") + TimerSecs + String(" secs from start of period\n"));
      if (LowFreqDisplay == 2) LowFreqDisplay = 1; // prevents half-wave being displayed
      if (SquareWaveSync) OldTime = WaveBit;
      else                OldTime = TimeIncrement;
      SecChanged = 0;
    }
    OldSec = sec;
  }
  else if (TimerMode == 0) LowFreqDisplay = 0;
  // at VERY, VERY low frequencies (like a period of hundreds of seconds) the Increment[] that determines the frequency is a very small number, for example just 5 or 6, so to avoid freq steps dithering of the Increment[] is used to achieve frequencies between the 5 & 6 for example
  // To use 64 bit numbers (for Increment[] etc) instead of dithering, a slower sample rate (in slow mode at all frequencies) would be needed
  if (DitherTime > 0 && millis() >= DitherTime && TargetWaveFreq < 0.1) // add increment dithering for very low (intermediate) freq compensation:
  {
    DitherTime = millis() + 100; // how often to dither
    int numDithPoints = min(1 / TargetWaveFreq, 1000); // number of points to dither over
    if (DitherPoint < numDithPoints - 1) DitherPoint++; // current Dither point number
    else DitherPoint = 0;
    if ((round(FreqIncrmt[0] * numDithPoints) % numDithPoints) + DitherPoint > numDithPoints - 1) Increment[0] = int(FreqIncrmt[0]) + 1; // add dither to 1st half of wave Increment[0] if required
    else Increment[0] = int(FreqIncrmt[0]); // add no dither
    if ((round(FreqIncrmt[1] * numDithPoints) % numDithPoints) + DitherPoint > numDithPoints - 1) Increment[1] = int(FreqIncrmt[1]) + 1; // add dither to 2nd half of wave Increment[1] if required
    else Increment[1] = int(FreqIncrmt[1]); // add no dither
  }
  else if (DitherTime > 0 && TargetWaveFreq >= 0.1) DitherTime = 0;
} // end of void loop()

void SetFreqPeriod() // FREQ or PERIOD ADJUSTMENT: in Hertz or Milliseconds
{
  if (UserChars[0] == 'm')
  {
    if (Control > 0) TargetWavePeriod = UserInput;
    if (Control != 1) TargetPeriod = UserInput;
    UserInput = 1000 / UserInput; // convert period into freq
  }
  if (UserInput >= 0.00001999 && UserInput <= 42000000)
  {
    if (Control > 0) // sync'ed waves
    {
      PotAdjFreq[1] = 0;
      if (TargetWavePulseWidth > 0) TargetWaveDuty = min(100, 100 * (TargetWavePulseWidth / (1000000 / min(UserInput, 100961.54)))); // convert pulse width input into %
      TargetWaveFreq = min(UserInput, 100961.54);
      SetWaveFreq(1);
    }
    if (Control != 1) // unsync'ed sq.wave
    {
      PotAdjFreq[0] = 0;
      if (TargetPulseWidth > 0) TargetDuty = min(100, 100 * (TargetPulseWidth / (1000000 / min(UserInput, 42000000)))); // convert pulse width input into %
      TargetFreq = UserInput;
      SetFreqAndDuty(1, 1);
      Serial.print("   Unsync'ed Sq.Wave Freq: "); PrintUnsyncedSqWaveFreq(); Serial.print(", Target: "); Serial.print(UserInput, 3);
      Serial.print(" Hz\n   Unsync'ed Sq.Wave Period: ");
      PrintUnsyncedSqWavePeriod(); // PrintUnsyncedSqWaveDuty();
      Serial.print("   Unsync'ed Sq.Wave Duty-cycle: "); Serial.print(ActualDuty); Serial.println(" %\n");
    }
    if ((!SquareWaveSync && TargetFreq < 0.5) || (SquareWaveSync && TargetWaveFreq < 0.5))
    {
      if (millis() - 500 < TouchedTime) // if freq or period entered twice (if double clicked)
      {
        if (SquareWaveSync) // sync'ed waves
        {
          WaveBit = 1; // reset sync'ed waves
          if (TargetWaveDuty == 0)
          {
            WaveHalf = 0;
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR;
          }
          else
          {
            WaveHalf = 1;
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET;
          }
        }
        else // unsync'ed sq.wave
        {
          TimeIncrement = 0; // reset unsync'ed sq wave
          if (TargetDuty < 100) PeriodHalf = 0;
          else PeriodHalf = 1;
          if (TargetDuty == 0) TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR;
          else TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET;
        }
        TC_Start(TC1, 1); // Reset timer
        TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // the counter is reset and the clock is started
        LowFreqDisplay = 1;
        TouchedTime = 0;
      }
      else TouchedTime = millis();
    }
  }
  else if (UserChars[0] == 'h')
  {
    Serial.print("   ");
    Serial.print(UserInput);
    Serial.println(" Hz is outside required freq range\n");
  }
  else
  {
    Serial.print("   ");
    Serial.print(1 / UserInput);
    Serial.println(" Secs is outside required period range\n");
  }
}

void SetDutyPulse() // DUTY CYCLE or PULSE WIDTH ADJUSTMENT: in % or microseconds
{
  if (Control > 0) // synchronized waves
  {
    PotAdjDuty[1] = 0;
    if (UserChars[0] == 'u')
    {
      TargetWavePulseWidth = UserInput;
      TargetWaveDuty = 100 * (UserInput / (1000000 / ActualWaveFreq)); // convert pulse width input into %
    }
    else
    {
      TargetWavePulseWidth = 0;
      TargetWaveDuty = UserInput;
    }
    CalculateWaveDuty(0);
    if (MinOrMaxWaveDuty) CalculateWaveDuty(0); // if at 0 or 100% duty-cycle re-calculate (for stability)
    CreateNewWave();
    Serial.print("   Analogue Wave Duty-cycle: "); Serial.print(ActualWaveDuty); Serial.print(" %, Target: "); Serial.print(TargetWaveDuty); Serial.println(" %");
    Serial.print("   Analogue Wave Period: "); PrintSyncedWavePeriod(); Serial.println("");
    if (Control < 2) Serial.println("");
  }
  if (Control != 1) // unsynchronized Square wave
  {
    PotAdjDuty[0] = 0;
    if (UserChars[0] == 'u')
    {
      TargetPulseWidth = UserInput;
      TargetDuty = 100 * (UserInput / (1000000 / ActualFreq)); // convert pulse width input into %
    }
    else
    {
      TargetPulseWidth = 0;
      TargetDuty = UserInput;
    }
    SetFreqAndDuty(0, 1);
    Serial.print("   Unsync'ed Sq.Wave Duty-cycle: "); Serial.print(ActualDuty); Serial.print(" %, Target: "); Serial.print(TargetDuty); Serial.println(" %");
    Serial.print("   Unsync'ed Sq.Wave Period: "); PrintUnsyncedSqWavePeriod(); Serial.println("\n");
  }
}

void SaveSliderDefaults()
{
  dueFlashStorage.write(4, 2); // GUI Setup Slider Lo limit
  dueFlashStorage.write(5, 19); // GUI Setup Slider Hi limit
  dueFlashStorage.write(6, 3); // GUI Setup Slider Lo limit
  dueFlashStorage.write(7, 18); // GUI Setup Slider Hi limit
  dueFlashStorage.write(8, 9); // GUI Setup Slider Lo limit
  dueFlashStorage.write(9, 12); // GUI Setup Slider Hi limit
  dueFlashStorage.write(10, 11); // GUI Setup Slider Lo limit
  dueFlashStorage.write(11, 15); // GUI Setup Slider Hi limit
  dueFlashStorage.write(12, 10); // GUI Setup Slider Lo limit
  dueFlashStorage.write(13, 17); // GUI Setup Slider Hi limit
  dueFlashStorage.write(14, 10); // GUI Setup Slider Lo limit
  dueFlashStorage.write(15, 17); // GUI Setup Slider Hi limit
  dueFlashStorage.write(16, 2); // GUI Setup Slider Lo limit
  dueFlashStorage.write(17, 19); // GUI Setup Slider Hi limit
  dueFlashStorage.write(18, 3); // GUI Setup Slider Lo limit
  dueFlashStorage.write(19, 18); // GUI Setup Slider Hi limit
  dueFlashStorage.write(20, 9); // GUI Setup Slider Lo limit
  dueFlashStorage.write(21, 12); // GUI Setup Slider Hi limit
  dueFlashStorage.write(22, 10); // GUI Setup Slider Lo limit
  dueFlashStorage.write(23, 15); // GUI Setup Slider Hi limit
  dueFlashStorage.write(24, 2); // GUI Setup Slider Lo limit
  dueFlashStorage.write(25, 19); // GUI Setup Slider Hi limit
  dueFlashStorage.write(26, 3); // GUI Setup Slider Lo limit
  dueFlashStorage.write(27, 18); // GUI Setup Slider Hi limit
  dueFlashStorage.write(28, 2); // GUI Setup Slider Lo limit
  dueFlashStorage.write(29, 19); // GUI Setup Slider Hi limit
  dueFlashStorage.write(30, 3); // GUI Setup Slider Lo limit
  dueFlashStorage.write(31, 18); // GUI Setup Slider Hi limit
  dueFlashStorage.write(32, 10); // GUI Setup Slider Lo limit
  dueFlashStorage.write(33, 11); // GUI Setup Slider Hi limit
  dueFlashStorage.write(34, 2); // GUI Setup Slider Lo limit
  dueFlashStorage.write(35, 19); // GUI Setup Slider Hi limit
  dueFlashStorage.write(36, 2); // GUI Setup Slider Lo limit
  dueFlashStorage.write(37, 19); // GUI Setup Slider Hi limit
  dueFlashStorage.write(38, 2); // GUI Setup Slider Lo limit
  dueFlashStorage.write(39, 19); // GUI Setup Slider Hi limit
  dueFlashStorage.write(40, 10); // GUI Setup Slider Lo limit
  dueFlashStorage.write(41, 18); // GUI Setup Slider Hi limit
  dueFlashStorage.write(42, 10); // GUI Setup Slider Lo limit
  dueFlashStorage.write(43, 20); // GUI Setup Slider Hi limit
}

void SendSettings(int preset) // to GUI
{
  Serial.println("<<"); // indicates GUI start receiving
  // SetupParams:
  Serial.println(SinAmp    * 100, 0); // = cfg.SinAmp; //         = 1.0;  // Amplitude
  Serial.println(SinVshift * 100, 0); // = cfg.SinVshift; //      = 0.5;  // Vertical shift
  Serial.println(SinPhase        ); // = cfg.SinPhase; //       = 0.5;  // Phase shift
  Serial.println(SinFreq2       , 0); // = cfg.SinFreq2; //       = 8;    // Sinewave 2 (2nd sinewave) Frequency Multiple. (X times Sinewave 1)
  Serial.println(SinAddMix      , 0); // = cfg.SinAddMix; //      = 0;    // Sinewave 2 percentage Mix in Add Waves mode
  Serial.println(SinMulMix      , 0); // = cfg.SinMulMix; //      = 0;    // Sinewave 2 percentage Mix in Multiply Waves mode
  Serial.println(TriAmp    * 100, 0); // = cfg.TriAmp; //         = 1.0;  // Amplitude / slope
  Serial.println(TriVshift * 100, 0); // = cfg.TriVshift; //      = 0.5;  // Vertical shift
  Serial.println(TriPhase        ); // = cfg.TriPhase; //       = 0.5;  // Phase shift
  Serial.println(TriNumS         ); // = cfg.TriNumS; //        = 0;    // Number of Steps per half wave (0 = off)
  Serial.println(ArbAmp    * 100, 0); // = cfg.ArbAmp; //         = 1.0;  // Amplitude
  Serial.println(ArbVshift * 100, 0); // = cfg.ArbVshift; //      = 0.5;  // Vertical shift
  Serial.println(ArbHzoom  * 100, 0); // = cfg.ArbHzoom; //       = 1.0;  // horizontal Zoom
  Serial.println(ArbHshift * 100, 0); // = cfg.ArbHshift; //      = 0.5;  // Horizontal shift
  Serial.println(ArbMirror       ); // = cfg.ArbMirror; //      = 0;    // half cycle Mirror effect (0 = off)
  Serial.println(ComSinAmp * 100, 0); // = cfg.ComSinAmp; //      = 0.5;  // Sine Wave mix
  Serial.println(ComTriAmp * 100, 0); // = cfg.ComTriAmp; //      = 0.5;  // Triangle Wave mix
  Serial.println(ComArbAmp * 100, 0); // = cfg.ComArbAmp; //      = 0.5;  // Arbitrary Wave mix
  Serial.println(NoiseAmp        ); // = cfg.NoiseAmp; //       = 100;  // Amplitude
  Serial.println(NoiseColour     ); // = cfg.NoiseColour; //    = 500;  // Noise colour: 500 = Pink noise
  // MainParams:
  Serial.println(TargetFreq      ); // = cfg.TargetFreq; //     = 1000;
  Serial.println(TargetWaveFreq  ); // = cfg.TargetWaveFreq; // = 1000;
  Serial.println(TargetPeriod    ); // = cfg.TargetPeriod; //   = 1.000;
  Serial.println(TargetWavePeriod); // = cfg.TargetWavePeriod;//= 1.000;
  Serial.println(TargetDuty      ); // = cfg.TargetDuty; //     = 50;
  Serial.println(TargetWaveDuty  ); // = cfg.TargetWaveDuty; // = 50;
  Serial.println(TargetPulseWidth    ); // = cfg.TargetPulseWidth; //     = 0; // 0 = not set
  Serial.println(TargetWavePulseWidth); // = cfg.TargetWavePulseWidth; // = 0; // 0 = not set
  // MainSettings:
  Serial.println(WaveShape       ); // = cfg.WaveShape; //      = 0;
  Serial.println(ExactFreqMode   ); // = cfg.ExactFreqMode; //  = 0;
  Serial.println(SquareWaveSync  ); // = cfg.SquareWaveSync; // = 0; // Sq Wave Sync always starts switched off, then is enabled later if required.
  Serial.println(TimerMode       ); // = cfg.TimerMode; //      = 0;
  Serial.println(TimerInvert     ); // = cfg.TimerInvert; //    = 0;
  Serial.println(SweepMode       ); // = cfg.SweepMode; //      = 0;
  Serial.println(SweepMinFreq    ); // = cfg.SweepMinFreq; //   = 20;
  Serial.println(SweepMaxFreq    ); // = cfg.SweepMaxFreq; //   = 20000;
  Serial.println(SweepRiseTime   ); // = cfg.SweepRiseTime; //  = 20;
  Serial.println(SweepFallTime   ); // = cfg.SweepFallTime; //  = 20;
  Serial.println(PeriodD         ); // = cfg.PeriodD; //        = 0;  // days - Target time period for timer
  Serial.println(PeriodH         ); // = cfg.PeriodH; //        = 0;
  Serial.println(PeriodM         ); // = cfg.PeriodM; //        = 0;
  Serial.println(PeriodS         ); // = cfg.PeriodS; //        = 10; // seconds - Target time period for timer
  if (preset < 0 || StartupTune > 0) // if at start-up send Preset Usage data
  {
    for (uint8_t i = 0; i <= 50; i++) // send Preset Usage data:
    {
      uint8_t musicMode = dueFlashStorage.read((i * 240) + 220); // saved interruptMode
      if (musicMode == 255) musicMode = 0; // if nothing saved
      Serial.write(dueFlashStorage.read((i * 240) + 3) + (20 * musicMode)); // add (20 * musicMode) to Usage data // send Preset Usage data
    }
    char ch = ' ';
    for (uint8_t i = 1; i <= 50; i++) // send Preset Name data. i = Preset Number
    {
      for (uint8_t ii = 0; ii < 22 ; ii++) // Read Name from flash memory, 1 character at a time
      {
        ch = dueFlashStorage.read((i * 240) + 221 + ii); // Read Name from flash starting at address: (Preset address) + 220
        Serial.print(ch);
        if (ch == '\n') break;
      }
    }
    for (uint8_t i = 0; i < 50; i++) // send Tune Usage data:
    {
      Serial.write(dueFlashStorage.read(226750 + i)); // send Tune Usage data: 255 = empty. 0 = indicates not linked to preset. 1 to 50 = preset number - sent to GUI
   /*   uint8_t linkedPreset = dueFlashStorage.read(226750 + i); // 255 = tune empty // 0 = indicates not empty & not linked to preset // 1 to 50 = preset number linked to //'
      uint8_t instrument = 0; // 0 = wave
      if (linkedPreset == 0) // 0 = tune not linked. // (if linked, instrument must be wave)
      {        
        instrument = dueFlashStorage.read(226850 + i); // saved Instrument: 0 = wave, 1 = piano, 2 = guitar, etc. (can't be 255 if linkedPreset not 255)
      }
      else if (linkedPreset <= 50) linkedPreset += 100;
      Serial.write(linkedPreset + instrument);*/ // send Tune Usage data: 255 = empty. 0 = indicates wave (and not empty) and not linked to preset. 101 to 150 = preset number (+ 100) the wave is linked to. 1 = piano, 2 = guitar, etc. sent to GUI
    }
    for (uint8_t tune = 0; tune < 50; tune++) // send Tune Name data:
    {
      for (uint8_t i = 0; i < 29 ; i++) // Read Tune Name from flash memory, 1 character at a time
      {
        ch = dueFlashStorage.read((tune * 29) + 226900 + i); // Read Tune Name character from flash
        Serial.print(ch);
        if (ch == '\n') break;
      }
    }
  }
  preset = max(0, preset);
  if (dueFlashStorage.read((preset * 240) + 3) <= 1) // will be 0 or 1 if Preset or start-up default was created by GUI, 10 or 11 if created by Serial Monitor, or 255 if empty. If 1 or 11 an arbitrary wave is included.
  {
    for (uint8_t i = 0; i < 40; i++) // send GUI Setup Slider limits:
    {
      Serial.write(dueFlashStorage.read((preset * 240) + 4 + i)); // if Preset or start-up default was created by GUI, send GUI Setup Slider limits
    }
  }
}

void SendArbitraryWave() // to GUI
{
  Serial.print("<"); // indicates start of Arbitrary wave
  for(int i = 0; i < ArbitraryPointNumber; i++)
  {
    if (ArbitraryWaveStep[i] > -1) { Serial.print(ArbitraryWaveStep[i]); Serial.print('-'); } // if stepped point, send 1st part of point
    Serial.print(ArbitraryWave[i]); // send normal point or 2nd part of stepped point
    if (i <= ArbitraryPointNumber - 2) Serial.print(','); // if not finished
    else Serial.print(';'); // if last point
  }
}

void EnterTimerMode()
{
  if (SweepMode == 0)
  {
    if (SquareWaveSync || (WaveShape == 4 && OldSquareWaveSync)) TimerMode = 2; // if SquareWaveSync was HIGH before entering timer mode
    else TimerMode = 1; // if SquareWaveSync was LOW before entering timer mode
    SquareWaveSync = LOW; // stop Synchronized Square wave
    TimerRun = 0;
    REG_PIOC_PER |= PIO_PER_P28; // PIO takes control of pin 3 from peripheral - similar to pinMode(3, OUTPUT)
    REG_PIOC_ODR |= PIO_ODR_P28; // PIO disables pin 3 (C28) - similar to pinMode(3, INPUT)
    PWMC_DisableChannel(PWM_INTERFACE, g_APinDescription[7].ulPWMChannel);
    NVIC_DisableIRQ(TC1_IRQn);
    pinMode(7, OUTPUT);
    if (TimerInvert) digitalWrite(7, HIGH);
    else digitalWrite(7, LOW);
    TimeUp = 0;
    if (UsingGUI) Serial.println("TimerOn");
    else
    {
      TC_setup5(); // use local timer instead of timer in GUI (if GUI not running)
      if (Control != 1) Serial.println("   Only the analogue wave can be controlled while in Timer Mode:\n   CONTROL >> Analogue Wave");
      else if (millis() < 180000) Serial.println("   You can still control the analogue wave while in Timer Mode"); // only shown for 1st 3 minutes
      Serial.println("\n           ********** TIMER MODE **********");
      Serial.println("\n        (Type ? for Timer Menu   Type q to quit)\n");
      //      Serial.println(String("   The current time period is: ") + PeriodD + String(" days, ") + PeriodH + String(" hours, ") + PeriodM + String(" mins, ") + PeriodS + String(" secs."));
      Serial.print("   The current time period is: "); Serial.print(PeriodD); Serial.print(" days, "); Serial.print(PeriodH); Serial.print(" hours, "); Serial.print(PeriodM); Serial.print(" mins, "); Serial.print(PeriodS); Serial.println(" secs.");
      if (PeriodH > 23 || PeriodM > 59 || PeriodS > 59 || PeriodD + PeriodH + PeriodM + PeriodS == 0 )
      {
        Serial.println("   THIS TIME PERIOD IS INVALID AND MUST BE CHANGED.");
        Serial.println("   To make changes enter the desired value followed by:\n   D for Days, H for Hours, M for Minutes and S for Seconds\n   For example: 1D 23H 59M 59S (Can be entered together without the spaces)\n");
      }
      else Serial.println("   To make changes enter the desired value followed by:\n   D for Days, H for Hours, M for Minutes and S for Seconds\n   For example: 1D 23H 59M 59S (Can be entered together without the spaces)\n\n          Type r to start the timer running\n\n       *** Type r again to stop the timer! ***\n");
    }
    Control = 1; // control analogue wave only
  }
  else Serial.println("\n   Timer cannot be started when in Sweep Frequency Mode!\n");
}

void timerRun()
{
  TimeUp = 0;
  if (TimerInvert) digitalWrite(7, HIGH);
  else digitalWrite(7, LOW);
  TimerSecs  = 0; // Reset timers
  TimerMins  = 0;
  TimerHours = 0;
  TimerDays  = 0;
  if (UsingGUI) TimerRun = 0; // reset only if using GUI
  else
  {
    if (!TimerRun && PeriodD + PeriodH + PeriodM + PeriodS == 0)
    {
      Serial.println("   WARNING! Timer is set to zero, so cannot run!\n");
      return; // break;
    }
    else if (!TimerRun && (PeriodH > 23 || PeriodM > 59 || PeriodS > 59 || PeriodD + PeriodH + PeriodM + PeriodS == 0))
    {
      Serial.println("   WARNING! THE TIME PERIOD IS INVALID AND MUST BE CHANGED!\n");
      return; // break;
    }
    TimerRun = !TimerRun; // toggle timer run if not using GUI
    TC_Start(TC1, 1);
  }
  if (TimerRun == 1) Serial.println("   Timer Running...\n");
  else Serial.println("   Timer Reset!\n");
}

void ExitTimerMode()
{
  byte timerMode = TimerMode;
  TimerMode = 0;
  if (TargetFreq < 163) // using interrupt
  {
    pinMode(7, INPUT);
    PIO_Configure(PIOC, PIO_PERIPH_B, PIO_PC28B_TIOA7, PIO_DEFAULT); // enable pin 3
    if (timerMode == 2 && WaveShape != 4) ToggleSquareWaveSync(0); // toggle SquareWaveSync to HIGH; (always LOW in timer mode) // if SquareWaveSync was HIGH before entering timer mode & noise not selected now, start Synchronized Square wave
    else if (timerMode == 1 || WaveShape == 4) NVIC_EnableIRQ(TC1_IRQn); // if SquareWaveSync was LOW & freq was low before entering timer mode OR noise is selected start Unsynchronized Square wave
  }
  else // if TargetFreq >= 163 - using PWM
  {
    if (timerMode == 2 && WaveShape != 4) ToggleSquareWaveSync(0); // toggle SquareWaveSync to HIGH; (always LOW in timer mode) // if SquareWaveSync was HIGH before entering timer mode & noise not selected now, start Synchronized Square wave
    else if (timerMode == 1 || WaveShape == 4) SetFreqAndDuty(1, 1); // if SquareWaveSync was LOW & freq was high before entering timer mode OR noise is selected start Unsynchronized Square wave (PWM)
  }
  TimerSecs  = 0;
  TimerMins  = 0;
  TimerHours = 0;
  TimerDays  = 0;
  TimerRun = 0;
  if (UsingGUI) Serial.println("TimerOff");
  else Serial.println("          *** You have exited the Timer ***\n");
}

void ChangeWaveShape(bool sentFromSerial)
{
  if (WaveShape == 4) // if exiting noise selection
  {
    NVIC_DisableIRQ(TC2_IRQn); // disable noise IRQ
    if (FastMode >= 0)
    {
      TC_setup();
      dac_setup();
    }
    else TC_setup2();
    if (OldSquareWaveSync)
    {
      if (UsingGUI) Serial.print("SyncOn");
      ToggleSquareWaveSync(1); // restore Sychronized Square Wave if necessary
    }
  }
  if (sentFromSerial)
  {
    if (UserChars[1] >= '0' && UserChars[1] <= '4') WaveShape = UserChars[1] - 48; // '0' = 48 in ASCII
    else if (Serial.peek() >= '0' && Serial.peek() <= '4') WaveShape = Serial.read() - 48; // '0' = 48 in ASCII
    else
    {
      if (Serial.peek() == 'r') // if reverse wave shape change
      {
        UserInput = 1;
        Serial.read(); // clear 'r' from serial
      }
      if      (WaveShape < NumWS && UserInput == 0) WaveShape++;
      else if (WaveShape > 0 && UserInput == 1) WaveShape--;
      else if (UserInput == 0) WaveShape = 0;
      else if (UserInput == 1) WaveShape = NumWS;
    }
    UserChars[1] = ' ';
  }  
  else // if (sent From Switch)
  {
    if (WaveShape < NumWS) WaveShape++;
    else WaveShape = 0;
  }
  if (WaveShape != 4 && UserChars[2] != '!') // ! indicates loading Preset
  {
    if (WaveShape == 0 || WaveShape == 3) CreateWaveFull(255); // quick change without update for waves 0 & 3
    else CreateWaveFull(WaveShape);
  }
  if      (WaveShape == 0) Serial.println("             ********** Sine Wave **********\n");
  else if (WaveShape == 1) Serial.println("             ******** Triangle Wave ********\n");
  else if (WaveShape == 2) Serial.println("             ******** Arbitrary Wave *******\n");
  else if (WaveShape == 3) Serial.println("             ******** Composite Wave *******\n");
  else if (WaveShape == 4) Serial.println("             ************ Noise ************\n");
  if (WaveShape == 4) // if entering noise selection
  {
    if (TimerMode == 2) OldSquareWaveSync = 1;
    else OldSquareWaveSync = SquareWaveSync;
    if (SquareWaveSync) ToggleSquareWaveSync(0); // change to Unsychronized Square Wave if sychronized
    NVIC_DisableIRQ(TC0_IRQn); // disable TC_setup2() SlowMode IRQ before setting TC_setup1()
    TC_setup1();
    dac_setup2();
//    NoiseFilterSetup();
  }
  else if (OldSquareWaveSync) OldSquareWaveSync = 0; // if exiting noise selection & changing back to Sychronized Square Wave
}

void ToggleExactFreqMode()
{
  SyncDelay = 0; // remove square wave sync delay for stability in case of sudden change to high freq
  ExactFreqMode = !ExactFreqMode;
  if (ExactFreqMode) Serial.println("   Exact Freq Mode is ON ");
  else Serial.println("   Exact Freq Mode is OFF");
  if (ExactFreqMode && TargetWaveDuty != 50) ExactFreqDutyNot50 = 1;
  else ExactFreqDutyNot50 = 0;
  SetWaveFreq(1);
  SyncDelay = (TimerCounts - Delay1) * Delay2 * max((Delay3 / (abs(ActualWaveDuty - 50.0) + Delay3)), int(MinOrMaxWaveDuty)); // re-apply square wave sync delay
}

void ToggleSquareWaveSync(bool exitingNoise) // exitingNoise: 1 = exiting noise selection. 0 = normal operation.
{
  if (((!SquareWaveSync && WaveShape != 4) || (exitingNoise && OldSquareWaveSync)) && TimerMode == 0) // toggle SquareWaveSync to ON if Noise not selected & Timer is off
  {
    NVIC_DisableIRQ(TC1_IRQn);
    PWMC_DisableChannel(PWM_INTERFACE, g_APinDescription[7].ulPWMChannel);
    pinMode(7, INPUT);
    PIO_Configure(PIOC, PIO_PERIPH_B, PIO_PC28B_TIOA7, PIO_DEFAULT); // enable pin 3
    SquareWaveSync = HIGH; // start Synchronized Square wave
    MinMaxDuty = 4; // allow min (& max) duty-cycle of 4 samples
    CalculateWaveDuty(0);
    CreateNewWave();
    WavePolarity();
    if (WaveShape != 4) Serial.println("   Square Wave is Synchronized with Analogue Wave\n");
  }
  else if (SquareWaveSync && TimerMode == 0) // toggle SquareWaveSync to OFF if Timer is off (if timer is ON do nothing)
  {
    SquareWaveSync = LOW; // stop Synchronized Square wave
    if (TargetFreq < 163)
    {
      pinMode(7, INPUT); // re-enable pin after stopping
      TC_setup3();
    }
    else
    {
      REG_PIOC_PER |= PIO_PER_P28; // PIO takes control of pin 3 from peripheral - similar to pinMode(3, OUTPUT) but faster
      REG_PIOC_ODR |= PIO_ODR_P28; // PIO disables pin 3 (C28) - similar to pinMode(3, INPUT)
      SetPWM(7, Period, Pulse); // PWMC_EnableChannel(PWM_INTERFACE, g_APinDescription[7].ulPWMChannel);
    }
    MinMaxDuty = 1; // allow min (& max) duty-cycle of 1 sample
    CalculateWaveDuty(ExactFreqMode);
    CreateNewWave();
    if (WaveShape != 4) Serial.println("   Square Wave is Unsynchronized\n");
  }
  if (ExactFreqMode) SetWaveFreq(0); // ensures exact freq maintained
}

void EnterSweepMode()
{
  if (SweepMode == 0 && TimerMode == 0)
  {
    if (!UsingGUI)
    {
      Serial.println("\n         ************ SWEEP FREQUENCY MODE ************");
      Serial.println("\n            (Type ? for Sweep Menu   Type q to quit)\n");
      Serial.print("   Current Settings are:\n\tHighest freq = "); Serial.print(SweepMaxFreq); Serial.print(" Hz  \tRise time = "); Serial.print(SweepRiseTime); Serial.print(" Sec\n  \tLowest freq  = "); Serial.print(SweepMinFreq); Serial.print(" Hz  \tFall time = "); Serial.print(SweepFallTime); Serial.println(" Sec");
      //             Serial.println(String("   Current Settings are:\n\tHighest freq = ") + SweepMaxFreq + String(" Hz  \tRise time = ") + SweepRiseTime + String(" Sec\n  \tLowest freq  = ") + SweepMinFreq + String(" Hz  \tFall time = ") + SweepFallTime + String(" Sec"));
      if (SweepMinFreq == 0 || SweepMinFreq >= SweepMaxFreq || SweepRiseTime + SweepFallTime == 0)
      {
        Serial.println("   These setting are incorrect and must be changed.");
        if (SweepMinFreq == 0 ) Serial.println("   The Lowest freq is set to zero!");
        if (SweepMinFreq >= SweepMaxFreq) Serial.println("   The Highest freq is not higher than the Lowest freq!");
        if (SweepRiseTime + SweepFallTime == 0) Serial.println("   Both Rise and Fall times are set to zero!");
        Serial.println("\n   To make changes enter the desired value followed by:\n\tH for Highest freq in Hz\tR for Rise time in seconds\n\tL for Lowest freq  in Hz\tF for Fall time in seconds\n   For example: 20L 20000H 20R 20F\n");
      }
      else Serial.println("\n   To make changes enter the desired value followed by:\n\tH for Highest freq in Hz\tR for Rise time in seconds\n\tL for Lowest freq  in Hz\tF for Fall time in seconds\n   For example: 20L 20000H 20R 20F\n\n               Type r to start the sweep Running\n\n          ***** Type r again to stop the sweep! *****\n");
    }
    else Serial.println("SweepOn");
    SweepMode = 1;
  }
}

void ExitSweepMode()
{
  if (SweepMode > 0)
  {
    SweepMode = 0;
    if (UsingGUI) Serial.println("SweepOff");
    else Serial.println("           *****  Exited Sweep Frequency Mode  *****\n");
  }
}

void SweepFreq()
{
  float oldFreq  = TargetFreq;
  float oldWaveFreq = TargetWaveFreq;
  int   SweepUpdatePeriod = 25; // change freq every SweepUpdatePeriod milliseconds
  float RiseIncrement = pow(float(SweepMaxFreq) / SweepMinFreq, 1 / (float(SweepRiseTime) / (SweepUpdatePeriod * 0.001))); // proportional freq increase required each SweepUpdatePeriod
  float FallIncrement = pow(float(SweepMinFreq) / SweepMaxFreq, 1 / (float(SweepFallTime) / (SweepUpdatePeriod * 0.001))); // proportional freq decrease required each SweepUpdatePeriod
  float TargetSweepFreq = SweepMinFreq;
  float SweepIncrement = RiseIncrement;
  unsigned long SweepUpdateTime = millis() + SweepUpdatePeriod;
  while (1)
  {
    if (TargetSweepFreq >= SweepMaxFreq) SweepIncrement = FallIncrement;
    else if (TargetSweepFreq <= SweepMinFreq) SweepIncrement = RiseIncrement;
    if (millis() >= SweepUpdateTime)
    {
      SweepUpdateTime = millis() + SweepUpdatePeriod; // set next SweepUpdateTime
      if (SweepFallTime == 0 && TargetSweepFreq >= SweepMaxFreq) TargetSweepFreq = SweepMinFreq;
      else if (SweepRiseTime == 0 && TargetSweepFreq <= SweepMinFreq) TargetSweepFreq = SweepMaxFreq;
      else TargetSweepFreq *= SweepIncrement;
      if (Control > 0)
      {
        TargetWaveFreq = min(TargetSweepFreq, 100000);
        SetWaveFreq(0);
        if (Control > 1 && !UsingGUI) Serial.println();
        if (!UsingGUI) Serial.print("\n   ");
        if (UsingGUI) Serial.print("AAF ");
        PrintSyncedWaveFreq();
      }
      if (Control != 1)
      {
        TargetFreq = TargetSweepFreq;
        SetFreqAndDuty(1, 1);
        if (UsingGUI) Serial.print("SAF ");
        else Serial.print("\n   ");
        PrintUnsyncedSqWaveFreq();
      }
    }
    if (Serial.available() > 0 || !digitalRead(21)) // if signalled to stop Running, by serial or switch
    {
      if (digitalRead(21)) UserChars[0] = Serial.read(); // if by serial
      if (UserChars[0] == 'r' || !digitalRead(21)) // if signalled to stop Running, by serial or switch
      {
        SweepMode = 1;
        if (Control > 0) // return to freq before sweep
        {
          TargetWaveFreq = oldWaveFreq;
          FreqIncrement = min(TargetWaveFreq, 100000) * 21475;
          SetWaveFreq(0);
          if (UsingGUI) Serial.print("AAF ");
          else Serial.print("\n   ");
          PrintSyncedWaveFreq();
        }
        if (Control != 1) // return to freq before sweep
        {
          TargetFreq = oldFreq;
          SetFreqAndDuty(1, 1);
          if (UsingGUI) Serial.print("SAF ");
          else Serial.print("\n   ");
          PrintUnsyncedSqWaveFreq();
        }
        if (!UsingGUI)
        {
          Serial.println("\n   Sweep stopped\n");
          Serial.print("   To repeat this freq sweep later, the following string can be entered:\n\n   "); // indent next line
          Serial.print(SweepMinFreq); Serial.print("L"); Serial.print(SweepMaxFreq); Serial.print("H"); Serial.print(SweepRiseTime); Serial.print("R"); Serial.print(SweepFallTime); Serial.println("F  (followed by 'r' to start the sweep running)");
          //        Serial.println(SweepMinFreq + String("L") + SweepMaxFreq + String("H") + SweepRiseTime + String("R") + SweepFallTime + String("Fr"));
          Serial.println("\n               Type q to quit the Freq Sweep Mode\n");
        }
        return;
      }
    }
  }
}

void PrintSyncedWaveFreq()
{
  if      (ActualWaveFreq < 1)       {
    Serial.print(ActualWaveFreq * 1000, 5);
    Serial.print(" mHz");
  }
  else if (ActualWaveFreq < 1000)    {
    Serial.print(ActualWaveFreq, 5);
    Serial.print(" Hz");
  }
  else                           {
    Serial.print(ActualWaveFreq / 1000, 5);
    Serial.print(" kHz");
  }
}

void PrintSyncedWavePeriod()
{
  float AnaPeriod = 1000 / ActualWaveFreq; // in mSeconds
  if (TargetWaveDuty == 0)
  {
    if (SquareWaveSync) AnaPulseWidth = 0.000048; // in mSeconds
    else AnaPulseWidth = 0.000350; // in mSeconds
  }
  else if (TargetWaveDuty >= 100)
  {
    if (SquareWaveSync) AnaPulseWidth = AnaPeriod - 0.000096; // in mSeconds
    else AnaPulseWidth = AnaPeriod - 0.000350; // in mSeconds
  }
  else AnaPulseWidth = ActualWaveDuty * 0.01 * AnaPeriod; // in mSeconds
  if      (AnaPeriod < 1)    {
    Serial.print(AnaPeriod * 1000, 5);
    Serial.print(" uS, Pulse Width: ");
  }
  else if (AnaPeriod < 1000) {
    Serial.print(AnaPeriod, 5);
    Serial.print(" mS, Pulse Width: ");
  }
  else if (AnaPeriod < 10000000) {
    Serial.print(AnaPeriod * 0.001, 5);
    Serial.print(" Sec, Pulse Width: ");
  }
  else                           {
    Serial.print(AnaPeriod * 0.001, 4);
    Serial.print(" Sec, Pulse Width: ");
  }
  if      (AnaPulseWidth < 0.001) {
    Serial.print(AnaPulseWidth * 1000000, 5);
    Serial.println(" nS");
  }
  else if (AnaPulseWidth < 1)     {
    Serial.print(AnaPulseWidth * 1000, 5);
    Serial.println(" uS");
  }
  else if (AnaPulseWidth < 1000)  {
    Serial.print(AnaPulseWidth, 5);
    Serial.println(" mS");
  }
  else if (AnaPulseWidth < 10000000) {
    Serial.print(AnaPulseWidth / 1000, 5);
    Serial.println(" Sec");
  }
  else                               {
    Serial.print(AnaPulseWidth / 1000, 4);
    Serial.println(" Sec");
  }
}

void PrintUnsyncedSqWaveFreq()
{
  if      (ActualFreq < 1)       {
    Serial.print(ActualFreq * 1000, 5);
    Serial.print(" mHz");
  }
  else if (ActualFreq < 1000)    {
    Serial.print(ActualFreq, 5);
    Serial.print(" Hz");
  }
  else if (ActualFreq < 1000000) {
    Serial.print(ActualFreq / 1000, 5);
    Serial.print(" kHz");
  }
  else                           {
    Serial.print(ActualFreq / 1000000, 5);
    Serial.print(" MHz");
  }
}

void PrintUnsyncedSqWavePeriod()
{
  if      (MicroPeriod < 1)       {
    Serial.print(MicroPeriod * 1000, 5);
    Serial.print(" nS, Pulse Width: ");
  }
  else if (MicroPeriod < 1000)    {
    Serial.print(MicroPeriod, 5);
    Serial.print(" uS, Pulse Width: ");
  }
  else if (MicroPeriod < 1000000) {
    Serial.print(MicroPeriod / 1000, 5);
    Serial.print(" mS, Pulse Width: ");
  }
  else if (MicroPeriod < 10000000000) {
    Serial.print(MicroPeriod / 1000000, 5);
    Serial.print(" Sec, Pulse Width: ");
  }
  else                            {
    Serial.print(MicroPeriod / 1000000, 4);
    Serial.print(" Sec, Pulse Width: ");
  }
  if      (MicroPulseWidth < 1)       {
    Serial.print(MicroPulseWidth * 1000, 5);
    Serial.println(" nS");
  }
  else if (MicroPulseWidth < 1000)    {
    Serial.print(MicroPulseWidth, 5);
    Serial.println(" uS");
  }
  else if (MicroPulseWidth < 1000000) {
    Serial.print(MicroPulseWidth / 1000, 5);
    Serial.println(" mS");
  }
  else if (MicroPulseWidth < 10000000000) {
    Serial.print(MicroPulseWidth / 1000000, 5);
    Serial.println(" Sec");
  }
  else                                {
    Serial.print(MicroPulseWidth / 1000000, 4);
    Serial.println(" Sec");
  }
}

float tcToFreq( int tc_cntr)
{
  float freq_hz;
  if      (tc_cntr == 0) return 1000;
  if      (FastMode == 3) freq_hz = (420000000.00 / tc_cntr) / NWAVETABLE; // display freq fractions
  else if (FastMode == 2) freq_hz = (168000000.00 / tc_cntr) / NWAVETABLE;
  else if (FastMode == 1) freq_hz = ( 84000000.00 / tc_cntr) / NWAVETABLE;
  else if (FastMode <= 0) freq_hz = ( 42000000.00 / tc_cntr) / NWAVETABLE;
  return freq_hz;
}

int freqToTc(float freq_hz)
{
  int tc_cntr = 0;
  if      (freq_hz == 0) return 25;
  if      (FastMode == 3) tc_cntr = (420000000UL / freq_hz) / NWAVETABLE;
  else if (FastMode == 2) tc_cntr = (168000000UL / freq_hz) / NWAVETABLE;
  else if (FastMode == 1) tc_cntr = ( 84000000UL / freq_hz) / NWAVETABLE;
  else if (FastMode <= 0) tc_cntr = ( 42000000UL / freq_hz) / NWAVETABLE;
  return tc_cntr;
}

void WavePolarity() // ensures the same wave polarity is maintained (relative to square wave)
{
  if (FastMode == 0)
  {
    DACC->DACC_TPR  =  (uint32_t)  Wave0[0];      // DMA buffer
    DACC->DACC_TNPR =  (uint32_t)  Wave0[1];      // next DMA buffer
  }
  else if (FastMode == 1)
  {
    DACC->DACC_TPR  =  (uint32_t)  Wave1[0];      // DMA buffer
    DACC->DACC_TNPR =  (uint32_t)  Wave1[1];      // next DMA buffer
  }
  else if (FastMode == 2)
  {
    DACC->DACC_TPR  =  (uint32_t)  Wave2[0];      // DMA buffer
    DACC->DACC_TNPR =  (uint32_t)  Wave2[1];      // next DMA buffer
  }
  else if (FastMode == 3)
  {
    DACC->DACC_TPR  =  (uint32_t)  Wave3[0];      // DMA buffer
    DACC->DACC_TNPR =  (uint32_t)  Wave3[1];      // next DMA buffer
  }
  DACC->DACC_TCR  =  Duty[0][FastMode];
  DACC->DACC_TNCR =  Duty[1][FastMode];
}

void SetWaveFreq(bool show) // FOR ANALOGUE & SYNCHRONIZED SQUARE WAVE:
{
  //  SET FREQUENCY:
  float dutyLimit = 0;
  float allowedWaveDuty = TargetWaveDuty;
  OldFastMode = FastMode; // old FastMode
  if      (!ExactFreqMode && TargetWaveFreq > 40000) FastMode =  3;
  else if (!ExactFreqMode && TargetWaveFreq > 20000) FastMode =  2;
  else if (!ExactFreqMode && TargetWaveFreq > 10000) FastMode =  1;
  else if (!ExactFreqMode && TargetWaveFreq >  1000) FastMode =  0;
  else // if slow mode (sample rate of 400kHz)
  {
    FastMode = -1;
    if (InterruptMode == 0) FreqIncrement = TargetWaveFreq * 21475;
    else FreqIncrement = TargetWaveFreq * 42950;
    double FreqIncr = FreqIncrement;
    //  Set Duty (if necessary):
    if (TargetWaveDuty > 0 && TargetWaveDuty < 100) dutyLimit = 1 / (4000 / TargetWaveFreq); // calculate percent duty limit of 1 sample at 400kHz (up to 25% at 100kHz)
    if (ExactFreqMode)
    {
      FreqIncr *= ExactFreqModeAccuracy; // accuracy adjustment
    }
    allowedWaveDuty = constrain(TargetWaveDuty, dutyLimit, 100 - dutyLimit);
    if (TargetWavePulseWidth == 0 && allowedWaveDuty == LastAllowedWaveDuty) // if PulseWidth not fixed & duty limit not changed, quick freq/duty calculation is okay:
    {
      if (allowedWaveDuty == 0)
      {
        FreqIncrmt[0] = FreqIncr / 2;
        WaveHalf = 0;
        MinOrMaxWaveDuty = 1;
      }
      else FreqIncrmt[0] = constrain((1.00 / (           allowedWaveDuty  / 50.000)) * FreqIncr, 0 , 4294967295);
      //      else FreqIncrmt[0] = IncrProportion[0] * FreqIncr;
      if (allowedWaveDuty == 100)
      {
        FreqIncrmt[1] = FreqIncr / 2;
        WaveHalf = 1;
        MinOrMaxWaveDuty = 1;
      }
      else FreqIncrmt[1] = constrain((1.00 / ((100.000 - allowedWaveDuty) / 50.000)) * FreqIncr, 0 , 4294967295);
      //     else FreqIncrmt[1] = IncrProportion[1] * FreqIncr;
      Increment[0] = FreqIncrmt[0]; // update quickly after calculating above
      Increment[1] = FreqIncrmt[1]; // update quickly after calculating above
      if (TargetWaveFreq < 0.1) DitherTime = millis() + 100;
    }
    else CalculateWaveDuty(0); // if duty limit changed, freq/duty will need to be fully re-calculated:
  }
  if (FastMode >= 0)               // if in fast mode
  {
    SyncDelay = 0; // remove square wave sync delay for stability in case of sudden change to high freq
    TimerCounts = freqToTc(TargetWaveFreq);
    if (OldFastMode < 0)           // if exiting slow mode
    {
      NVIC_DisableIRQ(TC0_IRQn);
      dac_setup();
      TC_setup();
      CalculateWaveDuty(0);
      CreateNewWave();
    }
    else
    {
      TC_setup();
      if (TargetWavePulseWidth > 0)
      {
        CalculateWaveDuty(0);
        CreateNewWave();
      }
    }
    if (FastMode == OldFastMode) SyncDelay = (TimerCounts - Delay1) * Delay2 * max((Delay3 / (abs(ActualWaveDuty - 50.0) + Delay3)), int(MinOrMaxWaveDuty)); // if fast mode changed calculate later
    else if (FastMode >= 0) ; // do nothing - Very slight delay needed anyway before proceeding
  }
  else // if (FastMode < 0)        // if in slow mode
  {
    TimerCounts = freqToTc(TargetWaveFreq);
    if (OldFastMode  >= 0)         // if exiting fast mode
    {
      dac_setup2();
      TC_setup2();
      NVIC_EnableIRQ(TC0_IRQn);
      CalculateWaveDuty(0);
    }
  }
  //  MEASURE FREQUENCY:
  if (ExactFreqMode) ActualWaveFreq = TargetWaveFreq; // if in ExactFreqMode - absolutely exact only at 50% duty-cycle
  else
  {
    if (TargetWaveFreq > 1000) ActualWaveFreq = tcToFreq(TimerCounts); // if in fast mode
    else                       ActualWaveFreq = 200000 / ceil(200000 / (TargetWaveFreq * 1.0000075)); // if in slow mode - previously 1.0002519
  }                                       //  increase this number if measurement is too low   ^^^

  //  MEASURE DUTY: (if necessary, due to duty limit changing with freq)
  if (FastMode >= 0) // if in fast mode
  {
    if (FastMode != OldFastMode || TargetWaveDuty > 0 || TargetWaveDuty < 100) // duty changes only when fast mode changes or at extreme duty-cycle
    {
      //      if (TargetWaveDuty == 0 || TargetWaveDuty == 100) ActualWaveDuty = TargetWaveDuty;
      if      (TargetWaveDuty == 0)   ActualWaveDuty = 0.0048 / (1000 / ActualWaveFreq);
      else if (TargetWaveDuty == 100) ActualWaveDuty = ((1000 / ActualWaveFreq) - 0.000048) / (10 / ActualWaveFreq);
      else ActualWaveDuty = (100.0 * Duty[0][FastMode]) / (Duty[0][FastMode] + Duty[1][FastMode]); // 1st wave half / (1st wave half + 2nd wave half)
      SyncDelay = (TimerCounts - Delay1) * Delay2 * max((Delay3 / (abs(ActualWaveDuty - 50.0) + Delay3)), int(MinOrMaxWaveDuty));
    }
  }
  else // if in slow mode, duty limit was calculated with freq above
  {
    if      (TargetWaveDuty == 0)   ActualWaveDuty = 0.0048 / (1000 / ActualWaveFreq); ////new6
    else if (TargetWaveDuty == 100) ActualWaveDuty = ((1000 / ActualWaveFreq) - 0.000048) / (10 / ActualWaveFreq);
    else ActualWaveDuty = constrain(TargetWaveDuty, dutyLimit, 100 - dutyLimit);
  }
  if (show)
  {
    Serial.print("   Analogue Wave Freq: ");
    PrintSyncedWaveFreq(); Serial.print(", Target: ");
    Serial.print(TargetWaveFreq, 3);
    Serial.print(" Hz\n   Analogue Wave Period: ");
    PrintSyncedWavePeriod();
    Serial.print("   Analogue Wave Duty-cycle: "); Serial.print(ActualWaveDuty); Serial.println(" %\n");
  }
  LastAllowedWaveDuty = allowedWaveDuty;
}

void CalculateWaveDuty(bool updateSlowMode) // FOR ANALOGUE & SYNCHRONIZED SQUARE WAVE:
{
  float dutyLimit = 0;
  float allowedWaveDuty = TargetWaveDuty;
  if (FastMode >= 0)
  {
    //  SET DUTY:
    int du[2][5];
    if (TargetWaveDuty > 0 && TargetWaveDuty < 100)
    {
      du[0][0] = constrain(round(TargetWaveDuty *  1.6), MinMaxDuty, 160 - MinMaxDuty); // 1st half of cycle at FastMode 0 (160 = SamplesPerCycle)
      du[0][1] = constrain(round(TargetWaveDuty *  0.8), MinMaxDuty,  80 - MinMaxDuty); // 1st half of cycle at FastMode 1
      du[0][2] = constrain(round(TargetWaveDuty *  0.4), MinMaxDuty,  40 - MinMaxDuty); // 1st half of cycle at FastMode 2
      du[0][3] = constrain(round(TargetWaveDuty * 0.16), MinMaxDuty,  16 - MinMaxDuty); // 1st half of cycle at FastMode 3
      du[1][0] = 160 - du[0][0]; // 2nd half of cycle at FastMode 0
      du[1][1] =  80 - du[0][1]; // 2nd half of cycle at FastMode 1
      du[1][2] =  40 - du[0][2]; // 2nd half of cycle at FastMode 2
      du[1][3] =  16 - du[0][3]; // 2nd half of cycle at FastMode 3
      for (byte i = 0; i <= 3; i++)
      {
        Duty[0][i] = du[0][i]; // update quickly after calculating above
        Duty[1][i] = du[1][i]; // update quickly after calculating above
      }
      MinOrMaxWaveDuty = 0;
      //      if (SquareWaveSync && (du[0][FastMode] <= 4 || du[1][FastMode] <= 4)) WavePolarity(); // MinMaxDuty = 4
      if (SquareWaveSync && du[0][FastMode] != du[1][FastMode]) WavePolarity();
    }
    else // if TargetWaveDuty = 0 or 100, display only one wave half:
    {
      if (TargetWaveDuty == 0) WaveHalf = 0; // display 2nd wave half only (neg going)
      else WaveHalf = 1; // display 1st wave half only (pos going)
      Duty[!WaveHalf][0] = 160;
      Duty[!WaveHalf][1] =  80;
      Duty[!WaveHalf][2] =  40;
      Duty[!WaveHalf][3] =  16;
      MinOrMaxWaveDuty = 1;
    }
    //  MEASURE DUTY:
    if      (TargetWaveDuty == 0)   ActualWaveDuty = 0.0048 / (1000 / ActualWaveFreq);
    else if (TargetWaveDuty == 100) ActualWaveDuty = ((1000 / ActualWaveFreq) - 0.000048) / (10 / ActualWaveFreq);
    else ActualWaveDuty = (100.0 * Duty[0][FastMode]) / (Duty[0][FastMode] + Duty[1][FastMode]); // 1st wave half / (1st wave half + 2nd wave half)
    SyncDelay = (TimerCounts - Delay1) * Delay2 * max((Delay3 / (abs(ActualWaveDuty - 50.0) + Delay3)), int(MinOrMaxWaveDuty));
  }
  if (FastMode < 0 || updateSlowMode)
  {
    //  SET DUTY:
    MinOrMaxWaveDuty = 0;
    double FreqIncr = FreqIncrement;
    if (TargetWaveDuty > 0 && TargetWaveDuty < 100) dutyLimit = 1 / (4000 / TargetWaveFreq); // calculate percent duty limit of 1 sample at 400kHz (up to 25% at 100kHz)
    if (ExactFreqMode)
    {
      FreqIncr *= ExactFreqModeAccuracy; // accuracy adjustment
      if (TargetWaveDuty != 50) ExactFreqDutyNot50 = 1;
      else ExactFreqDutyNot50 = 0;
    }
    allowedWaveDuty = constrain(TargetWaveDuty, dutyLimit, 100 - dutyLimit);
    if (allowedWaveDuty == 0)
    {
      FreqIncrmt[0] = FreqIncr / 2;
      WaveHalf = 0;
      MinOrMaxWaveDuty = 1;
    }
    else FreqIncrmt[0] = constrain((1.00 / (           allowedWaveDuty  / 50.000)) * FreqIncr, 0 , 4294967295);
    if (allowedWaveDuty == 100)
    {
      FreqIncrmt[1] = FreqIncr / 2;
      WaveHalf = 1;
      MinOrMaxWaveDuty = 1;
    }
    else FreqIncrmt[1] = constrain((1.00 / ((100.000 - allowedWaveDuty) / 50.000)) * FreqIncr, 0 , 4294967295);
    IncrProportion[0] = FreqIncrmt[0] / FreqIncr;
    IncrProportion[1] = FreqIncrmt[1] / FreqIncr;
    int dutyMulti1    = (IncrProportion[1] / IncrProportion[0]) * 1000;
    DutyMultiplier[0] = (IncrProportion[0] / IncrProportion[1]) * 1000;
    DutyMultiplier[1] = dutyMulti1;    // update quickly after calculating above
    Increment[0] = FreqIncrmt[0]; // update quickly after calculating above
    Increment[1] = FreqIncrmt[1]; // update quickly after calculating above
    if (TargetWaveFreq < 0.1) DitherTime = millis() + 100;
    //  MEASURE DUTY:
    if      (TargetWaveDuty == 0)   ActualWaveDuty = 0.0048 / (1000 / ActualWaveFreq);
    else if (TargetWaveDuty == 100) ActualWaveDuty = ((1000 / ActualWaveFreq) - 0.000048) / (10 / ActualWaveFreq);
    else ActualWaveDuty = constrain(TargetWaveDuty, dutyLimit, 100 - dutyLimit);
  }
}

void SetFreqAndDuty(bool setFreq, bool setDuty) // Sets freq & duty-cycle and calculates measurements - FOR UNSYNCHRONIZED SQUARE WAVE:
{
  if (setFreq)
  {
    if (TargetFreq >= 1300) // Period <= 64615) // >= 1300 Hz
    {
      Period = round(84000000 / TargetFreq);
      if (!SquareWaveSync && PWMfreq == 10) // if viewing unsynch'ed sq.wave and exiting slow mode
      {
        REG_PIOC_PER |= PIO_PER_P28; // PIO takes control of pin 3 from peripheral - similar to pinMode(3, OUTPUT)
        REG_PIOC_ODR |= PIO_ODR_P28; // PIO disables pin 3 (C28) - similar to pinMode(3, INPUT)
        NVIC_DisableIRQ(TC1_IRQn);
        PWMC_EnableChannel(PWM_INTERFACE, g_APinDescription[7].ulPWMChannel);
      }
      PWMfreq = 42000000;
      MicroPeriodMultiplier = 1;
    }
    else if (TargetFreq >= 650) // Period <= 129231) // >= 650 Hz
    {
      Period = round(42000000 / TargetFreq);
      if (!SquareWaveSync && PWMfreq == 10) // if viewing unsynch'ed sq.wave and exiting slow mode
      {
        REG_PIOC_PER |= PIO_PER_P28; // PIO takes control of pin 3 from peripheral - similar to pinMode(3, OUTPUT)
        REG_PIOC_ODR |= PIO_ODR_P28; // PIO disables pin 3 (C28) - similar to pinMode(3, INPUT)
        NVIC_DisableIRQ(TC1_IRQn);
        PWMC_EnableChannel(PWM_INTERFACE, g_APinDescription[7].ulPWMChannel);
      }
      PWMfreq = 10500000;
      MicroPeriodMultiplier = 2;
    }
    else if (TargetFreq >= 325) // Period <= 420000) // >= 325 Hz
    {
      Period = round(21000000 / TargetFreq);
      if (!SquareWaveSync && PWMfreq == 10) // if viewing unsynch'ed sq.wave and exiting slow mode
      {
        REG_PIOC_PER |= PIO_PER_P28; // PIO takes control of pin 3 from peripheral - similar to pinMode(3, OUTPUT)
        REG_PIOC_ODR |= PIO_ODR_P28; // PIO disables pin 3 (C28) - similar to pinMode(3, INPUT)
        NVIC_DisableIRQ(TC1_IRQn);
        PWMC_EnableChannel(PWM_INTERFACE, g_APinDescription[7].ulPWMChannel);
      }
      PWMfreq = 2;
      MicroPeriodMultiplier = 4;
    }
    else if (TargetFreq >= 163) // Period <= 420000) // >= 163 Hz
    {
      Period = round(10500000 / TargetFreq);
      if (!SquareWaveSync && PWMfreq == 10) // if viewing unsynch'ed sq.wave and exiting slow mode
      {
        REG_PIOC_PER |= PIO_PER_P28; // PIO takes control of pin 3 from peripheral - similar to pinMode(3, OUTPUT)
        REG_PIOC_ODR |= PIO_ODR_P28; // PIO disables pin 3 (C28) - similar to pinMode(3, INPUT)
        NVIC_DisableIRQ(TC1_IRQn);
        PWMC_EnableChannel(PWM_INTERFACE, g_APinDescription[7].ulPWMChannel);
      }
      PWMfreq = 4;
      MicroPeriodMultiplier = 8;
    }
    else // if (TargetFreq < 163) // if < 163 Hz, slow mode
    {
      Period = round(200000 / TargetFreq);
      if (!SquareWaveSync && PWMfreq != 10) // if viewing unsynch'ed sq.wave and entering slow mode
      {
        PWMC_DisableChannel(PWM_INTERFACE, g_APinDescription[7].ulPWMChannel);
        pinMode(7, INPUT);
        TC_setup3();
        PIO_Configure(PIOC, PIO_PERIPH_B, PIO_PC28B_TIOA7, PIO_DEFAULT);
      }
      PWMfreq = 10; // not used but identifies mode of operation
      MicroPeriodMultiplier = 420; // 210;
    }
  }
  if (setDuty) Pulse = constrain(round((TargetDuty / 100.0) * Period), 1, Period - 1);
  if (setDuty && !setFreq) ; // if setting duty only skip following freq measurement
  else // if setting freq (or if only measuring. ie: !setDuty && !setFreq)
  {
    MicroPeriod = (float(Period) * 0.0119047619047619) * MicroPeriodMultiplier;
    ActualFreq = 1000000.00 / MicroPeriod;
  }
  if (TargetFreq < 163) // load variables for interupt handler (and calculate MicroPulseWidth)
  {
    if (TargetDuty > 0 && TargetDuty < 100)
    {
      PulsePeriod[0] = Pulse;
      PulsePeriod[1] = Period;
      MinOrMaxDuty = 0;
      MicroPulseWidth = (float(Pulse) * 0.0119047619047619) * MicroPeriodMultiplier;
    }
    else
    {
      PulsePeriod[0] = Period;
      PulsePeriod[1] = Period;
      MinOrMaxDuty = 1;
      if (TargetDuty == 0)
      {
        PeriodHalf = 0;
        MicroPulseWidth = 0.096;
      }
      else
      {
        PeriodHalf = 1;
        MicroPulseWidth = MicroPeriod - 0.12;
      }
    }
  }
  else
  {
    if (!SquareWaveSync) SetPWM(7, Period, Pulse); // if viewing unsynch'ed sq.wave and freq >= 163Hz set up PWM
    MicroPulseWidth = (float(Pulse) * 0.0119047619047619) * MicroPeriodMultiplier;
  }
  ActualDuty = (MicroPulseWidth * 100) / MicroPeriod;
}

void SetPWM(byte pwmPin, uint32_t maxDutyCount, uint32_t duty) // sets up PWM for Unsynchronized Square Wave when at least 163Hz
{
  if (PWMfreq > 4) // if >= 650Hz
  {
    pmc_enable_periph_clk(PWM_INTERFACE_ID);
    PWMC_ConfigureClocks(ClkAFreq, 0, VARIANT_MCK);
    PIO_Configure(g_APinDescription[pwmPin].pPort,
                  g_APinDescription[pwmPin].ulPinType,
                  g_APinDescription[pwmPin].ulPin,
                  g_APinDescription[pwmPin].ulPinConfiguration);
    uint32_t channel = g_APinDescription[pwmPin].ulPWMChannel;
    PWMC_ConfigureChannel(PWM_INTERFACE, channel , PWMfreq, 0, 0);
    PWMC_SetPeriod(PWM_INTERFACE, channel, maxDutyCount);
    PWMC_EnableChannel(PWM_INTERFACE, channel);
    PWMC_SetDutyCycle(PWM_INTERFACE, channel, maxDutyCount - duty);
    //    pmc_mck_set_prescaler(2); // creates 84MHz output but can't change back!
  }
  else
  {
    // Select Instance = PWM; Signal = PWML6 (channel 6); I/O Line = PC23 (C23, Arduino pin 7, see pinout diagram) ; Peripheral = B
    PMC->PMC_PCER1 |= PMC_PCER1_PID36;                      // Enable PWM
    REG_PIOC_ABSR |= PIO_ABSR_P23;                          // Set PWM pin perhipheral type B
    REG_PIOC_PDR |= PIO_PDR_P23;                            // Set PWM pin to an output
    REG_PWM_CLK = PWM_CLK_PREA(0) | PWM_CLK_DIVA(PWMfreq);  // Set the PWM clock rate to 2MHz (84MHz/42) ; Adjust DIVA for the resolution you are looking for
    REG_PWM_CMR6 = PWM_CMR_CALG | PWM_CMR_CPRE_CLKA;       // The period is left aligned, clock source as CLKA on channel 6
    REG_PWM_CPRD6 = maxDutyCount;                         // Channel 6 : Set the PWM frequency
    REG_PWM_CDTY6 = duty;                                // Channel 6: Set the PWM duty cycle
    REG_PWM_ENA = PWM_ENA_CHID6;                        // Enable the PWM channel 6 (see datasheet page 973)
  }
}

void TC1_Handler() // used for Unsynchronized Square Wave when below 163Hz (200,000 clocks per Sec)
{
  // We need to get the status to clear it and allow the interrupt to fire again:
  TC_GetStatus(TC0, 1);
  TimeIncrement += 2; // 100,000 per Sec
  if (TimeIncrement >= PulsePeriod[PeriodHalf]) // if at end of pulse or period
  {
    if (MinOrMaxDuty) // if duty set to 0 or 100
    {
      if (PeriodHalf) // if duty set to 100
      {
        TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR; // set TIOA (sq. wave on pin 3) LOW when triggered (on next line)
        TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // software trigger also resets the counter and restarts the clock
        TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET; // set TIOA (sq. wave on pin 3) HIGH again when triggered (after else statement)
      }
      else // if duty set to 0
      {
        TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET;
        TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // output is set to 1, the counter is reset and the clock is started
        TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR;
      }
      TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // output is reset (creating 96nS pulse), the counter is reset and the clock is started
      TimeIncrement = 0; // if at end of period
    }
    else // if duty not set to 0 or 100
    {
      if (PeriodHalf)
      {
        TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET;
        TimeIncrement = 0; // if at end of period
      }
      else TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR;
      TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG;
      PeriodHalf = !PeriodHalf; // change to other half of period
    }
  }
}

void DACC_Handler(void) // write analogue & synchronized square wave to DAC with DMA - Fast Mode
{
  if      (FastMode == 3) DACC->DACC_TNPR = (uint32_t) Wave3[!WaveHalf]; // if (FastMode == 3) // next DMA buffer
  else if (FastMode == 2) DACC->DACC_TNPR = (uint32_t) Wave2[!WaveHalf]; // if (FastMode == 2) // next DMA buffer
  else if (FastMode == 1) DACC->DACC_TNPR = (uint32_t) Wave1[!WaveHalf]; // if (FastMode == 1) // next DMA buffer
  else if (FastMode == 0) DACC->DACC_TNPR = (uint32_t) Wave0[!WaveHalf]; // if (FastMode == 0) // next DMA buffer
  DACC->DACC_TNCR = Duty[!WaveHalf][max(0, FastMode)]; // number of counts until Handler re-triggered (when next half cycle finished)
  if (MinOrMaxWaveDuty) // if duty set to 0 or 100
  {
    if (SquareWaveSync) // creates squarewave synchronized with triangle or sine wave
    {
      for (int i = 0; i < SyncDelay; i++) {
        while (WaveHalf != WaveHalf); // delays of very short duration. ie: less than 1 microsecond amounts, to sync with analogue wave
      }
      if (WaveHalf)
      {
        TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR; // set TIOA (sq. wave on pin 3) LOW when triggered (on next line)
        TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG; // software trigger also resets the counter and restarts the clock
        TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET; // set TIOA (sq. wave on pin 3) HIGH again when triggered (after else statement)
      }
      else
      {
        TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET;
        TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG; // the counter is reset and the clock is started
        TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR;
      }
      TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // output is reset (creating 48nS pulse), the counter is reset and the clock is started
    }
  }
  else // if duty not set to 0 or 100
  {
    if (SquareWaveSync) // creates squarewave synchronized with triangle or sine wave
    {
      for (int i = 0; i < SyncDelay; i++) {
        while (WaveHalf != WaveHalf); // delays of very short duration. ie: less than 1 microsecond amounts, to sync with analogue wave
      }
      if (WaveHalf) TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR;
      else TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET;
      TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG; // the counter is reset and the clock is started
    }
    WaveHalf = !WaveHalf; // change to other wave half
  }
}

void TC0_Handler() // write analogue & synchronized square wave to DAC & pin 3 - Slow Mode (400,000 clocks per Sec) - (200,000 clocks per Sec if InterruptMode is 1 or 2 - TC_setup2a,b & c) - (20,000 - 28,000 clocks per Sec as set by NoteDivisor if InterruptMode is 3 - TC_setup2c)
{
  TC_GetStatus(TC0, 0);
  WaveBit += Increment[!WaveHalf]; // add appropriate Increment to WaveBit
  if (InterruptMode == 0)
  {
    if (WaveBit < Increment[!WaveHalf]) // if rolled over. If end of wave half (if 4294967295 exceeded - highest number for 32 bit int before roll-over to zero)
    {
      if (ExactFreqDutyNot50) // if in exact freq mode and not at 50% duty-cycle
        WaveBit = (WaveBit / 1000) * DutyMultiplier[WaveHalf]; // if not 50% duty-cycle TRY to maintain freq
      else if (!ExactFreqMode) WaveBit = 1; // if not in exact freq mode reset to 1, allowing next update to be lower at 0 (necessary for very low duty cycle)
      if (MinOrMaxWaveDuty) // if duty set to 0 or 100
      {
        if (!WaveHalf) DACC->DACC_CDR = (int16_t) (WaveFull2[WaveBit >> 20] * WaveAmp >> 16); // if displaying 2nd wave half only (just passed end), write to DAC (1048576 = 4294967296 / 4096)
        else           DACC->DACC_CDR = (int16_t) (WaveFull[WaveBit >> 20] * WaveAmp >> 16); // if displaying 1st wave half only (just passed end), write to DAC
        if (SquareWaveSync)
        {
          if (WaveHalf) // if duty set to 100
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR; // set TIOA (sq. wave on pin 3) LOW when triggered (on next line)
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // software trigger also resets the counter and restarts the clock
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET; // set TIOA (sq. wave on pin 3) HIGH when triggered (after else statement)
          }
          else // if duty set to 0
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET;
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // the counter is reset and the clock is started
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR;
          }
          TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // output is reset (creating 48nS pulse), the counter is reset and the clock is started
        }
  
      }
      else // if duty not set to 0 or 100
      {
        if (WaveHalf)
        {
          DACC->DACC_CDR = (int16_t) (WaveFull2[WaveBit >> 20] * WaveAmp >> 16); // if 2nd wave half (just passed end of 1st half), write to DAC (1048576 = 4294967296 / 4096)
          if (SquareWaveSync)
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR; // set TIOA (sq. wave on pin 3) LOW when triggered (on next line)
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // the counter is reset and the clock is started
          }
        }
        else
        {
          DACC->DACC_CDR = (int16_t) (WaveFull[WaveBit >> 20] * WaveAmp >> 16); // if 1st wave half (just passed end of 2nd half), write to DAC (1048576 = 4294967296 / 4096)
          if (SquareWaveSync)
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET; // set TIOA (sq. wave on pin 3) HIGH when triggered (on next line)
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // the counter is reset and the clock is started
          }
        }
        WaveHalf = !WaveHalf; // change to other wave half
      }
    }
    else // if not end of wave half
    {
      if (!WaveHalf) DACC->DACC_CDR = (int16_t) (WaveFull2[WaveBit >> 20] * WaveAmp >> 16); // if 2nd wave half, write to DAC (1048576 = 4294967296 / 4096)
      else           DACC->DACC_CDR = (int16_t) (WaveFull[WaveBit >> 20] * WaveAmp >> 16); // if 1st wave half, write to DAC (1048576 = 4294967296 / 4096)
    }
  }
  else if (InterruptMode == 10) // constrained only during calculating of WaveShape 3, composite wave, if mirror effect is OFF as constraining earlier causes clipping and incorrect mixing of high amplitude waves. This makes the wave look good during calculating, although it slows the calculating process a little.
  {
    if (WaveBit < Increment[!WaveHalf]) // if rolled over. If end of wave half (if 4294967295 exceeded - highest number for 32 bit int before roll-over to zero)
    {
      if (ExactFreqDutyNot50) // if in exact freq mode and not at 50% duty-cycle
        WaveBit = (WaveBit / 1000) * DutyMultiplier[WaveHalf]; // if not 50% duty-cycle TRY to maintain freq
      else if (!ExactFreqMode) WaveBit = 1; // if not in exact freq mode reset to 1, allowing next update to be lower at 0 (necessary for very low duty cycle)
      if (MinOrMaxWaveDuty) // if duty set to 0 or 100
      {
        if (!WaveHalf) DACC->DACC_CDR = constrain((int16_t) (WaveFull2[WaveBit >> 20] * WaveAmp >> 16), 0, WAVERESOL-1); // if displaying 2nd wave half only (just passed end), write to DAC (1048576 = 4294967296 / 4096)
        else           DACC->DACC_CDR = constrain((int16_t) (WaveFull[WaveBit >> 20] * WaveAmp >> 16), 0, WAVERESOL-1); // if displaying 1st wave half only (just passed end), write to DAC
        if (SquareWaveSync)
        {
          if (WaveHalf) // if duty set to 100
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR; // set TIOA (sq. wave on pin 3) LOW when triggered (on next line)
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // software trigger also resets the counter and restarts the clock
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET; // set TIOA (sq. wave on pin 3) HIGH when triggered (after else statement)
          }
          else // if duty set to 0
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET;
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // the counter is reset and the clock is started
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR;
          }
          TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // output is reset (creating 48nS pulse), the counter is reset and the clock is started
        }
      }
      else // if duty not set to 0 or 100
      {
        if (WaveHalf)
        {
          DACC->DACC_CDR = constrain((int16_t) (WaveFull2[WaveBit >> 20] * WaveAmp >> 16), 0, WAVERESOL-1); // if 2nd wave half (just passed end of 1st half), write to DAC (1048576 = 4294967296 / 4096)
          if (SquareWaveSync)
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR; // set TIOA (sq. wave on pin 3) LOW when triggered (on next line)
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // the counter is reset and the clock is started
          }
        }
        else
        {
          DACC->DACC_CDR = constrain((int16_t) (WaveFull[WaveBit >> 20] * WaveAmp >> 16), 0, WAVERESOL-1); // if 1st wave half (just passed end of 2nd half), write to DAC (1048576 = 4294967296 / 4096)
          if (SquareWaveSync)
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET; // set TIOA (sq. wave on pin 3) HIGH when triggered (on next line)
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // the counter is reset and the clock is started
          }
        }
        WaveHalf = !WaveHalf; // change to other wave half
      }
    }
    else // if not end of wave half
    {
      if (!WaveHalf) DACC->DACC_CDR = constrain((int16_t) (WaveFull2[WaveBit >> 20] * WaveAmp >> 16), 0, WAVERESOL-1); // if 2nd wave half, write to DAC (1048576 = 4294967296 / 4096)
      else           DACC->DACC_CDR = constrain((int16_t) (WaveFull[WaveBit >> 20] * WaveAmp >> 16), 0, WAVERESOL-1); // if 1st wave half, write to DAC (1048576 = 4294967296 / 4096)
    }
  }
  else  if (InterruptMode == 1) // Modulate with Analogue 2 input - constrained 
  {
    if (WaveBit < Increment[!WaveHalf]) // if rolled over. If end of wave half (if 4294967295 exceeded - highest number for 32 bit int before roll-over to zero)
    {
      if (ExactFreqDutyNot50) // if in exact freq mode and not at 50% duty-cycle
        WaveBit = (WaveBit / 1000) * DutyMultiplier[WaveHalf]; // if not 50% duty-cycle TRY to maintain freq
      else if (!ExactFreqMode) WaveBit = 1; // if not in exact freq mode reset to 1, allowing next update to be lower at 0 (necessary for very low duty cycle)
      if (MinOrMaxWaveDuty) // if duty set to 0 or 100
      {
        if (!WaveHalf) DACC->DACC_CDR = constrain(((WaveFull2[WaveBit >> 20] * Modulation) / WAVERESOL) + HALFRESOL, 0, WAVERESOL-1); // if displaying 2nd wave half only (just passed end), write to DAC (1048576 = 4294967296 / 4096)
        else           DACC->DACC_CDR = constrain(((WaveFull[WaveBit >> 20] * Modulation) / WAVERESOL) + HALFRESOL, 0, WAVERESOL-1); // if displaying 1st wave half only (just passed end), write to DAC
        if (SquareWaveSync)
        {
          if (WaveHalf) // if duty set to 100
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR; // set TIOA (sq. wave on pin 3) LOW when triggered (on next line)
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // software trigger also resets the counter and restarts the clock
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET; // set TIOA (sq. wave on pin 3) HIGH when triggered (after else statement)
          }
          else // if duty set to 0
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET;
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // the counter is reset and the clock is started
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR;
          }
          TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // output is reset (creating 48nS pulse), the counter is reset and the clock is started
        }
      }
      else // if duty not set to 0 or 100
      {
        if (WaveHalf)
        {
          DACC->DACC_CDR = constrain(((WaveFull2[WaveBit >> 20] * Modulation) / WAVERESOL) + HALFRESOL, 0, WAVERESOL-1); // if 2nd wave half (just passed end of 1st half), write to DAC (1048576 = 4294967296 / 4096)
          if (SquareWaveSync)
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR; // set TIOA (sq. wave on pin 3) LOW when triggered (on next line)
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // the counter is reset and the clock is started
          }
        }
        else
        {
          DACC->DACC_CDR = constrain(((WaveFull[WaveBit >> 20] * Modulation) / WAVERESOL) + HALFRESOL, 0, WAVERESOL-1); // if 1st wave half (just passed end of 2nd half), write to DAC (1048576 = 4294967296 / 4096)
          if (SquareWaveSync)
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_SET; // set TIOA (sq. wave on pin 3) HIGH when triggered (on next line)
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // the counter is reset and the clock is started
          }
        }
        WaveHalf = !WaveHalf; // change to other wave half
      }
    }
    else // if not end of wave half
    {
      if (!WaveHalf) DACC->DACC_CDR = constrain(((WaveFull2[WaveBit >> 20] * Modulation) / WAVERESOL) + HALFRESOL, 0, WAVERESOL-1); // if 2nd wave half, write to DAC (1048576 = 4294967296 / 4096)
      else           DACC->DACC_CDR = constrain(((WaveFull[WaveBit >> 20] * Modulation) / WAVERESOL) + HALFRESOL, 0, WAVERESOL-1); // if 1st wave half, write to DAC (1048576 = 4294967296 / 4096)
    }
  }
}

void TC2_Handler() // write TRNG noise to analogue DAC pin - clocked at 150 kHz
{
  TC_GetStatus(TC0, 2);
  int16_t newReading  = trng_read_output_data(TRNG);
  int16_t nextReading = TrngNum;
  TrngFast    += ((newReading - TrngFast) * 2 / 10); // fixed high freq filter
  int16_t fastR = newReading - TrngFast;
  nextReading += (newReading - TrngNum) * NoiseCol / 10000; // average
  TrngNum     += (nextReading - TrngNum) * NoiseCol / 10000; // average some more
  if (TrngCount == 3) // clocked at 50kHz
  {
    TrngCount = 0;
    TrngSlo += (newReading - TrngSlo) * NoiseLFC / 100; // low freq filter - average
  }
  else TrngCount++;
  DACC->DACC_CDR = constrain(((((TrngNum / 16) * NoiseFil / 100) + ((TrngSlo / 16) * NoiseLFB / 70) + ((fastR / 16) * NoiseHFB / 1000)) * NoiseAmp / 100) + HALFRESOL, 0, 4095); // reduce from 16 bit to 12 bit and adjust balance and amplitude
}

void TC_setup() // system timer clock set-up for analogue wave & synchronized square wave when in fast mode
{
  pmc_enable_periph_clk(TC_INTERFACE_ID);
  TcChannel * t = &(TC0->TC_CHANNEL)[0];
  t->TC_CCR = TC_CCR_CLKDIS;
  t->TC_IDR = 0xFFFFFFFF;
  t->TC_SR;
  t->TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 |  TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC;
  t->TC_RC = TimerCounts;
  t->TC_RA = TimerCounts / 2;
  t->TC_CMR = (t->TC_CMR & 0xFFF0FFFF) | TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_SET;
  t->TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
}

void TC_setup1() // system timer clock set-up for TRNG Noise
{
  pmc_set_writeprotect(false);     // disable write protection for pmc registers
  pmc_enable_periph_clk(ID_TC2);   // enable peripheral clock TC0
  // we want wavesel 01 with RC:
  TC_Configure(/* clock */TC0,/* channel */2, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1); // select 42 MHz clock
  TC_SetRC(TC0, 2, 280); // select divisor of 280 (42 MHz / 280) - clocks DAC at 150kHz
  TC_Start(TC0, 2);
  TC0->TC_CHANNEL[2].TC_IER = TC_IER_CPCS; // IER = interrupt enable register
  TC0->TC_CHANNEL[2].TC_IDR = ~TC_IER_CPCS; // IDR = interrupt disable register
  NVIC_EnableIRQ(TC2_IRQn);
}

void TC_setup2() // system timer clock set-up for analogue wave & triggering synchronized square wave when in slow mode
{
  pmc_set_writeprotect(false);     // disable write protection for pmc registers
  pmc_enable_periph_clk(ID_TC0);   // enable peripheral clock TC0
  // we want wavesel 01 with RC:
  TC_Configure(/* clock */TC0,/* channel */0, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1); // select 42 MHz clock
  TC_SetRC(TC0, 0, 105); // select divisor of 105 (42 MHz / 105) - clocks DAC at 400kHz
  TC_Start(TC0, 0);
  TC0->TC_CHANNEL[0].TC_IER = TC_IER_CPCS; // IER = interrupt enable register
  TC0->TC_CHANNEL[0].TC_IDR = ~TC_IER_CPCS; // IDR = interrupt disable register
  if (!StartupTune) NVIC_EnableIRQ(TC0_IRQn);
}

void TC_setup2a() // system timer clock set-up for analogue wave & triggering synchronized square wave when in slow mode - if just started or TargetFreq >= 163Hz
{
  pmc_set_writeprotect(false);     // disable write protection for pmc registers
  pmc_enable_periph_clk(ID_TC0);   // enable peripheral clock TC0
  // we want wavesel 01 with RC:
  TC_Configure(/* clock */TC0,/* channel */0, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1); // select 42 MHz clock
  TC_SetRC(TC0, 0, 210); // select divisor of 210 (42 MHz / 210) - clocks DAC at 200kHz
  TC_Start(TC0, 0);
  TC0->TC_CHANNEL[0].TC_IER = TC_IER_CPCS; // IER = interrupt enable register
  TC0->TC_CHANNEL[0].TC_IDR = ~TC_IER_CPCS; // IDR = interrupt disable register
  if (!StartupTune) NVIC_EnableIRQ(TC0_IRQn);
}

void TC_setup2b() // system timer clock set-up for analogue wave & triggering synchronized square wave when in slow mode - if not just started & TargetFreq < 163Hz
{
  pmc_set_writeprotect(false);     // disable write protection for pmc registers
  pmc_enable_periph_clk(ID_TC0);   // enable peripheral clock TC0
  // we want wavesel 01 with RC:
  TC_Configure(/* clock */TC0,/* channel */0, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1); // select 42 MHz clock
  TC_SetRC(TC0, 0, 420); // select divisor of 420 (42 MHz / 420) - clocks DAC at 100kHz
  TC_Start(TC0, 0);
  TC0->TC_CHANNEL[0].TC_IER = TC_IER_CPCS; // IER = interrupt enable register
  TC0->TC_CHANNEL[0].TC_IDR = ~TC_IER_CPCS; // IDR = interrupt disable register
  if (!StartupTune) NVIC_EnableIRQ(TC0_IRQn);
}

void TC_setup3() // system timer clock set-up for Unsynchronized Square Wave when below 163Hz (PWM used above 163Hz)
{
  //  Serial.println("Entering < 163Hz mode");
  pmc_set_writeprotect(false);     // disable write protection for pmc registers
  pmc_enable_periph_clk(ID_TC1);   // enable peripheral clock TC1 (TC0, channel 1)
  // we want wavesel 01 with RC:
  TC_Configure(/* clock */TC0,/* channel */1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1); // select 42 MHz clock
  TC_SetRC(TC0, 1, 420); // select divisor of 420 (42 MHz / 420) - clocks at 100kHz
  TC_Start(TC0, 1);
  TC0->TC_CHANNEL[1].TC_IER = TC_IER_CPCS; // IER = interrupt enable register
  TC0->TC_CHANNEL[1].TC_IDR = ~TC_IER_CPCS; // IDR = interrupt disable register
  // Enable the interrupt in the nested vector interrupt controller
  // TC1_IRQn where 1 is the timer number * timer channels (3) + the channel number (=(0*3)+1) for timer 0 channel 1:
  NVIC_EnableIRQ(TC1_IRQn);
}

void TC_setup4() // timer clock set-up for synchronized square wave
{
  pmc_enable_periph_clk (TC_INTERFACE_ID + 2 * 3 + 1); // clock the TC2 channel 1. for pin 3
  // pmc_enable_periph_clk (TC_INTERFACE_ID + 2*3+2); // clock the TC2 channel 2. for pin 11 instead
  TcChannel * t = &(TC2->TC_CHANNEL)[1];             // pointer to TC2 registers for its channel 1. for pin 3
  // TcChannel * t = &(TC2->TC_CHANNEL)[2];         // pointer to TC2 registers for its channel 2. for pin 11 instead
  t->TC_CCR = TC_CCR_CLKDIS;                       // disable internal clocking while setup regs
  t->TC_IDR = 0xFFFFFFFF;                         // disable interrupts
  t->TC_SR;                                      // read int status reg to clear pending
  t->TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 |      // use TCLK1 (prescale by 2, = 42MHz)
              TC_CMR_WAVE |                    // waveform mode
              TC_CMR_WAVSEL_UP |              // count-up without auto RC triggering
              TC_CMR_ASWTRG_SET |            // Software Trigger effect on TIOA
              TC_CMR_CPCTRG |               // enable manual RC triggering
              TC_CMR_ACPA_NONE |           // RA effect on TIOA
              TC_CMR_ACPC_NONE;           // RC effect on TIOA
  TC_Start(TC2, 1);
}

void TC_setup5() // timer clock set-up for timer - when operated from the serial monitor, & low freq info display also on GUI
{
  pmc_set_writeprotect(false);     // disable write protection for pmc registers
  pmc_enable_periph_clk(ID_TC4);   // enable peripheral clock TC1, channel 1
  TC_Configure(TC1, 1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK4); // select 656.250 kHz clock
  TC_SetRC(TC1, 1, 39375000); // select divisor of 39375000 (656250Hz / 39375000) - resets at 1 minute
  TC_Start(TC1, 1);
}



//void TC_setup7() // timer clock set-up for voice 2 pitch // not used
//{
//  pmc_set_writeprotect(false);     // disable write protection for pmc registers
//  pmc_enable_periph_clk(ID_TC3);   // enable peripheral clock TC1, channel 0
//  TC_Configure(/* clock */TC1,/* channel */0, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1); // select 42 MHz clock
//  TC_SetRC(TC1, 0, 1905); // select divisor of 1905 (42 MHz / 1905) - clocks DAC at 22047kHz (~half of 44100)
//  TC_Start(TC1, 0);
//  TC1->TC_CHANNEL[0].TC_IER = TC_IER_CPCS; // IER = interrupt enable register
//  TC1->TC_CHANNEL[0].TC_IDR = ~TC_IER_CPCS; // IDR = interrupt disable register
//  // Enable the interrupt in the nested vector interrupt controller
//  // TC3_IRQn where 3 is the timer number * timer channels (3) + the channel number (=(1*3)+0) for timer 1 channel 0:
//  NVIC_EnableIRQ(TC3_IRQn);
//}


void NoiseFilterSetup()
{
  NoiseCol = constrain(NoiseColour * 10, 0, 10000); // setup for TC2_Handler: (NoiseColour for white is 1000, pink is 500 & brown is 30)
  if      (NoiseCol > 7500) NoiseCol = map(NoiseCol, 10000, 7500, 10000, 4500); // Main Low Pass linearity - NoiseCol is 10000 when colour input NoiseCol is full 10000 (white), then deceases to 4500 as colour input reduces to 7500
  else if (NoiseCol > 5000) NoiseCol = map(NoiseCol, 7500, 5000, 4500, 3500);
  else                      NoiseCol = map(NoiseCol, 5000, 0, 3500, 0);         // Main Low Pass linearity - NoiseCol is 0 when colour input NoiseCol is 0 (< brown), then inceases to 4000 as colour input increases to 5000
  if      (NoiseCol > 6000) NoiseLFB = map(NoiseCol, 10000, 6000, 0, 10);      // Lo Freq boost - amplitude NoiseLFB is min 0 when colour input NoiseCol is full 10000 (white), then inceases to 10 as colour input reduces to 6000
  else if (NoiseCol > 4000) NoiseLFB = map(NoiseCol, 6000, 4000, 10, 150);
  else if (NoiseCol > 1500) NoiseLFB = map(NoiseCol, 4000, 1500, 150, 250);
  else if (NoiseCol >  150) NoiseLFB = map(NoiseCol, 1500, 150, 250, 0);       // Lo Freq boost - amplitude NoiseLFB is min 0 when colour input NoiseCol is below 150 (~ brown), then inceases to 250 as colour input increases to 1500
  else                      NoiseLFB = 0;
  NoiseLFC  = constrain(map(NoiseCol, 5500, 3800, 15, 2), 2, 15);              // Lo Freq boost - cut-off freq NoiseLFC is max 15 when colour input NoiseCol is full 10000 (white), then decreases to 2 just before colour input reduces to 3800 (just above pink at 3500 - with user input of 500 before multiplying to 5000)
  if      (NoiseCol > 6000) NoiseHFB = map(NoiseCol, 10000, 6000, 0, 350);     // Hi Freq boost - amplitude NoiseHFB is min 0 when colour input NoiseCol is full 10000 (white), then inceases to 350 as colour input reduces to 6000
  else if (NoiseCol > 4000) NoiseHFB = map(NoiseCol, 6000, 4000, 350, 250);
  else if (NoiseCol > 2000) NoiseHFB = map(NoiseCol, 4000, 2000, 250, 80);
  else if (NoiseCol >  150) NoiseHFB = map(NoiseCol, 4000, 150, 80, 0);        // Hi Freq boost - amplitude NoiseHFB is min 0 when colour input NoiseCol is below 150 (~ brown), then inceases to 80 as colour input increases to 4000
  else                      NoiseHFB = 0;
  if      (NoiseCol > 6500) NoiseFil = map(NoiseCol, 10000, 6500, 100, 66);    // Main Low Pass - amplitude NoiseFil is max 100 when colour input NoiseCol is full 10000 (white), then decreases to 66 as colour input reduces to 6500
  else if (NoiseCol > 4000) NoiseFil = map(NoiseCol, 6500, 4000, 66, 75);
  else if (NoiseCol > 1500) NoiseFil = map(NoiseCol, 4000, 1500, 75, 120);
  else if (NoiseCol >  700) NoiseFil = map(NoiseCol, 1500, 700, 120, 500);
  else if (NoiseCol >  300) NoiseFil = map(NoiseCol, 700, 300, 500, 1500);
  else                      NoiseFil = map(NoiseCol, 300, 0, 1500, 4000);      // Main Low Pass - amplitude NoiseFil is 4000 when colour input NoiseCol is 0 (sub-brown), then deceases to 1500 as colour input increases to 300
  if (WaveShape == 4)
  {
    Serial.print("   Noise Colour is "); Serial.print(NoiseColour);
    if      (NoiseColour >= 820) Serial.println(" - White\n");
    else if (NoiseColour >= 480 && NoiseColour <= 520) Serial.println(" - Pink\n");
    else if (NoiseColour >= 20 && NoiseColour <= 40) Serial.println(" - Brown\n");
    else Serial.println("\n");
  }
}

void dac_setup() // DAC set-up for analogue wave & synchronized square wave when in fast mode and using DMA (above 1kHz and Exact Freq Mode off)
{
  pmc_enable_periph_clk (DACC_INTERFACE_ID);   // start clocking DAC
  dacc_reset(DACC);
  dacc_set_transfer_mode(DACC, 0);
  dacc_set_power_save(DACC, 0, 1);              // sleep = 0, fast wakeup = 1
  dacc_set_analog_control(DACC, DACC_ACR_IBCTLCH0(0x02) | DACC_ACR_IBCTLCH1(0x02) | DACC_ACR_IBCTLDACCORE(0x01));
  dacc_set_trigger(DACC, 1);
  dacc_set_channel_selection(DACC, 0); // un-comment these 2 lines to enable DAC0 - also see dac_setup2() below
  dacc_enable_channel(DACC, 0);        // un-comment these 2 lines to enable DAC0
//  dacc_set_channel_selection(DACC, 1); // un-comment these 2 lines to enable DAC1
//  dacc_enable_channel(DACC, 1);        // un-comment these 2 lines to enable DAC1
  NVIC_DisableIRQ(DACC_IRQn);
  NVIC_ClearPendingIRQ(DACC_IRQn);
  NVIC_EnableIRQ(DACC_IRQn);
  dacc_enable_interrupt(DACC, DACC_IER_ENDTX);
  DACC->DACC_TPR  = (uint32_t) Wave0[0];     // DMA buffer
  DACC->DACC_TCR  = NWAVETABLE / 2;
  DACC->DACC_TNPR = (uint32_t) Wave0[1];     // next DMA buffer
  DACC->DACC_TNCR = NWAVETABLE / 2;
  DACC->DACC_PTCR = 0x00000100;
}

void dac_setup2() // DAC set-up for analogue & synchronized square wave when in slow mode (below 1kHz or Exact Freq Mode on at any freq)
{
  NVIC_DisableIRQ(DACC_IRQn);
  NVIC_ClearPendingIRQ(DACC_IRQn);
  dacc_disable_interrupt(DACC, DACC_IER_ENDTX); // disable DMA
  DACC->DACC_CR = DACC_CR_SWRST ;               // reset DAC
  dacc_set_channel_selection(DACC, 0);          // un-comment these 2 lines to enable DAC0 - also see dac_setup() above
  dacc_enable_channel(DACC, 0);                 // un-comment these 2 lines to enable DAC0
//  dacc_set_channel_selection(DACC, 1);          // un-comment these 2 lines to enable DAC1
//  dacc_enable_channel(DACC, 1);                 // un-comment these 2 lines to enable DAC1
}