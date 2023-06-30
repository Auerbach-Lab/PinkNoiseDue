#include <Arduino.h>
#include <DueFlashStorage.h>
#include <efc.h>
#include <flash_efc.h>

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
byte     MusicNotes[1100]; // stream of bytes received from GUI and played / stored in flash memory
uint16_t NotesCount;      // number of bytes in MusicNotes[1100] above
byte     Legato;         // 1 = 1st slurred note, 2 = following slurred notes, 3 = last slurred note
bool     Rest;          // rest length determined by NoteLength
byte     Tempo = 105;  // 15 will be added, representing a tempo of 15 to 270
uint16_t TempoCount;  // used by interrupt handler for timing of tempo, etc
bool     FadeOut = 0; // prevents note from ending abruptly (between notes) creating a click when NoteDivisor & NoteFreqBand set at atart of next note
bool     SlurVioFade = 0; // if end of slurred violin note, fade modulation by 50% to match quiet start of next note
bool     SlurFilter = 0;    // 1 = apply low pass filtering at end & beginning of adjacent slurred notes
int      SlurStrength = 10000; // strength of low pass filtering currently applied to slurred notes. 10000 = no filtering - increases with filtering strength
int      SlurMax = 0;      // max strength of low pass filtering
uint16_t  SlurIncr = 0;   //  incremental change of slur filter strength
byte     CresEndVel = 0; // crescendo end Velocity
uint16_t EnvelopeDivisor = 2100; // clocks envelope interrupt handler (TimingCount below) at 20kHz for envelope attack to smooth transition from one note to the next, then at 1kHz
uint16_t TimingCount; // timing for music note envelope, etc.
byte     Velocity = 50; // 0 = min velocity, 100 = max velocity or loudness
byte     Instrument = 0; // 0 = Wave, Other instruments stored in flash program memory. 1 = Piano, 2 = Guitar, 3 = Marimba, 4 = Trumpet, 5 = Saxophone, 6 = Violin
byte     NoteFreqBand = 4; // freq band of mini-soundfont instruments stored in flash program memory. 0 = lowest, 9 = highest
uint16_t StartBegin; // = pgm_read_word_near(&MsfConfig[Instrument -1][NoteFreqBand][0]); // read start chunk length / startCycle begin
uint16_t CycleLen; //   = pgm_read_word_near(&MsfConfig[Instrument -1][NoteFreqBand][1]); // read cycle length
uint16_t MidBegin; //   = pgm_read_word_near(&MsfConfig[Instrument -1][NoteFreqBand][2]); // midCycle begin
uint16_t EndBegin; //   = pgm_read_word_near(&MsfConfig[Instrument -1][NoteFreqBand][3]); // endCycle begin
bool     SkipMid = 0; // true if skipping mid cycle, mix from start cycle straight to end cycle (used with marimba)
bool     Vibrato = 0; // used for trumpet sax & violin
uint16_t NoteLength = 24; // time between notes // 96 = whole note, 48 = 1/2 note, 24 = 1/4 note, 12 = 1/8 note, 6 = 1/16 note
uint16_t NotePlayLen = 24; // play time of current note. Shorter if staccato // equals NoteLength if Legato > 0 or if not staccato
byte     MusicByt = 60; // // 60 = C4 note // smallest byte of music data - read sequentially
uint16_t NoteDivisor = 2112; // clocks note interrupt handler at frequency of required note - used with mini-soundfonts
volatile boolean NoteDivSync = 0; // ensures NoteDivisor resets sample rate at the right time during a slide to prevent clicking effect
int16_t  NoteFilter = 0; // lower velocity notes filtered
uint16_t NoteReadTotal = 0; // total samples in current note
uint16_t NoteReadCycle = 0; // current sample in single cycle of note
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
// SAVE MINI-SOUNDFONTS TO FLASH PROGRAM MEMORY:
// save configuration data for mini-soundfonts into a multi-dimensional array:
const PROGMEM uint16_t MsfConfig[][10][4] = // [instrument] [note freq band] [config data] (max total of 65535 for 1st + last 2 config data)
{// {  band 0 C2 - F2   },{  band 1 F#2 - B2  },{  band 2 C3 - F3   },{  band 3 F#3 - B3  },{  band 4 C4 - F4  },{ band 5 F#4 - B4  },{  band 6 C5 - F5  },{ band 7 F#5 - B5 },{ band 8 C6 - F6  },{ band 9 F#6 - B6   }  // each band = {number of samples in start chunk, number of samples in following cycles, number of samples from startCycle beginning to midCycle beginning (should be a multiple of the number of samples per cycle), number of samples from midCycle beginning to endCycle beginning (should be a multiple of the number of samples per cycle)}
  { {872,304,28272,49856},{672,215,20640,51600},{830,152,11856,48336},{591,107,10058,54784},{728,76,10032,27360},{1060,54,9990,24300},{1314,38,7866,21014},{900,27,7857,21006},{625,19,1995,16986},{428,13,1989,15990} }, // instrument 0 = Piano
  { {1931,215,8170,50095},{1931,215,8170,50095},{1366,152,7904,48336},{1145,107,6099,54784},{1038,76,4864,50616},{ 709,54,4536,39960},{ 499,38,4484,21014},{355,27,4293,23949},{250,19,4009,22021},{428,13,1989,15990} }, // instrument 1 = Guitar
  { {3528,215,4300, 2150},{3528,215,4300, 2150},{2348,152,3800, 1520},{1759,107,3745, 1070},{1478,76,3800,  760},{1050,54,3780,  540},{1006,38,3800,  380},{715,27,3780,  270},{656,19,3800,  190},{450,13,3796,  130} }, // instrument 2 = Marimba
  { {1234,304,2432,16720},{1697,215,3870,22145},{1208,152,1064,21888},{ 847,107,1070,22042},{ 600,76,1064,22040},{ 426,54,1026,22680},{ 300,38,1026,22002},{217,27,1026,22005},{152,19,1007,22021},{428,13,1989,15990} }, // instrument 3 = Trumpet
  { {1406,304,1520,22192},{770, 215, 860,22145},{698, 152, 912,21888},{ 630,107, 749,22042},{ 501,76, 684,22040},{ 417,54, 432,22680},{ 407,38, 418,22002},{321,27, 216,22005},{139,19, 209,22021},{428,13,1989,15990} }, // instrument 4 = Sax
  { {1406,304,1520,22192},{770, 215, 860,22145},{698, 152, 912,21888},{1890,107,2354,30174},{1343,76,2204,33972},{ 988,54,2268,25110},{ 680,38,2204,23104},{482,27,2214,22005},{340,19,2185,22021},{233,13,2184,21996} }  // instrument 5 = Violin - last instrument has no comma after final bracket
};
// the following arrays have a range from 0 to 254 max, with 127 representing zero (as created by the accompanying excel worksheet)
// save music-note start chuncks: (each chunk has a different number of samples, so can't be saved efficiently into a 2D or 3D array)
//**************** Piano C2 to F2: (Band 0)
const PROGMEM uint8_t PianoC2StartChunk872[] = // recorded at low sample rate (no high sample rate for C2)
{127,126,126,127,128,128,129,129,129,129,129,129,129,129,129,129,128,128,128,128,128,128,128,128,127,127,128,128,126,125,127,126,125,125,126,128,128,126,125,124,122,121,121,122,124,124,125,127,126,123,122,124,127,129,128,126,129,135,140,139,135,132,133,136,139,140,137,132,125,119,115,114,116,116,117,115,114,118,129,142,152,154,151,147,141,135,131,131,134,133,126,116,105,98,97,99,103,109,115,119,124,129,137,141,140,135,130,125,123,126,128,124,116,111,108,107,111,115,115,112,108,105,107,116,126,131,134,142,154,165,167,162,159,160,163,163,159,149, // [Piano] [C2 start chunk] [872 samples] 0dB
134,118,106,98,98,104,106,103,103,112,123,130,134,139,147,153,156,156,150,143,139,136,134,135,130,119,110,108,112,108,94,79,77,90,114,135,148,157,163,170,180,194,208,216,211,193,174,162,152,141,124,102,83,74,77,85,94,105,123,144,159,160,149,141,152,170,173,149,108,66,41,29,23,24,33,38,31,20,16,20,28,39,49,53,56,66,86,109,126,128,115,102,98,99,102,104,105,106,105,96,79,71,81,103,124,137,146,152,157,157,155,159,171,181,179,172,171,178,187,189,180,165,152,146,147,151,154,154,147,139,134,130,127,124,123,128,143,155,160,167,179,192,199,201,199,192,
183,178,174,170,163,152,134,118,115,122,131,131,123,116,116,120,124,131,146,167,185,196,201,201,200,205,213,218,221,220,213,202,189,176,166,162,156,147,141,144,151,156,156,156,157,158,162,164,171,187,199,199,192,180,165,155,150,141,128,115,103,95,90,89,88,84,82,80,77,75,75,76,80,87,90,88,86,87,88,93,98,97,87,74,60,48,52,71,90,98,101,106,112,117,116,105,88,69,55,47,43,41,43,50,60,61,53,43,38,37,42,53,65,74,78,83,96,112,126,129,122,113,107,103,100,100,102,106,111,118,123,126,126,123,125,134,144,143,135,134,145,161,168,160,143,128,115,98,82,72,68,
71,80,88,93,96,97,96,98,109,121,129,130,123,111,101,107,122,131,123,99,75,65,70,75,74,77,87,99,108,112,110,112,123,133,132,124,117,115,123,140,152,154,152,149,142,136,139,147,158,165,158,140,126,125,138,158,174,175,168,168,179,190,201,216,233,244,251,253,251,247,239,225,204,185,170,155,143,135,126,119,116,109,99,95,98,106,117,127,129,125,123,130,145,158,162,157,149,146,147,143,136,127,117,110,114,129,150,170,181,180,175,173,175,178,180,182,181,179,175,169,162,161,170,183,195,198,190,179,176,179,182,183,181,173,156,139,130,132,138,137,125,111,
104,103,105,116,135,154,166,170,161,147,141,143,151,161,170,170,159,146,139,135,133,130,119,100,83,72,74,91,110,121,126,133,143,154,164,170,170,172,177,175,167,162,163,161,159,151,134,111,89,73,65,67,69,67,64,67,74,84,99,116,122,120,115,116,125,139,144,135,123,113,100,84,66,48,31,20,14,9,5,8,19,32,48,61,66,67,68,74,81,89,97,99,91,80,78,84,94,100,97,90,89,96,102,103,99,90,84,81,76,68,61,59,61,66,67,64,58,55,60,64,64,59,53,48,48,51,57,70,87,95,90,83,85,97,107,105,94,92,105,122,128,122,113,108,119,140,157,160,155,150,149,154,162,164,158,150,144,
139,133,126,124,130,141,147,148,151,155,153,149,146,147,150,151,143,130,125,134,145,146,132,114,104,109,124,141,152,156,156,159,168,175,176,173,169,161,150,138,133,134,144,160,174,186,196,203,204,202,196,190,184,175,168,168,175,183,181,168,149,133,123,125,138,156,172,186,200,214,227,239,245,247,241,229,215,203,197,189,177,160,143,128,117,112,110,108,105,96,84,75,79,97,123,149,171,185,189,180,167,161,167,175,175,167,159,155,156,162,166,167,166,161,153,143,137,137,139,140,139,133,124,115,113,122,143,167,184,189,183,172,164,166,176,183,172,147};
const PROGMEM uint8_t PianoF2StartChunk872[] = // recorded at high sample rate
{142,155,160,155,142,124,111,108,113,117,121,126,131,130,120,105,96,100,116,139,159,168,166,156,142,131,122,117,120,131,148,165,178,186,188,185,179,175,178,188,200,204,199,185,163,137,110,90,82,88,108,129,138,132,119,105,94,91,97,110,122,124,117,107,96,90,86,90,101,118,130,135,130,119,104,90,81,81,88,100,111,115,112,106,95,85,78,78,85,97,105,107,101,92,86,88,95,107,126,148,168,177,171,156,139,127,121,120,122,127,135,140,135,122,105,92,89,97,112,128,137,135,124,110,97,91,91,97,104,110,111,104,90,75,64,62,67,75,82,82,74,62,51,40,28,22,25,40,57, // [Piano] [F#2 start chunk] [872 samples] 0dB (start at 537 in file)
71,79,83,86,89,95,105,117,130,143,153,160,165,168,166,162,162,168,177,182,181,176,172,170,168,167,167,169,174,180,182,179,172,164,157,154,156,161,171,180,186,187,182,173,166,166,173,184,194,197,192,180,168,161,158,159,163,170,178,183,181,172,158,144,135,137,150,169,184,191,185,171,155,138,124,114,114,124,141,158,167,164,150,132,117,113,120,129,137,144,147,145,139,133,128,127,129,135,143,146,143,134,121,111,110,120,137,152,160,163,160,153,145,134,127,131,143,159,169,171,165,155,141,129,123,126,136,149,159,161,157,148,139,136,139,150,166,181,
186,177,157,134,112,95,86,90,103,120,136,145,147,141,134,128,128,131,138,147,155,162,163,153,132,107,89,84,94,112,129,135,127,106,82,65,60,67,83,102,117,122,112,91,70,58,63,83,111,139,159,169,167,159,149,138,125,115,110,111,113,107,89,63,40,28,28,36,47,56,63,63,55,39,20,9,9,22,41,58,71,76,72,62,49,38,35,38,44,51,59,61,55,43,31,29,39,56,75,88,95,94,83,67,54,52,68,97,128,153,167,171,169,165,162,163,167,173,177,179,176,169,157,142,131,128,135,144,150,151,150,149,148,146,143,141,144,154,168,175,173,163,151,139,130,123,119,120,123,124,122,114,99,
79,60,45,37,36,39,43,45,46,45,42,38,38,45,57,73,88,97,99,97,100,107,116,124,130,135,138,136,130,123,119,118,119,122,130,140,146,146,143,143,148,158,170,177,181,184,189,192,193,193,198,206,213,218,221,222,222,218,212,208,211,223,237,247,250,247,239,228,214,200,191,193,205,221,233,238,239,237,231,222,214,213,215,216,214,210,205,202,198,193,188,183,178,171,165,158,154,153,155,158,164,172,179,184,183,176,164,149,139,137,143,151,157,158,156,151,143,137,135,137,142,147,151,150,145,136,128,126,130,139,146,147,143,140,136,131,124,117,110,106,110,124,
146,168,180,177,162,143,126,116,113,113,117,121,120,108,88,70,63,73,94,115,129,135,135,130,120,111,107,111,119,127,133,135,130,116,95,76,67,68,77,88,97,103,102,92,74,53,40,39,45,53,60,66,72,77,80,85,96,111,130,147,156,156,148,133,112,92,78,74,77,82,88,90,84,69,48,28,17,21,35,51,58,55,47,40,38,41,48,59,69,75,73,62,46,29,18,15,22,34,48,59,67,68,65,57,47,38,37,40,46,51,56,59,58,58,63,70,78,87,98,111,120,125,125,121,115,111,109,108,108,110,114,115,113,109,106,105,106,111,120,133,144,146,140,131,124,123,125,129,133,138,143,147,147,143,136,130,125,
122,120,119,118,115,110,102,96,95,97,96,92,86,84,84,86,88,92,100,109,120,131,138,143,146,149,152,153,154,154,154,154,155,157,158,157,158,164,171,175,178,180,178,172,164,158,157,161,168,175,180,182,181,174,166,159,161,171,185,198,211,222,230,233,232,227,221,214,209,210,216,222,227,230,232,234,238,242,243,243,242,240,237,232,229,229,234,242,250,254,253,244,229,213,202,200,206,211,209,201,193,191,192,191,188,185,184,181,178,175,171,170,170,168,163,159,159,165,173,182,187,186,174,155,139,133,140,155,169,174,169,157,139};

//**************** Piano F#2 to B2: (Band 1)
const PROGMEM uint8_t PianoF2StartChunk672[] = // recorded at low sample rate
{131,136,136,135,138,135,127,128,140,144,137,132,130,131,136,141,137,129,125,119,112,116,130,133,127,121,115,114,126,141,135,117,108,110,113,118,124,124,120,115,108,110,126,140,133,113,102,107,119,129,138,138,126,112,110,127,148,160,153,130,110,110,116,122,130,129,111,96,105,135,162,168,155,137,123,117,127,150,173,186,188,181,175,184,201,204,187,156,119,88,83,106,134,135,116,98,91,102,120,123,111,96,87,89,106,128,135,125,105,85,80,90,107,115,110,97,82,77,86,102,107,98,87,89,102,126,157,177,168,144,126,120,121, // [Piano] [F#2 start chunk] [672 samples] 0dB (start at 321 in file)
129,139,134,112,92,91,109,131,137,123,104,91,92,102,111,108,91,70,62,69,80,81,69,53,36,22,26,48,70,80,85,89,100,117,136,151,162,168,166,162,168,180,182,175,171,168,167,169,176,182,179,169,158,154,159,172,184,187,180,167,167,180,194,197,183,166,159,159,166,178,184,175,156,138,137,158,182,191,177,154,131,115,116,136,160,167,151,125,113,122,134,144,147,140,131,127,130,139,146,141,124,110,115,137,156,163,159,148,134,127,142,163,171,164,147,130,123,133,152,161,157,143,136,141,161,182,184,159,126,98,86,97,121,141,
148,139,130,128,134,146,158,163,151,118,89,86,108,131,131,105,72,59,71,97,119,118,92,64,60,88,129,160,170,161,146,129,114,110,113,102,69,37,26,36,50,62,62,45,19,7,19,46,68,75,67,49,36,36,45,56,61,50,32,30,50,76,92,94,78,56,53,84,128,160,171,168,163,164,170,177,179,172,155,135,129,139,149,151,149,148,145,141,147,164,175,169,151,136,124,119,121,124,120,102,74,49,36,37,43,45,45,40,37,44,63,85,98,97,100,112,123,132,138,135,126,119,118,121,132,145,146,143,147,162,176,181,187,193,193,197,208,217,222,223,220,211,209,
225,243,250,246,232,213,194,193,212,231,239,239,231,218,213,216,215,209,204,198,192,185,177,168,159,153,155,160,169,180,184,177,159,141,138,148,157,158,152,141,135,137,144,151,149,139,128,127,139,147,144,139,134,124,114,106,113,139,170,180,163,137,118,113,115,121,117,92,67,68,95,123,135,134,123,110,108,119,130,135,127,103,75,66,75,91,101,102,83,53,37,43,54,63,72,78,84,99,124,148,157,149,126,96,77,75,82,89,86,64,34,16,26,49,57,49,39,38,48,62,74,71,52,28,15,20,37,56,67,67,58,43,36,41,49,56,58,58,64,75,87,104,120,
125,122,114,110,108,109,113,115,111,106,105,110,124,141,146,135,124,123,128,134,141,147,146,137,128,122,120,119,115,107,97,95,97,92,85,84,86,90,100,114,129,140,145,150,153,154,154,154,156,158,157,161,170,176,180,178,168,158,158,166,176,182,181,171,160,163,181,199,217,230,234,229,220,211,211,219,226,231,233,238,243,244,242,239,232,229,234,245,254,253,236,213,200,205,212,204,193,192,191,187,184,181,176,172,170,169,162,158,165,178,187,183,161,138,136,155,172,171,154};
const PROGMEM uint8_t PianoC3StartChunk672[] = // recorded at high sample rate
{130,138,143,144,142,144,153,165,173,170,158,144,137,137,144,153,162,170,174,174,173,171,168,162,154,146,137,121,97,74,63,69,85,102,115,124,129,128,124,121,121,123,122,119,115,117,123,127,126,125,128,134,141,138,122,101,83,78,87,104,118,123,119,109,98,93,96,99,94,81,68,59,56,58,60,59,53,48,48,56,66,76,82,85,85,84,85,90,98,99,92,81,74,74,80,88,96,99,97,94,93,96,104,116,129,142,156,168,176,179,174,164,152,146,144,145,147,154,163,170,172,171,169,170,175,185,198,216,229,234,233,232,232,229,224,216,202,187,175,165, // [Piano] [C3 start chunk] [672 samples] 0dB
155,147,141,137,132,122,106,89,77,69,65,67,75,84,86,80,68,57,51,52,55,57,59,64,75,87,97,103,106,108,109,107,103,99,92,84,77,75,81,95,107,110,102,89,83,90,106,125,139,149,155,159,162,162,164,171,180,187,192,194,193,190,187,187,193,201,205,202,197,195,198,203,204,203,201,206,217,229,234,228,213,195,182,180,186,195,202,199,188,179,185,208,233,249,248,236,217,196,173,150,130,113,97,85,78,76,76,77,80,85,92,99,102,99,89,79,75,81,93,108,122,132,136,133,124,111,96,82,69,62,65,74,85,94,98,96,90,85,86,88,87,80,66,51,41,
39,43,50,57,56,49,42,39,40,46,55,65,71,74,74,74,74,75,80,89,98,102,102,98,91,81,70,61,58,61,71,87,110,133,148,153,151,149,149,150,150,145,138,131,127,125,125,132,143,154,159,161,165,172,181,185,189,199,213,227,232,228,223,220,219,217,212,204,196,184,169,150,131,117,107,98,92,92,97,102,100,88,71,57,53,57,61,61,57,53,55,67,89,112,128,131,122,109,99,92,87,83,82,84,86,86,85,81,76,72,73,79,84,86,86,87,93,102,109,117,126,138,151,163,171,172,166,154,143,143,155,176,196,200,190,177,174,183,197,208,213,212,208,204,203,
209,220,227,225,218,214,211,206,197,189,186,195,211,228,242,250,253,250,244,238,234,229,221,203,175,147,126,112,103,98,98,101,102,100,96,91,84,76,71,76,91,108,120,124,122,114,107,104,103,102,99,93,84,75,70,70,73,74,73,71,68,64,57,49,46,48,52,54,51,46,38,31,27,26,30,36,44,52,58,62,63,61,60,63,70,79,90,101,109,111,108,100,95,93,93,90,88,89,98,108,116,120,126,138,155,166,169,169,168,165,158,149,145,147,149,147,141,138,144,155,164,169,176,186,197,204,206,205,204,204,208,215,223,226,218,198,176,159,151,146,139,124,
106,94,90,91,92,93,92,87,77,67,57,48,43,44,53,68,86,99,108,111,112,111,113,118,125,129,131,132,133,132,126,113,98,89,91,100,107,108,99,87,79,80,90,104,121,133,136,135,139,150,163,170,170,169,173,175,172,167,163,164,165,167,172,179,182,181,176,174,178,192,211,225,230,226,217,204,191,183,181,184,185,184,183,189,199,207,214,222,231,240,243,240,228,212,191,169,151,140,136,133};

//**************** Piano C3 to F3: (Band 2)
const PROGMEM uint8_t PianoC3StartChunk830[] = // recorded at low sample rate
{127,127,130,134,132,130,135,137,134,133,133,128,124,123,121,124,133,134,127,125,127,133,136,128,116,115,131,145,145,134,123,122,124,125,123,118,121,133,139,134,124,123,126,127,121,114,116,123,130,131,124,123,132,142,144,143,152,168,172,156,139,137,147,160,171,174,173,171,163,152,141,119,84,63,74,98,117,128,128,122,121,123,120,115,120,127,126,127,136,141,123,92,78,90,114,123,115,99,93,99,92,73,59,56,60,58,50,48,60,74,83,85,84,87,97,97,83,73,76,88,98,98,94,95,105,122,141,161,175,179,169,153,145,145,148,160,170,172,169,171,182,201,224,234, // [Piano] [C3 start chunk] [830 samples] 0dB
232,232,229,220,202,181,166,153,143,137,128,107,84,70,65,71,84,84,70,54,51,55,58,63,78,94,103,107,108,106,100,91,79,75,87,106,108,92,82,98,124,143,154,160,162,165,175,187,193,194,189,187,193,203,203,196,197,203,204,202,208,226,235,222,197,181,184,196,202,188,180,203,238,251,236,208,176,144,118,95,81,76,76,79,86,96,102,95,80,76,89,109,128,136,130,114,94,74,62,68,83,96,97,89,85,88,85,70,48,39,42,53,57,47,39,40,49,64,72,74,74,74,79,92,102,101,94,81,65,58,63,81,112,141,153,151,149,151,148,138,129,125,126,139,154,161,164,175,185,190,206,227,
232,224,220,219,212,201,187,164,136,116,102,92,93,101,99,78,57,54,61,60,54,55,77,110,131,125,108,94,87,82,83,86,86,81,74,72,80,86,86,90,101,112,124,140,158,171,171,156,142,150,178,201,191,174,180,200,212,213,207,203,212,226,225,216,211,204,191,187,203,228,246,254,249,241,234,227,206,167,131,111,100,98,102,101,95,87,75,72,88,112,124,121,110,104,103,100,92,79,70,71,73,73,69,63,53,46,49,53,50,41,31,26,28,37,49,58,63,61,60,67,80,96,108,111,102,94,93,90,87,96,110,119,127,147,166,170,168,164,153,145,148,148,140,141,156,167,175,190,203,206,205,
204,210,222,226,207,176,155,147,135,112,93,90,92,93,88,75,61,48,42,50,73,95,108,112,111,114,123,129,131,133,130,116,95,89,100,108,100,84,79,91,113,132,136,137,153,169,170,171,175,171,164,164,166,171,181,182,176,175,193,218,230,225,209,191,181,183,186,183,190,204,215,226,239,244,234,211,182,153,138,134,126,109,91,79,70,70,82,92,94,100,113,125,128,121,108,98,98,104,105,97,84,77,79,84,89,92,86,71,56,51,50,48,52,58,58,51,50,56,68,86,99,98,89,84,84,72,56,51,56,65,76,88,98,112,124,124,123,131,137,126,105,97,106,118,127,136,148,163,178,191,200,
211,222,223,218,217,214,200,176,152,142,147,150,145,135,121,103,88,81,88,107,117,114,109,117,138,156,158,152,148,143,136,130,121,108,95,84,78,81,88,89,81,78,89,110,129,133,125,120,130,149,157,153,146,140,141,151,166,180,196,207,205,199,201,204,200,192,190,196,209,226,243,254,254,248,238,228,220,207,187,164,143,127,107,84,69,64,69,86,101,106,101,91,84,83,86,89,89,83,72,61,52,52,61,70,69,55,40,37,47,53,47,37,32,33,37,38,40,56,77,83,80,84,89,83,71,59,54,59,68,73,74,78,93,108,119,133,141,135,126,126,128,126,119,108,111,134,160,174,179,189,
207,222,227,227,225,218,203,182,169,175,186,182,165,144,127,119,119,116,106,98,100,106,117,131,140,141,144,156,173,186,183,164,143,133,135,135,128,117,108,109,115,119,121,125,124,119,122,141,159,166,166,155,135,125,132,143,150,157,162,169,188,210,216,196,170,157,158,168,186,202,214,227,239,241,236,230,224,217,212,204,185,157};
const PROGMEM uint8_t PianoF3StartChunk830[] = // recorded at high sample rate
{127,122,117,114,117,123,126,125,125,135,153,168,173,173,175,176,172,166,162,162,158,152,148,143,134,126,122,120,120,122,126,128,125,120,117,123,136,147,148,144,144,145,143,134,120,107,95,85,79,80,87,93,94,92,94,101,109,114,117,124,132,135,128,113,96,84,79,77,74,75,77,76,72,68,64,62,66,74,81,83,84,86,87,83,71,59,55,59,67,74,84,97,108,112,106,100,101,108,117,131,147,161,169,174,174,169,168,168,168,168,173,180,186,192,195,186,170,159,160,169,180,192,205,213,210,195,174,158,154,157,162,166,171,179,184,179,170,165,161,153,145,142,140,140, // [Piano] [F#3 start chunk] [830 samples] 0dB (new)
141,142,137,127,119,121,130,140,141,133,122,113,105,97,92,94,104,118,131,141,144,138,131,129,133,139,143,142,136,128,119,109,101,96,92,88,88,95,103,106,104,100,97,102,117,135,150,163,174,179,178,170,158,146,135,128,125,124,123,124,129,137,144,149,154,160,162,158,145,132,125,124,117,104,91,81,71,61,54,50,50,55,60,61,60,58,53,46,39,34,28,24,28,39,50,56,59,63,64,60,58,66,80,94,107,119,133,151,170,183,187,188,188,185,180,180,187,193,193,191,189,187,184,183,187,195,203,210,214,212,205,196,189,187,187,183,176,172,176,187,195,197,194,188,181,
174,166,155,143,132,123,121,124,124,121,119,122,125,122,115,102,90,82,82,87,92,97,98,95,91,93,103,116,127,135,140,143,143,137,124,109,95,89,90,93,94,90,83,76,72,67,62,63,76,96,112,126,141,156,165,167,164,161,165,170,170,163,156,151,147,145,146,152,160,168,175,182,181,172,165,164,160,151,140,129,116,100,85,71,61,59,60,61,66,75,81,76,60,42,31,24,18,17,20,26,29,29,29,30,31,31,30,30,36,49,66,86,105,118,125,132,139,144,148,157,170,185,195,195,188,184,190,198,203,210,216,221,227,234,239,238,232,224,215,209,208,207,205,203,203,206,210,210,206,
199,192,186,182,177,167,152,140,135,137,141,144,140,131,121,109,98,91,90,87,81,77,77,77,74,70,69,74,84,99,113,124,130,131,126,117,111,109,105,101,102,106,103,93,81,70,59,50,51,62,77,92,106,116,124,130,133,135,136,138,139,137,134,131,130,128,128,135,147,157,162,168,178,186,188,182,174,171,173,171,162,150,135,118,101,89,84,87,94,98,95,86,76,66,56,47,38,27,21,24,28,26,22,21,21,19,14,8,8,18,36,55,73,87,98,106,114,124,137,152,162,164,164,167,170,173,176,178,183,195,208,218,224,229,231,232,234,236,231,223,220,222,225,225,224,223,222,220,222,
227,232,230,220,202,181,167,160,154,152,156,160,157,149,141,132,118,105,98,98,100,100,96,91,83,75,69,68,73,83,96,107,116,124,128,124,117,113,113,113,115,120,120,112,97,80,66,57,52,49,55,70,85,94,99,105,115,126,136,143,146,150,153,151,147,143,138,134,138,148,160,171,181,189,191,188,184,186,193,199,195,184,171,159,147,135,123,114,112,113,113,110,108,103,96,85,71,53,37,29,28,33,41,42,34,20,8,3,0,0,6,16,25,32,41,51,60,65,72,84,99,116,130,137,139,141,144,147,150,154,158,163,174,193,207,211,208,206,207,207,207,211,218,220,215,207,203,205,213,
223,228,230,231,226,214,199,185,174,167,169,175,179,178,172,165,158,150,140,132,130,129,122,108,96,88,79,69,63,66,79,95,104,105,104,106,106,101,96,96,102,110,115,114,104,91,80,74,69,61,52,52,58,64,69,74,81,91,103,112,120,132,146,150,144,135,127,126,132,139,141,142,145,150,156,161,167,174,181,187,190,189,182,172,166,161,153,141,132,127,127,130,132,127};

//**************** Piano F#3 to B3: (Band 3)
const PROGMEM uint8_t PianoF3StartChunk591[] = // recorded at low sample rate
{123,121,125,130,137,136,135,136,126,115,118,125,125,128,150,170,173,175,175,166,162,160,151,145,134,124,120,120,125,128,122,117,128,146,147,143,145,139,121,102,87,79,84,94,93,94,104,113,118,129,135,123,99,82,78,74,76,76,71,65,62,71,81,84,86,86,75,58,57,67,78,95,110,109,100,104,117,136,158,171,174,169,168,168,169,178,187,195,187,164,159,172,187,206,213,196,167,154,157,164,171,182,180,168,163,152,143,140,140,142,135,121,121,135,142,130,115,104,94,94,110,129,143,140,131,130,139,144,138,126,113,100,94,89,90,101,106,102,97,108,132,154,171,179, // [Piano] [F#3 start chunk] [591 samples] 0dB (new)
175,159,141,129,124,123,124,133,144,150,158,162,151,132,124,119,100,84,70,57,50,52,59,61,59,53,42,35,27,26,41,54,59,64,61,59,75,95,113,132,158,181,187,188,185,179,186,194,192,189,185,183,190,202,212,213,204,192,187,186,178,172,182,195,196,189,179,168,153,137,124,122,125,120,120,125,119,103,86,82,88,95,98,93,92,107,124,136,142,143,132,112,93,89,94,93,84,75,69,62,70,96,118,139,159,167,164,162,170,168,157,150,146,146,156,167,178,182,170,164,160,146,131,112,90,70,59,59,62,72,81,67,42,27,19,17,24,29,29,30,31,30,30,44,68,97,117,128,138,145,153,
171,190,196,186,188,198,207,216,223,233,239,234,223,211,208,207,204,203,208,210,203,193,184,179,165,145,135,138,144,139,124,108,94,90,85,78,77,75,70,70,83,103,121,131,129,117,110,106,101,105,103,87,71,56,50,64,86,106,120,129,134,136,138,138,134,131,129,128,143,157,165,177,188,184,173,172,171,157,138,114,93,84,90,98,92,78,64,51,38,24,24,28,23,21,20,14,7,15,41,67,88,102,113,128,149,162,164,167,171,175,179,191,210,222,229,231,234,235,225,220,223,225,224,222,220,225,232,227,205,176,162,154,154,160,154,142,129,109,98,99,100,95,85,73,68,73,89,106,
118,127,123,114,113,113,119,119,103,79,61,52,50,65,87,97,105,119,135,144,148,153,149,143,136,136,151,167,182,191,189,184,191,199,189,171,154,137,121,112,113,112,108,101,88,67,42,29,30,40,41,24,7,1,0,10,23,34,48,60,69,83,105,127,138,140,144,148,154,159,170,195,211,208,206,207,208,216,220,211,203,207,221,229,231,226,207,187,171,167,176,179,172,162,152,138,130,129,115,97,86,72,63,72,95,105,104,107,103,96,99,109,116,106,88,76,69,56,52,60,67,75,87,103,115,130,148,147,134,126,132,140,142,146,154,161,170,181,188,190,180,168,161,148,133,127,129,132};
const PROGMEM uint8_t PianoC4StartChunk591[] = // recorded at high sample rate
{133,137,142,147,151,152,150,145,146,156,169,178,182,184,188,194,199,198,190,172,154,143,141,144,144,138,130,123,119,120,124,124,120,114,107,103,105,111,116,119,119,113,106,101,99,98,98,97,92,82,71,65,63,62,64,73,86,98,103,102,92,79,68,64,62,59,57,57,57,54,49,43,36,27,23,27,36,46,54,59,67,79,94,108,124,136,139,134,122,111,109,116,127,135,143,151,158,164,170,178,187,194,196,196,197,200,201,199,192,181,171,167,174,185,195,200,199,194,188,189,198,211,219,219,214,206,200,197,197,195,187,173,159,151,152,156,159,159,157,151,149,153,159,160,153,142,134, // [Piano] [C4 start chunk] [591 samples] 0dB (new)
132,135,142,150,156,158,154,142,121,99,81,73,76,89,104,117,121,115,102,88,76,64,55,52,54,60,63,59,50,37,22,9,2,4,14,28,39,47,58,73,91,109,123,132,132,124,115,110,112,121,130,134,131,123,116,115,119,123,128,135,146,159,173,182,184,176,161,150,148,155,164,170,173,171,167,166,170,176,180,181,181,181,181,179,177,173,167,158,152,150,150,151,149,142,133,128,127,128,127,122,119,120,122,122,120,117,116,121,132,148,161,161,147,122,98,80,74,75,82,96,113,127,133,130,123,111,93,76,66,68,77,86,90,85,74,61,52,47,45,41,36,30,29,36,55,85,115,137,146,144,140,139,
143,151,160,164,162,160,160,159,153,143,132,126,128,141,162,184,194,189,176,163,157,160,167,176,182,184,186,190,198,206,208,203,193,183,179,180,180,175,166,159,156,153,150,147,142,138,135,134,134,133,130,128,129,137,145,148,143,133,129,133,144,160,178,194,200,191,171,149,130,118,111,112,117,125,134,140,139,128,112,96,84,77,75,76,80,79,72,64,60,59,58,52,40,24,8,1,9,30,54,74,90,103,114,125,134,140,142,140,139,144,153,159,158,147,131,118,114,119,132,146,157,159,154,148,146,151,159,166,169,172,177,185,193,197,198,197,194,189,180,170,159,147,135,128,127,
127,124,121,123,132,141,141,131,119,109,104,107,116,128,136,135,125,111,101,99,107,123,140,153,161,163,158,145,127,111,102,101,108,121,136,144,141,130,116,103,95,92,92,89,81,72,69,73,81,86,83,72,54,34,18,11,11,15,23,35,52,74,96,113,120,121,121,125,133,144,153,155,146,130,115,108,111,118,124,128,130,132,136,137,135,132,132,137,147,161,175,182,181,179,183,191,195,191,181,173,170,170,169,165,156,143,133,130,132,138,143,144,138,129,122,125,136,148,155,156,149,135,119,107,106,113,128,147,167,180,185,180,166,147,128,114,112,122,136,145,146,138,128,119,116,115,112};

//**************** Piano C4 to F4: (Band 4)
const PROGMEM uint8_t PianoC4StartChunk728[] = // recorded at low sample rate
{128,124,124,129,132,131,127,123,129,136,143,149,152,148,145,158,174,182,185,192,199,194,172,148,141,144,141,130,121,120,124,122,113,104,105,112,119,118,109,101,99,98,96,87,71,64,63,66,81,98,103,94,75,65,61,58,57,57,51,42,31,23,30,44,55,63,79,99,120,137,137,122,109,115,129,140,151,161,169,181,192,196,196,199,201,194,179,167,173,189,200,199,190,188,203,218,218,209,199,197,196,184,163,151,154,159,159,153,149,156,160,150,135,132,138,150,158,156,139,108,82,73,86,108,121,114,95,78,61,52,55,62,60,47,27,8,2,13,32,45,59,83,109,127,133,122,111,113,125, // [Piano] [C4 start chunk] [728 samples] 0dB (new)
133,129,118,115,121,127,137,155,174,185,177,157,147,156,167,173,170,165,171,179,181,181,181,179,175,167,155,150,151,150,141,130,127,128,124,119,121,122,119,116,122,141,161,157,128,94,75,75,87,110,129,132,123,103,78,66,73,86,88,76,58,48,45,39,31,30,49,89,129,146,143,139,145,157,164,161,160,158,147,132,126,139,169,192,188,169,158,162,174,182,185,190,201,208,202,187,179,180,176,165,157,153,149,143,137,134,134,132,128,131,143,148,138,129,136,155,180,199,192,163,134,116,111,117,129,139,137,119,96,80,75,77,80,71,62,59,57,45,22,2,8,38,69,92,110,124,
137,142,140,141,153,160,150,128,114,120,139,155,158,150,146,155,165,170,176,186,195,198,196,190,179,164,147,132,127,127,122,123,137,141,128,112,104,111,127,136,129,110,99,106,129,150,162,161,145,121,103,102,115,136,144,132,113,97,92,91,82,70,71,82,86,74,48,22,10,13,22,40,68,99,118,121,122,131,146,155,146,123,108,112,122,128,131,135,136,133,132,141,159,178,182,179,187,195,188,175,170,170,165,151,135,130,135,143,142,131,122,132,149,157,150,130,110,106,119,145,172,184,178,156,129,112,119,138,147,138,124,116,115,106,84,63,60,70,73,62,49,38,25,10,
0,4,26,54,71,73,80,109,141,159,159,148,139,139,142,144,155,169,170,159,148,145,147,152,160,173,184,189,192,190,188,190,189,178,164,155,153,155,153,144,139,145,160,178,183,169,148,134,130,141,164,187,196,182,156,138,138,148,154,153,153,157,158,147,130,117,110,107,106,101,96,93,78,47,19,11,20,35,40,36,43,70,99,120,132,135,129,117,110,115,123,129,136,141,138,129,121,118,125,139,150,153,154,160,174,188,191,186,179,172,170,169,165,155,148,151,161,173,182,181,168,146,125,122,141,169,185,179,162,149,141,133,129,131,135,141,141,132,123,119,113,101,90,
84,88,93,80,49,21,9,10,10,5,3,9,20,40,69,96,114,119,113,105,104,109,116,124,133,142,144,137,128,132,147,158,162,160,161,171,187,194,189,180,175,176,178,173,163,150,141,137,141,155,174,177,156,128,114,119,136,151,153,151,150,145,132,120,117,125,138,142,139,140,145,143,125,104,100,114,120,109,88,65,45,31,22,16,12,9,12,21,40,66,91,108,114,114,112,108,106,111,120,130,135,134,130,130,136,144,147,144,144,153,168,182,185,182,183,188,190,192,194,187,170,152,148,162,187,201,194,173,154,147,149,153,159,168,175,171,155,137,129,131,127,120,123,136,142,133};
const PROGMEM uint8_t PianoF4StartChunk728[] = // recorded at high sample rate
{127,136,142,143,143,144,149,155,161,167,171,170,177,181,166,150,141,128,121,124,117,110,121,142,166,183,190,190,189,184,174,157,140,138,139,136,130,122,112,102,93,90,95,106,105,101,99,104,109,106,98,91,95,98,92,81,65,49,40,39,36,38,49,67,84,95,96,91,92,96,102,102,104,112,126,139,142,142,143,146,155,173,178,175,177,186,195,191,182,173,167,161,156,138,115,110,117,130,153,180,199,209,211,207,201,194,177,165,167,177,184,177,163,150,152,157,159,153,145,147,152,156,150,137,122,123,131,131,128,120,104,90,81,64,50,50,56,65,76,86,91,90,88,89,84,79,80, // [Piano] [F#4 start chunk] [728 samples] 0dB
84,89,93,97,99,103,109,114,119,126,139,151,161,170,179,195,199,193,188,179,167,151,136,126,131,148,165,182,197,213,221,219,209,199,200,204,207,203,196,186,182,180,173,165,159,156,155,155,147,134,122,109,98,93,94,98,94,84,68,47,28,17,7,1,2,11,24,36,42,38,36,39,46,51,52,57,65,80,88,89,90,96,109,118,126,137,150,163,176,188,200,212,218,209,198,186,171,157,148,141,148,171,193,206,209,208,208,210,211,209,199,188,185,180,172,165,159,156,156,158,159,156,150,142,133,124,114,107,107,107,105,102,92,78,65,48,29,18,15,18,22,27,33,39,44,40,35,36,42,48,47,46,
50,58,64,62,62,66,74,84,92,95,103,125,147,165,178,187,190,186,176,162,144,135,149,166,180,193,204,210,218,226,234,235,233,230,227,222,217,210,200,193,191,196,198,194,186,174,161,150,140,131,119,108,110,114,108,92,72,53,41,33,17,5,4,13,22,27,28,27,30,32,34,36,40,48,53,56,62,67,71,76,83,88,93,99,109,118,129,147,168,188,195,191,179,163,149,139,134,134,142,153,162,170,178,186,195,203,208,211,210,209,206,200,188,177,177,183,190,189,183,179,173,164,147,132,125,122,121,119,117,112,102,86,67,51,40,36,30,25,31,40,48,51,50,47,50,56,62,63,59,60,64,72,76,
80,85,94,106,113,115,116,123,139,163,186,203,210,209,202,196,188,177,169,166,167,173,183,189,196,205,213,221,233,240,237,226,217,216,210,201,193,193,199,204,201,189,175,162,152,136,118,107,104,105,103,97,86,72,59,50,39,27,27,32,37,42,46,49,50,51,53,57,61,64,66,65,64,65,72,81,88,97,104,105,104,105,114,131,155,177,193,199,200,198,197,190,180,167,162,171,178,179,177,180,189,205,217,221,222,222,224,217,206,197,192,192,199,208,212,210,200,187,171,156,143,134,123,117,115,118,113,100,82,65,52,45,39,28,25,29,36,38,36,31,31,39,48,52,49,44,39,40,47,56,
63,69,77,80,77,73,75,87,106,127,145,163,178,188,189,183,178,175,173,168,163,165,168,170,176,185,196,209,222,226,224,220,216,210,200,193,194,201,209,212,206,196,186,175,161,142,128,122,122,121,116,107,96,83,67,52,39,31,27,27,29,32,30,27,33,42,51,56,59,58,51,44,47,57,70,80,82,76,77,80,79,76,79,97,121,143,158,168,173,177,179,178,172,165,165,166,163,163,168,175,184,195,206,216,225,226,221,212,206,203,199,197,199,207,217,219,212,196,182,170,159,147,139,134,132,129};

//**************** Piano F#4 to B4: (Band 5)
const PROGMEM uint8_t PianoF4StartChunk1060[] = // recorded at low sample rate
{129,131,132,133,122,128,128,127,128,137,149,146,149,160,148,148,148,139,132,125,144,156,169,169,166,160,143,127,129,135,137,136,129,118,108,108,114,114,115,107,86,73,79,84,74,60,66,75,78,83,90,85,92,100,84,90,105,133,123,120,136,143,149,160,176,175,172,172,158,136,115,99,98,100,130,157,179,180,169,158,130,117,120,135,119,104,101,106,115,126,143,151,137,136,129,127,131,119,111,90,102,107,107,123,145,139,114,110,118,114,112,126,142,152,160,172,181,185,185,191,188,175,156,134,118,109,120,137,158,179,189,187,163,132,111,115,112,117,106,82,73,77,97, // [Piano] [F#4 start chunk] [1060 samples] 0dB (start at 53) (ee)
112,124,119,94,94,108,96,84,75,69,46,43,61,83,95,91,92,87,91,94,109,123,134,142,143,144,152,160,169,170,178,175,152,138,122,123,113,119,151,180,190,189,185,168,143,137,138,129,118,104,92,92,105,104,99,106,108,97,92,98,89,70,47,39,36,40,62,86,97,91,94,100,103,108,127,141,142,143,151,172,177,176,190,194,181,170,162,152,121,110,123,151,188,207,211,204,194,171,165,179,182,162,150,156,159,147,147,155,152,132,121,131,130,121,99,84,62,48,56,68,84,91,88,88,82,79,86,91,97,100,108,115,124,139,157,168,184,199,193,184,168,146,128,132,157,180,203,220,218,202,
200,206,205,194,183,180,169,160,155,155,147,128,111,96,93,97,90,70,41,19,7,0,10,29,41,37,37,46,52,55,68,85,89,92,106,120,132,151,169,186,204,217,208,193,172,155,142,150,183,205,209,208,210,211,199,186,181,170,161,156,157,159,155,145,133,119,108,107,106,101,85,66,42,20,16,20,27,35,43,40,34,43,48,46,51,62,62,63,72,87,94,104,135,163,181,190,185,170,145,138,162,181,200,210,220,232,235,231,227,220,211,199,191,195,197,188,171,154,140,126,110,111,111,91,62,42,29,8,5,18,27,27,29,33,35,41,50,56,63,69,76,85,92,101,115,130,156,186,195,183,162,144,134,135,150,
163,174,186,198,208,211,210,206,196,179,178,188,189,181,175,160,137,125,122,120,116,105,84,58,41,34,26,32,45,51,49,48,57,63,59,61,70,77,83,94,110,115,117,132,164,195,210,207,198,187,172,166,169,180,190,201,213,225,239,235,220,215,207,194,194,202,201,183,164,148,124,107,105,103,94,76,58,45,29,28,36,42,48,50,52,55,62,65,65,63,71,82,93,104,105,105,117,146,178,196,200,198,193,179,163,170,179,177,180,197,216,222,222,223,211,197,192,197,209,211,200,181,158,141,127,116,116,115,95,71,51,43,30,25,34,38,33,31,42,51,49,41,40,50,62,71,80,77,73,85,111,139,164,
184,189,182,176,173,165,165,169,174,186,203,221,226,221,216,204,193,196,208,211,200,186,170,146,126,122,121,112,97,78,57,38,29,27,31,31,27,37,50,57,59,49,45,57,74,82,77,79,79,76,93,126,154,168,175,179,177,166,166,165,162,170,181,197,212,224,225,214,205,200,197,201,215,219,203,182,165,149,138,132,129,119,93,62,45,35,22,12,7,8,12,28,37,27,21,27,41,57,67,70,66,61,64,76,98,127,149,158,168,176,175,167,167,175,176,181,202,227,237,239,241,234,222,216,221,237,237,223,209,191,171,156,155,152,135,119,93,62,45,35,22,12,7,8,12,28,37,27,21,27,41,57,67,70,66,61,
64,76,98,127,149,158,168,176,175,167,167,175,176,181,202,227,237,239,241,234,222,216,221,237,237,223,209,191,171,156,155,152,135,119,93,62,45,35,22,12,7,8,12,28,37,27,21,27,41,57,67,70,66,61,64,76,98,127,149,158,168,176,175,167,167,175,176,181,202,227,237,239,241,234,222,216,221,237,237,223,209,191,171,156,155,152,135,119,93,62,45,35,22,12,7,8,12,28,37,27,21,27,41,57,67,70,66,61,64,76,98,127,149,158,168,176,175,167,167,175,176,181,202,227,237,239,241,234,222,216,221,237,237,223,209,191,171,156,155,152,135,119,93,62,45,35,22,12,7,8,12,28,37,27,21,27,
41,57,67,70,66,61,64,76,98,127,149,158,168,176,175,167,167,175,176,181,202,227,237,239,241,234,222,216,221,237,237,223,209,191,171,156,155,152,135,119,93,62,45,35,22,12,7,8,12,28,37,27,21,27,41,57,67,70,66,61,64,76,98,127,149,158,168,176,175,167,167,175,176,181,202,227,237,239,241,234,222,216,221,237,237,223,209,191,171,156,155,152,135,119,93,62,45,35,22,12,7,8,12,28,37,27,21,27,41,57,67,70,66,61,64,76,98,127,149,158,168,176,175,167,167,175,176,181,202,227,237,239,241,234,222,216,221,237,237,223,209,191,171,156,155,152,135};
const PROGMEM uint8_t PianoC5StartChunk1060[] = // recorded at high sample rate
{130,132,125,117,115,115,111,106,105,107,119,135,150,154,151,152,155,155,153,141,128,121,127,138,141,138,133,127,128,131,134,132,136,147,153,150,137,122,108,99,90,78,65,65,76,92,102,104,100,97,102,115,124,128,128,125,120,118,113,105,94,91,98,109,122,132,134,140,149,162,165,157,148,138,135,141,144,144,137,138,141,142,143,146,155,168,183,190,184,176,171,164,151,134,116,105,109,121,133,135,134,135,137,148,162,173,178,180,184,183,177,169,153,138,133,136,141,141,142,146,152,164,174,173,163,153,148,143,138,131,124,118,115,109,99,91,97,111,125,134,138,137,137,140, // [Piano] [C5 start chunk] [1060 samples] 0dB (start at 278)
133,117,96,81,74,76,88,95,96,95,96,104,115,130,145,154,167,180,189,186,174,162,151,147,148,145,139,134,140,150,159,162,158,150,144,139,130,119,111,107,101,89,73,59,51,56,65,73,78,84,92,99,103,98,81,63,51,49,56,64,72,74,74,82,91,103,115,130,149,170,192,205,205,199,193,192,190,187,180,173,172,181,190,195,195,194,191,184,173,161,150,146,144,132,114,93,77,69,67,69,70,72,80,91,100,102,93,75,53,41,38,40,42,44,45,48,54,63,69,76,90,113,139,160,172,174,173,175,179,177,169,161,159,163,171,179,186,193,200,200,191,182,176,175,176,173,162,143,124,112,102,98,94,93,96,106,
125,139,142,132,113,93,79,73,69,63,60,61,65,67,67,65,67,81,105,129,144,153,160,166,174,174,168,159,153,153,152,155,163,174,187,193,193,187,180,181,185,188,185,175,160,142,130,119,106,95,89,93,108,126,141,142,130,114,98,83,69,56,45,40,44,47,42,31,24,27,41,61,76,88,100,116,129,135,136,133,129,126,122,121,123,135,153,167,176,177,175,175,181,192,199,197,190,181,171,163,152,136,118,111,119,138,158,172,172,164,155,144,127,106,86,77,77,83,79,66,52,47,53,63,74,86,99,116,134,147,154,157,160,157,149,140,134,137,150,168,182,187,186,184,187,196,206,210,206,199,194,190,
181,163,139,116,108,118,136,151,159,163,165,163,154,129,103,84,80,82,78,68,53,42,39,39,43,48,61,78,95,112,127,139,149,153,152,141,128,121,124,136,153,165,167,165,166,174,184,191,193,188,186,189,191,181,158,128,107,101,110,121,130,140,154,167,168,154,131,110,100,97,93,86,75,65,55,47,43,43,48,60,75,90,105,122,140,154,163,162,149,134,126,130,142,153,159,159,156,161,171,179,180,176,175,181,190,192,176,149,123,107,99,96,97,105,121,143,154,152,137,121,107,97,90,83,74,65,54,42,30,22,21,25,33,44,57,71,90,113,134,146,146,136,124,121,129,140,148,150,150,156,167,178,183,
179,179,188,202,212,209,195,175,157,142,124,113,112,125,145,163,174,172,163,151,140,128,119,112,104,93,80,65,49,36,31,32,34,38,44,56,76,102,126,137,136,128,122,124,132,139,139,139,146,159,171,177,174,172,178,194,208,214,212,206,194,178,156,132,117,115,126,142,156,165,167,161,152,141,130,119,112,105,95,80,61,43,30,23,21,17,12,13,23,46,74,97,107,106,104,107,113,119,118,117,122,137,153,160,161,160,165,176,190,201,209,215,218,213,197,173,150,133,129,137,150,163,173,178,176,169,160,149,141,136,130,119,101,80,64,52,46,39,28,16,12,23,46,71,88,94,98,105,115,119,116,
112,115,126,141,152,155,156,159,165,172,180,190,202,213,220,218,202,178,154,136,127,129,137,147,158,166,168,161,152,144,139,137,131,118,98,78,65,58,51,39,20,5,2,15,35,50,61,70,84,98,108,108,104,105,113,124,135,143,149,154,160,165,170,177,188,204,221,233,233,221,200,178,160,147,142,146,156,168,176,177,171,163,159,160,159,151,135,114,99,91,87,74,53,32,19,19,27,36,43,52,68,86,99,106,106,106,110,118,126,134,141,148,155,160,162,164,168,181,200,220,232,236,229,214,194,174,157,147,148,158,170,176,175,169,164,166,170,169,156,137,120,113,108,99,79,55,37,28,25,23,23,27,
38,56,73,84,90,94,99,105,111,118,125,134,145,154,157,156,156,162,175,195,215,230,238,238,229,212,190,170,157,158,167,177,178,173,169,172,180,184,177,159,144,135,131,123,106,84,63,46,35,23,13,7,10,20,33,46,55,61,67,73,77,80,85,93,105,117,126,127,123,122,128,141,159,179,199,213,222,219,203,178,156,146,149,157,162,156,152,156,168,178,177,167,155,148,144,138,126,110,92,75,59,43,27,12,4,5,12,22,32,42,51,60,66,68,67,72,83,97,109,115,115,113,112,115,124,141,163,187,212,229,232,216,189,174,173,178,179,174,170,173,184,199,206,204,199,195,191,186,178,165,149,134};

//**************** Piano C5 to F5: (Band 6)
const PROGMEM uint8_t PianoC5StartChunk1314[] = // recorded at low sample rate
{127,132,134,133,129,127,127,130,134,131,126,125,125,123,121,123,125,125,128,129,127,123,120,120,120,120,119,116,117,118,119,121,126,132,132,132,135,140,143,145,148,149,147,152,159,162,160,155,151,140,130,131,133,130,124,124,128,125,115,103,100,101,92,79,71,72,77,77,82,90,97,103,107,112,114,118,132,138,131,125,139,164,174,176,181,183,171,151,144,146,144,138,134,138,138,127,118,118,122,113,96,84,75,67,63,74,91,97,103,117,126,126,130,151,161,146,134,141,160,172,183,203,213,206,186,173,172,165,154,149,154,153,135,122,125,132,130,118,110,93,68,56,61,70,72,82,103, // [Piano] [C5 start chunk] [1314 samples] 0dB (start at 8) (qhf)
105,98,105,128,142,129,115,113,116,120,129,151,171,171,161,153,151,141,126,130,141,141,124,112,119,123,123,126,128,115,86,71,69,64,66,83,104,104,93,103,122,132,125,116,115,110,104,108,126,149,153,151,155,154,142,123,126,140,140,132,126,131,133,135,148,152,137,115,100,87,68,66,86,102,103,97,106,122,128,127,120,117,107,92,95,110,127,135,141,159,164,152,138,137,145,141,137,141,143,146,159,180,189,179,171,159,136,112,106,123,135,135,135,145,166,177,181,184,178,163,141,133,139,142,143,151,168,175,161,149,143,135,125,117,111,98,93,110,129,137,137,139,133,107,82,74,
84,96,95,97,108,128,147,162,182,188,175,157,147,148,141,134,144,158,162,153,143,135,119,109,102,85,63,51,61,73,80,91,100,101,80,56,49,58,70,73,77,92,108,128,155,185,205,202,193,191,188,178,171,181,193,195,193,186,171,154,146,140,117,88,71,67,70,72,84,99,101,83,53,38,40,43,45,49,59,69,81,109,145,170,174,174,179,174,163,159,167,179,189,199,198,184,175,175,173,154,127,109,98,94,94,106,131,143,129,100,78,71,64,60,63,67,66,67,91,125,147,158,168,175,169,156,152,153,159,175,191,194,184,180,186,187,175,152,131,116,98,89,100,126,143,133,110,88,69,50,41,45,44,29,24,41,
68,86,104,125,136,135,130,125,121,125,146,168,177,175,176,189,199,194,181,168,154,131,112,121,149,171,170,157,141,114,86,76,82,76,55,47,59,74,91,113,138,152,158,159,149,137,136,153,178,187,185,187,199,210,204,196,190,175,143,112,113,138,156,163,165,156,124,89,80,81,69,49,39,40,45,61,85,109,131,146,154,147,129,121,133,156,167,165,169,183,193,190,186,191,181,145,109,103,118,131,147,166,164,136,107,98,93,80,66,52,44,43,54,75,96,119,144,161,161,141,126,134,151,160,157,161,176,180,176,179,191,185,149,115,100,96,102,124,151,152,131,110,95,85,74,60,44,27,21,26,39,57,
78,109,138,147,135,121,128,144,150,151,162,179,181,179,192,210,208,183,157,134,114,114,138,165,174,163,147,131,118,107,93,73,50,34,31,35,41,55,87,123,138,131,122,128,138,139,144,162,176,174,174,192,211,213,204,185,155,124,114,130,152,166,164,152,137,121,111,99,79,52,30,23,18,12,17,46,85,106,105,105,114,119,117,127,150,161,160,165,183,200,211,218,211,184,150,130,135,153,170,178,173,160,146,137,129,108,79,57,47,36,18,13,36,71,91,97,108,119,116,112,125,146,155,156,162,173,184,200,216,220,199,164,136,127,135,150,164,167,156,144,138,133,114,85,65,55,40,14,1,18,44,60,
75,96,109,105,105,119,135,146,153,161,168,177,195,220,234,225,197,167,147,143,154,171,178,170,160,160,157,139,110,93,86,65,33,17,25,37,47,68,92,105,106,108,119,131,140,151,159,163,166,182,210,231,235,218,191,164,147,152,168,176,171,164,168,169,149,123,111,104,78,45,28,24,22,28,48,73,87,93,100,109,119,130,144,156,156,157,169,196,223,237,236,217,186,161,158,172,179,172,170,180,181,160,139,132,120,93,62,41,24,10,8,22,41,55,63,72,78,83,95,112,125,126,122,130,152,180,206,221,215,185,154,146,158,160,152,159,175,176,161,148,142,129,106,81,59,35,13,3,10,23,37,51,63,68,
68,79,99,113,115,113,114,127,154,189,221,233,207,176,174,179,174,170,182,202,205,199,193,186,174,153,129,101,73,44,22,16,18,26,43,58,61,62,74,94,111,119,119,118,121,133,169,215,232,216,194,189,183,169,165,175,190,196,193,192,188,175,161,146,127,101,73,44,22,16,18,26,43,58,61,62,74,94,111,119,119,118,121,133,169,215,232,216,194,189,183,169,165,175,190,196,193,192,188,175,161,146,127,101,73,44,22,16,18,26,43,58,61,62,74,94,111,119,119,118,121,133,169,215,232,216,194,189,183,169,165,175,190,196,193,192,188,175,161,146,127,101,73,44,22,16,18,26,43,58,61,62,74,94,111,
119,119,118,121,133,169,215,232,216,194,189,183,169,165,175,190,196,193,192,188,175,161,146,127,101,73,44,22,16,18,26,43,58,61,62,74,94,111,119,119,118,121,133,169,215,232,216,194,189,183,169,165,175,190,196,193,192,188,175,161,146,127,101,73,44,22,16,18,26,43,58,61,62,74,94,111,119,119,118,121,133,169,215,232,216,194,189,183,169,165,175,190,196,193,192,188,175,161,146,127,101,73,44,22,16,18,26,43,58,61,62,74,94,111,119,119,118,121,133,169,215,232,216,194,189,183,169,165,175,190,196,193,192,188,175,161,146,127,101,73,44,22,16,18,26,43,58,61,62,74,94,111,119,119,
118,121,133,169,215,232,216,194,189,183,169,165,175,190,196,193,192,188,175,161,146,127,101,73,44,22,16,18,26,43,58,61,62,74,94,111,119,119,118,121,133,169,215,232,216,194,189,183,169,165,175,190,196,193,192,188,175,161,146,127,101,73,44,22,16,18,26,43,58,61,62,74,94,111,119,119,118,121,133,169,215,232,216,194,189,183,169,165,175,190,196,193,192,188,175,161,146,127};
const PROGMEM uint8_t PianoF5StartChunk1314[] = // recorded at high sample rate
{129,124,124,127,130,133,134,133,131,126,122,120,118,120,122,122,121,126,134,137,138,134,127,125,127,127,124,120,115,111,108,107,116,126,123,114,109,106,111,121,129,134,137,139,143,151,157,157,155,156,157,154,151,148,145,143,141,135,124,116,113,111,108,101,90,81,78,80,84,93,102,108,110,110,113,117,123,139,149,152,153,149,146,155,164,167,169,174,178,173,161,151,147,147,144,133,121,117,114,106,96,81,60,44,37,38,45,54,65,77,83,82,93,127,157,173,185,192,192,181,173,179,190,200,203,199,194,184,171,165,166,160,143,124,104,88,79,73,63,50,37,31,37,50,56,69,87,83,70,68, // [Piano] [F#5 start chunk] [1314 samples] 0dB (start at 29) (n1)
85,114,138,158,178,184,178,172,175,190,206,213,221,226,214,197,193,196,201,198,180,160,142,124,109,100,85,62,52,51,48,51,59,68,66,48,35,34,38,57,85,110,130,136,134,143,159,168,178,192,201,200,188,177,175,175,174,172,173,164,147,138,128,108,93,86,88,90,88,97,108,103,87,68,54,55,60,65,84,101,106,112,124,134,142,147,162,184,195,195,192,193,192,191,198,200,193,179,164,150,125,96,84,77,64,58,60,69,75,67,54,47,40,37,46,60,75,85,94,115,134,143,150,160,173,184,193,200,200,198,200,208,214,207,197,192,173,143,122,112,106,98,90,94,110,116,112,103,88,79,71,68,75,77,76,81,98,
120,134,137,138,142,150,159,169,172,164,164,174,177,180,190,188,169,145,123,112,108,97,91,93,107,128,135,138,140,128,116,113,114,116,111,107,118,135,143,141,136,128,122,126,131,125,117,113,112,121,138,155,165,156,129,106,94,90,83,74,79,94,108,128,144,150,152,149,147,148,143,129,122,132,147,157,164,161,149,144,149,154,154,142,124,117,115,121,139,154,155,137,110,94,83,72,64,60,63,73,82,92,104,109,104,104,105,97,84,79,84,96,112,124,126,121,119,131,147,151,150,146,140,142,151,171,200,218,214,196,174,157,138,119,113,111,104,98,100,102,103,104,107,106,96,82,73,73,84,
102,120,127,124,129,144,160,172,172,168,165,156,152,165,189,209,213,200,180,151,121,103,95,88,76,60,56,61,62,67,76,80,75,66,60,59,69,93,114,124,130,138,156,179,194,203,209,208,201,192,192,203,216,226,225,209,184,154,128,115,106,89,74,63,52,44,43,50,59,67,67,61,58,65,83,105,122,129,136,150,169,186,199,209,213,209,200,194,196,208,220,224,220,199,165,140,125,111,99,84,67,52,42,40,46,57,62,61,57,52,52,63,81,98,110,119,133,150,166,182,198,209,210,195,180,182,191,204,219,222,208,183,156,137,125,111,95,77,56,40,35,38,45,55,60,58,54,51,56,73,88,96,105,115,130,147,164,185,
205,208,198,186,179,183,196,213,225,220,195,170,150,133,122,110,87,63,44,31,27,31,35,40,45,43,39,42,50,63,75,85,99,113,124,145,174,199,210,206,193,185,185,199,224,242,245,235,214,192,179,168,151,131,105,79,63,53,46,45,51,56,56,51,46,52,61,71,84,95,104,119,139,169,202,221,225,218,202,194,204,223,243,254,247,228,209,190,175,162,138,107,81,61,44,32,24,24,31,33,27,22,19,25,36,45,60,72,78,95,125,157,186,203,201,191,182,181,196,217,231,235,230,215,201,191,175,152,126,98,76,57,35,19,15,21,28,26,22,26,30,33,46,60,67,76,89,110,142,174,197,210,207,194,189,195,208,224,231,
227,218,207,195,185,168,140,116,98,77,55,33,21,21,22,21,25,29,29,35,46,58,71,80,92,114,139,166,194,212,218,215,210,213,223,233,240,247,244,236,231,222,206,184,161,140,119,91,63,47,35,27,22,19,19,21,22,28,35,37,44,59,76,99,127,157,183,198,202,205,207,209,216,228,236,238,236,232,228,219,200,180,164,144,120,96,78,63,48,37,33,32,32,35,37,36,36,38,43,55,70,90,116,141,160,175,185,188,190,195,204,218,227,230,234,237,230,217,204,190,172,148,125,104,85,67,51,37,31,32,33,35,34,30,28,32,41,54,72,91,113,137,158,175,185,185,183,189,199,207,213,219,223,221,213,202,191,175,156,
136,116,97,74,50,35,27,24,26,29,28,24,22,26,35,48,62,78,100,125,152,177,194,200,202,207,216,224,230,237,245,247,243,234,222,206,187,168,151,129,105,81,58,42,34,33,35,34,26,20,21,27,36,46,60,75,94,120,151,175,186,190,196,204,210,216,223,234,242,243,240,231,217,203,189,173,153,127,99,74,52,36,32,31,27,21,13,10,13,18,27,38,48,61,86,118,146,163,173,184,194,199,205,216,229,241,250,254,252,244,233,223,213,195,174,149,118,89,72,62,59,55,44,34,26,21,20,24,30,34,38,55,83,109,128,144,159,168,172,175,180,192,206,218,226,228,222,215,210,202,192,175,149,119,89,64,51,45,39,31,
23,16,12,11,18,25,27,31,44,66,92,116,136,154,169,174,175,180,189,200,215,227,231,231,228,224,222,215,202,184,155,122,94,75,65,57,48,40,32,23,19,24,30,34,35,40,57,80,104,129,153,170,180,185,186,190,199,210,222,230,229,227,227,226,225,219,206,185,156,125,104,90,77,68,62,54,43,35,36,44,47,46,47,54,66,85,110,134,154,167,173,176,179,183,193,204,211,215,214,214,216,215,213,209,194,166,139,116,94,77,66,62,55,43,32,30,35,39,41,41,42,49,62,83,111,134,152,165,172,175,180,185,194,202,205,206,210,211,211,214,213,201,182,158,132,105,82,67,60,53,41,26,18,21,27,31,35,35,33,41,60,
83,107,129,144,155,164,169,178,188,193,198,204,210,214,217,221,223,220,209,191,169,141,112,93,83,74,59,42,29,22,23,29,31,26,25,28,39,58,81,101,121,137,148,158,169,177,182,186,192,201,205,209,215,221,224,221,213,198,174,148,126};

//**************** Piano F#5 to B5: (Band 7)
const PROGMEM uint8_t PianoF5StartChunk900[] = // recorded at low sample rate
{132,140,147,157,156,156,157,153,148,144,142,132,118,113,110,101,86,79,80,89,102,110,110,114,120,139,151,152,147,152,165,168,174,177,164,149,147,144,127,117,112,99,79,50,37,40,53,67,82,81,106,153,176,191,191,176,178,194,203,199,189,171,165,163,138,112,86,77,64,46,31,39,53,69,88,72,70,103,139,168,184,176,173,192,211,220,224,201,193,199,198,171,145,119,104,83,56,50,49,56,69,57,35,34,52,90,124,136,136,157,170,188,202,194,177,175,174,172,169,148,134,112,89,87,89,92,107,99,72,54,58,66,92,106,114,131,141,153,181,197,192,193,191,197,198,182,162,136,96,81,67,57,66,75,60, // [Piano] [F#5 start chunk] [900 samples] 0dB (start at 50)
47,39,43,63,81,95,123,143,151,168,184,197,200,198,206,214,201,192,161,125,111,102,90,101,116,110,93,77,69,73,77,78,103,129,138,138,149,161,173,163,169,176,184,190,166,131,113,104,92,95,121,136,139,130,113,115,115,108,116,140,141,135,123,127,129,118,112,117,139,162,156,121,96,90,77,79,100,124,146,151,149,147,144,125,128,148,162,161,146,147,155,147,124,115,120,144,157,133,101,84,69,60,64,79,92,108,105,105,100,83,80,95,116,127,120,125,146,151,146,141,146,173,211,216,189,162,135,114,111,101,99,103,104,107,103,84,72,79,104,125,124,133,157,172,170,164,153,161,195,213,
200,166,124,99,91,73,56,61,64,74,80,69,60,62,90,118,129,140,168,193,206,209,199,191,203,221,226,204,164,127,111,90,70,55,43,47,58,68,62,59,75,107,126,135,155,182,200,212,209,198,194,211,224,220,184,143,121,103,83,58,43,41,55,62,59,52,56,78,102,115,134,158,181,202,211,192,179,191,210,223,203,166,136,120,97,71,44,35,41,54,59,55,51,66,88,100,113,135,158,188,209,199,183,180,199,221,220,184,153,130,116,86,53,32,28,34,41,45,39,44,61,77,93,113,131,170,203,208,192,184,197,232,245,232,200,179,162,134,98,67,52,44,50,57,53,47,57,71,89,103,122,157,203,224,220,199,200,225,251,
248,221,194,173,150,107,72,47,30,23,32,30,22,20,33,48,68,79,106,152,191,203,190,180,193,222,235,228,207,192,167,132,93,64,35,16,20,28,23,27,31,45,63,73,92,128,174,204,208,192,192,209,229,228,215,198,184,152,117,90,59,29,21,21,22,28,30,42,60,76,91,123,159,198,217,215,210,221,234,245,243,233,223,199,167,137,103,63,42,28,21,19,21,24,35,38,54,79,114,155,190,202,205,208,216,232,238,235,229,218,190,165,136,102,75,54,37,32,31,36,36,36,40,53,76,109,144,168,184,189,194,207,225,230,236,232,213,195,169,135,105,78,54,35,31,33,35,31,28,38,56,81,111,144,171,185,183,189,203,212,
220,222,212,197,177,150,122,94,60,36,25,25,29,25,22,29,46,66,92,127,165,193,201,205,218,228,237,246,244,232,212,185,160,132,98,64,40,32,35,31,21,22,33,47,66,92,129,169,186,193,204,212,221,236,244,240,225,205,185,160,125,86,53,34,31,26,16,10,15,25,41,55,88,133,162,176,191,200,210,228,245,254,251,237,223,206,178,141,97,70,60,55,41,28,21,22,30,35,51,90,122,145,164,172,176,188,208,223,228,219,210,200,182,148,105,67,49,41,30,19,12,13,23,27,37,67,102,133,158,174,176,184,198,219,231,231,225,222,210,187,148,103,74,61,49,38,26,19,27,34,36,52,85,119,154,175,184,187,196,211,
227,229,227,227,224,213,185,143,107,87,70,61,49,35,38,47,46,51,66,95,130,158,171,176,180,192,207,214,214,215,215,212,198,161,125,94,71,62,53,35,31,37,41,40,47,65,99,134,158,171,176,183,195,204,206,211,211,214,207,183,148,111,78,62,53,34,19,23,30,35,33,40,68,101,132,151,164,172,186,194,202,210,216,220,223,212,188,154,113,88,77,56,33,22,26,31,26,27,40,71,100,127,146,160,174,182,188,199,206,212,222,223,214,191,154,124};
const PROGMEM uint8_t PianoC6StartChunk900[] = // recorded at high sample rate
{129,123,119,119,118,120,127,134,135,131,130,128,124,117,109,103,108,113,110,105,104,113,123,133,141,152,162,168,171,171,168,163,158,151,137,122,110,99,86,69,56,51,55,65,71,71,72,83,104,122,135,148,163,177,186,193,197,197,196,194,191,183,171,154,136,113,85,64,57,60,62,62,65,76,93,110,127,141,155,171,190,205,212,212,208,202,194,182,167,152,135,105,68,34,17,13,12,11,12,21,40,64,87,106,124,146,172,199,221,233,239,242,241,234,222,211,197,175,139,98,66,49,38,26,13,5,5,19,39,58,74,93,123,155,183,204,222,234,241,241,241,241,241,231,207,169,133,109,94,78,59,39,25,22,27, // [Piano] [C6 start chunk] [900 samples] 0dB (start at 36)
34,39,49,68,96,125,152,175,197,211,217,219,225,232,235,223,193,157,126,103,86,69,50,35,28,31,36,40,43,56,78,104,127,148,170,187,195,196,196,200,208,205,187,158,127,101,84,66,47,26,15,15,22,27,32,43,63,86,108,132,157,182,197,201,201,208,217,219,210,189,161,135,115,101,84,66,52,50,56,63,69,78,93,111,128,145,167,188,206,211,211,215,224,230,228,213,189,160,137,120,103,80,59,48,45,44,40,41,51,66,82,100,122,152,176,188,193,197,204,213,217,211,192,166,140,121,102,77,54,38,33,29,26,27,35,47,61,75,95,121,148,167,178,184,192,204,217,224,219,200,178,160,140,115,86,66,54,47,
40,38,42,53,65,77,96,122,151,175,191,200,206,215,226,235,233,218,199,180,159,133,103,77,60,47,36,27,26,34,43,54,70,95,127,160,186,204,216,226,240,252,254,245,227,206,183,156,124,94,71,55,41,27,21,24,33,43,56,76,105,136,163,184,196,203,215,228,236,231,219,205,189,166,137,106,82,64,46,28,16,13,16,21,28,41,63,91,122,149,164,176,191,211,227,235,234,228,219,204,181,151,125,104,87,67,50,41,40,40,39,41,51,70,96,121,138,149,159,173,190,201,203,202,200,195,178,155,131,112,94,74,55,43,37,34,31,31,35,49,75,103,126,141,154,170,188,203,211,214,218,216,207,187,165,144,125,104,
85,69,60,56,55,52,51,60,80,108,134,152,166,182,199,213,219,221,222,222,215,198,177,156,136,115,92,71,56,48,45,42,39,40,53,76,101,118,132,147,164,178,188,194,200,205,203,193,176,158,139,117,96,74,56,48,46,46,43,42,50,71,96,118,135,152,170,186,196,199,202,204,204,195,181,163,144,122,98,71,48,31,25,23,22,22,29,49,74,98,119,140,164,188,203,213,221,230,235,235,226,212,193,171,145,116,84,59,43,34,25,13,9,18,38,62,84,105,131,156,177,192,204,216,226,232,232,225,214,200,183,159,130,102,82,69,56,42,29,25,33,49,64,82,103,127,149,166,180,193,207,217,223,222,216,207,194,175,
148,116,92,74,59,42,26,16,18,28,42,59,80,105,127,146,162,177,191,202,209,211,208,202,194,180,155,126,100,81,66,50,35,24,23,30,42,58,78,101,125,146,164,180,195,209,220,225,224,219,214,205,188,161,133,111,93,74,52,34,24,21,26,37,53,75,99,122,143,161,178,196,213,223,226,227,225,221,207,185,156,130,108,87,65,44,28,19,18,23,35,53,75,99,121,140,158,177,197,213,224,229,235,239,237,223,200,176,155,134,110,85,62,44,32,27,29,39,53,73,93,112,128,146,165,183,193,199,205,211,212,203,185,163,142,121,100,77,56,40,27,20,21,28,41,59,81,99,116,133,153,172,186,195,202,210,216,216,
204,186,166,147,126,104,81,61,44,33,27,28,36,52,72,91,107,123,141,160,176,185,193,202,212,217,214,203,190,175,157,139,119,99,81,66,55,48,47,55,69,85,97,110,125,144,159,170,177,185,195,203,205,200,191,179,165,149,132,113,93,76,62,51,45,46,56,70,84,96,110,128,145,158,167,176,188,199,203,203,197,187,175,161,146,127,105,86,68,54,40,35,39,49,60,69,81,97,115,130,142,153,168,182,194,201,202,197,189,178,166,150,130};

//**************** Piano C6 to F6: (Band 8)
const PROGMEM uint8_t PianoC6StartChunk625[] = // recorded at low sample rate // same chunk used for all notes in band
{122,111,104,111,110,103,111,126,138,153,166,171,171,164,157,143,122,105,87,64,52,56,69,71,75,100,126,143,165,182,192,197,196,193,187,170,146,115,77,57,61,62,65,83,108,130,150,173,199,212,211,203,192,173,151,123,71,27,13,12,11,21,50,84,111,138,175,211,233,240,242,232,215,197,160,102,59,41,24,7,5,27,56,78,113,158,195,221,237,241,240,241,231,189,137,104,83,56,29,22,30,38,53,86,128,165,196,214,219,226,235,221,176,129,98,74,47,29,31,38,42,60,96,129,160,186,197,195,202,208,185,143,104,78,52,24,14,22,29,41,69,101,134,171,196,201,206,218,215,188,148,117,96,70,51,52,63, // [Piano] [C6 start chunk] [625 samples] 0dB (start at 34)
72,90,115,140,169,199,211,212,222,230,220,187,148,122,96,63,47,45,40,44,63,86,115,155,184,192,199,211,217,201,164,131,104,69,41,32,27,27,40,59,80,113,150,174,184,196,215,225,208,177,151,118,79,56,46,38,42,58,75,102,142,177,196,206,218,234,231,205,179,147,106,71,50,34,25,34,47,67,103,150,188,210,226,245,254,241,214,182,141,97,65,45,26,22,33,48,72,113,155,186,200,214,233,233,216,195,165,122,84,58,33,15,14,21,33,59,99,141,166,183,210,231,234,226,211,179,138,107,81,54,40,40,39,44,66,103,134,150,166,189,203,202,200,186,153,122,96,67,45,36,32,31,39,70,110,137,155,180,
203,212,217,215,195,164,135,106,79,62,56,53,51,67,104,139,162,183,207,219,221,222,212,185,156,126,94,65,49,45,40,40,62,98,122,142,165,184,194,202,205,190,165,137,107,76,53,46,46,41,50,81,115,139,164,188,198,201,205,197,177,151,121,85,50,28,23,22,25,49,85,116,146,180,204,217,230,236,229,208,179,144,101,61,40,27,12,13,38,71,102,137,171,194,210,225,233,227,211,190,157,116,84,65,45,27,29,48,71,99,133,161,181,201,217,223,218,205,183,146,104,76,54,29,15,23,42,67,101,133,157,178,197,208,210,203,192,165,125,90,67,45,26,24,36,58,88,122,151,175,197,215,225,222,215,202,172,
132,102,76,46,26,22,31,53,85,119,148,173,198,219,226,226,222,203,167,129,98,67,38,21,18,29,53,85,117,145,171,199,219,229,237,238,218,185,154,123,88,56,35,27,34,53,81,109,132,159,184,197,204,212,206,180,149,120,89,59,35,21,22,35,59,89,113,138,166,187,198,210,217,207,182,154,125,93,63,40,28,29,45,72,98,120,146,172,186,198,212,217,206,187,164,138,110,82,62,50,48,63,84,103,123,148,167,177,190,202,203,193,176,155,131,103,78,58,46,47,64,84,102,125,149,164,177,194,203,201,190,172,152,125,95,70,49,35,40,56,69,88,112,133,150,169,189,201,201,191,176,156,129};

//**************** Piano F#6 to C7: (Band 9)
const PROGMEM uint8_t PianoF6StartChunk428[] = // recorded at low sample rate // same chunk used for all notes in band
{128,121,105,111,106,111,133,152,170,170,163,145,118,92,61,52,70,71,101,135,163,188,197,196,189,165,125,71,57,62,72,109,140,173,207,212,201,177,146,87,21,12,10,35,84,124,173,223,241,241,219,190,119,54,31,4,15,55,93,156,208,236,242,242,224,151,99,66,27,25,38,66,126,179,214,221,235,212,141,93,56,28,35,42,76,128,172,197,197,208,176,113,74,32,13,26,41,85,132,185,201,210,218,178,125,91,55,53,67,90,127,168,207,212,226,225,177,129,90,50,44,39,64,98,153,190,198,216,207,155,112,63,33,26,32,59,94,149,179,195,221,214,171,128,73,49,37,50,75,120,176,201,217,237,212,172, // [Piano] [F#6 start chunk] [428 samples] 0dB (start at 24)
118,66,39,25,41,67,126,187,217,243,254,220,174,108,61,31,22,40,73,134,185,206,231,230,200,155,93,53,19,14,26,59,121,165,194,230,233,216,169,115,75,43,39,40,66,120,149,176,202,203,191,145,103,62,37,32,33,71,125,154,191,212,218,201,156,115,75,57,52,57,104,152,182,215,222,221,192,148,103,61,45,39,49,98,131,164,189,202,202,171,130,84,50,45,41,65,115,151,187,200,205,192,157,113,58,26,22,25,69,116,163,204,223,237,224,186,133,70,37,16,13,57,102,155,193,218,234,223,195,147,91,62,31,30,60,100,148,181,209,224,215,190,135,82,49,17,24,55,101,146,177,203,211,201,173,115,
73,40,21,37,73,122,164,196,222,224,213,180,123,83,41,20,32,69,119,161,197,224,227,219,175,120,76,33,16,30,70,118,157,198,225,237,235,192,147,98,50,27,34,68,109,145,183,200,212,200,156,112,66,31,19,36,76,114,152,187,203,218,200,160,117,71,36,26,46,87,121,160,186,204,218,201,169,131,89,59,46,64,95,123,159,177,196,205,189,160,124,84,55,44,66,93,126,157,177,199,203,185,157,118,77,45,36,57,78,113,141,168,195,202,187,161};


//**************** Guitar F#2 to B2: (Band 1) (and E2 & F2 in band 0)
const PROGMEM uint8_t GuitarF2StartChunk1931[] = // recorded at low sample rate
{114,88,66,58,71,103,125,114,78,49,49,66,82,83,75,66,62,67,76,82,80,73,70,78,92,101,99,94,92,99,122,155,180,183,166,149,151,173,196,208,204,195,193,202,214,222,222,219,219,220,221,217,212,208,208,208,207,206,205,203,198,191,183,178,176,175,171,166,160,155,152,148,145,142,139,134,129,124,121,118,115,111,107,105,106,108,110,109,104,98,94,94,97,97,93,87,85,88,91,93,93,90,87,86,86,87,89,89,87,87,88,90,95,100,100,94,87,84,87,94,96,93,89,90,92,91,88,84,82,84,90,99,103,100,91,83,82,88,96,98,98,99,101,98,92,90,98,108,114,111,106,109,118,125,120,111,108,117,134,150,152,136, // [Guitar] [F#2 start chunk] [1931 samples] (start at 286 in file)
115,109,121,140,152,156,156,152,140,126,123,136,155,159,148,140,150,172,186,179,159,147,158,188,214,216,190,159,151,172,204,225,226,212,191,171,162,167,185,204,212,208,202,200,202,198,178,147,124,119,127,130,115,85,58,49,56,68,71,58,35,16,6,4,8,13,16,17,15,10,5,1,0,1,3,8,17,28,40,52,60,68,77,89,100,109,115,122,132,144,155,163,172,183,193,198,200,205,214,222,224,222,220,223,230,235,236,232,228,228,231,233,232,225,217,213,214,214,212,209,204,199,194,191,190,189,184,173,164,161,163,167,167,163,156,146,139,141,148,151,146,134,127,130,138,138,130,121,121,128,133,132,128,
122,116,113,114,119,126,130,130,125,115,105,104,116,131,131,117,102,103,115,122,113,97,89,91,100,105,103,94,83,76,76,81,90,97,95,81,60,47,56,81,104,103,79,55,53,70,89,92,81,72,79,96,110,109,94,77,72,83,104,127,138,132,111,93,90,107,133,149,145,127,109,104,115,132,144,147,146,152,161,166,161,151,144,150,167,187,201,205,198,186,176,174,179,186,191,194,192,188,188,196,208,215,215,209,202,193,179,163,150,143,139,134,127,118,107,93,77,62,52,49,49,49,44,35,26,22,24,25,20,9,0,0,5,8,8,8,13,20,30,40,52,62,68,70,74,84,97,109,117,125,134,144,154,167,177,182,180,180,188,203,216,
220,218,214,213,215,222,231,234,229,221,218,221,223,220,214,213,214,212,208,206,207,205,194,180,172,176,185,188,179,163,152,153,162,170,167,153,140,135,137,142,144,142,139,134,130,127,129,133,133,124,113,109,120,137,145,133,109,96,104,128,146,144,125,103,96,105,122,133,131,119,109,107,111,114,111,104,97,95,100,111,119,113,92,69,62,74,96,109,103,82,61,53,62,78,90,90,81,72,71,74,76,77,79,83,87,90,91,89,85,81,83,93,109,124,129,121,107,97,95,102,114,126,133,136,135,132,127,126,131,140,149,153,153,156,162,170,174,176,180,188,194,192,183,174,170,173,181,191,199,205,209,
210,211,209,205,199,194,191,187,182,180,178,171,154,134,119,110,102,92,82,74,68,63,59,57,53,42,30,24,24,24,18,8,1,0,2,4,7,12,20,27,31,33,36,44,57,70,77,80,85,96,113,128,134,134,137,148,163,175,180,180,182,190,201,209,214,217,219,217,212,211,218,227,232,225,213,206,212,222,227,219,206,197,199,210,214,203,182,165,166,180,193,194,181,164,152,150,155,163,164,156,142,134,136,146,153,149,137,124,119,125,134,136,128,115,109,117,133,143,139,124,111,107,115,128,137,138,129,116,106,105,112,123,130,129,122,114,109,107,105,103,105,111,120,122,114,99,85,79,80,84,88,91,92,87,76,
65,61,67,79,90,92,86,76,68,65,67,68,70,77,88,97,97,91,87,88,93,97,98,99,100,102,105,109,114,118,120,122,122,119,117,117,123,131,139,146,154,163,172,176,173,168,166,169,174,178,177,177,180,187,196,201,201,200,202,208,213,211,206,201,203,207,206,198,187,180,175,165,146,124,110,106,105,101,93,85,79,74,65,55,46,39,33,28,21,15,11,11,12,8,2,0,8,22,31,29,23,25,38,56,68,72,71,75,88,106,121,128,127,124,128,142,160,174,181,182,181,182,191,204,215,216,207,198,199,210,224,228,219,207,201,206,217,222,218,207,198,195,197,198,195,189,181,176,175,179,184,185,176,159,144,142,151,163,
166,160,151,142,138,135,134,133,133,135,135,132,124,116,114,118,125,130,132,133,133,128,120,113,113,121,130,132,129,121,114,111,113,118,122,125,124,121,116,111,105,105,110,120,126,124,114,100,89,82,81,83,88,93,95,93,87,82,80,81,81,79,75,73,74,78,79,76,73,75,84,95,98,92,83,78,81,88,93,96,100,105,110,110,107,104,101,100,100,102,108,118,129,139,144,144,144,147,154,159,160,158,159,164,170,173,174,177,183,187,189,189,191,198,206,211,210,206,205,209,215,217,209,195,182,176,174,169,155,135,120,114,113,115,114,106,90,73,61,58,60,59,51,36,23,19,23,28,25,13,2,1,12,28,36,34,28,
27,35,48,62,74,80,82,84,91,103,116,125,129,130,134,144,159,172,177,176,175,182,194,203,205,201,198,199,202,206,210,214,217,216,211,204,199,198,200,203,205,203,199,193,186,180,176,176,180,184,184,179,168,157,151,151,156,162,165,163,158,149,140,131,127,128,133,137,139,136,130,122,116,115,120,127,135,139,136,128,117,111,113,121,127,127,122,119,118,118,116,112,110,111,115,120,124,123,120,115,112,111,109,107,104,101,96,90,87,90,97,101,97,88,81,78,78,79,78,77,77,78,81,84,84,82,78,74,73,74,79,86,92,95,92,88,88,93,99,101,96,89,86,90,98,106,110,110,112,119,129,138,142,140,137,
137,141,150,162,170,171,165,160,162,173,184,188,186,183,187,196,208,215,215,211,208,209,213,214,210,201,189,177,169,166,166,161,146,127,113,110,113,112,101,85,70,62,58,55,49,41,33,27,22,17,11,6,3,2,5,10,17,26,31,32,31,32,40,55,70,82,88,92,95,100,106,113,121,133,146,157,163,165,164,165,169,178,189,197,202,202,200,198,196,198,202,208,212,211,207,201,197,195,194,194,196,199,200,197,189,178,169,168,173,180,182,177,169,161,158,158,159,160,160,158,153,144,137,133,132,132,132,132,133,134,133,129,123,117,116,120,127,134,136,133,127,122,119,118,118,118,119,118,116,115,117,121,
125,126,123,119,118,121,123,122,114,106,103,107,114,118,116,109,100,92,89,89,91,91,87,84,85,89,94,94,87,76,69,69,76,83,83,79,75,77,86,94,96,92,86,80,78,79,83,87,89,89,88,91,97,105,110,109,107,110,118,130,137,137,134,133,139,149,158,162,161,160,160,163,168,173,180,188,195,200,203,206,209,211,211,209,207,206,207,204,197,187,177,168,163,160,155,145,133,122,114,107,100,93,86,81,74,64,53,43,37,33,30,26,21,16,10,6,4,6,13,23,32,37,37,36,39,47,59,70,78,86,95,103,107,109,112,120,132,146,156,162,163,162,163,167,175,185,194,201,204,203,199,196,196,199,202,204,204,204,203,201,
197,194,193,194,195,194,191,185,179,175,172,171,171,173,175,177,174,166,158,152,151,151,150,146,142,139,138,138,137,134,130,125,122,121,122,125,127,127,125,125,129,133,134,128,118,110,109,114,119,121,121,120,123,127,129,126,120,114,112,114,118,123,126,125,121,117,114,115,115,113,106,98,93,95,99,102,101,97,94,93,94,92,86,78,73,73,77,83,88,90,90,87,84,82,82,82,83,82,80,78,77,79,82,85,87,87,90,95,100,103,104,106,110,116,123,129,133,136,138,140,142,145,149,154,160,165,168,171,175,182,189,196,201,204,208,210,209,207,206,207,208,208,203,194,184,175,170,166,161,153,144,135,
126,117,107,98,91,87,83,77,69,58,46,36,29,27,26,25,22,17,11,8,10,17,26,33,36,38,42,48,55,62,69,78,88,98,104,108,111,117,125,134,142,148,154,160,165,169,172,176,182,190,196,199,200,199,197,194,194,196,199,203,203,201,197,195,195,194,190,184,180,180,181,182,179,174,170,170,171,171,169,165,160,155,152,151,150,148,145,141,138,136,135,134,130,125,120,120,126,132,135,133,128,124,123,124,125,124,122,120,120,123,125,126,124,121,120,120,121,122,123,123,122,121,122,124,127,128,126,121,117,114,113,114,114,113,111,109,108,107,104,100,96,94,92,91,90,89,88,87,86,86,86,88,90,90,89,
86,83,81,79,78,78,78,79,80,79,78,78,79,83,90,96,100,101,101,104,108,114,119,123,125,127,131,135,139,143,146,151,156,162,169,174,179,183,186,191,197,203,207,208,205,203,202,204,206,204,198,189,181,174,168,163,157,151,144,135};
const PROGMEM uint8_t GuitarC3StartChunk1931[] = // recorded at high sample rate
{112,96,78,59,44,41,53,79,111,135,139,121,89,56,36,32,44,63,82,94,96,89,76,64,56,54,59,68,78,83,83,80,77,77,80,85,90,93,99,112,133,156,174,178,166,143,121,109,114,134,160,182,194,195,187,177,172,173,179,188,196,199,198,195,191,190,193,198,203,206,205,201,195,191,189,189,191,193,194,192,189,185,183,181,181,181,181,180,178,176,174,171,167,163,160,157,155,154,154,153,153,153,152,151,150,148,146,144,141,139,138,137,137,138,139,139,138,136,134,132,132,132,132,131,129,126,123,120,119,120,122,123,123,120,117,114,112,113,115,118,119,117,112,104,95,87,84,85,91,97,100,99,97,94, // [Guitar] [C3 start chunk] [1931 samples] (start at 418 in file)
93,93,90,85,78,73,73,81,92,99,97,86,69,55,52,63,85,113,135,147,145,133,116,103,97,100,110,125,140,151,157,157,154,150,148,147,149,151,153,157,161,168,175,180,180,173,163,153,150,157,175,197,215,221,211,186,150,115,91,76,71,74,85,98,110,114,108,95,79,66,60,60,60,58,52,47,43,44,47,48,44,35,25,18,19,28,41,50,50,40,23,7,0,3,19,42,66,84,91,89,81,73,70,72,80,89,98,104,107,109,111,115,121,129,138,144,148,149,150,152,156,160,165,170,174,177,180,182,184,186,187,188,187,187,186,187,189,191,193,194,194,193,193,192,191,190,190,191,193,195,196,194,190,183,176,171,170,174,180,188,
193,196,194,189,183,178,175,174,174,174,176,178,180,182,184,184,183,182,179,174,168,164,165,171,179,184,183,174,161,152,149,154,163,171,173,169,160,152,146,144,141,136,127,117,112,114,122,133,140,139,129,113,97,86,85,93,106,120,127,124,111,91,72,59,56,64,78,95,107,113,114,112,110,109,110,112,113,114,115,120,127,137,146,150,148,142,135,132,136,146,159,171,178,181,176,166,152,140,135,141,159,183,207,223,225,215,196,177,160,148,138,128,118,108,101,100,103,109,112,108,97,83,70,64,66,72,78,78,69,52,35,22,18,24,36,49,58,58,49,34,18,5,0,1,8,18,26,33,39,44,49,54,59,64,68,71,
73,75,77,81,86,91,97,103,109,114,119,123,128,132,135,139,143,146,150,154,159,164,169,173,176,177,176,175,175,176,179,183,187,189,190,187,183,180,179,181,185,190,193,194,193,191,188,183,179,175,174,176,179,181,181,180,180,181,185,188,188,183,175,167,162,165,172,180,185,185,179,173,170,171,175,179,179,175,169,163,162,165,171,175,175,171,163,156,152,154,160,168,171,167,155,139,124,116,118,126,137,145,146,138,123,107,97,94,100,110,118,121,118,110,103,98,95,92,87,81,74,70,70,75,83,91,97,100,100,99,97,97,99,105,114,124,132,134,130,121,112,109,114,129,147,163,172,172,163,152,
142,137,137,142,147,153,159,167,179,192,204,210,207,196,179,162,149,142,142,143,143,137,124,107,92,83,83,89,99,105,105,96,79,60,44,33,30,32,39,45,50,50,46,39,30,22,16,11,7,4,2,1,2,6,11,18,26,32,37,42,46,50,53,56,59,61,65,70,76,82,89,96,102,108,112,115,116,118,123,129,137,144,149,153,155,157,159,162,164,166,169,172,176,179,180,179,177,176,176,179,183,185,184,182,180,181,185,190,193,191,185,176,169,167,171,179,187,191,189,184,179,176,177,180,182,181,176,169,164,165,171,181,189,191,186,177,167,161,162,169,178,186,188,182,172,161,155,156,165,177,186,187,178,164,150,141,
139,143,148,152,151,147,142,138,134,130,126,121,117,113,112,113,115,118,121,122,120,115,105,94,82,75,74,79,88,95,97,92,82,73,69,74,87,104,118,124,123,116,108,102,101,106,113,120,126,132,137,143,151,157,161,159,150,139,129,126,131,145,162,178,189,192,189,184,179,179,182,188,192,191,184,170,154,137,125,118,116,119,122,124,122,117,108,99,88,79,70,62,56,54,54,56,58,58,56,51,45,38,32,26,20,15,11,9,7,7,8,10,15,21,28,34,37,39,40,41,45,49,54,59,63,68,74,81,88,92,96,97,99,103,108,116,123,128,131,134,138,143,150,155,157,157,154,154,157,165,173,179,181,178,172,167,166,170,175,
181,184,183,181,179,179,181,183,183,179,174,169,168,171,179,188,195,196,190,179,168,161,160,167,178,188,192,188,178,167,161,162,169,178,185,187,184,177,171,168,168,170,173,176,178,179,180,179,177,174,169,163,158,153,150,149,149,150,153,156,156,151,142,129,117,110,109,116,127,136,140,136,126,113,102,96,97,100,104,104,99,90,81,75,73,75,79,83,87,89,92,97,102,107,110,109,104,98,93,94,100,113,128,142,150,152,148,140,131,127,127,132,139,147,152,154,154,156,159,166,174,183,190,194,196,195,191,184,174,164,154,148,144,143,143,143,141,136,130,123,114,104,94,85,79,76,74,73,71,70,
68,66,63,60,55,48,40,34,28,25,22,18,13,9,8,11,17,25,31,33,32,30,30,32,37,44,49,53,57,61,67,75,82,87,89,88,87,88,95,105,116,126,131,132,131,130,132,137,143,149,155,158,161,163,166,168,170,170,169,167,165,164,167,172,179,185,187,184,176,167,161,161,167,176,186,191,190,184,176,169,166,168,172,178,182,183,182,178,174,170,167,167,168,171,175,178,178,176,174,172,171,172,173,175,176,177,178,181,185,187,185,179,170,160,152,151,156,165,173,176,172,162,149,137,130,129,132,136,138,137,133,128,125,124,125,127,128,125,120,113,107,102,99,96,92,87,81,75,73,76,83,93,102,108,107,102,
93,85,81,83,90,101,111,119,125,127,129,129,129,130,131,133,136,138,138,136,134,133,134,139,147,156,167,176,183,189,191,190,186,180,173,167,163,161,160,160,159,157,155,151,146,139,129,117,106,98,94,92,91,88,83,77,72,70,70,72,73,69,61,50,39,31,26,23,21,19,17,15,16,20,25,30,31,30,26,24,23,27,35,45,54,61,64,64,63,62,65,71,79,87,95,100,103,105,109,113,118,122,124,124,126,131,140,151,161,168,169,163,155,149,147,152,161,172,180,182,180,173,166,161,159,161,167,174,180,183,182,177,171,166,164,166,171,176,179,180,179,177,175,173,173,172,170,168,166,166,168,173,177,178,175,169,
162,158,159,166,177,188,195,195,188,177,166,160,158,162,168,174,176,175,171,166,162,159,156,153,148,143,138,135,134,134,136,136,136,133,129,126,126,128,132,134,132,125,114,102,90,82,78,80,85,93,100,105,106,103,97,91,87,85,87,91,96,101,104,107,111,115,119,124,129,133,137,139,140,139,136,131,127,126,127,132,139,148,158,167,175,182,186,187,184,179,173,169,168,170,173,175,174,170,164,159,155,153,151,146,138,127,114,104,97,93,91,90,88,87,85,83,82,81,77,71,62,51,40,30,25,23,26,30,32,31,27,21,16,15,18,24,30,33,35,35,36,39,44,49,54,56,56,57,61,68,79,90,98,101,99,95,91,91,96,
106,119,132,142,147,147,145,141,139,140,146,154,161,167,168,165,161,157,156,158,163,167,169,169,168,167,167,169,171,171,169,166,163,162,165,171,178,182,182,178,170,163,160,162,168,175,179,179,173,165,158,154,155,160,168,175,180,182,181,179,177,176,176,175,173,171,168,168,169,171,175,177,176,173,168,162,157,153,151,150,149,146,142,136,131,128,127,130,136,142,147,149,145,137,126,116,107,101,98,97,97,98,98,99,99,99,98,97,97,96,96,96,95,94,93,92,93,96,102,110,119,126,132,136,139,139,137,134,128,123,120,120,125,133,142,150,156,160,163,167,171,176,179,181,180,177,173,170,
169,169,171,173,173,171,167,162,158,153,148,141,131,119,106,96,91,92,96,102,104,102,94,84,74,67,63,60,57,51,44,37,32,30,29,30,28,25,21,17,17,20,27,34,40,42,40,35,30,28,30,38,50,63,74,81,82,79,74,71,72,78,87,98,107,113,116,117,118,121,127,133,140,145,148,148,149,150,152,155,156,155,154,152,152,156,162,169,173,174,170,163,156,153,154,159,166,172,176,175,171,166,163,162,165,170,174,177,177,174,169,165,162,160,159,159,159,161,163,168,172,177,179,180,177,173,169,167,166,168,172,175,178,178,176,171,167,164,165,167,170,172,169,164,155,145,137,132,130,131,134,138,143,146,148,
148,145,139,132,126,121,118,116,113,110,106,102,99,98,99,100,102,104,105,105,103,101,97,92,88,87,88,94,103,113,123,129,133,133,133,131,131,131,131,131,130,128,128,130,134,141,149,156,162,166,168,170,172,176,179,180,179,176,171,168,168,171,176,180,181,178,170,161,152,144,138,132};

//**************** Guitar C3 to F3: (Band 2)
const PROGMEM uint8_t GuitarC3StartChunk1366[] = // recorded at low sample rate
{122,102,76,50,40,60,104,137,131,89,46,32,48,76,95,93,78,61,54,60,74,83,82,77,77,83,90,95,110,140,170,178,155,122,109,128,164,191,195,183,172,174,185,196,199,195,191,192,199,205,205,199,192,188,190,193,193,189,184,181,181,181,180,178,174,170,165,159,156,154,154,153,152,152,150,148,144,141,138,137,138,139,139,137,133,132,132,132,130,125,121,119,121,123,122,118,114,112,115,119,118,111,99,88,84,89,98,100,97,93,93,89,80,72,76,91,100,89,66,52,63,97,132,147,137,115,99,99,115,136,152,158,154,149,147,149,152,157,164,175,181,176,161,150,157,183,212,220,197,149,103,78,71, // [Guitar] [C3 start chunk] [1366 samples] (start at 295 in file)
81,100,113,109,89,68,60,60,58,50,44,45,48,43,30,19,21,37,51,46,23,3,2,24,58,84,91,82,72,71,81,94,104,108,110,116,126,138,146,149,151,154,161,168,174,178,182,185,187,188,187,186,187,190,193,194,193,192,191,190,190,193,195,195,188,178,171,171,179,190,195,193,186,179,175,174,175,177,181,183,184,183,180,173,166,165,174,183,182,167,152,150,162,172,171,160,149,144,140,129,116,112,121,136,140,126,103,86,88,104,122,126,109,81,59,58,75,97,112,114,111,109,111,113,114,117,126,140,150,147,138,132,140,156,173,181,176,159,141,135,152,187,217,225,208,181,158,142,128,113,102,100,
107,112,103,84,67,65,74,79,69,45,24,19,31,50,59,50,29,8,0,5,17,29,38,45,52,60,66,71,73,77,82,89,96,105,113,120,126,132,137,142,147,153,159,166,173,176,176,175,175,179,184,189,189,185,180,179,185,191,194,193,189,183,177,174,176,180,181,180,181,186,188,182,170,162,167,179,186,182,173,169,174,179,177,168,162,165,172,176,169,158,152,156,166,171,160,138,120,117,130,144,145,130,108,95,99,112,121,117,107,98,94,89,80,71,70,78,90,98,100,99,97,99,107,121,132,133,122,110,112,131,157,172,169,154,140,137,142,150,158,171,188,205,210,197,173,151,142,143,143,132,110,89,82,90,103,
106,91,65,42,30,32,42,49,49,42,30,19,11,6,2,1,4,11,21,31,38,44,50,55,58,62,69,77,86,95,105,111,115,117,123,132,143,150,154,157,160,164,167,171,176,180,179,176,176,180,184,184,181,181,186,192,190,180,169,168,178,188,190,184,177,177,181,182,175,166,165,175,187,190,181,167,161,167,181,188,182,166,155,159,174,187,183,165,146,139,144,151,151,145,138,133,127,121,115,112,114,118,121,121,115,100,84,74,77,89,97,93,78,69,76,97,118,125,118,106,101,106,116,126,133,142,152,160,159,146,131,126,139,163,184,192,188,180,179,185,192,189,174,150,129,118,117,122,124,119,107,93,79,67,
58,54,55,58,57,52,43,34,25,18,12,8,7,8,13,21,30,37,39,41,45,51,58,65,73,82,90,95,98,102,110,120,128,132,137,145,154,157,156,154,160,171,180,180,172,166,169,177,183,183,180,179,182,183,179,171,168,174,187,196,193,179,164,160,170,185,192,183,168,160,167,180,187,183,174,168,168,172,176,179,180,179,175,168,160,153,149,149,151,155,156,147,131,114,109,117,133,140,133,116,100,96,101,104,100,88,76,73,77,83,88,91,98,105,110,107,99,93,97,114,135,150,151,142,130,126,132,142,151,154,155,160,170,182,191,196,195,187,175,160,149,144,143,143,139,131,120,107,93,82,76,73,72,69,67,
63,58,49,39,30,24,20,14,9,10,18,29,33,32,30,33,41,50,55,60,69,80,87,88,87,90,102,118,129,132,130,132,138,147,155,160,163,166,169,170,168,165,165,171,181,187,184,172,161,162,174,187,191,184,173,166,169,176,182,183,179,173,168,167,170,175,178,177,173,171,172,174,176,177,180,185,187,180,166,154,151,161,173,175,164,146,132,129,134,138,136,130,125,124,127,127,121,112,104,99,94,88,79,73,76,88,102,108,103,91,82,83,95,109,121,127,129,129,130,132,135,138,137,134,133,138,149,163,177,186,191,189,182,172,164,161,160,160,157,153,146,134,119,104,95,92,89,83,75,70,71,73,69,57,41,
30,24,21,18,15,17,24,30,31,27,23,26,37,51,61,64,63,63,68,80,92,100,104,108,114,120,124,125,130,142,158,168,166,156,147,150,163,177,182,177,167,160,160,167,177,183,181,173,166,165,170,177,180,179,175,173,172,170,167,166,169,175,178,174,164,158,163,177,191,195,186,170,159,160,168,175,175,170,164,159,155,149,142,136,133,135,136,135,130,126,127,132,133,127,112,94,81,78,84,95,104,106,100,92,86,87,92,99,104,109,114,120,127,133,138,140,138,132,127,126,132,142,156,169,180,186,186,180,172,168,171,175,174,168,160,154,152,146,133,116,102,93,91,89,87,84,83,80,74,62,46,32,24,
25,30,32,27,19,15,19,27,33,35,35,40,47,54,56,57,63,76,92,101,100,93,90,98,114,132,145,148,144,139,141,149,161,168,166,160,156,158,164,169,169,167,167,170,171,168,164,162,168,177,183,179,168,160,162,171,179,178,168,157,153,160,171,179,182,180,177,176,175,173,169,168,170,174,177,174,167,159,153,151,149,145,138,131,127,130,138,146,148,141,127,112,102,98,97,98,99,99,99,98,97,96,96,95,93,92,95,102,114,125,133,138,139,136,129,122,120,126,138,150,158,163,168,174,180,181,177,172,169,170,172,173,170,163,157,150,141,126,108,94,91,97,103,102,91,76,66,61,57,49,38,31,29,29,27,
21,17,19,28,38,42,38,31,28,35,50,69,80,81,75,71,74,87,102,112,116,117,121,129,139,146,148,149,151,155,156,154,152,154,162,171,174,168,158,153,156,165,174,176,171,164,162,166,174,178,175,169,163,160,159,159,161,165,172,178,180,177,171,167,167,171,176,178,175,169,164,165,170,172,167,155,142,133,130,133,139,144,148,147,141,131,123,118,115,111,105,100,98,99,102,104,105,103,99,93,87,88,96,109,123,131,133,132,131,131,131,130,128,129,135,145,156,164,168,170,174,179,180,176,170,168,172,179,181,175,163,150,140,132};
const PROGMEM uint8_t GuitarF3StartChunk1366[] = // recorded at high sample rate
{133,128,122,117,113,106,96,79,60,47,46,64,94,124,136,124,95,63,43,42,59,81,99,102,92,73,57,49,53,66,82,97,108,117,125,135,142,141,131,114,101,100,113,134,153,160,154,141,130,128,138,155,173,185,188,182,172,163,161,167,177,188,194,194,189,181,175,171,171,174,177,181,184,186,187,186,183,179,174,171,168,167,168,169,170,170,168,166,162,159,156,154,152,150,148,147,147,148,149,148,146,141,136,132,130,130,133,135,135,132,127,121,117,117,121,126,129,128,124,117,112,111,113,118,122,124,123,121,125,137,155,174,183,178,159,135,118,116,129,151,171,181,178,165,151,144,145, // [Guitar] [F#3 start chunk] [1366 samples] (start at 383 in file)
154,166,173,173,167,160,157,154,143,115,71,28,3,6,39,88,130,145,127,85,39,9,3,17,39,57,62,51,30,11,1,0,6,11,15,19,29,47,67,80,81,69,49,34,31,41,60,79,90,90,81,71,66,69,81,96,109,115,114,110,106,108,115,125,133,138,140,140,139,139,138,137,136,136,137,142,149,156,162,165,163,160,157,157,160,165,171,173,172,168,164,163,163,165,167,168,167,165,164,165,167,170,172,172,169,165,162,161,163,166,169,169,165,159,153,151,153,158,163,167,166,161,154,149,148,152,159,162,159,151,144,145,159,181,203,217,215,199,177,159,152,159,174,191,202,202,194,183,177,177,183,188,187,183,182,
189,202,210,202,169,119,68,36,31,54,91,127,145,138,112,77,50,40,45,59,69,70,63,52,45,42,40,35,25,16,14,25,47,72,91,97,88,72,57,50,55,68,83,94,97,93,86,80,79,80,85,90,95,101,107,114,120,122,121,119,118,120,126,132,138,139,136,129,122,119,121,128,137,146,151,152,149,146,143,143,145,148,152,155,156,156,154,152,149,148,148,148,150,152,155,156,158,157,156,153,150,149,150,154,158,160,159,155,148,142,138,140,146,154,159,159,153,145,140,141,148,155,157,151,141,131,131,142,163,185,201,205,196,180,163,154,157,168,181,189,188,183,179,181,185,187,183,173,165,168,184,208,226,
225,200,154,103,64,48,56,81,111,131,133,118,95,74,63,63,67,70,70,67,65,63,61,54,42,27,16,14,23,41,63,79,86,83,74,65,60,62,69,78,86,90,91,88,84,79,76,75,78,85,95,105,113,118,118,115,112,111,114,121,128,135,137,136,131,126,121,120,122,127,135,143,149,152,152,149,144,141,141,144,149,154,158,158,156,152,149,146,145,146,149,152,156,158,158,156,153,149,147,147,150,155,160,162,159,151,143,137,137,142,150,155,156,151,145,142,145,151,156,156,149,137,127,125,133,151,173,190,197,192,178,166,160,164,172,178,179,176,173,177,185,192,192,182,167,155,157,173,200,224,234,222,188,
142,99,73,67,79,99,116,122,117,105,92,82,76,73,71,70,71,74,76,74,65,50,33,20,16,21,33,50,65,74,76,72,65,60,59,64,73,82,89,90,86,78,70,65,64,69,78,89,98,105,107,107,105,105,106,110,115,121,126,129,131,129,126,121,117,117,120,127,136,145,149,150,147,143,140,139,142,147,151,155,155,155,153,151,149,148,148,149,151,153,155,157,157,155,152,148,147,149,154,159,162,160,153,146,141,142,146,151,153,150,145,141,142,148,156,162,162,154,140,126,119,125,142,163,181,188,185,176,168,167,170,174,175,171,167,168,175,186,194,194,183,166,151,148,160,185,214,233,233,211,175,135,104,
89,90,98,107,112,112,108,102,96,89,82,77,74,75,79,83,83,77,65,48,33,22,20,27,40,55,67,72,69,63,57,57,62,73,83,90,90,85,75,65,58,58,63,71,81,89,95,99,100,101,101,101,102,106,111,118,125,129,128,124,118,113,112,115,123,133,142,148,149,147,143,141,141,143,146,150,153,154,155,155,154,154,153,153,153,153,155,157,159,159,158,154,151,150,153,159,163,164,160,153,148,146,149,154,158,157,152,144,137,137,144,157,168,171,163,147,129,119,121,134,153,168,176,177,175,173,174,176,175,171,165,162,165,174,187,197,198,187,168,150,141,149,172,202,225,233,222,197,166,139,120,111,107,
106,106,106,106,106,105,101,95,86,80,77,80,86,92,92,83,66,47,31,23,25,36,49,59,64,62,58,54,56,62,72,82,87,87,80,71,63,57,55,58,63,69,76,83,89,93,94,94,92,92,95,101,109,117,121,121,117,111,106,105,109,116,125,133,139,141,141,140,138,138,139,141,143,145,147,149,150,152,153,153,152,152,152,154,157,160,159,156,151,148,149,154,160,164,163,158,151,146,145,150,158,164,165,158,146,135,131,138,153,168,175,170,156,139,127,125,131,142,153,161,167,172,178,183,185,182,174,164,158,159,170,187,202,206,197,177,155,142,145,163,188,212,225,225,213,195,175,156,140,126,115,107,104,
105,110,114,114,108,97,86,79,80,87,96,101,96,83,64,46,35,32,36,43,49,53,53,51,51,55,62,70,78,82,81,77,70,64,59,56,55,57,61,67,74,80,84,86,86,85,86,89,96,103,111,115,115,113,108,105,104,106,112,119,126,131,135,137,139,139,140,141,141,141,141,142,145,148,151,154,154,152,152,152,155,159,161,160,156,151,147,147,151,157,162,163,159,151,144,142,146,156,165,168,162,149,136,129,133,145,158,167,167,160,149,139,134,133,136,140,145,151,160,171,182,189,188,179,166,156,155,165,183,200,208,203,187,167,152,148,157,174,193,208,216,217,212,203,190,173,153,134,118,109,109,114,120,
122,117,108,96,88,87,91,98,103,102,95,82,67,55,48,44,44,44,45,45,47,50,54,61,67,73,76,77,76,72,66,61,56,53,53,56,61,67,72,76,77,77,77,79,82,88,96,102,106,108,106,103,101,101,103,107,112,118,123,128,131,133,136,139,140,140,139,138,138,140,144,148,150,150,149,149,151,155,159,161,161,157,151,146,144,148,154,160,162,158,150,141,138,143,153,162,166,162,151,139,132,132,139,149,156,159,157,152,147,142,139,136,132,131,134,144,159,175,187,189,181,168,156,153,160,176,193,203,203,193,177,164,156,156,164,175,188,201,211,217,218,212,198,179,157,137};

//**************** Guitar F#3 to B3: (Band 3)
const PROGMEM uint8_t GuitarF3StartChunk1145[] = // recorded at low sample rate
{125,120,114,115,125,136,140,135,125,118,120,130,139,141,131,118,112,115,126,138,145,141,128,116,114,122,131,136,133,126,119,120,126,133,134,130,124,122,126,131,132,128,123,122,126,131,133,132,131,130,130,131,132,133,134,134,133,133,134,136,137,136,134,133,134,136,138,136,131,127,126,131,136,136,130,122,115,108,93,67,46,52,91,130,130,90,49,42,68,97,101,79,56,50,65,88,106,118,131,142,137,116,99,108,137,158,154,136,128,142,168,186,186,172,162,165,179,193,194,185,175,171,173,178,183,186,187,184,178,172,168,167,168,170,169,165,160,156,153,150,148,147,148,149,146, // [Guitar] [F#3 start chunk] [1145 samples] (start at 196 in file)
139,132,129,132,135,134,127,119,117,122,128,128,121,113,111,116,123,123,121,128,151,177,182,157,126,116,136,167,181,171,151,143,153,168,173,166,158,154,135,81,21,1,41,110,145,115,50,6,9,39,61,53,25,3,1,9,15,22,43,70,83,68,41,30,46,74,91,86,71,66,78,100,114,114,107,108,119,131,139,140,139,139,138,136,137,143,153,162,164,161,157,158,166,172,172,167,163,164,167,168,166,164,165,169,172,171,165,162,163,167,169,164,156,151,154,162,167,164,154,148,152,160,161,150,143,158,190,215,212,185,158,154,173,195,203,192,179,177,185,188,182,186,204,209,171,99,40,34,77,129,145,
115,67,41,47,66,70,58,46,42,37,24,14,24,56,88,96,78,56,51,66,87,97,92,82,79,82,89,96,105,115,121,121,118,119,127,136,139,133,123,119,126,139,150,152,148,143,143,147,152,156,156,153,150,148,148,150,153,156,158,156,152,149,150,156,160,159,151,142,138,145,156,160,152,142,141,151,157,149,133,131,153,185,204,199,176,156,157,175,188,187,180,181,187,183,169,166,190,222,223,177,105,55,53,88,125,133,107,76,62,65,70,68,65,62,55,38,19,14,32,61,83,84,72,61,63,74,85,91,89,83,77,75,80,93,107,117,118,114,111,116,127,135,137,131,123,120,123,133,144,151,152,147,142,142,148,155,
158,156,151,147,145,148,152,157,158,155,150,147,149,155,161,159,149,139,137,146,155,154,146,142,148,156,154,139,126,130,154,183,197,187,168,160,169,178,177,173,179,190,191,173,155,162,196,229,228,183,118,73,70,95,118,120,104,87,76,72,70,72,75,74,59,36,18,18,35,58,74,75,67,59,62,73,86,90,84,72,64,66,78,93,104,107,106,105,108,115,123,128,131,128,121,117,119,130,143,150,149,143,139,141,148,154,155,154,151,149,148,149,152,155,157,155,151,147,150,157,162,158,147,141,145,152,152,146,141,146,157,163,153,133,119,128,157,182,187,176,167,169,175,173,167,171,185,195,186,
163,147,160,198,231,229,187,130,94,89,102,111,111,104,95,86,77,74,78,83,80,65,41,24,21,35,56,70,69,60,56,64,79,90,88,76,62,57,64,77,89,97,100,101,101,103,108,118,127,129,123,115,112,119,132,144,149,146,142,141,144,149,153,155,155,154,153,153,153,156,158,159,156,151,151,157,164,162,153,146,148,155,158,151,140,137,148,165,170,154,129,118,132,158,174,177,174,174,176,172,164,163,175,193,198,182,154,141,161,202,231,225,189,147,119,108,106,106,106,106,103,94,83,77,82,91,91,74,47,26,24,39,57,64,60,54,58,70,84,88,80,67,57,56,61,70,80,89,94,94,92,94,102,113,121,120,113,
106,107,116,129,138,141,140,138,139,141,144,147,149,151,153,153,152,153,157,160,158,151,148,153,161,164,157,148,145,153,163,164,150,134,133,151,171,172,153,131,125,136,151,163,170,178,184,182,170,158,161,181,203,204,179,150,143,167,203,225,221,198,169,145,125,110,104,107,113,113,101,85,79,86,98,98,80,53,35,33,42,51,53,51,53,61,73,81,80,73,64,57,55,58,65,75,83,86,86,86,91,101,111,116,113,107,104,107,116,126,133,137,139,140,141,141,141,143,147,152,154,152,152,155,160,161,155,148,147,154,162,162,153,143,143,156,167,164,145,130,133,152,167,165,151,138,133,136,142,
150,163,179,189,184,167,154,162,187,206,202,177,153,150,169,196,214,217,209,192,168,140,117,108,113,121,119,105,91,87,94,103,101,85,65,50,44,44,44,45,48,55,64,73,77,76,71,63,56,53,56,63,71,76,77,77,80,87,98,105,108,105,101,101,106,113,121,128,132,136,139,141,139,138,140,146,150,150,149,152,157,161,160,152,145,146,154,162,159,147,138,144,158,166,158,141,131,136,150,158,157,150,143,138,134,131,138,157,179,189,180,161,152,166,190,204,197,176,159,156,167,185,202,215,218,207,182,151,128,119,122,126,121,109,99,96,100,105,103,94,79,64,53,46,44,45,50,56,63,70,75,76,71,
62,55,52,56,62,69,72,72,72,76,85,94,101,103,101,99,100,105,112,118,123,129,136,141,141,138,137,141,146,148,148,148,153,160,164,161,152,147,151,160,163,155,143,140,149,161,163,154,142,137,140,147,151,152,153,151,143,130,122,130,154,178,185,173,157,153,167,188,199,195,180,164,156,161,175,195,213,220,211,189,162,141,131,130,128};
const PROGMEM uint8_t GuitarC4StartChunk1145[] = // recorded at high sample rate
{125,125,130,136,139,135,125,115,112,118,131,141,145,140,132,124,120,122,127,133,135,134,132,131,135,141,145,145,140,130,120,114,117,126,137,143,140,132,125,123,126,132,135,132,125,120,121,131,142,149,146,136,127,124,127,134,139,141,140,138,137,135,133,131,131,131,132,134,135,134,130,121,107,90,72,57,48,48,60,81,103,112,100,72,44,30,37,58,81,95,98,96,95,102,115,125,124,111,95,86,94,116,142,157,157,146,136,135,146,161,172,171,164,158,160,170,181,187,186,181,175,172,174,178,181,183,182,179,177,177,181,185,187,183,174,164,158,157,161,165,167,166,161,154,149,148, // [Guitar] [C4 start chunk] [1145 samples] (start at 310 in file)
151,153,151,143,135,130,134,144,153,155,149,141,136,141,155,177,198,209,205,186,162,145,144,159,179,192,195,189,180,170,162,151,132,101,65,35,24,40,76,114,130,113,70,25,1,1,19,41,52,49,37,29,33,49,65,71,60,40,23,20,33,56,76,83,77,66,64,74,89,100,102,98,95,99,108,118,125,128,128,128,127,128,131,136,142,144,143,141,141,147,155,162,163,158,150,144,141,141,144,146,145,141,135,129,130,136,145,148,142,129,119,118,126,138,148,151,146,138,135,142,162,188,209,217,208,188,170,163,169,184,200,210,210,206,203,204,205,199,178,140,98,68,62,81,117,149,159,140,102,65,46,48,
62,75,77,69,61,60,70,85,96,95,82,63,50,52,67,84,95,94,88,85,90,100,109,112,111,110,112,118,125,132,137,140,141,143,146,152,159,163,163,158,153,152,158,169,179,185,184,177,167,159,155,155,156,153,146,138,134,139,148,155,155,145,132,120,116,120,130,141,146,142,131,120,119,132,154,177,193,197,188,174,161,156,160,171,182,188,188,187,192,202,209,203,176,133,90,63,60,80,112,137,142,123,92,65,52,53,60,64,61,54,49,54,68,83,89,82,66,52,47,54,66,75,78,77,77,81,88,94,99,100,101,102,105,111,118,125,129,131,132,137,144,152,157,157,153,148,146,148,156,167,177,183,181,173,
164,158,157,157,153,146,139,136,139,147,154,156,150,139,128,119,118,125,137,146,147,139,126,118,119,132,153,173,188,192,187,176,166,163,168,176,181,181,179,184,198,217,227,219,187,141,99,75,76,96,121,138,137,121,98,79,71,70,71,66,56,48,49,62,79,90,88,76,62,55,56,61,67,71,72,73,76,80,86,92,97,98,98,98,101,108,115,120,123,126,130,137,144,150,153,153,149,144,141,143,152,164,174,179,177,171,164,159,156,152,146,141,138,138,142,147,152,153,147,136,124,116,117,126,138,146,145,136,123,113,113,125,146,168,183,186,179,170,166,169,175,177,173,166,165,176,200,226,239,227,
191,144,105,86,89,106,125,133,128,114,99,91,88,86,77,61,47,42,49,66,82,89,84,72,61,56,58,63,66,68,67,67,70,76,83,90,94,95,94,94,97,101,108,114,120,124,129,134,140,146,150,150,146,142,139,141,149,160,170,176,177,173,166,158,152,148,146,143,141,140,142,148,153,154,148,136,124,116,117,127,140,150,151,140,122,109,109,123,146,167,178,177,172,168,172,179,184,179,167,155,155,173,204,233,246,232,197,152,117,101,104,118,129,129,121,110,105,105,106,98,81,60,45,42,51,67,81,86,82,72,63,58,59,63,65,65,64,63,65,71,79,87,92,93,92,90,90,94,101,110,117,121,125,129,135,142,146,
146,143,139,137,138,144,155,166,174,176,171,162,154,150,148,147,143,140,138,141,147,153,154,148,136,122,112,114,127,145,156,153,138,119,107,109,125,145,162,168,167,165,169,179,189,190,178,159,146,150,172,206,236,248,236,202,162,132,119,121,128,130,124,116,112,115,120,119,107,86,64,50,47,55,68,80,85,82,75,68,64,64,66,66,64,62,61,64,70,79,87,92,92,90,87,87,92,100,108,113,116,121,127,134,141,144,143,138,133,130,133,142,154,165,170,168,161,155,151,150,149,146,141,136,135,139,148,156,157,147,130,114,108,116,132,148,156,150,134,116,108,113,128,144,153,155,155,161,
174,188,196,191,173,153,141,147,172,206,235,245,233,204,173,151,140,137,133,125,118,115,119,126,131,126,111,91,71,58,54,58,67,75,80,80,77,74,71,69,67,65,61,59,60,64,72,82,89,91,89,86,86,89,95,102,107,110,113,118,126,136,142,143,139,131,126,128,136,148,159,164,162,157,152,151,152,154,153,146,136,129,130,140,152,158,154,139,121,109,108,119,136,149,151,141,126,114,112,120,130,136,137,138,144,158,177,193,197,188,167,146,138,147,173,204,227,233,224,206,188,173,161,149,135,122,115,117,126,135,138,131,116,97,80,69,63,63,65,69,73,77,79,80,79,76,70,63,57,55,58,66,75,81
,84,84,82,83,87,92,98,101,101,102,105,114,126,137,142,139,131,124,123,129,141,152,159,158,152,146,145,150,157,159,153,142,131,126,132,144,155,158,150,134,116,107,111,126,141,149,145,133,122,117,121,127,131,130,126,126,136,156,180,197,199,185,162,143,139,153,177,201,217,221,217,210,202,193,179,160,140,125,118,122,132,141,144,136};

//**************** Guitar C4 to F4: (Band 4)
const PROGMEM uint8_t GuitarC4StartChunk1038[] = // recorded at low sample rate
{126,136,133,116,107,120,141,145,130,114,112,125,137,138,130,128,137,145,140,124,113,118,131,144,146,136,123,119,126,132,130,127,133,142,143,134,126,126,130,133,134,133,129,125,124,128,133,132,126,119,116,120,126,125,120,116,116,115,114,115,121,124,119,112,113,121,131,133,126,120,122,129,130,126,125,133,139,132,116,112,125,141,144,134,123,121,127,134,134,131,133,141,146,141,127,115,118,132,143,138,127,123,129,135,130,121,123,138,149,142,128,124,131,139,141,139,136,134,131,131,132,134,135,129,114,91,66,49,49,71,102,110,78,39,32,58,88,98,95,99,115,126,113,91,90, // [Guitar] [C4 start chunk] [1038 samples] (start at 147 in file)
119,151,158,143,134,148,168,172,161,159,172,185,186,178,172,174,180,183,181,177,178,184,187,179,165,157,160,166,167,161,152,148,151,152,144,132,132,146,155,149,138,139,159,191,209,198,165,143,153,180,195,190,177,165,149,117,68,29,34,83,127,114,52,4,4,33,52,44,30,36,60,70,52,25,22,48,78,81,67,66,85,101,100,95,102,116,126,129,128,127,130,138,144,143,141,146,158,163,157,147,141,142,146,145,138,129,131,143,148,135,120,120,135,149,148,138,136,157,193,216,207,179,163,172,195,210,209,203,205,202,174,118,69,66,108,153,152,103,55,46,65,78,69,59,67,88,97,81,55,51,72,93,
94,86,90,104,112,111,111,118,128,136,140,142,146,155,163,162,155,152,162,177,185,180,167,157,155,156,148,137,136,148,156,148,130,117,120,135,146,140,124,119,140,173,195,192,174,158,159,174,187,188,189,201,209,186,128,73,59,90,133,141,106,65,51,58,64,57,49,59,81,89,72,51,49,64,76,77,78,84,94,99,101,102,108,118,127,130,133,141,152,158,154,147,146,156,171,182,179,167,158,157,154,144,136,140,151,156,147,131,119,121,136,147,141,124,117,132,161,186,192,181,166,164,175,181,179,185,207,227,211,155,95,73,94,128,139,118,86,71,71,67,54,47,62,84,89,73,57,56,64,70,72,75,81,
89,96,98,98,102,111,120,124,129,138,148,153,151,145,141,148,164,177,177,169,161,156,149,141,137,140,148,153,148,132,117,118,133,146,143,126,113,119,146,175,186,177,167,169,177,174,165,171,202,234,229,177,115,86,97,123,133,118,97,89,86,72,50,42,59,82,88,74,59,57,63,67,67,68,74,84,93,95,94,96,102,111,119,126,133,141,149,150,145,139,143,156,170,177,174,164,154,148,145,141,140,146,153,152,137,121,116,128,146,151,134,111,110,136,167,179,173,169,176,184,175,156,158,193,235,243,201,140,103,105,124,130,117,106,105,103,82,53,41,54,76,86,78,64,58,61,65,65,63,66,77,88,93,
92,90,92,102,114,121,126,134,143,147,143,138,137,146,162,174,174,163,153,149,147,141,138,143,151,154,143,123,111,123,147,156,139,113,107,128,156,168,166,168,182,191,177,152,148,180,227,248,221,166,126,119,128,127,116,113,119,118,96,65,47,52,70,83,82,72,65,65,66,64,61,63,72,85,92,91,87,88,98,109,115,120,129,139,144,140,133,131,140,157,169,168,158,151,150,148,141,135,138,150,157,145,121,108,120,145,156,142,117,108,124,146,155,155,165,186,196,181,152,141,168,215,244,231,189,153,139,134,124,116,118,128,128,109,79,58,54,65,76,80,77,73,70,67,63,59,61,71,84,91,89,85,
89,98,106,110,115,126,138,144,137,128,128,141,157,164,159,152,151,154,152,140,129,133,150,158,144,120,107,118,141,151,139,118,112,123,135,137,140,158,184,198,184,153,138,157,199,230,228,205,180,162,145,125,115,121,134,137,121,94,73,63,63,68,74,78,80,78,72,62,55,59,70,81,84,83,83,89,97,101,101,106,120,136,142,133,123,125,141,156,159,151,145,150,158,155,139,127,132,149,158,146,121,107,118,140,149,137,121,118,127,131,127,127,146,179,200,189,157,139,153,188,215,221,213,202,187,163,135,119,123,137,144,132,110,90,77,71,68,69,75,84,88,81,66,55,58,70,79,79,77,80,90,98,
99,96,99,113,131,137,130,121,123,138,153,154,144,139,146,156,154,140,127,130,145,156,146,121,106,116,136,143,132,118,117,125,127,118,113,130,166,192,187,159,140,149,176,199,209,211,211,203,181,151,129,125,136,145,142,126,107,92,82,73,68,72,83,92,89,75,62,61,70,76,76,74,78,89,98,99,95,97,108,123,131,127,121,124,137,149,150,142,138,143,152,155,145,132,131,145,155,147,126,112,117,134,141,133,120,119,126,128,116,106,118,152,182,185,163,143,145,165,184,196,204,211,210,194,168,143,130,132,141,144,136};
const PROGMEM uint8_t GuitarF4StartChunk1038[] = // recorded at high sample rate
{128,134,140,143,144,140,133,125,120,120,125,133,138,137,131,123,119,122,131,143,150,149,139,126,115,113,122,137,149,153,145,129,113,106,111,125,141,150,147,134,118,107,105,112,121,127,124,113,96,76,58,46,41,46,59,76,94,106,110,105,95,86,86,96,117,138,153,153,138,114,91,79,82,98,117,134,143,143,138,133,130,130,132,135,138,141,146,151,153,153,150,147,147,150,157,165,169,168,162,155,150,151,157,166,173,176,174,169,166,171,187,212,238,253,251,230,198,170,157,160,176,190,191,173,135,87,43,22,31,61,98,139,165,165,142,110,82,70,78,100,128,150,156,144,119,89,66,57,63, // [Guitar] [F#4 start chunk] [1038 samples] (start at 458 in file)
78,96,108,112,108,100,94,92,92,94,96,98,100,104,109,112,113,111,107,104,107,115,125,133,134,130,122,116,115,122,132,143,150,150,147,144,149,165,191,218,234,232,213,185,160,149,154,168,181,182,165,130,85,46,27,34,63,103,142,167,166,146,118,93,81,87,108,136,158,165,154,128,98,73,62,66,80,94,103,105,101,96,94,96,99,101,102,101,102,107,114,121,125,124,118,111,109,114,123,133,138,136,130,123,120,123,132,143,151,154,151,149,153,168,193,219,236,236,217,189,163,150,154,167,181,184,168,134,88,46,26,33,61,102,144,171,174,156,128,104,91,95,114,139,160,167,155,130,98,71,56,
55,65,78,90,96,97,96,95,94,93,91,89,88,91,98,106,113,114,110,102,94,93,99,110,121,126,124,116,109,105,108,117,129,137,141,139,137,141,155,180,206,224,225,209,182,157,145,147,160,175,180,167,134,90,48,25,31,59,102,146,175,181,165,137,110,95,98,117,145,169,178,168,142,109,80,64,64,76,93,108,116,118,116,111,107,106,105,106,108,113,119,126,131,131,126,118,111,111,119,130,140,145,142,135,127,123,126,134,146,154,158,156,154,157,170,193,219,237,240,225,199,173,158,158,170,184,190,180,149,104,61,35,37,64,107,153,184,191,174,143,113,95,96,116,145,170,181,170,142,107,76,
59,59,72,91,107,116,117,113,106,101,98,97,99,102,108,114,120,122,120,113,105,100,101,109,121,131,135,132,124,116,111,114,122,133,142,146,145,142,144,156,178,204,223,227,214,190,164,148,147,157,172,179,170,142,98,54,27,27,53,97,143,176,184,168,136,105,85,86,107,137,164,175,164,135,98,67,50,52,68,90,107,116,116,109,101,96,94,95,98,103,109,115,119,120,117,111,104,100,103,112,123,133,137,133,126,118,114,117,125,136,145,149,148,146,148,160,181,207,226,231,220,196,171,154,151,161,175,183,176,149,107,63,35,33,57,100,148,183,192,176,144,110,89,89,110,141,168,180,169,139,
101,69,53,56,74,97,116,124,121,113,103,97,96,99,104,109,115,119,121,121,118,112,106,104,106,114,125,134,138,135,127,119,115,118,126,137,147,151,150,146,147,157,178,204,224,231,221,197,171,152,148,158,172,182,177,151,110,65,34,30,54,97,145,181,191,174,141,106,84,84,105,137,164,176,164,133,95,62,48,53,72,96,114,121,116,106,97,91,92,96,102,108,112,114,115,113,110,106,102,100,103,111,122,130,133,130,123,116,112,114,122,133,143,147,146,142,143,153,173,200,221,229,220,197,171,151,147,156,171,182,178,154,113,67,36,31,54,96,144,180,190,174,141,106,84,84,105,137,164,174,
163,132,93,62,49,56,77,101,118,123,117,105,95,91,93,100,107,112,115,114,113,111,109,106,104,104,107,114,122,130,133,131,125,119,115,116,123,133,143,148,147,144,144,153,173,199,221,229,221,200,174,154,148,156,171,183,180,158,117,71,39,33,55,97,145,181,190,174,141,106,85,85,105,136,163,172,160,129,92,63,52,60,80,103,119,122,115,103,94,90,94,100,107,111,112,109,106,104,103,103,103,104,107,113,120,126,130,129,125,119,116,117,123,132,140,145,145,142,143,153,172,197,220,229,221,200,175,155,149,156,172,184,183,161,121,76,43,36,57,98,145,181,191,174,142,108,87,86,106,
136,162,171,158,129,94,67,58,67,88,109,122,123,114,103,95,94,99,106,111,112,110,106,103,102,104,105,106,107,108,112,118,125,129,129,125,119,116,116,121,130,139,144,144,141,141,149,168,193,215,225,218,198,173,154,147,154,169,181,181,160,122,77,45,37,57,96,141,175,186,171,140,108,87,86,104,132,156,164,153,126,94,70,61,70,90,109,120,120,112,102,95,95,100,107,112,112,109,104,101,101,103,106,108,108,109,112,118,124,129,129,125,120,116,117,122,130,139,144,144,141,141,150,169,194,215,225,218,199,174,156,149,155,170,182,182,162,125};

//**************** Guitar F#4 to B4: (Band 5)
const PROGMEM uint8_t GuitarF4StartChunk709[] = // recorded at low sample rate
{115,105,112,125,125,109,82,57,43,45,64,89,107,108,95,85,93,120,148,153,129,95,79,90,118,139,143,137,131,130,133,137,143,149,153,152,148,147,155,166,169,162,153,150,158,170,176,173,166,173,201,238,254,234,191,159,161,183,192,164,103,40,23,57,111,160,163,125,84,71,94,132,155,144,107,70,57,71,96,111,109,99,92,92,95,98,101,107,113,113,107,104,112,126,135,130,119,115,123,139,150,149,144,151,180,218,236,217,178,151,155,175,183,157,100,43,27,60,116,162,165,131,95,81,102,140,165,155,117,77,62,74,94,105,102,95,95,99,102,101,103,111,122,125,119,110,112,124,136,137,127, // [Guitar] [F#4 start chunk] [709 samples] (start at 354 in file)
120,125,139,151,153,149,155,182,219,238,221,182,153,154,174,184,161,103,43,26,57,116,166,173,142,105,91,108,143,166,156,118,76,55,60,79,93,97,96,94,93,90,88,93,103,113,113,103,93,97,111,124,124,114,105,109,124,137,140,137,143,169,206,226,213,176,147,148,167,180,160,105,45,25,55,116,169,180,151,112,94,111,149,176,168,130,86,63,70,93,112,119,115,108,105,105,107,114,124,131,129,118,111,116,132,144,143,132,123,127,141,154,158,154,159,183,219,240,228,192,161,158,176,190,173,119,58,32,61,122,178,190,158,115,93,110,150,179,170,129,82,57,66,91,112,118,111,102,98,98,
102,109,118,122,117,106,99,106,123,134,132,121,112,115,129,142,146,142,146,169,204,227,218,183,152,147,164,178,164,113,51,23,50,112,169,183,152,106,83,101,142,172,164,121,72,49,61,90,112,116,107,97,94,96,103,111,118,120,115,105,100,109,125,136,134,123,114,118,131,145,150,146,150,172,207,230,223,190,158,152,167,183,171,122,60,30,54,115,175,192,160,112,87,103,146,177,169,125,74,52,66,98,120,122,110,99,96,101,109,116,121,121,115,107,104,112,127,137,135,124,115,119,132,146,151,147,148,169,204,229,224,191,157,149,164,181,172,124,61,28,51,112,173,190,158,108,81,99,
142,173,164,118,68,47,64,96,118,118,104,93,92,99,107,113,115,113,108,102,101,109,123,132,131,120,112,115,129,143,147,143,144,164,200,227,223,191,157,147,162,181,174,127,63,30,51,111,172,190,158,108,82,99,142,173,163,117,67,49,67,101,122,118,102,92,94,103,112,115,114,111,107,104,104,111,123,132,132,123,115,117,129,143,148,145,145,164,199,226,224,193,160,148,163,182,176,131,68,32,52,112,173,189,157,108,82,99,141,171,160,115,68,52,71,103,122,117,101,91,94,104,111,111,107,104,103,103,105,111,121,129,129,123,117,118,128,140,146,143,144,163,198,226,223,194,161,149,
163,183,179,135,72,36,54,113,173,190,158,110,84,100,141,170,159,115,72,58,79,109,123,116,101,93,99,108,112,109,104,102,104,106,107,111,119,127,129,123,116,117,126,139,144,141,142,160,194,221,221,192,159,147,160,180,177,135,73,38,54,110,168,185,156,110,85,99,137,163,153,113,74,62,82,109,122,114,99,94,100,110,112,107,101,101,105,107,108,111,119,127,129,124,117,117,126,139,145,142,142,160,194,221,220,193,161,149,162,181,178,138};
const PROGMEM uint8_t GuitarC5StartChunk709[] = // high sample rate
{126,124,123,123,122,117,105,87,63,42,33,42,72,113,142,144,118,83,63,74,110,154,182,177,139,89,53,54,86,130,163,169,150,121,101,100,116,140,158,162,154,142,135,137,148,162,176,188,198,210,224,236,239,227,205,186,179,184,193,193,172,128,75,35,27,59,126,193,224,205,151,91,61,77,127,182,214,201,149,86,43,44,80,126,157,160,135,100,74,72,89,114,132,135,125,111,101,102,113,127,142,153,164,176,189,200,201,189,169,152,147,154,164,163,141,98,47,13,6,38,103,169,201,184,131,74,45,61,110,164,195,186,138,78,35,34,70,115,148,153,131,98,74,70,86,109,129,134,127,114,105,105, // [Guitar] [C5 start chunk] [709 samples] (start at 412 in file)
114,128,143,157,169,181,195,205,205,193,175,160,156,162,170,168,147,105,54,17,11,43,107,173,204,188,137,81,53,67,115,169,200,191,143,83,43,41,74,118,150,156,134,101,76,71,86,110,128,134,127,114,104,103,111,126,143,158,170,181,192,200,199,188,171,158,155,160,167,163,141,100,52,17,11,43,104,169,200,183,133,78,51,66,112,164,195,187,141,81,41,39,72,114,145,150,128,93,65,56,68,90,112,124,123,113,102,98,104,119,139,159,175,187,196,199,194,183,169,159,159,166,172,168,146,106,58,24,20,53,114,176,207,192,145,92,67,84,132,188,222,216,172,113,70,66,95,137,170,175,151,
111,78,65,77,105,133,150,148,134,117,108,114,131,154,175,190,198,202,203,198,189,177,168,167,171,174,168,146,107,62,27,22,53,112,173,205,190,143,91,66,83,129,183,218,212,167,107,63,60,90,132,163,167,142,103,70,59,73,103,132,146,142,125,108,101,108,128,151,170,183,189,192,193,191,184,174,165,162,164,166,161,141,103,58,23,18,48,105,165,196,183,138,87,60,77,124,178,212,205,160,101,58,54,85,127,158,159,133,93,62,54,70,101,130,142,135,116,99,94,104,126,149,167,177,181,183,186,187,182,172,162,158,159,161,156,137,101,57,22,15,43,102,162,193,181,137,88,63,77,124,177,
211,204,158,99,58,55,85,125,154,156,129,91,62,55,74,106,134,144,134,114,98,96,108,130,153,169,176,179,182,187,190,186,177,167,161,160,162,159,141,106,63,28,20,48,105,165,196,185,142,93,67,82,128,181,213,205,160,102,62,58,89,128,156,156,129,92,65,61,82,113,138,145,133,113,99,99,113,136,157,170,175,177,181,188,193,190,180,169,161,160,162,159,143,109,65,29,21,48,104,164,195,184,142,95,71,84,129,181,212,203,159,101,63,60,88,126,151,151,125,89,64,63,85,115,137,141,128,110,98,100,115,136,156,166,169,171,177,186,192,189,179,167,158,156,159,157,142,108,65,29,20,47,
101,160,193,184,142,94,70,84,127,177,206,198,156,100,62,58,86,122,146,145,120,87,66,67,88,116,135,136,123,106,97,101,117,137,153,161,163,167,176,187,194,191,179,165,156,155,158,158,143,110,65,29,20,47,102,160,192,183,143,97,73,87,129,178,207,198,156,102,65,63,88,121,143,142,119,90,71,73,93,118,134,134,122,108,101,106,121,140,153,159,162,167,178,190,196,192,180,165,156,156,160,160,145};

//**************** Guitar C5 to F5: (Band 6)
const PROGMEM uint8_t GuitarC5StartChunk499[] = // recorded at low sample rate
{126,123,123,121,109,82,50,33,54,109,146,126,78,65,110,169,180,126,61,56,109,162,165,127,99,109,141,162,155,139,136,150,171,188,203,222,238,232,203,180,184,195,178,118,47,27,88,187,223,167,84,65,127,200,206,132,52,46,104,156,153,107,72,81,115,135,127,107,101,115,136,153,169,187,202,194,167,148,154,165,148,87,23,7,65,163,201,148,67,49,110,181,190,123,45,37,94,147,148,105,72,78,111,133,128,110,104,116,137,157,174,193,206,198,173,156,161,171,153,95,28,11,71,167,204,153,75,56,115,186,195,127,52,44,97,149,151,108,73,79,111,133,129,111,102,113,136,157,175,190,200, // [Guitar] [C5 start chunk] [499 samples] (start at 290 in file)
192,170,155,160,167,147,90,27,12,69,162,200,148,71,55,112,181,191,126,50,41,94,144,145,101,62,61,91,120,124,109,98,106,130,159,181,195,198,187,168,158,165,172,153,96,34,21,79,170,207,160,86,72,132,206,220,157,80,67,117,168,170,120,73,70,106,143,149,128,109,116,144,175,194,202,202,193,176,166,171,174,152,98,38,22,78,168,205,158,85,71,129,202,216,152,74,61,112,162,161,111,66,65,105,141,143,119,101,111,141,170,186,192,193,187,173,163,164,166,147,94,34,18,72,159,196,152,80,65,123,196,209,145,68,56,107,156,153,102,58,61,103,138,137,110,94,107,139,167,179,183,187,
184,171,159,159,160,142,92,33,15,68,156,194,151,82,66,123,195,208,143,67,56,106,153,149,99,58,65,108,141,136,108,95,111,144,169,178,181,188,188,176,163,160,162,147,98,39,21,72,159,197,155,86,71,128,198,209,145,71,60,110,155,149,99,62,72,115,144,135,108,97,116,148,170,175,180,190,192,179,164,160,162,148,100,40,21,71,158,196,155,89,74,128,197,208,144,72,62,108,151,144,96,62,75,117,142,130,105,98,118,148,166,170,176,188,191,178,161,156,159,147,100,40,21,69,154,194,155,89,73,126,193,203,141,71,60,105,145,138,94,64,78,117,138,125,102,99,120,147,161,164,174,189,
193,178,160,155,159,148,101,40,20,70,154,193,156,91,76,128,194,202,142,74,64,105,143,136,96,70,84,119,136,124,105,104,124,148,159,164,176,192,194,178,160,156,161,150};
const PROGMEM uint8_t GuitarF5StartChunk499[] = // high sample rate
{124,124,116,100,83,66,47,32,25,32,56,85,106,108,97,84,80,92,115,138,147,137,114,90,81,90,112,137,158,169,175,183,199,222,243,249,237,214,193,184,183,183,171,139,91,49,37,64,123,184,214,204,165,125,108,121,155,186,195,176,140,104,88,94,116,142,160,167,165,166,178,199,218,225,213,189,164,150,146,145,134,104,57,18,5,26,80,142,176,168,131,91,72,84,118,150,162,145,109,75,58,66,90,118,137,144,143,144,157,179,201,209,198,173,149,135,133,135,127,99,53,13,0,21,77,140,175,167,130,90,72,84,118,150,161,145,111,79,65,74,99,124,141,148,147,151,165,187,207,214,202,178,155, // [Guitar] [F#5 start chunk] [499 samples] (start at 432 in file)
144,143,145,137,108,62,19,3,25,79,141,177,172,136,96,75,86,118,151,163,148,114,81,66,74,97,122,139,146,146,149,162,183,202,208,196,173,152,141,140,142,133,104,60,18,2,23,75,135,171,167,133,93,73,83,114,145,158,145,113,81,66,74,97,122,139,144,144,147,160,180,199,205,194,172,151,140,140,142,134,107,64,24,7,26,75,133,169,169,138,101,80,87,114,145,159,149,121,91,76,82,103,127,144,151,151,153,165,185,204,210,200,179,159,148,147,148,142,118,78,38,19,34,80,137,175,177,149,111,88,92,119,150,166,157,129,98,82,87,108,133,150,157,156,158,169,188,206,211,201,181,163,153,
152,154,147,122,82,43,25,40,85,140,177,179,151,114,91,94,120,150,165,157,131,101,85,90,109,131,147,154,154,157,168,185,201,205,195,177,161,152,150,151,143,119,81,42,23,35,77,131,169,174,148,112,87,88,110,138,152,143,114,80,60,62,83,110,131,141,141,141,150,168,188,199,194,176,154,139,133,135,134,118,85,48,28,39,82,140,186,197,173,134,104,100,122,151,166,156,124,86,65,70,96,128,151,158,154,150,156,174,196,208,203,183,160,142,136,138,137,121,89,51,29,38,78,134,180,191,168,129,99,95,114,142,157,148,117,81,61,67,92,122,144,151,147,144,150,169,190,201,196,178,155,
139,133,135,134,120,89,53,30,37,75,130,176,189,168,131,101,95,112,139,154,146,117,83,65,70,94,123,143,150,146,143,150,168,188,199,194,176,155,139,133,135,135};

//**************** Guitar F#5 to B5: (Band 7)
const PROGMEM uint8_t GuitarF5StartChunk355[] = // recorded at low sample rate
{124,121,102,78,53,30,27,54,94,109,95,80,91,125,146,133,99,81,98,133,161,173,184,209,240,248,222,192,183,184,164,105,46,46,119,199,210,159,112,121,168,196,170,117,88,102,137,163,166,167,187,216,224,197,163,147,145,127,71,15,11,76,158,173,124,76,84,131,161,139,87,58,75,113,140,144,145,166,198,208,182,147,133,135,121,67,9,6,72,156,172,123,76,83,131,161,139,90,65,83,120,144,147,151,174,205,213,186,154,142,145,130,76,16,10,75,158,177,129,81,85,132,163,142,93,66,83,118,142,146,149,171,200,207,181,151,140,142,127,73,14,8,71,152,172,126,78,82,127,157,139,92,66,83, // [Guitar] [F#5 start chunk] [355 samples] (start at 307 in file)
118,141,144,147,169,197,204,179,149,139,142,128,77,20,13,71,149,172,132,86,86,127,158,145,102,76,90,123,146,151,154,174,201,209,187,158,146,148,137,90,34,22,77,153,180,143,95,91,131,164,153,110,82,95,129,153,156,158,178,204,210,188,162,152,154,141,94,39,29,82,156,182,145,98,94,132,164,153,112,85,97,127,149,154,157,176,199,204,183,160,150,151,138,93,39,26,74,148,176,143,95,88,121,151,138,93,60,69,105,135,141,141,157,186,200,183,153,135,135,131,95,44,29,78,159,198,167,114,100,133,165,151,100,65,79,123,154,156,150,163,193,208,191,158,138,137,134,100,47,29,75,
153,192,162,110,95,125,156,143,94,61,76,117,147,149,144,158,187,202,184,154,135,135,132,99,49,30,72,149,189,162,111,95,123,153,142,96,64,78,118,146,148,143,157,186,200,183,153,135,135,133};
const PROGMEM uint8_t GuitarC6StartChunk355[] = // high sample rate
{123,112,89,63,37,24,38,79,111,110,91,87,112,142,146,117,86,86,117,153,173,182,200,231,250,237,203,180,176,169,128,62,31,76,167,216,191,132,107,139,184,185,139,92,87,117,152,167,167,178,205,225,211,174,147,141,134,93,31,3,39,125,181,160,102,76,106,150,154,113,70,64,94,131,148,150,161,189,209,196,162,137,131,126,90,31,0,35,123,180,160,104,77,105,148,155,118,75,69,100,135,153,155,167,193,211,200,167,142,136,132,99,38,3,38,124,181,164,109,78,103,147,156,119,75,69,99,134,151,153,163,188,207,196,162,137,133,131,97,36,3,36,118,177,163,107,77,102,144,152,117,77,   // [Guitar] [C6 start chunk] [355 samples] (start at 286 in file)
71,100,134,150,152,163,187,205,192,161,137,133,129,98,41,7,37,118,178,167,114,83,103,144,156,125,85,78,105,138,154,158,169,193,208,196,166,145,141,137,106,49,14,44,122,180,173,123,91,109,150,163,132,93,85,110,142,158,162,173,196,212,201,173,151,146,142,110,55,22,47,119,179,175,128,95,111,149,161,133,95,86,109,140,156,159,169,192,209,199,171,149,143,138,109,56,19,39,114,175,171,123,90,105,143,158,131,92,82,104,133,147,151,164,187,201,190,165,146,142,138,108,52,14,36,108,167,167,122,89,102,140,155,130,93,84,103,130,145,152,164,185,199,190,166,147,142,137,
108,55,19,37,103,160,159,110,72,82,123,146,126,89,76,97,131,152,154,158,175,194,193,176,158,155,156,136,88,47,56,119,178,177,124,82,93,140,168,148,104,86,105,140,160,159,159,175,192,193,175,156,150,150,132};

//**************** Guitar C6 to F6: (Band 8) (highest guitar note = E6)
const PROGMEM uint8_t GuitarC6StartChunk250[] = // recorded at low sample rate // same chunk used for all notes in band
{123,111,76,40,24,68,113,99,88,126,148,108,82,121,166,182,211,248,230,186,176,155,69,37,143,217,156,107,160,189,125,83,121,163,167,186,223,205,154,142,119,38,6,101,183,126,76,126,157,100,61,98,143,150,170,206,190,143,132,113,38,2,99,182,127,78,124,158,105,66,104,147,155,175,209,194,149,137,121,44,5,101,183,133,79,122,159,106,66,103,146,153,171,205,190,144,134,119,43,5,95,180,131,77,121,154,105,68,104,145,152,171,203,187,143,133,119,48,8,95,181,137,83,120,157,113,76,109,149,158,177,207,190,151,141,127,56,16,100,183,146,91,126,164,121,83,114,153,162,180,210,  // [Guitar] [C6 start chunk] [250 samples] (start at 201 in file)
196,157,146,131,61,22,98,183,150,95,127,162,122,85,112,150,159,177,207,194,155,143,128,62,17,92,179,145,90,120,158,120,81,107,143,151,172,200,186,151,142,128,58,14,87,172,143,89,117,155,119,83,106,139,151,171,197,185,153,142,128,61,18,83,164,134,72,98,145,116,76,101,144,154,163,191,191,164,155,151,94,43,99,183,149,82,111,166,136,87,109,153,159,163,189,190,162,150,146};


//**************** Marimba F#2 to B2: (Band 1) (and E2 & F2 in band 0)
const PROGMEM uint8_t MarimbaC3StartChunk3528[] = // same chunk used for all notes in band
{118,109,95,80,66,56,50,45,41,38,35,34,36,39,45,54,64,75,84,89,93,96,102,111,122,134,141,146,149,154,164,176,186,192,194,195,198,201,203,204,205,207,207,201,191,174,160,150,144,138,132,125,117,110,108,111,118,125,128,130,131,133,136,142,149,157,164,168,169,166,163,162,163,167,172,176,180,182,184,184,185,187,191,193,190,181,170,159,152,147,144,140,133,123,112,103,96,94,95,98,101,103,105,106,110,116,123,132,136,134,129,124,123,126,132,138,143,146,147,147,147,148,150,153,157,160,162,163,164,166,167,167,164,158,148,135,122,109,99,92,89,87,83,78,72,66,64,63,65,67,67,67,67,69,71, // [Marimba] [C3 start chunk] [3528 samples] (start at 8 in file)
74,76,77,77,74,69,63,61,63,70,80,88,95,99,104,109,116,125,132,137,141,144,146,149,152,156,160,161,160,155,149,142,138,136,137,138,137,135,134,132,131,129,127,123,118,112,103,94,86,80,78,77,76,74,69,64,61,60,64,70,77,86,95,104,112,119,125,129,134,138,141,141,140,140,141,145,150,154,155,152,149,145,142,139,138,138,137,136,135,133,130,127,124,122,119,116,113,110,108,110,113,119,124,127,129,130,131,133,137,141,147,152,156,160,163,167,170,174,176,178,179,180,181,183,186,192,198,205,211,214,215,214,213,212,212,212,210,206,201,196,192,188,183,177,171,164,157,151,145,141,139,138,
139,138,136,131,126,121,119,117,116,114,113,112,113,115,118,121,123,124,125,125,126,127,130,134,138,141,143,143,142,140,137,135,133,132,130,128,125,123,120,118,115,112,106,99,91,82,75,69,64,61,57,52,47,42,36,32,29,28,28,29,31,34,37,42,48,54,60,64,66,67,69,72,76,81,85,89,92,94,94,95,95,95,96,98,99,100,102,104,106,109,110,111,110,107,103,100,97,95,93,91,89,87,85,82,80,79,79,79,80,80,81,83,86,90,95,100,104,106,108,109,109,111,113,118,123,127,131,134,137,141,146,151,155,157,160,162,164,167,169,172,174,175,174,172,168,165,163,163,164,164,165,165,165,165,167,169,171,172,172,172,
172,172,173,174,175,176,176,175,173,170,168,167,166,168,170,172,175,178,181,186,190,195,199,202,203,203,203,203,204,205,206,205,203,199,195,192,189,187,186,185,185,184,182,181,179,178,177,175,171,165,158,152,147,142,138,133,129,124,120,117,114,113,113,114,116,119,122,126,129,133,136,138,140,139,138,135,133,131,131,130,130,130,130,129,128,127,126,125,124,122,120,118,115,111,107,102,96,89,81,72,63,54,46,41,37,34,32,30,29,28,29,30,31,33,35,37,39,41,44,47,49,51,53,54,55,56,58,61,66,72,79,84,90,95,99,103,106,108,109,109,108,106,104,102,101,98,95,92,88,85,82,80,79,78,79,81,83,85,
87,88,89,90,91,92,92,90,89,89,89,91,94,96,99,103,107,111,116,121,128,135,142,149,155,161,165,169,172,174,175,175,174,173,173,172,173,173,174,175,176,176,177,179,180,182,184,184,184,183,181,180,178,175,172,169,165,162,159,157,157,157,159,162,164,166,169,171,174,177,181,183,185,186,187,187,187,187,187,187,188,189,189,191,193,196,200,205,210,213,215,216,217,217,216,215,212,209,204,199,193,187,181,176,170,165,159,154,150,146,143,140,138,135,132,128,124,120,116,112,109,106,103,99,97,95,95,95,97,99,101,103,105,108,110,112,115,117,118,119,118,115,112,109,106,104,101,97,93,90,87,85,
83,82,81,79,77,75,72,69,67,64,61,58,55,51,46,42,39,36,35,34,33,34,35,37,40,44,48,52,56,59,61,63,65,67,69,71,72,73,72,72,72,71,72,73,75,76,78,80,83,86,89,92,95,97,98,98,97,97,96,96,95,94,93,92,91,91,91,93,95,97,99,102,105,108,111,115,119,121,124,125,125,126,126,127,129,131,134,137,140,144,149,154,160,166,172,178,183,188,192,196,200,202,204,204,203,200,198,196,194,192,190,189,187,186,185,185,184,184,184,184,183,182,181,179,178,177,175,173,171,169,166,165,164,165,167,169,172,175,179,184,189,194,199,203,207,209,211,211,212,212,211,209,207,204,202,199,196,194,192,191,189,188,186,
184,181,179,176,172,168,163,157,150,143,136,130,124,118,114,110,106,104,103,102,102,103,104,106,107,108,109,110,110,110,109,107,105,103,101,100,99,99,99,100,101,103,104,105,107,108,109,110,109,107,105,102,98,93,88,81,75,69,63,58,53,49,46,43,42,40,39,38,38,38,38,38,38,37,36,36,35,35,35,35,35,36,38,40,44,48,53,58,64,70,76,81,86,90,94,97,98,99,99,99,97,96,95,93,92,90,89,89,88,88,89,90,91,92,94,95,95,96,96,96,95,95,93,92,92,92,92,94,96,100,104,109,114,120,126,133,141,149,156,163,169,175,180,183,186,189,191,192,194,195,197,199,201,204,207,209,212,213,215,215,215,215,214,211,208,
205,200,194,189,183,178,173,168,165,162,160,160,160,161,163,166,169,172,175,178,181,184,186,189,191,193,194,196,198,199,201,203,205,207,209,212,214,217,219,220,221,221,220,218,214,209,204,197,191,184,176,169,162,155,149,144,139,135,131,127,124,121,119,117,115,112,110,107,104,100,97,94,92,90,89,89,90,91,93,96,100,103,107,110,113,115,117,119,120,120,120,118,115,111,107,103,98,93,88,84,81,77,75,72,70,68,66,63,61,58,55,51,48,45,41,37,33,30,26,23,21,20,20,21,23,26,29,34,39,44,50,55,60,64,68,71,74,77,79,80,82,83,84,86,87,89,91,94,96,98,101,103,105,106,107,107,107,106,104,101,98,
95,92,88,86,83,82,82,82,83,85,88,91,95,99,104,109,114,119,123,128,132,136,139,142,144,147,150,154,158,163,168,174,180,186,192,198,204,209,213,216,218,220,221,221,220,218,215,212,208,205,201,197,194,192,190,189,188,188,188,187,187,186,185,184,183,181,179,178,176,175,174,173,173,173,174,176,178,182,186,191,196,201,206,210,214,217,219,219,220,219,218,216,213,211,208,205,202,199,196,193,190,188,185,183,180,177,173,169,164,159,153,146,139,131,124,117,110,105,100,96,93,91,90,90,90,91,93,95,97,99,100,101,102,101,101,100,98,96,95,93,92,91,90,90,91,92,92,93,94,94,93,92,91,89,85,82,
77,72,66,60,54,48,43,38,33,30,28,26,25,25,25,26,27,28,28,29,30,30,31,31,31,32,33,34,36,38,41,44,48,53,58,64,70,76,81,87,92,96,99,102,103,104,104,104,103,102,101,99,98,98,97,97,97,98,99,100,101,102,104,105,105,106,106,106,105,104,104,103,103,103,104,106,108,111,115,120,125,131,138,145,151,158,165,171,176,181,185,189,192,194,196,198,200,202,204,206,207,209,211,213,215,217,218,219,219,219,218,216,214,211,208,203,199,194,189,185,181,178,176,175,175,174,175,176,178,181,183,186,188,190,191,193,194,195,196,196,196,197,197,197,198,199,201,203,205,207,209,211,212,212,212,211,209,206,
201,196,191,185,178,172,165,158,151,145,139,134,130,126,123,120,118,115,113,110,107,104,101,98,94,90,86,83,80,78,76,76,76,77,78,80,82,85,88,91,94,96,98,99,99,98,96,94,92,88,84,80,77,74,71,69,66,64,62,60,58,56,54,52,50,47,44,40,37,32,28,24,20,16,13,11,10,9,10,12,16,19,24,28,33,38,42,47,52,56,60,63,66,69,71,73,75,77,80,82,85,88,91,95,99,103,107,110,112,114,115,116,116,115,113,111,109,106,104,102,100,98,97,97,97,98,99,102,105,109,113,117,121,125,129,132,135,138,141,144,148,151,154,158,162,167,172,178,183,189,195,200,206,212,217,221,225,227,229,230,229,228,226,223,221,218,215,
212,209,207,205,203,202,200,199,197,195,194,193,191,190,188,186,184,182,180,178,177,177,177,178,179,182,185,189,193,197,201,205,209,212,214,216,218,219,219,219,218,216,213,210,207,204,201,197,194,191,188,185,181,178,174,169,164,159,153,146,140,133,126,119,112,106,99,94,89,85,82,79,78,77,78,78,79,81,82,83,84,84,85,84,84,83,82,82,81,80,80,80,80,80,80,81,81,81,81,81,80,78,76,74,71,67,63,58,54,49,45,40,36,33,30,27,25,24,23,22,22,22,22,22,22,23,23,23,23,24,24,25,26,28,30,33,37,41,46,51,57,63,68,74,80,85,90,94,98,100,102,104,105,106,106,106,106,106,106,106,106,107,107,108,109,110,
111,111,112,113,113,113,113,113,113,113,113,113,114,115,117,120,124,128,132,137,143,148,154,160,166,173,179,184,190,195,199,203,206,209,212,214,216,218,220,222,224,226,228,229,230,231,231,231,230,228,226,224,221,218,214,210,206,202,199,196,193,191,189,188,188,188,189,191,192,194,195,197,198,199,200,200,201,202,202,203,203,204,205,206,207,208,209,210,211,212,213,213,213,212,211,209,205,202,197,191,186,179,173,166,160,154,148,142,137,132,127,123,118,114,110,107,103,100,97,93,90,86,83,80,77,74,72,70,70,69,70,71,72,74,76,78,79,80,81,82,82,82,81,80,79,76,74,71,68,65,62,60,57,55,
53,51,49,48,46,44,42,39,37,33,30,27,23,20,17,14,11,9,7,6,6,6,7,9,11,14,18,22,26,30,34,38,42,45,48,51,54,57,61,64,67,70,74,78,82,85,89,93,97,100,103,106,109,110,112,112,112,112,111,109,108,106,105,104,103,103,103,104,105,107,109,112,115,118,121,125,129,132,136,140,144,147,151,154,158,162,166,170,175,180,186,192,198,203,209,215,220,224,228,231,233,235,236,237,237,237,235,234,232,229,227,225,223,221,219,218,217,216,215,213,212,210,208,207,204,202,200,198,197,195,194,193,193,192,193,193,195,196,199,201,204,207,209,212,214,215,216,216,216,215,214,212,210,208,206,203,200,197,194,
190,187,184,180,177,173,168,164,159,154,149,143,136,130,123,117,110,104,99,94,89,86,82,80,77,76,74,73,73,72,72,71,71,71,71,70,70,69,68,68,67,66,66,65,66,66,66,66,66,66,66,64,63,61,59,57,54,50,47,43,39,35,31,27,23,20,17,14,12,11,9,9,8,8,8,8,8,8,9,9,10,11,12,14,16,18,21,24,28,32,36,41,46,51,56,61,67,71,76,79,83,85,88,90,92,93,94,95,96,97,98,100,101,102,103,104,105,106,107,108,109,110,111,112,112,113,114,115,116,118,119,121,124,127,131,135,140,145,150,155,161,166,171,177,183,188,193,198,203,207,211,215,218,221,224,226,228,230,233,235,237,238,239,240,240,240,239,237,236,234,232,
229,226,223,220,217,214,211,209,207,205,204,202,202,202,202,203,204,205,205,206,206,207,208,208,209,209,210,210,211,212,213,214,215,215,216,216,216,215,215,214,212,210,208,204,200,195,190,185,179,173,167,161,156,150,145,139,134,129,124,119,114,109,105,100,96,92,89,85,81,78,75,72,70,68,67,66,66,66,66,67,68,69,69,70,70,70,70,70,69,68,67,65,64,61,59,57,54,52,49,47,44,42,40,38,36,33,31,28,26,23,20,16,13,11,8,6,4,2,1,0,0,0,1,2,3,5,8,11,14,18,22,26,29,33,37,40,44,48,52,55,59,63,67,72,76,80,84,88,92,95,98,101,103,106,107,109,110,110,111,110,110,109,109,109,108,108,109,110,111,113,
115,117,120,122,125,128,131,134,138,141,145,149,153,157,161,165,170,175,180,185,191,196,202,208,214,219,224,229,233,236,239,241,243,244,244,244,244,243,241,240,238,237,235,234,232,230,229,228,226,225,223,222,220,218,216,215,213,211,210,208,207,207,206,206,207,207,209,210,212,213,215,217,218,220,221,222,222,222,222,221,219,218,215,213,210,208,205,202,199,195,192,189,185,181,177,172,167,162,157,151,145,139,133,127,120,114,108,102,97,92,88,84,81,79,77,76,74,73,72,71,71,70,69,68,67,66,65,64,63,62,61,60,60,60,60,60,60,60,59,59,58,57,55,53,50,47,44,40,36,32,28,25,21,18,15,12,10,8,
6,5,4,3,3,2,2,3,3,4,4,5,7,8,9,11,14,16,19,23,27,31,35,40,45,50,55,60,64,69,73,77,81,84,87,90,92,94,95,97,99,100,102,104,105,107,109,110,112,113,114,116,117,118,119,120,121,121,122,124,125,126,128,131,133,137,141,145,150,155,160,166,171,177,182,187,193,197,202,207,211,215,219,223,226,229,232,235,237,239,242,244,245,247,248,248,249,248,247,246,244,242,239,237,234,231,228,225,222,220,218,216,214,213,212,211,211,211,211,212,212,213,213,213,214,214,214,214,214,215,215,215,216,216,217,217,217,217,216,216,215,213,211,209,206,202,198,194,189,184,178,173,167,161,156,150,145,140,135,
130,125,121,116,112,107,103,99,95,91,87,83,80,77,74,71,69,67,65,64,64,63,63,63,64,64,64,64,64,64,63,62,61,60,59,57,55,53,51,49,47,45,43,41,39,37,35,33,31,29,26,24,21,19,16,13,10,8,6,4,3,2,1,1,1,1,2,3,5,6,8,10,13,16,19,22,26,29,33,37,41,45,49,53,58,63,67,72,76,81,85,90,94,98,101,104,107,109,112,114,115,117,117,118,118,119,119,120,120,121,123,124,125,126,128,129,132,134,136,138,141,144,147,150,154,158,162,166,170,175,179,184,189,195,200,205,210,215,220,225,229,233,236,239,241,243,245,246,247,247,248,247,247,246,244,243,241,240,238,237,235,233,230,228,226,224,222,220,218,216,
214,213,211,210,210,210,209,209,209,210,210,211,212,214,215,216,216,217,217,218,218,218,217,216,214,213,211,209,206,204,201,198,194,190,186,182,177,173,168,163,158,152,146,141,135,129,124,118,112,107,102,97,93,89,86,83,80,78,75,73,71,70,69,67,66,65,64,63,62,61,60,59,58,58,57,56,56,55,55,55,55,54,53,52,51,49,48,46,44,41,37,34,30,27,24,21,19,16,13,11,10,8,7,6,5,5,4,4,5,5,6,7,8,9,10,12,14,16,20,23,27,31,36,40,45,50,55,59,64,69,74,78,81,85,88,91,94,96,98,100,102,104,106,108,111,113,114,116,116,117,118,120,121,122,123,124,125,126,127,128,130,132,134,137,139,142,146,149,154,159,
164,169,174,179,184,189,194,199,204,208,212,216,220,224,228,231,234,237,240,242,243,245,246,247,248,248,248,248,247,246,245,244,242,240,238,236,233,230,228,226,224,222,220,219,217,216,215,214,214,214,214,213,213,212,212,212,211,211,211,211,211,210,209,209,209,209,209,209,208,207,205,204,202,200,198,195,192,188,184,179,175,170,165,160,155,150,144,139,134,130,125,121,116,112,107,103,99,95,92,88,84,81,77,74,71,68,66,64,62,60,59,57,56,56,55,55,55,55,55,54,53,52,52,51,50,49,48,46,45,43,42,40,39,37,35,33,31,29,28,26,24,23,21,19,17,15,13,11,10,9,8,7,6,6,7,7,8,9,11,13,15,17,20,23,
27,30,34,38,41,45,49,53,57,61,65,70,74,78,82,86,90,94,98,101,105,108,110,113,115,118,120,122,124,125,126,127,128,129,130,132,133,134,135,137,138,140,142,144,146,148,151,153,156,159,162,165,169,172,176,180,184,188,193,197,202,206,211,215,219,223,227,231,234,237,239,241,243,244,245,246,246,246,246,245,245,243,242,241,239,238,236,234,231,229,227,225,223,221,219,217,215,213,211,210,209,208,208,207,207,206,206,206,206,206,206,206,206,206,205,205,204,204,203,202,200,199,197,194,192,190,187,184,181,177,173,169,165,161,157,152,147,142,137,131};

//**************** Marimba C3 to F3: (Band 2)
const PROGMEM uint8_t MarimbaF3StartChunk2348[] = // same chunk used for all notes in band
{120,112,96,76,60,50,44,42,42,46,53,63,77,88,95,98,102,110,120,129,135,141,153,166,175,180,184,188,191,194,197,193,179,162,150,142,135,129,124,125,133,141,146,148,149,151,157,164,167,164,158,154,155,160,166,171,174,177,182,185,182,172,159,149,143,138,130,119,109,105,106,110,115,118,122,129,136,138,134,126,122,124,130,136,139,140,141,144,149,154,158,161,163,164,160,151,135,117,103,94,91,89,85,80,78,78,79,79,79,80,80,81,79,73,65,59,61,70,81,91,100,108,118,126,132,136,139,143,148,151,151,145,138,134,134,136,137,138,138,137,133,128,119,107,95,87,84,82,78,73,69,70,77,87,98, // [Marimba] [F#3 start chunk] [2348 samples] (start at 132 in file) - also use for note C3 because similar waveshape - saves memory
110,119,124,128,130,130,128,127,131,137,141,142,140,137,135,135,135,136,136,134,131,128,124,120,116,116,121,127,132,134,135,136,140,145,150,155,159,163,167,170,172,173,176,181,190,200,207,211,211,210,209,207,203,198,193,187,181,174,166,158,151,148,147,148,145,139,133,127,122,119,117,116,118,121,123,124,124,124,126,130,134,137,138,135,132,129,127,125,123,121,119,117,112,103,93,83,75,69,64,58,52,45,40,37,37,38,42,46,53,59,63,65,65,67,71,76,81,84,85,86,87,89,92,96,100,104,108,110,109,105,101,97,95,92,90,87,85,84,84,84,85,87,91,96,101,105,107,107,107,110,114,120,125,130,
136,142,148,152,156,159,163,167,170,170,167,164,162,162,163,165,167,169,172,174,176,175,175,175,175,176,176,173,170,167,165,166,169,174,179,185,191,197,200,201,201,200,201,201,200,196,192,188,185,185,185,185,185,184,183,179,173,165,156,148,141,135,129,124,120,119,119,122,126,130,134,137,139,139,136,131,127,125,124,124,125,125,125,125,124,122,119,116,111,104,96,85,73,61,51,45,41,39,38,38,38,39,40,42,43,45,46,48,48,48,49,51,56,64,73,82,89,95,100,102,102,101,99,97,95,92,89,86,83,82,83,85,89,92,94,95,96,95,93,90,88,89,91,94,99,104,111,118,127,137,146,154,160,164,166,167,
166,165,165,166,169,172,174,177,181,184,186,187,186,184,181,177,172,167,163,160,159,161,164,168,171,175,179,182,184,185,184,184,183,183,183,184,187,191,196,203,209,214,216,216,214,210,204,197,189,181,174,168,162,157,152,150,147,145,141,136,130,124,118,113,108,104,102,102,103,106,108,111,114,116,118,118,116,113,108,103,98,94,90,87,86,85,85,84,82,80,76,73,68,63,57,50,45,41,39,38,39,42,45,50,54,57,59,61,62,63,63,63,62,62,63,64,67,71,76,81,86,91,94,96,96,95,95,93,92,91,90,91,93,96,100,104,108,112,116,119,120,120,120,120,121,123,126,131,138,145,154,162,171,178,184,189,193,
195,194,192,190,188,187,186,186,187,188,189,191,191,191,189,187,184,181,177,173,170,168,168,170,174,179,185,191,196,201,204,205,206,205,203,200,197,194,192,191,190,190,190,189,187,184,180,174,166,156,147,137,129,122,117,113,111,110,111,112,113,113,112,111,109,105,101,98,96,95,95,97,100,103,106,109,111,111,109,105,100,92,84,76,69,62,58,54,52,51,50,49,48,47,46,43,40,38,36,36,36,38,41,46,53,60,67,74,79,84,87,89,90,89,87,85,84,83,82,82,83,85,88,90,92,93,94,93,92,91,90,89,89,91,94,99,105,112,120,129,139,148,155,162,167,171,174,177,180,183,187,192,197,202,206,210,212,213,212,
210,205,199,192,186,180,175,172,170,170,171,174,177,179,182,185,187,189,190,190,191,192,195,197,201,204,208,212,216,218,218,217,214,209,202,195,186,178,170,163,157,152,148,144,140,136,132,128,122,116,110,105,101,98,96,96,98,101,105,109,112,114,116,117,116,113,109,104,99,93,89,85,82,79,78,76,74,71,67,63,58,53,47,41,35,30,27,26,27,30,34,39,44,49,54,58,62,64,66,68,69,70,73,76,80,85,90,95,99,102,104,104,103,100,96,92,88,85,83,82,83,86,90,95,100,105,110,115,119,122,125,127,130,133,138,144,152,161,170,179,188,196,201,206,208,209,209,207,204,201,198,196,194,194,195,195,196,
196,195,193,190,187,184,180,177,175,175,176,178,183,188,195,201,207,212,215,216,216,214,212,209,207,204,202,200,199,198,197,195,192,188,182,174,165,155,144,134,125,118,112,108,106,105,106,107,108,109,108,107,104,100,97,93,91,89,88,89,91,93,95,96,96,94,92,87,81,73,66,58,51,44,40,37,36,35,36,36,36,35,34,33,31,30,29,29,30,32,36,41,48,56,63,71,77,83,87,89,90,90,89,89,88,88,89,90,92,95,97,100,102,103,103,102,101,99,98,97,97,99,102,107,113,121,129,137,146,154,161,167,171,175,179,182,186,190,194,199,204,209,214,217,219,220,220,217,213,208,202,196,191,187,184,182,182,183,184,
186,188,190,191,192,192,192,192,192,192,193,195,198,202,207,211,214,215,215,213,210,204,198,190,183,175,167,160,154,149,145,141,137,133,129,123,117,111,104,97,92,88,85,84,84,86,88,91,94,97,99,99,98,95,91,87,83,79,76,73,71,69,68,66,64,61,57,53,48,41,35,28,22,18,15,14,16,19,23,28,33,37,42,46,49,52,55,57,59,62,66,70,75,81,88,93,99,103,105,106,106,104,102,100,97,95,93,93,93,95,98,102,107,112,116,120,124,127,130,133,137,141,147,153,160,168,176,184,193,201,207,212,216,217,217,216,214,212,210,209,207,207,206,205,204,203,202,200,198,195,191,188,185,183,182,183,185,189,193,198,
203,207,211,214,216,216,216,215,212,210,207,203,201,198,196,193,190,185,180,173,165,156,147,138,128,119,112,105,100,96,94,93,93,93,93,93,91,89,87,85,83,81,80,80,80,81,82,83,83,83,81,79,75,71,65,59,54,48,44,40,36,34,32,31,30,29,29,28,27,26,25,24,25,26,29,33,38,44,51,58,65,71,77,81,85,88,90,91,92,93,94,96,98,100,102,105,106,108,109,109,109,109,108,108,109,110,112,116,121,126,133,140,147,154,162,169,175,181,186,190,194,198,202,206,210,214,218,221,224,225,225,224,222,219,216,212,208,204,201,199,197,196,196,197,199,200,201,201,201,201,201,201,201,202,203,204,206,208,210,212,
213,214,213,211,208,203,197,191,183,176,169,162,156,150,145,139,134,128,123,117,112,106,100,94,89,85,82,80,79,80,81,83,84,84,84,83,82,80,78,75,71,68,65,63,61,59,58,56,54,52,48,44,39,34,29,24,20,17,14,13,13,14,16,19,22,26,30,33,36,39,42,45,48,52,56,61,67,73,79,84,89,93,97,99,101,101,101,100,99,98,98,99,100,103,106,109,113,116,120,124,127,131,135,138,142,146,150,156,163,170,178,186,194,201,208,213,218,221,223,224,224,224,224,223,223,223,223,223,223,222,221,219,216,213,210,206,203,200,199,198,197,198,200,203,206,209,212,214,215,216,215,214,212,210,207,205,202,200,197,194,
190,186,181,175,168,160,152,143,134,125,117,110,104,99,95,92,89,87,84,82,80,78,76,74,72,71,69,69,68,68,69,69,69,68,67,65,62,58,54,49,43,38,33,29,25,21,19,17,15,14,12,11,10,9,9,9,9,10,13,16,20,26,31,38,44,51,58,63,69,73,77,80,83,85,87,90,92,94,97,99,100,102,103,104,105,105,106,106,107,108,111,114,118,123,129,135,142,150,157,164,172,179,185,191,197,202,207,211,216,220,224,227,230,232,233,233,232,231,229,226,223,220,217,214,212,211,210,210,210,210,211,212,212,213,213,213,213,214,214,215,217,218,220,221,221,221,220,218,215,211,205,199,193,186,179,172,165,158,151,145,139,
132,126,119,113,106,100,95,90,85,82,79,77,76,76,76,75,75,75,74,73,71,68,66,63,60,57,54,51,48,45,43,39,36,32,28,24,19,15,10,6,3,1,0,0,0,2,4,7,11,14,18,22,26,30,34,39,43,48,54,59,65,71,76,81,86,89,92,95,96,98,98,99,99,100,101,102,105,107,111,114,118,121,125,129,133,137,141,146,152,158,165,172,179,187,195,202,210,216,221,225,229,231,232,232,233,233,233,232,232,232,231,230,229,228,227,225,223,221,219,217,215,214,214,215,216,218,220,222,224,225,226,227,226,225,224,222,219,216,213,210,206,203,200,195,191,185,179,172,164,156,148,139,131,123,116,110,104,100,96,93,91,88,86,83,
80,77,74,71,69,67,65,64,63,63,62,61,60,59,56,53,49,44,39,33,28,23,19,16,13,11,9,7,6,5,4,4,3,3,3,4,5,7,10,14,19,25,31,36,42,48,54,59,63,67,71,74,77,80,83,87,90,93,96,99,102,104,106,108,110,111,112,114,116,119,122,126,132,137,144,151,158,165,172,179,185,191,197,202,207,212,217,222,226,231,235,238,241,243,243,243,242,240,238,235,233,230,227,225,224,223,222,222,222,222,222,223,222,222,222,221,221,221,221,222,222,223,223,223,223,221,218,215,211,205,199,193,186,179,173,166,160,154,148,142,136,129,123,117,110,104,98,93,88,84,80,77,75,74,73,72,71,70,68,66,64,62,59,56,53,50,48,
45,43,40,38,35,33,29,26,22,18,14,10,6,4,1,0,0,0,1,2,5,7,10,13,16,20,23,27,32,36,41,47,53,59,65,70,76,81,85,89,92,95,97,99,101,102,104,106,108,111,114,117,120,124,127,131,134,138,142,146,151,156,162,168,175,182,189,196,203,210,216,221,225,229,232,235,236,238,239,240,240,240,240,239,238,237,236,234,232,229,227,225,223,221,220,220,220,220,221,222,223,224,225,225,225,225,224,222,220,218,216,213,210,207,204,200,195,190,184,178,171,164,156,149,141,133,126,119,113,108,103,99,95,92,88,85,81,78,74,71,69,66,64,63,61,60,60,59,58,57,55,53,50,47,43,39,35,31,27,24,20,18,15,13,11,9,7,
6,5,4,3,3,4,5,7,11,15,19,25,30,36,42,47,53,58,63,67,72,76,80,83,87,91,94,98,101,104,107,109,111,113,114,116,117,119,122,125,128,132,137,142,148,155,161,167,174,180,186,192,198,203,209,214,219,224,228,232,236,239,241,243,243,243,243,241,239,237,235,233,231,229,228,227,226,225,225,224,224,223,223,222,222,222,221,221,220,220,220,219,219,218,217,215,213,210,206,201,196,190,184,178,171,165,159,153,147,141,135,129};

//**************** Marimba F#3 to B3: (Band 3)
const PROGMEM uint8_t MarimbaF3StartChunk1759[] = // same chunk used for all notes in band
{120,104,77,56,45,42,44,53,69,87,97,101,111,126,135,146,164,176,183,189,193,197,189,166,148,138,129,124,132,143,147,149,154,163,166,159,154,158,166,173,177,183,183,169,152,143,135,120,107,105,111,117,122,132,139,132,123,124,133,139,141,143,150,156,161,164,161,147,123,101,92,89,83,78,79,80,79,80,81,77,67,59,65,82,95,107,120,131,137,141,148,152,147,137,134,136,137,138,136,129,117,100,87,83,79,72,70,78,93,109,121,127,131,129,128,134,141,141,138,135,135,136,135,131,127,121,116,118,126,133,134,137,143,150,157,163,168,171,173,178,189,203,211,211,209,207,200,193,185,176,165, // [Marimba] [F#3 start chunk, also use for C4] [1759 samples] (start at 93 in file)
154,148,148,146,137,128,122,117,116,119,123,124,124,126,132,137,137,133,129,126,123,120,117,110,96,82,71,64,56,46,39,36,39,44,52,61,64,65,69,76,82,85,86,89,93,98,104,110,109,104,99,94,91,88,85,84,84,86,91,98,105,107,107,110,117,125,132,141,149,154,159,165,169,169,165,162,163,165,168,171,175,176,175,175,176,175,171,166,166,169,176,184,193,200,201,200,201,201,197,190,186,185,185,185,184,180,171,159,148,138,130,123,119,119,124,130,135,139,138,133,127,124,124,125,125,124,123,119,114,105,93,77,60,47,41,38,37,39,40,42,44,46,48,48,49,54,64,77,88,97,102,102,100,97,94,90,85,83,
83,87,91,95,96,95,91,88,89,94,100,109,119,132,146,156,163,167,167,165,165,168,172,177,181,185,187,186,182,176,169,163,159,161,166,171,176,181,184,185,184,183,183,185,189,196,206,213,216,215,210,201,190,179,169,161,154,149,146,142,134,126,118,110,104,102,103,106,110,114,117,118,115,109,102,96,90,86,85,84,83,79,74,68,60,52,44,40,38,40,45,51,57,60,61,63,63,63,62,64,68,73,81,88,94,96,96,95,93,91,91,92,97,102,108,114,119,120,120,120,122,126,133,144,156,168,178,187,193,195,193,189,187,186,187,188,190,191,191,188,185,180,174,169,168,171,176,184,192,200,204,206,205,202,197,193,
191,190,190,189,186,181,172,160,146,134,123,115,111,110,111,113,113,111,108,102,98,95,95,98,102,107,110,111,107,100,90,78,68,60,54,52,50,49,48,45,42,38,36,36,38,44,53,63,73,81,86,89,90,87,85,83,82,82,85,89,92,93,93,92,90,89,89,94,101,109,121,134,147,158,166,172,176,180,185,191,199,205,210,213,212,208,200,190,182,175,170,170,172,176,180,184,187,189,190,191,194,198,203,208,214,218,218,215,208,198,187,175,165,156,149,143,138,133,126,118,109,102,98,96,98,102,108,112,115,117,115,110,103,95,88,83,80,77,75,71,65,59,51,42,34,28,26,28,33,40,48,55,60,64,67,69,71,75,81,88,95,100,
104,104,101,96,90,85,82,83,86,93,100,107,114,120,124,128,132,138,147,159,172,185,196,204,208,209,207,203,199,196,194,195,195,196,195,192,187,182,178,175,175,178,185,194,203,211,215,216,214,211,207,204,201,199,197,195,191,183,172,159,144,130,118,111,106,105,107,108,109,107,103,98,93,90,88,90,92,95,96,94,90,82,71,60,50,42,37,36,36,36,35,34,32,30,29,29,33,39,48,59,70,79,86,89,90,89,88,88,89,91,95,99,102,103,102,100,98,97,98,102,109,118,130,142,154,163,170,176,181,186,192,198,206,213,218,220,220,216,209,201,192,186,183,182,183,185,188,190,192,192,192,192,193,196,201,207,
212,215,215,211,204,194,183,172,162,153,146,141,136,129,122,112,103,94,88,84,84,86,90,95,98,99,97,92,86,80,76,72,70,67,64,61,55,48,39,29,21,16,14,17,22,29,36,43,48,52,56,59,63,69,76,85,93,100,105,106,105,102,98,95,93,93,95,100,107,114,120,125,129,133,139,146,155,166,178,190,201,210,215,217,217,214,211,209,207,206,205,204,202,199,195,190,186,183,183,185,191,198,204,210,214,216,216,214,210,206,202,198,194,190,183,174,163,150,137,124,112,103,97,94,93,93,93,92,89,86,83,81,80,80,82,83,83,81,78,71,63,55,48,41,36,33,31,30,29,28,26,25,24,26,30,36,44,54,64,73,80,85,89,91,92,94,
96,99,103,106,108,109,109,109,108,109,111,115,122,131,141,151,161,171,179,187,193,198,204,210,215,220,224,225,224,221,216,211,205,201,198,196,197,198,200,201,201,201,201,201,202,205,207,210,213,214,213,209,203,194,184,173,163,155,147,139,131,123,116,107,99,91,85,81,79,80,82,84,84,83,81,78,74,70,65,62,59,57,55,51,46,39,32,25,19,15,13,13,16,20,25,31,35,39,43,47,53,60,68,76,84,91,96,100,101,100,99,98,98,100,103,107,113,118,123,128,134,139,144,150,158,168,180,191,202,211,217,222,224,224,224,223,223,223,223,222,220,217,213,208,204,200,198,198,199,203,207,211,214,216,215,213,
210,207,203,200,195,190,184,176,166,155,143,130,118,108,100,95,90,87,84,81,78,75,72,70,69,68,69,69,69,67,64,59,52,45,37,31,25,20,17,15,13,11,10,9,9,10,13,19,26,34,43,53,62,69,75,80,84,87,90,93,97,100,102,104,105,105,106,107,109,113,119,127,136,146,157,167,177,187,195,202,209,215,221,226,230,232,233,232,229,225,221,216,213,211,210,210,210,211,212,213,213,213,214,215,217,220,221,221,220,216,210,202,193,183,173,163,154,145,136,127,117,108,99,92,85,81,77,76,76,75,75,74,72,69,65,61,57,53,49,45,40,35,30,24,17,11,5,1,0,0,2,5,10,15,20,26,32,38,45,52,60,68,76,83,89,93,96,98,99,
99,100,102,105,109,114,119,124,130,135,142,149,158,167,178,189,200,210,218,225,229,232,232,233,233,232,232,231,230,228,226,223,220,217,215,214,215,217,220,222,225,226,226,225,223,220,215,211,206,201,196,189,180,170,159,147,135,124,114,106,99,95,91,87,83,79,75,71,67,65,64,63,62,61,59,55,49,42,35,27,21,16,12,9,7,6,4,4,3,3,5,8,12,19,27,36,44,52,59,65,71,75,80,84,89,93,98,102,105,108,110,112,114,117,122,128,136,145,155,165,175,184,192,200,207,214,221,228,234,238,242,243,243,241,237,233,230,226,224,222,222,222,222,223,222,222,221,221,221,222,223,223,223,220,216,210,202,193,
184,174,165,156,147,139,130,121,112,103,95,88,82,78,75,73,72,70,68,66,62,58,54,50,46,43,40,36,32,28,22,16,11,6,2,0,0,1,3,6,10,15,20,25,31,38,45,53,62,70,78,84,90,94,97,100,102,105,107,111,115,120,125,130,135,141,147,154,162,171,180,191,201,210,218,225,230,234,237,238,240,240,240,239,238,236,233,230,226,223,221,220,220,220,222,223,224,225,225,224,222,219,215,211,207,202,196,188,180,170,159,148,137,127,118,110,103,97,92,87,82,77,72,68,65,63,61,60,59,57,55,52,47,42,36,30,25,20,17,13,10,8,6,4,3,3,5,8,13,20,27,35,43,51,59,65,72,77,83,88,93,98,103,107,110,112,114,117,119,123,
127,133,141,149,158,167,176,185,193,201,209,216,223,229,235,239,242,243,243,241,239,236,233,230,228,226,225,224,224,223,223,222,222,221,220,220,219,219,217,214,211,205,198,190,182,173,164,155,147,138,130,121,112,104,96,88,82,77,74,71,70,68,66,64,61,59,55,52,48,45,42,38,34,30,25,19,14,9,6,3,2,3,5,9,13,18,23,28,34,41,49,57,65,73,80,87,92,97,101,103,105,108,110,114,117,122,126,131,136,142,149,156,163,171,181,191,200,209,217,224,230,234,237,238,239,239,239,238,237,235,232,229,225,222,219,218,217,217,218,219,220,220,220,218,216,213,210,206,201,195,189,182,173,164,154,143,133};

//**************** Marimba C4 to F4: (Band 4)
const PROGMEM uint8_t MarimbaC4StartChunk1478[] = // same chunk used for all notes in band
{122,101,62,49,44,52,74,96,99,123,137,154,178,185,192,199,182,147,137,118,124,139,139,149,164,156,154,168,175,181,185,160,145,133,108,103,112,116,132,133,120,132,142,142,150,159,164,163,139,104,93,86,75,78,77,79,81,69,62,87,102,119,136,142,151,155,140,135,138,135,131,117,93,82,79,67,75,97,118,128,134,131,141,147,139,136,137,133,126,118,112,123,131,132,143,154,162,170,173,179,198,209,206,205,196,184,172,155,143,144,136,123,118,115,121,125,125,131,141,138,132,129,124,119,106,84,70,61,46,38,40,47,61,68,71,82,90,92,95,100,107,113,107,98,93,87,83,83,86,94,105,107,109,121,131, // [Marimba] [C4 start chunk, also use for F#4] [1478 samples] (start at 3 in file)
143,153,158,166,169,162,159,162,164,169,170,169,172,170,163,163,170,180,193,198,197,199,195,185,182,182,180,176,162,146,134,123,117,120,128,136,140,133,128,128,128,127,124,118,107,89,65,47,42,39,41,44,48,53,55,57,70,87,100,107,106,101,96,88,83,84,90,93,94,90,89,95,105,118,136,153,164,168,166,166,170,174,179,183,179,173,163,156,156,164,170,178,182,181,181,183,188,200,211,213,208,195,180,166,154,146,142,133,122,112,104,102,107,112,118,122,117,108,99,91,88,86,82,74,66,54,44,42,44,53,61,65,68,69,68,69,75,83,93,98,97,95,92,92,96,103,111,118,120,120,123,130,144,160,175,187,193,
190,186,184,183,184,186,184,179,173,165,164,170,180,192,201,203,202,196,191,189,188,184,177,163,144,128,115,109,109,112,112,111,104,99,99,104,109,114,112,103,89,72,61,55,52,49,47,43,39,37,41,52,67,80,90,94,92,90,87,88,92,96,97,96,91,89,93,104,119,138,155,168,175,180,188,197,206,211,209,201,187,173,164,162,166,173,179,184,187,191,196,204,212,218,217,208,191,174,158,146,136,129,122,111,101,94,95,102,111,118,122,120,111,100,89,83,79,73,64,53,41,30,26,30,40,52,63,70,74,77,82,90,99,105,106,101,93,84,82,85,93,104,115,123,130,136,148,164,183,199,208,211,208,200,193,190,190,189,
186,180,174,170,171,178,191,204,213,214,211,206,200,195,191,185,174,157,136,118,105,100,101,105,106,104,99,94,92,95,99,99,94,83,68,52,41,37,36,39,40,39,38,40,45,56,72,84,93,95,92,90,89,91,97,102,104,103,99,98,103,113,129,147,162,172,179,185,192,203,212,218,219,212,199,186,177,175,178,181,185,187,187,188,191,199,208,215,213,204,189,172,156,144,136,129,120,107,94,85,81,84,91,98,102,99,91,82,75,72,69,65,57,46,32,21,16,21,30,42,51,58,64,69,77,88,100,109,112,109,102,96,92,93,99,109,118,124,130,138,151,166,184,201,213,218,217,211,206,204,201,198,194,188,182,176,176,183,194,204,
212,215,215,211,205,199,194,186,172,154,134,116,101,92,90,91,92,90,86,83,83,85,87,88,84,75,63,51,41,35,32,30,28,26,25,27,35,47,62,76,87,93,96,97,99,102,105,108,109,107,105,105,112,123,137,153,168,181,191,199,206,215,222,225,224,218,208,199,192,189,191,194,196,197,197,199,203,209,214,218,216,207,193,177,162,149,137,125,115,102,90,81,77,79,84,87,88,85,80,73,66,62,59,54,45,34,23,14,10,12,18,27,35,42,48,56,66,78,89,98,104,104,100,97,96,99,105,113,121,129,136,145,157,173,191,207,218,224,226,224,221,219,218,216,212,205,198,194,193,195,201,209,215,216,214,210,206,200,194,185,173,
158,140,122,107,97,90,86,82,79,76,73,71,72,73,72,68,60,50,40,29,22,17,15,12,10,9,12,18,28,41,55,67,76,82,86,91,95,99,101,103,103,104,105,111,121,135,149,165,179,192,203,212,221,229,234,234,230,223,216,210,206,205,206,208,208,209,210,212,216,220,221,218,210,198,184,169,156,143,130,117,104,92,83,78,76,77,78,78,75,70,64,58,51,46,40,31,21,11,3,0,0,4,11,20,27,36,45,56,69,80,90,96,99,100,99,99,102,108,114,121,129,137,148,162,177,195,211,223,230,233,233,231,230,228,226,222,217,212,209,208,211,216,221,225,226,224,219,214,207,201,191,178,163,145,128,113,101,94,89,84,79,74,70,67,67,
67,66,62,53,42,31,21,15,10,7,4,2,2,4,10,20,32,45,57,67,74,80,86,92,98,103,106,108,109,112,118,128,141,155,170,183,196,207,217,227,236,242,244,241,235,228,222,218,217,217,218,218,218,218,220,222,224,224,220,212,200,186,172,159,147,134,121,108,96,86,79,75,74,73,72,68,63,57,52,47,42,36,28,19,10,4,0,0,2,7,13,21,29,38,49,62,74,85,92,97,100,102,104,108,114,120,127,134,143,153,166,181,196,211,222,230,235,238,239,239,236,232,227,222,217,215,214,216,220,223,226,228,226,222,218,211,203,192,178,164,148,132,118,107,98,92,85,79,73,69,65,63,62,59,55,49,41,32,24,17,12,8,5,3,2,5,10,19,29,
41,53,63,72,80,88,95,100,105,109,112,115,119,125,135,147,159,173,186,198,210,220,228,236,241,243,242,238,234,230,226,224,223,222,222,221,221,222,223,223,221,217,209,198,186,173,160,147,134,121,109,97,87,80,75,72,70,67,63,59,54,49,44,39,33,26,17,10,4,1,0,2,6,12,19,27,37,48,59,71,81,90,96,101,104,108,113,117,123,130,138,147,157,170,183,197,211,222,230,236,240,241,242,241,238,234,230,225,222,220,220,222,223,224,223,221,217,212,205,197,187,174,160,146,131,117,106,97,88,81,74,69,64,61,59,57,53,49,43,36,29,22,15,10,6,3,1,1,3,8,16,27,38,49,60,69,79,87,95,102,107,111,114,118,123,
129,138,149,161,173,185,198,209,220,230,237,242,244,244,241,238,234,231,228,226,225,223,222,222,222,222,221,218,213,205,194,182,170,157,144,131,118,106,95,85,77,72,68,65,62,58,54,49,44,40,35,30,23,15,9,4,2,2,3,7,13,19,27,37,48,60,71,82,91,97,102,106,110,115,120,126,133,141,150,160,172,186,200,213,224,232,238,241,242,243,242,240,237,233,229,226,223,223,224,225,225,223,219,215,209,201,193,183,171,157,143,129,116,105,96,88,80,73,67,62,58,55,52,49,44,39,32,25,19,14,10,6,4,3,3,5,10,18,28,40,51,61,70,79,88,96,102,108,113,117,120,125,132,140,151,162,175,187,198,209,220,229,237,
242,244,244,242,239,235,233,230,228,227,225,223,222,221,220,218,214,209,201,191,180,168,156,144,132};

//**************** Marimba F#4 to B4: (Band 5)
const PROGMEM uint8_t MarimbaC5StartChunk1050[] = // same chunk used for all notes in band
{114,70,52,60,96,113,125,137,158,167,174,158,127,117,133,155,166,172,156,165,172,173,143,126,105,115,131,147,130,134,138,145,153,154,118,90,86,89,94,97,85,75,99,120,134,137,137,120,126,130,123,96,90,83,100,128,139,128,130,131,125,126,124,117,121,136,141,153,162,166,170,193,198,192,179,165,149,149,145,132,127,133,132,136,138,127,119,114,98,76,65,53,54,67,80,79,86,86,88,96,105,97,90,86,88,94,109,113,116,127,141,149,157,156,150,155,165,170,171,171,163,167,182,196,194,191,180,175,177,176,158,139,127,126,137,147,138,125,123,123,119,108,84,57,50,54,60,63,62,60,76,94,99,92,85,78, // [Marimba] [C5 start chunk, also use for F#4] [1050 samples] (start at 8 in file)
83,96,102,96,96,107,126,149,159,156,153,160,171,178,173,162,156,165,177,185,182,178,182,197,207,200,179,161,150,148,142,128,115,113,119,124,120,105,91,85,86,81,70,55,49,54,67,71,70,66,67,77,90,95,92,88,92,104,117,121,120,125,143,165,181,184,176,173,177,183,183,177,168,170,184,199,202,195,184,180,181,177,160,136,121,119,123,123,113,102,101,108,112,103,83,63,57,58,58,53,46,46,62,79,89,86,79,78,86,95,98,93,93,105,127,151,165,169,174,187,199,200,188,171,164,172,184,191,192,192,197,206,210,199,176,156,145,141,134,120,107,105,115,122,120,106,89,79,77,72,61,45,35,40,56,69,72,71,
75,86,97,99,89,81,83,98,115,127,132,139,158,182,197,199,190,183,185,191,190,183,176,179,194,210,212,203,193,187,184,177,157,131,115,113,118,119,109,95,91,95,96,85,65,46,40,44,47,43,40,44,60,79,89,88,83,85,94,103,105,101,102,115,137,158,171,176,182,195,207,209,199,184,177,182,192,197,194,192,196,205,208,196,174,153,142,137,129,113,97,91,97,105,103,90,74,67,65,60,49,32,23,29,45,57,62,63,70,84,99,103,97,90,92,104,120,131,138,148,165,187,204,209,202,196,196,198,197,191,184,186,199,210,214,208,197,188,182,171,151,128,109,102,103,103,95,86,81,82,82,73,58,44,37,37,37,35,33,39,54,
72,84,88,88,91,99,106,109,109,113,126,145,165,180,189,196,206,215,216,209,199,194,196,203,205,203,201,205,209,209,199,180,160,146,135,123,109,94,87,89,91,87,77,65,57,54,50,38,25,18,20,30,41,47,52,62,77,90,96,95,92,96,108,121,133,142,154,172,193,209,216,215,212,212,214,213,206,201,200,206,213,216,210,202,193,185,173,156,134,114,103,97,91,85,77,71,69,67,59,46,33,24,20,19,17,17,23,34,50,64,73,77,83,90,97,102,105,111,123,141,160,178,192,203,213,222,225,222,215,210,209,212,215,215,214,215,217,214,205,188,170,154,140,125,109,95,86,83,82,77,68,58,48,41,34,24,13,6,7,14,23,32,41,52,
66,79,88,91,93,97,108,120,132,143,158,176,196,213,222,223,222,223,224,223,219,216,216,221,225,226,221,212,202,193,180,162,141,121,108,100,94,84,74,67,64,61,53,39,25,16,12,11,10,9,13,24,39,53,63,70,78,87,97,103,109,115,127,145,165,182,196,208,220,230,235,234,228,223,221,223,225,225,223,222,222,219,209,195,179,163,148,131,115,98,86,79,75,69,62,53,45,37,29,19,8,2,1,5,13,23,34,47,60,73,83,90,94,101,110,121,133,148,164,181,200,216,226,232,234,234,233,230,225,222,223,225,227,228,225,217,209,197,182,163,144,125,112,102,92,82,73,65,60,55,47,36,24,15,8,5,3,3,9,21,35,49,62,72,81,91,
100,107,114,122,134,152,171,188,203,217,227,236,241,240,236,233,230,230,230,230,229,227,225,219,209,194,176,158,142,126,110,95,83,75,70,64,57,49,40,32,24,15,6,1,0,4,12,22,33,46,60,73,84,92,100,107,116,127,139,154,170,187,205,219,230,236,239,240,238,235,232,229,228,229,229,226,222,214,204,191,176,158,139,121,106,94,83,73,65,59,53,47,39,29,20,11,5,1,0,1,8,19,33,47,60,72,83,94,103,110,118,127,140,156,173,189,205,219,231,239,243,243,240,237,235,233,232,230,227,225,222,216,205,190,173,155,138,121,104,89,78,70,64,57,50,42,35,28,20,11,3,0,0,3,11,20,31,45,60,74,87,96,103,111,120,
131,143,157,172,190,208,223,234,241,243,244,244,241,237,233,231,231,231,228,222,213,202,189,174,156,136,118,103,91,80,70,61,53,48,42,34,25,16,8,4,1,0,2,7,18,32,47,61,73,84,95,105,113,121,130,143,158,175,192,207,220,233,241,246,246,243,239,237,235,233,231,229,225,221,214,202,188,172,154,137};

//**************** Marimba C5 to F5: (Band 6)
const PROGMEM uint8_t MarimbaF5StartChunk1006[] = // same chunk used for all notes in band
{118,71,54,97,126,140,159,168,148,109,121,158,171,160,170,172,136,110,119,148,138,139,145,157,125,83,85,97,97,81,110,134,140,123,116,123,97,83,94,136,140,133,127,124,120,116,134,147,162,164,183,194,180,158,144,146,134,137,140,145,132,119,101,74,58,58,80,86,89,86,96,97,84,81,91,110,116,130,145,156,151,150,163,169,168,162,181,197,192,178,174,174,150,130,131,151,143,126,124,116,89,54,53,65,69,66,84,99,89,74,77,95,95,97,120,151,159,150,159,173,168,154,164,183,185,179,194,207,187,158,147,144,128,117,125,130,110,89,85,80,61,49,62,74,71,66,78,93,89,85,98,117,118,125,153,179,179, // [Marimba] [F#5 start chunk, also use for C5] [1006 samples] (start at 28 in file)
168,173,181,174,167,184,205,200,183,179,176,149,123,122,130,118,105,111,113,87,59,56,60,52,49,71,90,85,74,81,94,91,93,118,153,167,170,186,195,178,160,170,191,196,197,206,210,184,153,141,137,120,110,122,129,111,84,76,70,51,37,50,73,76,74,86,96,85,75,92,119,131,140,170,194,191,177,179,187,181,177,196,216,209,191,184,175,145,117,117,126,114,97,98,96,70,43,42,49,44,46,68,89,85,79,88,100,98,102,129,161,174,180,195,204,190,173,180,196,199,196,205,208,183,151,138,132,113,98,105,112,95,72,65,58,38,24,39,60,66,68,85,99,92,83,97,122,136,149,176,202,203,191,190,193,189,185,200,216,
213,196,183,170,141,113,106,111,102,89,86,82,62,41,35,38,36,40,62,83,87,85,93,103,105,112,138,167,185,194,205,211,201,190,194,206,206,206,211,210,188,159,140,127,109,94,96,97,82,64,55,47,30,18,26,43,51,60,78,91,90,87,100,121,138,155,182,206,212,207,206,209,204,200,207,219,216,202,189,173,147,118,105,99,91,80,75,68,50,30,20,20,18,24,41,62,73,76,84,93,99,109,133,162,185,200,212,219,214,206,207,215,218,218,220,216,197,169,148,129,110,95,91,87,74,57,44,32,16,7,11,25,37,50,67,81,85,87,99,119,137,158,186,210,219,217,217,219,217,215,222,230,226,212,198,180,154,126,110,102,91,77,
70,62,43,22,12,11,11,15,30,50,63,70,81,93,102,113,136,165,188,205,220,229,226,219,218,224,227,226,226,220,201,175,153,134,114,96,88,82,69,53,40,29,14,4,6,17,29,43,61,76,84,90,103,121,139,161,187,211,222,226,227,227,223,222,227,231,228,216,202,183,156,130,113,101,87,75,67,58,42,24,13,9,7,12,28,47,62,73,85,96,105,118,140,167,190,209,224,233,231,227,226,229,230,229,228,220,202,177,154,133,112,94,84,77,65,51,39,26,12,4,5,15,28,43,61,77,87,95,107,124,143,165,190,213,226,230,232,231,227,226,229,231,226,214,199,179,153,128,110,95,82,70,62,51,36,21,11,5,3,10,26,45,60,73,87,98,108,
122,144,169,192,211,227,235,234,230,230,231,230,228,226,217,199,175,152,130,108,90,80,71,59,45,34,23,9,1,4,14,25,41,60,78,88,97,110,127,146,168,194,216,229,234,236,236,232,229,231,233,226,213,198,178,152,127,108,94,79,65,56,47,32,18,9,5,3,9,24,44,60,74,89,104,116,128,147,170,192,210,227,238,238,232,230,231,229,226,224,216,198,174,151,130,107,88,77,70,58,45,34,23,10,0,1,11,23,39,59,78,90,98,110,127,145,167,192,215,228,234,236,236,232,228,229,231,226,214,200,180,155,130,111,96,81,70,62,51,36,21,12,7,5,10,24,42,58,72,87,99,109,122,143,167,188,207,224,235,236,232,232,233,231,
228,226,218,201,179,157,137,115,96,84,76,64,50,38,27,14,5,5,13,24,37,56,74,85,94,107,123,141,161,185,209,223,230,234,235,232,229,230,231,226,216,203,185,160,136,117,102,86,73,63,53,39,25,15,10,7,10,23,39,55,69,84,97,108,120,138,162,183,203,220,231,233,230,230,230,229,226,224,217,202,180,159,139,118,98,85,76,63,50,39,29,16,6,6,13,23,36,53,71,84,93,105,121,138,156,179,203,218,225,229,231,228,225,226,227,223,212,199,182,159,135,116,102,87,73,63,54,41,27,17,12,9,12,23,39,55,68,83,97,107,118,136,158,179,198,214,226,229,227,226,227,225,222,219,213,198,178,158,138,118,100,87,77,
65,52,42,32,21,12,11,16,25,37,54,71,84,95,107,122,137,155,177,199,214,222,227,229,227,224,224,224,220,210,197,181,159,137};

//**************** Marimba F#5 to B5: (Band 7)
const PROGMEM uint8_t MarimbaF5StartChunk715[] = // same chunk used for all notes in band
{110,54,92,134,153,169,122,120,168,162,173,153,108,134,144,137,156,122,78,99,89,97,138,131,118,116,82,103,142,132,127,120,120,143,159,172,194,175,148,143,134,141,142,124,99,64,58,82,88,88,98,86,82,105,118,138,155,149,158,171,162,180,198,180,175,161,128,141,146,124,122,85,50,65,67,77,99,79,78,97,95,125,159,151,165,171,154,177,185,183,206,180,148,143,121,124,127,94,85,72,50,67,73,66,87,90,87,113,119,139,179,174,171,181,168,184,206,185,180,161,122,127,121,105,115,85,55,60,49,63,91,78,80,94,90,124,164,170,192,183,160,183,196,200,211,177,143,136,113,120,127,92,75,62,37,58,77, // [Marimba] [F#5 start chunk, also use for C6] [715 samples] (start at 20 in file)
74,93,86,78,113,132,155,194,185,177,187,177,196,217,194,183,158,116,123,117,96,99,67,40,49,43,60,89,80,87,100,99,134,171,179,201,194,173,191,198,199,209,175,141,132,104,103,110,79,65,50,24,46,66,69,94,93,85,116,137,163,201,198,189,194,185,200,218,199,181,153,112,109,105,87,85,60,36,38,36,54,84,85,92,104,109,142,179,193,209,204,190,202,206,207,211,181,146,127,101,95,94,69,55,39,18,32,50,61,86,90,89,116,140,169,206,210,206,208,200,208,220,205,186,157,118,102,93,78,71,49,24,19,19,35,64,75,83,95,106,137,176,200,216,216,205,211,218,218,218,190,155,129,102,91,85,62,43,25,7,15,
34,52,75,85,90,114,140,173,210,218,216,219,215,222,230,215,194,165,125,106,93,74,65,41,16,11,11,25,52,67,80,96,109,140,179,204,224,228,218,222,227,226,222,195,160,134,105,88,79,58,39,22,5,9,25,45,70,84,93,116,142,175,210,224,227,227,222,227,231,219,198,166,129,107,90,72,61,40,18,9,7,23,50,68,84,100,114,145,181,209,228,232,226,228,230,229,222,196,162,132,103,85,74,56,38,19,4,8,24,45,70,86,99,119,146,179,212,228,232,230,226,229,230,217,195,163,127,103,84,68,55,35,15,6,4,20,47,67,86,101,118,148,182,211,231,235,230,231,230,228,220,193,160,129,99,81,68,49,33,16,2,6,22,43,70,
88,101,123,149,182,216,232,236,236,230,231,231,216,194,162,125,101,81,63,50,31,13,5,4,19,46,67,88,108,124,150,183,209,232,239,232,230,229,225,218,193,159,129,98,78,67,49,33,17,1,4,19,41,70,89,101,122,148,181,215,231,236,235,229,229,230,217,196,164,129,104,83,67,55,35,16,7,5,19,44,65,86,103,118,146,180,207,229,236,232,233,231,227,220,196,165,136,106,86,73,54,37,21,5,7,21,39,66,85,98,119,144,174,208,226,233,235,230,230,230,218,199,170,135,110,88,70,57,38,19,10,7,18,42,62,83,101,116,142,174,202,225,233,230,230,229,225,219,197,166,138,108,87,73,54,38,23,7,8,20,38,63,83,97,
117,140,169,202,221,228,231,226,226,226,214,195,168,134,109,89,70,57,40,21,12,9,19,42,62,82,101,115,140,171,197,220,229,226,227,225,221,214,194,165,137,109,88,74,56,41,26,12,12,23,39,64,84,98,118,140,168,199,218,226,229,225,224,223,212,193,167,136};

//**************** Marimba C6 to F6: (Band 8)
const PROGMEM uint8_t MarimbaF6StartChunk656[] = // same chunk used for all notes in band
{113,61,110,179,148,105,146,178,147,111,137,157,144,89,93,105,144,129,97,82,134,136,113,126,149,180,181,146,128,145,144,123,64,81,98,103,86,83,102,136,140,149,155,169,188,180,156,138,152,144,105,62,70,95,96,76,81,106,143,151,150,154,173,195,194,157,131,139,130,103,72,67,76,80,82,89,101,135,166,163,158,175,200,192,157,132,130,130,108,68,53,72,89,78,75,100,148,177,165,153,182,213,196,153,126,137,134,97,59,53,77,90,78,75,108,154,179,168,163,187,213,199,156,127,130,122,91,53,46,69,86,80,79,106,154,184,178,166,188,213,199,153,124,121,119,86,48,38,65,83,84,76,110,158,190,180, // [Marimba] [F#6 start chunk, also use for C6] [656 samples] (start at 2 in file)
172,193,219,203,155,120,118,113,82,48,35,59,83,84,82,112,162,194,189,182,199,219,205,159,122,115,107,78,41,27,47,72,79,77,109,159,197,195,189,201,223,209,165,123,109,102,73,34,18,37,65,73,74,103,158,199,199,194,208,229,215,170,126,113,101,70,29,14,31,61,71,75,103,159,201,204,201,217,234,220,175,131,112,99,69,27,11,25,53,68,75,103,158,202,212,206,218,236,222,178,135,113,96,67,28,8,21,51,69,77,105,158,203,215,210,221,236,225,181,136,111,95,67,28,7,20,51,70,78,106,160,206,218,214,224,239,225,180,135,109,93,65,26,5,18,49,69,81,107,160,206,220,216,225,237,223,179,134,106,89,
61,24,4,17,47,69,80,109,161,208,222,220,227,238,223,179,133,105,86,58,22,5,15,46,70,82,111,163,210,225,223,229,239,223,180,134,105,83,56,20,0,12,44,69,83,111,163,210,227,224,230,239,223,181,134,103,82,54,20,0,10,42,68,82,110,161,210,228,227,232,240,226,184,138,105,84,56,21,0,9,39,66,82,109,160,208,228,229,234,241,227,187,142,108,85,57,23,0,7,38,64,81,108,157,206,227,229,234,241,228,189,143,110,85,57,23,2,9,36,63,81,108,155,204,226,228,233,239,227,188,143,109,84,55,23,2,6,34,61,80,107,153,201,224,227,231,237,224,187,143,109,84,56,24,0,7,34,62,81,107,153,199,223,227,231,
235,223,186,142,109,83,56,25,4,8,34,62,82,109,153,199,223,227,231,235,222,186,143,109,83,56,26,5,9,34,62,83,110,153,198,223,228,232,235,222,187,144,110,84,56,27,6,9,33,61,83,110,152,197,223,229,232,235,222,188,146,111,85,57,28,8,10,33,61,83,110,151,196,222,229,232,234,222,189,147,113,86,58,29,9,11,34,61,84,110,150,194,221,228,231,233,222,189,149,115,88,60,31,12,13,33,60,83,109,148,191,218,227,230,233,222,190,150,116,89,61,33,14,14,33,60,83,108,146,189,216,225,229,231,220,190,151,118,90,63,35,16,15,34,60,83,108,145,186,214,223,227,229,218,188,151,118,90,63,36,17,16,34,60,
83,108,144,185,212,222,226,228,217,188,151};

//**************** Marimba F#6 to C7: (Band 9)
const PROGMEM uint8_t MarimbaF6StartChunk450[] = // same chunk used for all notes in band
{129,67,142,160,109,179,132,125,160,112,86,130,132,83,125,130,117,164,181,142,134,149,84,78,105,86,89,131,144,154,176,186,148,146,141,74,71,100,77,90,139,152,152,181,197,149,133,129,81,68,78,83,92,132,168,157,185,197,150,128,129,83,54,83,80,82,145,176,153,194,206,142,132,130,71,55,87,78,86,152,177,162,197,207,146,127,120,65,47,80,81,87,150,185,167,197,206,144,121,115,61,40,78,82,87,155,189,171,203,211,144,117,110,59,37,73,84,91,159,195,182,206,213,148,115,104,55,28,62,78,87,156,199,188,209,217,154,111,99,49,18,53,73,82,154,203,193,216,223,158,115,98,44,14,48,72,83,155, // [Marimba] [F#6 start chunk, also use for C6] [450 samples] (start at 1 in file)
206,200,223,227,164,116,96,43,10,41,68,84,153,210,206,225,229,167,118,93,43,8,37,69,86,153,211,210,226,231,170,117,92,43,7,37,70,87,155,214,214,230,232,169,115,90,41,5,34,69,89,155,215,216,229,230,169,113,86,38,4,32,69,90,157,217,220,232,230,168,112,83,36,4,30,69,91,158,220,223,233,230,169,112,80,34,0,28,69,92,158,220,225,234,230,170,112,79,34,0,26,67,92,156,220,227,236,232,174,114,81,34,0,24,65,92,155,219,229,237,233,177,118,82,36,0,22,64,90,152,217,229,237,234,179,118,82,36,1,22,62,90,150,215,228,235,233,178,119,81,36,1,19,61,90,148,212,227,234,230,177,118,80,36,0,20,
62,91,148,211,227,233,228,176,118,80,37,3,20,62,92,148,211,227,232,227,177,119,80,37,4,21,62,93,148,211,228,233,228,178,120,80,38,5,20,61,93,147,209,228,233,228,179,121,81,39,7,21,61,94,147,208,228,233,228,180,123,82,41,9,21,61,94,146,206,228,232,227,181,125,84,42,11,21,60,93,144,204,226,232,226,182,127,85,44,13,22,60,93,142,201,224,230,225,182,128,87,46,15,23,60,93,141,199,222,228,222,181,128,87,47,17,24,60,93,140,197,221,227,221,181,128};


//**************** Trumpet F#2 to B2: (Band 1)
const PROGMEM uint8_t TrumpetF2C3StartChunk1697[] = // same chunk used for all notes in band
{127,127,127,128,128,128,128,128,128,128,128,128,128,128,127,127,127,126,126,126,126,125,125,125,126,126,126,126,126,126,126,126,126,126,126,126,126,126,125,125,125,125,125,125,126,126,126,127,127,128,128,129,129,129,129,129,129,129,129,129,128,128,128,128,127,127,127,127,127,127,127,127,127,127,128,128,128,128,128,128,128,128,128,127,127,127,126,126,126,126,126,126,126,126,125,125,125,125,125,125,126,126,126,126,126,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,126,126,126,126,126,126,126,126,127,127,127,127,128,128,128,129,129,129, // [Trumpet] [(F#2 &) C3 start chunk] [1697 samples] (start at 36 in file)
129,129,129,129,128,128,127,127,127,127,127,126,127,127,127,127,127,127,127,127,127,127,127,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,127,127,127,127,127,127,127,127,127,127,128,128,128,128,128,128,129,129,129,129,128,128,128,127,127,127,126,125,125,125,125,124,124,124,124,124,124,124,124,124,124,124,123,123,124,124,124,123,123,123,124,124,125,125,126,126,127,128,128,129,129,129,128,128,128,127,127,127,127,127,127,127,127,127,128,128,127,127,127,127,127,127,127,128,128,127,127,127,126,126,126,126,126,126,126,127,128,128,128,128,129,130,130,
130,130,129,128,127,127,126,126,125,125,125,125,125,125,125,125,125,126,126,127,128,129,129,129,129,129,128,127,126,125,125,124,124,124,124,124,124,124,124,125,125,126,126,126,126,126,126,126,125,125,124,125,125,125,125,124,124,125,125,125,126,127,127,126,126,126,127,128,129,131,132,132,132,132,132,131,130,129,128,127,126,126,127,127,127,126,126,126,126,126,126,127,128,129,129,129,130,130,129,129,128,128,127,125,124,124,123,123,123,124,124,124,124,124,124,125,125,126,126,128,129,129,130,130,130,130,129,128,127,126,125,124,124,123,123,124,124,124,124,124,125,125,
124,124,126,127,128,128,129,130,129,128,127,127,127,126,125,125,125,125,125,125,126,128,129,129,130,130,130,129,129,128,128,126,125,124,124,124,124,124,124,124,124,124,125,126,126,126,126,125,125,125,126,126,127,128,127,127,126,126,125,125,124,123,123,122,122,122,123,124,125,126,127,128,129,129,130,130,131,132,131,131,130,129,129,127,125,123,122,121,121,121,120,121,122,124,126,128,129,129,130,130,128,126,125,125,125,125,125,125,125,125,126,127,128,127,126,125,125,125,125,126,127,128,130,131,131,130,130,130,130,129,128,128,128,128,126,125,124,124,124,123,122,122,
122,124,125,127,128,130,131,132,133,132,131,129,128,127,125,124,124,124,124,124,124,125,127,128,128,129,129,130,130,131,132,131,130,129,128,128,128,128,129,131,132,132,133,133,133,133,133,132,132,132,132,131,129,128,127,127,127,127,128,128,128,127,127,127,127,127,127,127,127,126,126,125,124,124,124,123,123,123,123,123,124,124,124,124,124,124,124,124,125,126,127,128,129,129,129,129,128,128,127,126,126,126,126,126,126,125,126,126,126,127,127,128,128,127,126,126,127,127,127,127,127,127,126,126,125,125,124,124,124,124,124,125,126,126,126,126,126,126,126,126,126,126,
126,126,126,126,126,126,126,126,126,126,127,127,127,127,126,126,126,125,125,124,124,124,125,125,125,125,124,124,124,125,126,127,127,127,127,127,127,128,128,128,128,129,129,129,128,128,127,127,126,126,125,125,125,126,126,126,126,126,126,126,127,128,128,129,129,129,129,127,125,124,124,123,122,122,122,123,123,124,124,125,126,127,127,128,127,126,125,125,125,125,125,126,127,128,128,129,130,131,131,131,131,133,133,133,132,131,131,131,130,129,128,128,128,127,127,128,129,131,131,132,132,133,134,135,136,136,136,136,136,136,135,134,134,134,133,132,131,131,131,130,130,130,
130,130,129,129,128,127,127,127,126,125,124,123,122,121,120,120,120,121,121,122,123,124,124,124,124,125,125,125,125,125,125,125,125,125,125,125,126,126,126,126,126,125,124,123,122,122,122,122,123,124,125,126,126,126,126,126,126,125,125,125,124,124,124,125,125,125,125,125,125,125,126,126,126,127,127,127,127,127,128,128,127,126,125,125,125,125,126,126,127,128,128,128,129,129,128,126,125,125,124,124,124,124,123,123,124,124,124,124,124,125,126,126,126,126,126,126,126,126,126,127,127,127,127,127,126,125,124,123,123,122,121,120,121,121,122,122,122,122,122,122,123,122,
122,122,122,121,120,119,119,119,120,120,119,120,122,123,125,126,127,128,129,130,131,132,133,134,135,135,134,135,136,136,137,138,140,141,143,144,146,149,151,152,154,156,157,158,159,159,158,157,155,153,152,150,147,144,139,134,129,124,119,114,110,107,106,105,104,105,106,108,110,112,115,117,119,121,122,124,125,125,126,126,126,126,126,126,125,124,123,123,122,121,121,120,120,120,120,119,119,119,119,119,119,120,121,122,123,123,124,125,126,126,126,126,126,126,127,126,126,126,125,125,124,124,124,124,124,124,123,123,122,123,123,124,125,126,126,126,126,125,124,124,123,123,
123,123,123,123,123,123,123,123,123,123,124,124,125,125,126,126,127,128,129,129,130,130,130,130,130,129,129,128,128,127,127,127,126,126,126,126,126,126,126,126,126,127,127,127,128,128,128,128,127,126,126,125,124,123,122,122,121,121,121,122,122,122,121,121,122,123,123,124,125,125,124,123,121,121,120,120,120,121,122,123,123,124,125,126,126,127,127,128,129,130,131,132,133,134,135,136,137,139,140,142,143,145,146,147,149,149,150,150,150,149,149,147,146,145,144,143,142,142,140,139,139,138,138,138,139,140,141,142,143,143,142,141,139,136,132,127,124,120,117,114,112,111,
110,110,111,111,112,112,113,113,114,114,115,115,116,116,117,118,118,119,120,120,121,121,121,121,121,121,121,121,120,120,120,120,120,120,120,120,120,121,122,123,124,125,126,127,127,127,127,126,125,125,124,124,124,123,123,123,123,123,123,123,123,123,124,124,125,126,126,127,128,128,128,128,128,128,127,126,125,124,124,123,123,122,122,123,123,124,124,124,125,125,126,126,126,127,127,128,128,128,128,128,128,127,126,126,125,125,124,124,124,124,124,124,124,124,124,124,124,125,125,125,126,126,126,126,127,127,127,127,127,128,128,129,129,129,128,127,127,126,126,125,125,124,
124,125,125,125,124,123,122,121,120,119,119,118,118,118,118,119,119,118,118,118,118,118,118,118,118,119,120,122,123,124,125,126,128,129,132,135,138,141,145,149,152,157,161,165,169,172,175,177,179,180,181,181,181,181,179,176,172,167,162,156,150,144,138,131,124,117,111,106,101,98,95,94,94,95,96,98,101,104,107,110,112,114,116,117,118,119,119,119,119,119,119,119,119,120,120,120,119,119,119,119,118,117,117,117,116,116,116,116,116,116,116,117,118,119,120,121,123,124,125,126,126,127,128,128,129,129,129,129,129,129,129,128,128,127,127,126,126,126,125,125,125,125,125,125,
125,126,126,126,126,126,126,126,126,125,125,125,125,124,124,124,124,124,124,124,124,124,124,124,124,124,124,124,125,125,125,125,125,126,126,126,126,126,126,127,127,126,126,126,126,126,126,126,127,126,126,126,126,126,126,126,127,127,127,128,128,129,129,130,131,131,131,131,131,131,131,130,130,129,128,127,126,125,124,122,121,120,119,117,117,116,115,114,113,113,112,111,111,112,113,114,115,116,117,119,121,123,125,127,130,135,139,144,150,155,160,165,169,172,176,179,181,183,184,184,183,182,180,178,175,171,167,162,157,152,146,139,130};

//**************** Trumpet C3 to F3: (Band 2)
const PROGMEM uint8_t TrumpetC3StartChunk1208[] = // recorded at low sample rate
{127,127,127,127,127,127,127,127,127,127,128,128,128,128,128,128,128,128,127,127,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,125,125,126,126,126,127,127,128,129,129,129,129,129,129,128,128,128,127,127,127,127,127,127,127,128,128,128,128,128,128,127,127,127,126,126,126,126,126,126,125,125,125,126,126,126,126,127,127,127,127,127,127,127,127,127,127,127,127,127,127,126,126,126,126,126,127,127,127,128,128,129,129,129,129,129,128,128,127,127,127,127,127,127,127,127,127,127,127,127,126,126,126,126,126,126,126,126,126,126,127,127,127,127,127,127,127, // [Trumpet] [C3 start chunk] [1208 samples] (start at 18 in file)
127,128,128,128,128,128,129,128,128,128,127,127,126,125,125,125,124,124,124,124,124,124,124,124,124,124,124,124,124,124,125,125,126,127,128,128,129,129,128,128,127,127,127,127,127,127,128,128,127,127,127,127,128,128,127,127,126,126,126,126,126,128,128,128,129,129,130,129,129,128,127,126,126,125,125,125,125,125,125,126,128,129,129,128,128,128,126,125,125,124,124,124,124,124,125,126,126,126,126,126,126,125,125,125,125,125,125,125,125,126,127,127,126,126,128,130,131,132,132,131,130,129,127,126,126,127,127,126,126,126,126,127,129,129,129,130,129,128,127,126,125,124,
123,124,124,124,124,125,125,125,127,128,129,130,130,130,129,127,125,124,124,124,124,124,124,125,125,125,125,127,128,129,129,128,127,127,126,125,125,125,125,126,128,129,130,130,129,129,128,126,125,124,124,124,124,124,124,125,126,126,125,125,125,127,127,127,127,126,125,125,124,123,122,123,124,125,126,128,129,129,130,131,131,130,129,128,126,123,122,121,121,121,122,125,128,129,129,129,127,125,125,125,125,125,125,126,128,127,125,125,125,126,127,129,130,130,130,130,129,128,128,127,126,125,124,123,122,122,124,126,128,130,132,132,131,129,128,126,125,124,124,124,125,127,
128,129,129,130,131,131,130,128,128,128,129,131,132,132,133,133,132,132,132,131,129,128,127,127,128,128,128,127,127,127,127,127,126,126,125,124,124,123,124,124,124,125,124,124,124,125,126,127,128,129,129,128,128,127,126,126,126,126,126,126,126,127,128,127,126,126,127,127,127,127,126,125,125,124,124,124,125,126,126,126,127,126,126,126,126,126,126,126,126,126,126,127,127,127,126,126,125,124,124,125,125,125,125,124,125,126,127,127,127,127,128,128,128,129,128,128,127,127,126,125,125,126,126,126,126,126,127,128,129,129,129,127,125,124,123,122,123,123,124,125,126,127,
127,127,125,125,125,125,126,127,128,129,131,131,131,132,133,132,131,131,130,129,128,128,127,128,130,131,132,132,133,135,136,135,135,135,134,134,133,132,131,130,130,130,130,129,129,128,127,127,126,124,123,121,121,121,121,122,123,124,125,125,125,125,125,125,125,125,125,126,126,126,126,125,124,123,122,122,123,125,126,126,127,126,126,125,125,124,125,125,125,125,125,125,126,126,127,127,127,127,128,127,125,125,125,126,127,128,128,129,129,127,125,125,124,124,124,124,124,124,124,125,126,126,126,126,126,126,127,127,127,127,126,124,123,122,121,121,122,122,122,122,123,123,
123,122,122,121,120,120,120,120,120,122,124,126,127,129,130,131,132,134,134,134,135,136,137,138,140,143,145,148,150,153,155,156,156,156,154,151,149,146,142,136,129,122,116,111,108,106,106,108,110,113,116,118,121,123,125,126,126,126,126,126,125,124,123,122,121,121,120,120,120,120,120,120,121,122,123,124,125,126,126,126,126,127,126,126,125,125,124,125,125,124,123,123,123,124,125,126,126,126,125,124,123,123,123,123,123,123,123,123,124,125,125,126,127,128,129,130,130,130,130,129,128,127,127,126,126,126,126,126,126,126,127,127,128,128,128,127,126,125,124,122,122,122,
122,122,122,122,122,123,124,125,124,123,122,121,121,121,123,123,124,125,126,127,128,129,130,132,133,134,135,137,139,141,143,145,146,148,148,148,148,146,145,143,142,141,140,138,137,137,138,139,140,142,142,141,138,134,129,124,120,116,113,112,112,112,113,113,114,115,115,116,117,117,118,120,121,121,121,121,121,121,121,121,121,120,120,121,121,121,122,124,125,126,127,127,127,126,125,124,124,123,123,123,123,123,123,124,125,126,126,127,128,128,128,127,127,125,124,124,123,123,123,124,124,125,125,126,126,127,127,128,128,128,128,127,126,125,125,125,125,125,124,124,125,125,
125,125,126,126,126,127,127,127,128,128,129,129,128,127,126,126,125,125,125,125,125,124,122,121,120,119,119,119,119,119,119,119,119,118,118,119,121,122,124,125,127,129,133,137,141,146,151,156,162,167,171,173,176,177,177,177,174,169,163,156,149,141,132,123,114,107,102,98,97,97,99,102,105,109,113,116,117,119,119,119,119,119,120,120,120,120,120,120,119,118,118,117,117,116,116,117,118,119,120,122,123,125,126,127,128,128,129,129,129,129,128,128,127,127,126,125,125,125,125,125,126,126,126,126,126,126,125,125,125,124,125,125,124,124,124,124,124,124,124,125,125,125,125,
126,126,126,126,127,127,126,126,126,126,127,126,126,126,126,126,127,127,127,128,129,130,131,131,131,131,131,130,129,128,126,125,123,121,120,118,117,116,115,114,113,112,112,114,116,117,118,121,123,126,130,136,143,150,156,162,168,172,176,179,180,179,178,175,172,167,161,154,147,138,127};
const PROGMEM uint8_t TrumpetF3StartChunk1208[] = // recorded at high sample rate
{127,127,127,127,127,126,126,126,126,126,126,126,126,127,127,127,127,127,127,126,126,126,126,126,126,126,126,126,126,126,126,127,127,127,127,127,127,127,126,126,126,125,125,125,125,125,125,125,125,125,125,125,125,126,126,126,126,126,125,125,126,126,126,127,127,127,128,128,127,127,127,126,125,125,125,125,125,125,124,124,125,125,125,125,125,125,125,126,126,126,126,126,126,125,125,125,125,125,125,125,125,125,126,126,127,127,127,127,128,128,128,128,128,128,127,127,127,127,127,127,127,128,128,128,128,128,128,127,126,126,125,125,125,124,124,125,125,126,127,127,128,128, // [Trumpet] [F#3 start chunk] [1208 samples] (start at 126 in file)
128,127,127,127,126,126,126,126,125,125,125,125,125,125,125,125,125,125,125,125,125,125,124,125,125,126,127,127,127,127,128,128,128,127,126,126,126,127,127,126,126,125,125,125,125,126,126,126,126,126,127,127,128,128,129,128,127,126,125,125,125,125,125,125,126,127,127,127,127,128,127,126,126,125,125,125,125,126,126,126,126,127,127,127,127,127,127,127,127,128,128,128,128,128,128,128,127,127,127,127,127,127,127,126,126,125,124,123,122,122,122,122,123,125,126,128,128,129,129,129,129,129,129,128,128,128,128,127,126,125,126,126,127,128,129,129,130,130,130,130,131,131,
130,129,128,127,126,126,125,125,125,125,124,125,126,127,128,128,129,129,129,128,128,127,127,127,127,127,127,127,128,128,128,127,127,127,128,128,129,129,129,128,128,127,126,125,124,124,124,124,124,125,125,126,126,126,127,127,126,125,125,125,125,125,125,125,125,126,126,125,124,124,123,124,125,125,125,125,125,125,124,124,123,123,123,125,126,127,127,127,127,126,125,125,125,125,126,127,127,128,128,128,128,127,126,126,125,125,124,124,124,124,125,126,127,129,131,131,130,129,128,127,125,125,125,126,127,127,125,125,125,125,125,126,126,127,127,126,126,126,126,126,126,127,
127,126,126,126,127,128,129,128,127,126,126,127,128,130,131,132,132,131,129,128,130,133,135,137,136,134,131,128,128,130,132,133,131,128,126,124,123,122,123,124,125,125,124,123,122,122,122,123,123,124,124,124,124,124,124,125,127,127,128,128,128,128,127,127,127,127,126,124,123,123,124,124,125,125,126,126,126,125,125,124,124,124,125,126,127,127,127,126,126,126,125,123,122,122,123,124,125,125,126,125,124,122,122,122,122,121,121,123,124,125,125,125,126,126,126,125,125,124,124,124,124,124,124,125,125,125,125,125,127,129,128,127,125,124,124,124,124,125,125,127,128,129,
129,129,130,131,131,132,132,132,131,130,130,131,132,133,134,137,139,140,142,143,144,144,142,142,142,142,141,141,139,137,134,132,130,128,126,125,124,123,123,123,122,122,121,121,121,121,122,122,123,124,124,124,125,125,125,125,125,125,124,123,122,122,122,123,124,124,125,125,125,124,124,124,124,124,124,123,122,122,122,121,122,122,124,125,125,125,125,124,124,124,124,124,124,123,123,123,124,124,124,124,124,124,123,123,124,125,125,124,123,123,123,123,123,123,123,123,122,122,121,120,119,118,119,119,119,119,118,118,117,117,116,115,116,117,118,119,120,120,121,121,122,122,
121,121,122,123,125,127,129,130,130,131,132,134,136,139,142,146,149,152,156,159,162,165,168,171,174,175,176,177,177,176,174,170,164,156,147,136,125,115,107,101,97,95,94,96,98,100,104,108,112,115,118,121,123,125,125,125,125,125,125,125,125,125,125,125,124,124,123,122,121,120,119,118,117,117,117,117,117,118,118,120,121,121,122,123,124,124,123,123,122,122,121,120,120,120,121,122,123,124,124,125,126,125,125,125,126,127,129,130,131,131,132,131,131,130,128,126,124,122,121,119,118,118,117,117,116,115,115,115,115,116,116,115,114,113,111,110,110,110,111,111,110,110,110,
111,113,116,118,120,122,123,125,127,128,130,132,134,137,140,144,148,152,157,162,165,169,172,174,176,178,179,179,177,174,170,165,161,156,152,148,144,141,138,137,134,131,127,122,118,114,110,108,107,108,108,109,111,114,116,119,121,123,123,124,123,123,122,121,120,119,118,118,118,119,119,119,120,120,120,119,119,118,118,117,116,115,114,113,114,114,114,115,116,117,118,119,120,121,121,121,122,122,123,124,126,128,130,131,131,131,131,132,132,133,133,132,131,130,129,128,128,127,126,125,124,123,122,121,119,118,116,115,114,114,113,113,113,113,113,112,112,112,112,111,111,111,
111,111,111,112,113,114,115,116,118,120,121,123,124,126,129,130,132,133,134,136,138,141,145,149,153,156,159,161,164,166,167,168,169,169,168,167,165,163,160,158,155,152,149,146,143,140,138,135,134,135,135,134,133,132,129,127,125,123,122,121,120,120,120,120,121,122,123,122,121,119,117,116,116,115,114,114,114,113,113,113,113,113,114,114,114,114,114,114,114,113,113,113,112,112,113,113,114,116,117,118,120,122,124,125,126,127,129,130,131,132,132,133,133,133,133,133,132,131,130,129,128,127,125,123,122,120,118,116,115,115,115,115,115,115,115,115,116,116,116,117,117,118,
118,119,119,120,120,121,121,122,122,122,122,123,123,123,123,123,123,123,123,123,123,123,123,123,123,123,124,125,125,126,126,126,126,126,127,128,128,128,130,131,134,137,141,145,149,152,155,158,161,164,167,169,171,172,173,173,174,174,174,173,172,170,168,165,161,157,151,146,141,135,129};

//**************** Trumpet F#3 to B3: (Band 3)
const PROGMEM uint8_t TrumpetF3StartChunk847[] = // recorded at low sample rate
{126,126,126,126,126,126,127,127,127,127,126,126,126,125,125,125,126,126,126,127,127,127,127,126,126,125,125,125,125,125,124,125,125,125,126,126,126,126,125,126,126,127,127,128,128,127,127,126,125,125,125,124,124,124,124,124,125,125,126,126,126,126,125,125,125,125,125,125,125,126,127,127,127,128,128,128,128,127,127,127,127,127,128,128,128,128,127,126,125,125,124,124,125,126,127,128,128,128,127,126,126,126,125,125,125,125,125,125,125,125,125,124,124,125,126,127,127,128,128,128,126,126,126,127,126,126,125,124,125,126,126,126,127,128,129,128,127,125,125,125,124, // [Trumpet] [F#3 start chunk] [847 samples] -7dB (start at 92 in file)
125,126,127,127,128,127,126,125,125,125,125,126,126,127,127,127,127,127,128,128,128,128,128,127,127,127,127,127,126,125,124,122,121,121,122,124,127,128,129,129,129,129,129,129,128,127,126,126,127,128,129,130,130,131,131,131,130,128,126,126,125,125,124,124,125,127,129,129,129,128,127,127,127,127,127,127,128,128,127,127,128,129,129,128,127,126,124,123,123,124,125,125,126,126,127,126,125,124,125,125,125,126,126,125,123,123,124,125,125,124,124,124,122,122,124,126,127,127,126,125,124,125,126,127,128,128,128,127,126,125,124,124,124,124,126,129,131,131,129,127,125,
124,126,127,126,124,125,125,126,127,127,126,126,126,126,127,127,125,126,128,129,128,126,126,128,130,132,132,130,128,132,136,138,135,130,127,130,133,132,128,124,122,122,124,125,124,122,122,122,123,123,124,124,124,125,127,128,128,128,127,127,127,126,123,123,124,124,125,126,126,125,124,124,125,126,127,127,126,126,124,122,122,123,124,125,125,123,122,122,121,121,123,125,125,125,126,126,125,124,124,124,124,124,125,125,125,128,129,126,124,124,124,124,125,128,129,129,131,132,132,133,132,130,131,132,134,137,141,143,145,146,144,143,143,143,141,138,134,130,128,125,123,
123,122,121,121,120,120,121,122,123,124,125,125,125,125,124,123,122,122,123,124,124,125,124,124,124,124,123,122,121,121,121,123,125,125,124,123,123,123,123,123,123,123,124,124,124,123,123,124,125,124,123,122,122,122,122,122,121,120,118,118,118,118,118,117,116,115,114,115,117,119,119,120,121,121,121,122,124,127,129,131,132,134,137,142,147,152,158,163,168,172,177,180,181,182,181,177,168,155,140,122,108,98,93,91,93,97,102,109,114,119,123,125,125,125,125,125,125,125,125,124,123,121,120,118,116,116,116,116,117,119,120,121,123,123,123,122,121,120,119,120,121,123,
124,125,125,125,125,126,128,130,132,132,131,130,128,124,121,119,117,117,116,115,114,114,115,115,114,112,109,108,108,109,109,108,109,112,117,120,122,125,127,130,133,136,141,147,154,162,168,173,178,181,184,184,181,176,169,162,155,149,143,139,136,132,125,118,112,106,105,106,107,111,115,118,122,123,123,123,122,120,118,117,117,118,119,119,119,118,118,117,116,114,112,112,113,113,115,116,118,119,120,121,121,123,124,127,130,131,131,131,133,133,133,131,130,129,128,127,125,123,122,119,117,115,113,112,112,112,111,111,111,110,110,109,109,109,110,112,113,115,118,120,122,
125,129,131,134,136,138,143,149,155,160,164,168,171,172,173,172,170,167,163,159,154,150,145,141,137,135,135,135,133,130,126,123,121,120,119,119,121,122,122,120,117,115,114,113,113,112,111,111,112,112,113,112,112,112,112,111,111,111,112,114,116,118,121,124,126,127,129,131,132,133,134,134,133,132,131,130,128,125,122,119,116,114,113,113,113,114,114,115,115,116,116,117,118,119,120,120,121,122,122,122,122,123,123,123,123,122,122,122,122,123,125,125,126,126,126,127,128,128,130,134,139,145,151,155,160,166,170,173,176,178,178,179,179,177,175,171,166,159,150,142,133};
const PROGMEM uint8_t TrumpetC4StartChunk847[] = // recorded at high sample rate
{127,127,127,126,126,125,125,125,125,125,125,125,125,125,126,126,126,125,125,125,124,124,124,124,124,124,124,125,126,127,127,127,127,127,127,128,128,127,127,126,125,124,123,123,123,123,124,124,125,125,125,126,126,126,126,125,125,125,125,124,125,125,125,126,126,126,126,127,127,127,127,127,127,128,128,128,129,129,129,129,129,127,126,126,125,124,124,125,126,127,127,127,126,126,126,125,125,124,124,124,124,124,125,125,125,125,124,124,125,126,127,127,128,128,127,126,126,126,127,126,125,124,124,125,125,125,126,127,129,130,129,128,127,126,125,125,125,126,127,127,127, // [Trumpet] [C4 start chunk] [847 samples] -7dB (start at 91 in file)
127,125,125,125,125,126,127,127,127,128,127,127,127,127,127,128,128,127,127,126,126,126,125,125,124,123,121,121,121,121,124,126,128,128,128,128,128,128,128,128,127,126,127,128,129,130,130,131,131,130,130,129,127,125,124,124,124,124,124,125,127,129,129,129,129,127,127,127,126,126,127,128,128,127,127,127,128,128,127,126,125,124,123,123,124,125,126,127,127,128,128,126,125,125,125,125,125,126,125,124,123,123,124,124,124,124,124,123,122,122,124,126,127,127,125,124,124,125,126,128,129,129,128,127,125,124,124,123,123,124,126,129,131,131,129,127,125,124,125,127,126,
125,126,126,127,128,128,127,126,126,125,126,126,125,126,129,130,129,127,126,128,129,131,131,130,127,129,134,137,136,132,127,128,132,132,129,125,122,121,123,125,125,123,121,121,121,121,122,122,122,123,125,127,128,129,129,129,128,128,126,123,122,123,124,125,126,125,124,123,123,123,125,126,126,125,125,124,122,122,123,124,125,126,124,122,122,121,120,121,123,123,123,124,125,124,124,124,125,125,125,126,126,125,127,129,127,124,123,123,124,124,127,129,130,130,132,132,132,132,131,130,130,132,134,138,141,143,146,146,144,143,143,143,141,138,133,130,127,124,123,122,121,
121,120,120,120,120,121,123,123,124,125,125,125,125,124,123,122,123,124,124,125,124,123,123,124,124,123,123,122,122,122,124,124,124,122,122,122,122,122,122,122,122,123,123,123,123,123,124,125,123,122,122,122,122,122,121,120,118,116,116,116,116,116,116,115,114,115,116,118,119,120,120,121,121,121,122,124,127,129,130,131,133,136,140,146,151,157,163,168,173,178,181,183,183,183,179,170,157,141,123,108,97,91,90,91,95,101,107,114,119,124,126,126,126,125,125,124,124,124,123,122,122,121,119,118,117,116,117,117,119,120,121,122,122,122,122,121,120,119,119,120,122,123,
124,125,125,124,125,127,129,131,132,131,130,128,124,121,119,117,116,115,115,113,113,114,115,114,112,110,108,107,108,108,107,108,110,115,119,122,125,127,130,132,135,139,144,150,158,165,170,176,179,182,184,183,178,172,165,158,152,146,141,138,134,127,120,113,107,104,104,105,108,112,116,120,122,123,123,123,121,119,118,118,119,120,120,120,119,118,117,116,114,111,110,110,111,112,113,116,118,120,120,121,122,124,126,129,131,131,131,132,133,133,132,130,129,128,127,126,124,122,121,119,116,114,112,111,111,110,110,109,109,109,109,109,109,110,112,114,116,118,120,121,124,
127,129,132,134,136,140,146,153,159,163,167,170,172,173,173,172,169,165,161,156,151,147,142,138,135,135,135,134,131,127,124,122,120,118,118,119,121,122,121,118,116,115,114,114,113,113,113,113,113,113,112,112,111,111,110,109,109,110,111,114,116,119,122,125,127,129,132,133,135,135,135,134,133,131,130,128,125,122,119,116,113,112,112,112,113,113,114,114,115,116,118,119,119,120,121,121,121,121,121,121,121,121,121,121,121,120,120,120,121,123,124,125,125,125,126,127,128,129,132,137,143,150,155,160,165,170,174,177,179,180,180,180,179,176,173,168,161,153,145,137,127};

//**************** Trumpet C4 to F4: (Band 4)
const PROGMEM uint8_t TrumpetC4StartChunk600[] = // recorded at low sample rate
{127,126,125,125,125,125,124,125,125,126,126,125,125,124,124,124,124,124,125,127,127,127,127,127,128,127,127,125,123,123,123,124,124,125,125,126,126,126,125,125,124,125,125,125,126,126,126,127,127,127,127,128,128,129,129,129,128,126,125,124,125,126,127,127,126,126,126,125,124,124,124,125,125,125,124,125,126,127,128,128,126,126,127,126,125,124,125,125,126,128,130,129,127,126,125,125,126,127,127,126,125,125,126,127,127,128,127,127,127,128,128,127,126,126,126,125,124,122,121,121,123,127,128,128,128,128,128,128,126,127,129,130,131,131,130,129,126,124,124,124,124, // [Trumpet] [C4 start chunk] [600 samples] -7dB (start at 66 in file)
126,129,129,129,127,127,126,126,128,128,127,127,128,127,126,124,123,123,125,127,127,128,127,125,125,125,125,126,124,123,124,124,124,124,122,122,125,127,127,125,124,125,127,129,128,127,125,124,123,123,125,130,131,129,126,124,126,127,125,126,127,128,127,126,125,126,126,126,129,130,127,126,129,131,131,127,131,137,136,129,128,132,129,124,121,123,125,123,121,121,121,122,122,123,127,129,129,129,128,126,123,123,124,125,125,124,123,123,125,126,125,125,122,122,124,125,124,122,122,120,122,123,124,125,124,124,125,125,126,126,126,129,126,123,123,124,126,130,130,132,132,
133,130,130,132,136,141,145,146,144,144,143,140,134,129,125,123,122,121,120,120,120,122,123,124,125,125,124,123,123,124,124,124,123,124,124,123,122,122,123,124,123,122,122,122,121,122,123,123,123,123,125,124,122,122,122,122,120,117,116,116,116,116,115,114,116,119,120,121,121,121,123,127,129,130,133,138,146,154,162,169,176,182,184,183,178,163,141,116,99,90,90,95,103,113,120,126,126,126,125,124,124,123,122,120,118,117,116,117,118,120,121,122,122,121,119,119,121,123,124,125,124,126,129,131,132,130,127,122,118,116,115,114,113,114,114,112,108,107,108,107,107,112,
119,123,126,130,133,138,146,156,166,174,180,184,184,178,169,159,150,142,138,130,120,110,104,104,106,112,118,122,123,123,121,119,118,119,120,120,119,117,115,112,110,110,112,114,118,120,121,122,125,129,131,131,132,133,132,130,128,127,125,123,120,117,113,111,110,110,109,109,109,109,109,111,114,117,120,122,126,130,133,137,143,153,161,166,171,174,174,172,167,161,154,147,140,136,135,135,131,126,122,119,118,119,122,121,117,115,114,113,113,112,113,112,112,111,110,109,109,110,113,117,122,126,129,132,134,135,135,133,131,129,125,121,117,113,112,112,113,113,114,116,117,
119,120,121,121,121,121,121,121,121,120,120,120,122,124,125,125,126,127,129,133,140,150,157,164,171,176,180,180,180,179,175,168,159,147,135};
const PROGMEM uint8_t TrumpetF4StartChunk600[] = // recorded at high sample rate
{128,128,128,127,127,127,127,126,125,125,124,123,123,123,123,124,124,124,124,126,127,127,127,128,128,127,127,127,126,126,125,126,126,127,127,128,128,127,126,126,126,125,125,125,125,125,125,125,125,125,125,125,125,125,125,124,124,124,124,124,124,124,124,124,123,123,123,123,124,125,125,125,125,125,124,124,123,123,124,124,125,124,124,123,124,125,125,126,126,126,125,124,124,124,124,124,124,125,125,125,125,125,125,126,126,126,126,125,125,125,125,126,126,127,128,129,129,129,129,130,131,131,131,131,131,131,130,129,129,128,128,127,126,126,126,126,126,127,127,127,125, // [Trumpet] [F#4 start chunk] [600 samples] (approx-5dB?) (start at 95 in file)
124,123,123,123,124,124,124,124,125,125,125,124,124,124,124,123,123,122,122,122,123,123,123,123,123,123,123,123,123,123,124,124,125,125,125,125,125,125,127,127,126,125,124,123,123,122,122,122,122,123,124,125,125,125,125,126,127,127,127,128,129,130,131,132,134,135,136,138,140,142,142,142,141,138,136,132,128,125,122,120,118,118,118,119,120,121,122,122,123,124,125,126,126,127,127,127,127,126,124,123,122,121,120,120,121,122,122,122,123,124,125,125,124,123,123,122,121,122,123,124,125,123,123,123,124,125,124,122,120,120,120,121,122,122,123,124,124,125,125,126,127,
129,131,134,136,140,144,148,151,155,159,159,157,152,145,137,129,122,117,113,111,110,111,112,114,117,119,120,123,125,127,128,127,125,124,124,124,123,123,122,120,120,121,124,124,124,123,123,123,124,125,126,125,123,121,121,121,119,118,118,118,116,116,116,117,118,118,120,120,119,118,120,122,124,126,129,132,137,142,149,156,165,176,185,193,193,179,156,131,112,100,91,89,91,93,98,105,110,115,118,121,124,126,127,128,128,127,125,124,122,121,120,120,120,119,120,121,123,123,124,124,124,123,122,122,121,122,123,122,120,119,118,118,118,118,119,121,119,118,116,115,117,119,
121,122,126,129,133,138,144,150,156,162,170,179,190,201,204,187,153,120,101,90,85,86,89,94,101,108,115,120,123,126,127,126,125,122,121,120,120,120,120,120,119,120,119,119,120,122,124,124,123,122,121,120,120,121,121,122,123,123,122,122,121,120,119,119,118,116,114,113,112,112,113,114,114,116,119,123,128,133,136,141,145,149,153,156,159,161,162,163,164,166,168,164,155,144,132,121,113,109,106,106,108,111,114,115,116,118,117,117,118,120,122,123,123,121,121,121,121,122,123,123,122,123,123,124,125,125,125,124,124,124,124,124,124,123,122,121,119,118,117,117,116,115,
115,116,116,117,117,118,118,118,118,118,119,119,121,125,130,135,140,147,153,160,165,168,168,166,164,161,159,158,160,163,165,160,146,130};

//**************** Trumpet F#4 to B4: (Band 5)
const PROGMEM uint8_t TrumpetF4C5StartChunk426[] = // same chunk used for all notes in band
{128,128,127,127,127,126,125,124,123,123,124,124,124,126,127,127,128,127,126,127,126,126,126,127,128,128,127,126,126,125,125,125,125,125,125,125,125,125,124,124,124,124,124,124,124,123,123,123,124,125,125,125,124,124,123,124,125,124,123,124,125,126,126,125,124,124,124,124,124,125,125,125,126,126,126,126,125,125,126,126,127,129,129,129,130,131,131,131,131,130,129,128,128,127,126,126,126,127,127,125,124,123,123,124,124,124,125,125,124,124,124,123,122,122,123,123,123,123,123,123,123,124,124,125,125,125,126,127,126,125,124,122,122,122,122,123,125,125,125,126,127, // [Trumpet] [(F#4 &) C5 start chunk!] [426 samples] (approx-5dB?) (start at 68 in file)
128,129,130,131,133,135,137,140,142,142,140,137,132,127,122,119,118,119,120,121,122,123,124,125,126,127,127,126,125,123,121,120,120,121,122,123,124,125,124,123,122,121,122,124,124,123,123,124,124,122,120,120,122,122,123,124,125,126,127,130,133,136,141,147,152,158,159,155,146,135,124,116,112,110,111,113,117,120,123,126,128,127,124,124,124,123,121,120,121,124,124,123,123,124,125,125,122,121,121,118,118,117,116,116,117,118,120,120,118,121,124,127,131,138,146,156,170,184,194,186,155,122,101,90,90,94,101,109,116,120,124,127,128,127,125,124,122,120,120,119,120,122,
123,124,124,123,122,122,123,122,120,118,118,118,119,120,118,116,116,119,121,125,130,136,144,152,161,172,186,201,200,160,115,93,85,87,93,103,113,120,125,127,126,123,121,120,121,120,119,120,119,120,122,124,123,122,120,120,121,122,123,122,121,120,119,118,116,114,112,112,114,115,117,123,130,136,142,148,153,157,160,162,164,166,167,157,141,125,112,107,106,108,113,115,117,118,117,120,122,123,121,121,121,123,122,123,122,124,125,125,124,124,124,124,123,122,120,118,117,116,115,115,116,117,118,119,118,118,119,121,126,132,140,149,159,166,169,166,163,160,158,162,165,155,132};

//**************** Trumpet C5 to F5: (Band 6)
const PROGMEM uint8_t TrumpetC5F4StartChunk300[] = // same chunk used for all notes in band
{127,125,123,122,123,123,125,127,128,128,127,126,125,126,127,126,126,125,125,125,125,125,125,125,124,124,124,124,124,123,123,124,125,125,124,123,124,124,123,124,125,126,125,124,124,124,125,125,125,126,126,125,125,126,128,129,129,130,130,130,130,128,127,127,126,126,127,125,122,122,123,123,125,124,124,123,122,122,123,123,123,123,123,123,125,125,125,126,125,123,122,122,122,124,125,125,127,128,129,131,134,137,140,142,140,135,128,121,118,118,120,122,123,124,126,127,126,124,121,120,120,121,122,124,123,122,121,122,124,123,123,124,121,120,122,122,124,125,126,130,135,141,149,157,159,150, // [Trumpet] [C5 (& F#5) start chunk] [300 samples] (approx-5dB) (start at 66 in file)
134,119,111,109,112,117,121,125,127,125,123,123,121,120,123,124,123,124,125,123,120,119,118,116,116,117,119,119,118,123,127,134,145,162,182,195,170,121,93,87,95,106,116,122,127,128,126,123,120,119,119,121,123,124,123,121,122,122,118,118,118,120,118,115,117,121,127,137,148,160,176,198,199,139,91,83,89,103,116,124,127,124,120,120,120,119,119,119,123,123,121,120,121,122,122,121,120,118,114,112,112,114,117,125,134,143,151,157,162,164,167,162,141,119,106,105,110,115,117,117,119,123,121,120,122,123,122,124,125,124,123,124,123,121,118,116,115,115,116,117,118,117,118,122,130,141,155,166,169,164,160,161,165,147};

//**************** Trumpet F#5 to B5: (Band 7)
const PROGMEM uint8_t TrumpetF5C6StartChunk217[] = // same chunk used for all notes in band
{126,125,124,123,124,125,126,126,125,126,129,131,132,132,131,129,127,126,127,126,122,121,123,124,124,123,121,122,122,122,122,124,125,125,126,123,121,121,123,124,125,128,131,136,141,146,142,131,119,115,117,121,124,127,127,125,120,119,120,122,123,121,120,122,122,123,120,119,121,123,126,130,139,152,165,164,140,114,102,106,115,123,127,124,122,119,120,123,123,125,122,118,116,114,114,117,117,121,129,146,171,205,190,119,75,79,102,120,128,128,123,118,116,119,123,123,120,119,117,115,116,115,113,118,129,146,168,198,218,149,76,70,95,120,128,123,118,117,116,117,121,121,118,119,120,119,117, // [Trumpet] [(F#5 &) C6 start chunk!] [217 samples] (approx-3dB?) (start at 26 in file)
112,108,109,115,128,144,158,168,173,176,156,118,97,100,111,115,118,121,120,120,122,123,124,123,123,121,117,114,112,112,114,115,115,122,137,160,177,178,169,171,158,109,80,85,107,121,126,127,126,120,115,113,115,117,120,121,121,119,120,120,119,118,124,135,149,156,163,179,213,154};

//**************** Trumpet C6 to F6: (Band 8)
const PROGMEM uint8_t TrumpetC6StartChunk152[] = // recorded at low sample rate // same chunk used for all notes in band
{125,123,124,125,126,125,128,131,132,132,129,126,127,124,121,123,124,122,121,122,122,123,125,126,123,121,123,124,127,132,139,146,138,120,115,120,124,127,125,119,120,123,122,120,123,122,120,120,124,128,140,159,165,131,104,107,120,126,123,119,121,123,124,120,116,113,116,118,126,145,190,194,100,75,105,126,127,121,115,121,123,121,117,116,115,114,118,141,167,216,160,66,89,121,127,117,118,116,122,118,120,119,117,110,108,117,138,159,171,176,145,99,103,113,118,120,120,122,124,123,122,117,113,111,115,114,123,147,177,173,171,151,88,85,114,125,127,122,114,114,118,120,121,119,121,118,121,133,153,158,190,189}; // [Trumpet] [C6 start chunk] [152 samples] (approx-3dB) (start at 19 in file)


//**************** Sax C2 to F2: (Band 0)
const PROGMEM uint8_t SaxC2F2StartChunk1406[] = // recorded at high sample rate // same chunk used for all notes in band
{126,124,124,123,123,124,123,124,125,125,122,121,119,121,123,128,130,128,124,121,111,102,105,116,123,123,121,121,118,116,121,118,106,114,143,142,116,114,126,118,111,122,123,113,118,121,121,118,112,109,115,115,115,115,113,113,111,118,130,136,133,132,129,118,99,95,113,125,125,126,125,112,108,112,112,106,108,112,118,120,115,109,114,111,112,115,114,112,113,115,112,103,98,107,119,130,143,152,148,141,126,101,90,108,132,136,131,122,105,102,111,114,106,103,105,112,117,119,122,135,141,146,146,133,105,88,99,125,135,130,126,120,112,113,119,114,107,110,120,124,120,118,123,125,123,121,   // [Sax] [C2 (& F#2) start chunk] [1406 samples] (start at 103 in file)
116,114,115,115,117,120,114,113,119,130,144,150,151,142,133,127,116,103,114,138,138,121,121,126,124,124,130,127,122,123,128,134,132,126,126,132,135,133,125,131,133,131,132,126,122,123,126,128,136,142,152,165,170,171,161,143,116,113,143,163,153,147,154,150,140,134,128,123,134,154,168,161,144,133,131,135,137,139,140,140,135,129,128,131,134,138,147,152,146,142,144,143,137,141,153,160,163,156,144,136,134,134,139,136,130,132,140,137,138,139,140,140,141,150,149,137,131,139,143,139,137,137,135,133,132,129,125,120,116,123,130,130,128,131,137,141,143,137,129,128,132,132,130,131,125,
121,126,129,129,130,127,129,135,133,122,120,127,135,134,124,110,114,116,116,119,122,120,121,126,129,127,123,123,123,122,116,110,112,116,121,123,119,113,106,105,107,109,111,111,110,112,112,111,111,114,120,124,128,126,122,118,114,114,115,110,107,110,111,112,109,111,115,120,120,113,109,114,114,109,111,114,115,116,119,118,109,106,108,113,118,121,121,119,119,119,119,117,121,124,124,121,115,109,107,112,118,123,120,114,112,115,116,117,116,115,119,126,125,118,110,108,109,111,118,121,120,121,124,123,114,108,106,106,111,120,126,124,122,120,120,121,123,122,117,117,118,117,120,125,122,
119,120,117,111,110,112,109,109,117,121,120,119,121,123,124,125,128,125,121,122,127,127,126,124,118,117,120,123,123,124,126,130,128,122,119,121,121,124,128,130,130,129,125,121,117,116,117,126,134,138,140,140,141,142,139,132,132,136,137,136,140,144,141,141,142,141,138,136,134,135,136,137,141,145,145,143,140,138,138,140,143,147,150,151,152,148,145,147,151,152,151,151,150,144,142,146,153,153,150,149,148,144,143,144,145,145,148,152,153,151,148,150,147,144,144,143,141,142,142,142,143,144,147,145,144,147,146,142,143,143,140,141,142,143,141,142,141,137,138,141,139,138,138,133,129,
128,129,131,131,132,134,135,133,131,132,134,131,131,137,137,134,131,132,131,126,129,135,134,128,130,132,127,120,117,112,113,116,121,123,120,117,117,118,120,121,118,118,121,124,126,126,121,115,114,116,114,116,119,123,125,122,119,117,112,111,113,114,113,114,116,118,117,113,113,112,108,104,106,107,109,114,119,120,119,117,116,114,111,109,112,114,114,114,116,112,110,111,112,111,109,110,112,113,112,112,112,113,115,117,116,114,112,112,114,113,112,112,112,113,115,114,108,108,112,111,110,111,112,112,115,121,120,118,117,115,113,116,115,110,107,108,108,107,111,114,114,112,110,108,107,
111,116,116,117,116,116,116,113,109,107,110,112,113,115,119,116,111,109,111,108,111,115,116,116,114,113,115,118,118,119,117,115,116,119,117,116,116,116,118,121,122,122,120,117,116,120,123,121,117,119,123,127,125,125,129,130,127,129,132,128,126,129,131,131,133,135,134,134,134,137,141,139,137,140,143,141,139,139,140,141,142,143,144,143,143,146,150,153,154,152,151,149,148,150,154,154,151,150,151,148,145,147,151,150,151,154,155,155,157,159,159,158,157,159,162,161,160,160,158,157,158,161,162,163,162,158,156,155,153,152,152,152,151,153,155,154,153,151,151,150,146,143,140,143,147,
148,149,148,145,144,144,141,142,145,146,144,143,145,142,139,138,135,133,131,131,133,134,134,134,133,130,126,127,130,131,132,135,134,132,129,129,131,130,128,127,125,123,124,127,123,118,120,122,121,120,121,122,120,116,114,115,115,115,115,119,118,112,112,117,118,117,120,121,115,112,111,108,106,109,114,112,109,108,106,104,105,107,110,113,114,112,112,113,112,108,108,110,111,110,111,113,112,113,116,114,111,111,112,111,109,111,116,120,121,123,123,119,114,110,109,108,108,109,108,109,107,106,106,106,106,110,111,112,113,111,109,109,109,106,108,113,115,115,115,115,114,113,113,114,116,
120,121,117,115,114,113,113,113,115,117,117,113,113,117,120,115,111,111,113,109,106,109,110,108,111,115,114,111,110,111,109,109,109,109,106,105,105,105,103,102,105,108,105,102,105,107,103,101,106,109,109,111,114,111,110,110,111,112,112,111,114,117,118,117,118,118,117,120,121,117,118,123,127,127,126,125,123,125,127,128,130,130,133,136,137,136,135,137,138,140,143,144,144,145,147,149,149,149,150,152,155,156,158,158,157,158,159,161,163,168,169,166,165,163,161,164,165,162,161,161,161,161,162,163,164,165,168,169,166,164,166,167,166,167,169,170,167,163,162,164,162,161,162,164,166,
165,165,165,165,164,163,163,160,156,151,148,149,151,153,154,153,147,147,148,147,147,150,151,151,151,152,151,151,150,148,147,146,147,145,144,140,138,134,129,126,124,124,124,123,125,130,132,134,134,133,128,126,128,128,127,124,125,128,128,126,124,120,117,119,122,122,119,117,115,114,113,113,112,112,113,114,113,113,112,109,108,108,107,105,103,102,102,103,106,109,111,111,109,105,104,105,102,100,100,101,101,103,104,103,103,107,107,108,108,109,110,108,109,109,106,105,106,107,110,111,111,110,109,106,105,103,102,104,105,107,106,105,105,107,107,108,111,112,109,109,110,108,107,106,107,
108,111,114,116,114,117,115,113,112,112,110,109,110,111,113,114,113,116,120,121,123,122,120,119,117,115,112,109,108,109,110,110,112,118,120,114,113,112,108,104,105,106,102,101,104,104,102,104,102,103,103,102,102,99,95,94,93,92,95,98,99,98,97,96,94,97,101,104,104,102,97,93,96,101,102,103,106,105,102,102,101,99,98,99,102,105,108,110,112,114,118,123,125,127,128,127,124,125,129,130};

//**************** Sax F#2 to B2: (Band 1)
const PROGMEM uint8_t SaxF2StartChunk770[] = // recorded at low sample rate
{126,126,126,125,122,118,118,119,120,122,120,118,117,115,115,117,118,121,122,122,120,117,115,115,120,127,131,128,122,114,104,104,117,127,126,124,121,115,117,113,103,122,138,115,111,125,112,115,120,109,113,116,115,115,114,120,119,115,110,107,108,117,132,135,133,130,120,104,110,125,121,123,121,110,113,113,108,112,116,122,119,113,115,112,111,113,111,114,116,113,108,113,124,132,143,146,144,125,97,95,124,136,133,126,112,118,127,119,114,116,120,125,129,143,150,153,148,121,105,122,143,142,135,131,125,130,131,125,127,138,139,135,136,138,139,139,134,137,138,140,137,130,133,144,157,   // [Sax] [F#2 start chunk] [770 samples] (start at 68 in file) 
164,161,152,147,140,126,142,160,141,134,141,136,138,141,135,137,146,150,142,138,145,149,141,138,144,141,141,133,126,129,130,140,148,161,171,176,165,138,105,122,161,150,141,148,136,125,118,110,126,153,155,132,117,119,125,126,128,126,119,110,113,117,119,129,131,119,119,122,115,124,139,144,138,121,110,107,108,110,104,111,121,119,119,118,115,118,129,118,104,113,117,113,117,119,118,117,109,100,91,99,111,111,115,124,128,127,118,109,115,119,115,117,108,107,114,116,118,117,123,128,115,112,124,131,124,105,105,110,109,116,115,117,127,132,126,123,121,118,113,108,116,123,126,117,105,
100,102,107,110,110,113,117,118,118,122,126,130,130,123,117,115,121,120,116,120,121,117,113,116,119,119,113,119,123,117,118,121,120,123,124,117,118,124,130,136,136,133,131,129,125,129,137,137,135,128,127,134,139,139,132,131,138,140,139,136,140,150,150,142,138,137,138,143,145,143,150,151,143,139,141,143,154,160,155,152,150,150,152,150,145,147,147,151,157,151,149,149,142,139,140,137,143,149,145,142,145,148,151,153,150,144,148,149,144,139,133,135,139,137,137,140,141,133,129,129,127,129,133,135,136,134,125,121,117,123,132,134,134,133,132,125,117,121,128,126,128,127,120,121,122,
118,115,112,112,110,113,118,119,116,113,112,113,113,115,115,118,115,111,114,117,115,115,115,108,109,119,122,116,113,109,106,110,111,107,111,114,115,112,115,111,110,110,106,108,108,108,110,114,114,113,117,112,109,111,107,107,110,110,114,116,114,116,117,115,116,108,100,101,103,107,111,116,115,114,114,114,108,112,117,114,114,116,113,113,120,117,111,115,115,110,107,103,106,109,113,112,110,111,110,112,110,111,119,126,128,126,120,124,127,127,131,134,133,129,127,126,129,132,133,134,135,137,137,137,141,140,136,138,137,142,149,152,150,149,150,151,150,153,155,154,159,158,150,149,152,
153,152,155,156,156,159,160,161,164,164,163,163,165,165,163,161,157,156,157,152,153,157,155,155,156,153,154,152,147,151,150,149,151,145,140,140,142,142,145,144,138,134,129,129,136,140,140,140,137,133,128,129,130,131,130,132,130,126,126,126,125,129,128,124,120,121,123,122,121,115,112,116,115,116,118,120,126,126,120,113,106,104,105,102,101,104,107,106,107,113,111,110,116,113,106,105,103,105,112,116,119,119,120,112,102,106,110,110,113,112,108,106,107,106,105,107,112,112,110,110,109,108,109,111,108,107,111,108,107,113,111,109,110,110,110,113,111,110,108,111,115,114,113,115,115,
115,117,116,113,111,110,109,110,110,107,104,107,106,105,105,105,103,100,101,104,108,109,110,109,107,104,103,110,117,112,115,119,116,116,115,115,117,119,118,118,123,128,125,125,129};
const PROGMEM uint8_t SaxC3StartChunk770[] = // recorded at high sample rate
{126,124,123,121,120,121,122,122,121,119,117,117,118,119,121,122,123,123,122,121,120,120,121,123,123,123,120,118,116,118,120,121,123,122,120,118,116,116,116,117,117,118,118,116,116,116,116,118,120,122,121,119,119,118,114,112,116,120,118,116,119,121,120,122,122,118,115,124,134,122,108,117,125,116,114,120,117,113,117,122,120,117,116,116,117,118,117,115,114,116,120,124,129,130,128,132,132,119,107,110,122,126,127,130,129,124,126,128,122,118,120,126,130,131,128,126,129,128,128,128,127,127,128,129,130,125,127,136,141,145,152,153,150,146,137,124,125,143,154,150,144,139,133,136,143, // [Sax] [C3 start chunk] [770 samples] (start at 49 in file)
142,137,138,142,146,146,145,149,155,157,158,154,141,127,129,145,156,153,147,144,142,139,142,143,139,137,142,145,142,137,138,141,141,141,140,140,139,138,135,135,133,129,128,133,140,145,146,141,137,133,130,121,119,134,145,133,121,125,127,122,122,125,120,117,119,124,125,121,119,122,126,125,119,115,119,115,113,114,111,111,114,115,115,118,122,131,138,140,136,123,105,90,106,136,140,123,119,120,112,105,105,104,107,120,133,132,117,106,105,109,113,114,113,113,111,107,104,106,110,111,116,123,121,113,113,116,113,110,120,130,133,133,127,117,113,109,107,109,108,109,119,125,121,117,115,
114,112,117,126,121,111,115,124,124,118,117,120,120,119,117,113,111,105,107,117,122,119,118,122,125,128,129,124,118,122,126,122,121,122,118,120,128,129,131,132,132,135,137,128,119,125,138,144,142,131,126,130,129,128,133,138,138,143,149,150,146,146,149,149,146,139,135,139,145,150,150,146,140,138,139,142,143,143,142,143,146,147,147,148,152,155,157,159,156,151,147,148,151,150,147,147,149,148,143,139,140,145,147,145,141,142,146,142,135,138,141,139,138,139,136,130,131,137,140,140,140,138,136,137,139,137,133,135,135,132,130,126,121,122,125,126,124,123,122,124,124,122,120,118,119,
125,130,125,118,113,112,111,115,120,120,117,118,120,115,108,107,110,111,114,120,120,117,116,116,115,117,118,114,108,109,113,114,116,116,111,110,113,110,106,107,109,106,107,114,114,109,108,112,116,117,119,119,114,111,112,114,112,111,111,112,113,114,114,114,114,114,113,108,105,107,109,108,112,116,116,115,114,110,106,105,105,107,114,121,118,115,114,116,117,113,110,114,120,118,117,120,120,114,112,114,113,112,115,116,116,116,119,124,129,130,129,125,122,121,124,129,135,135,137,138,133,131,136,142,144,145,147,144,138,140,147,152,149,147,149,150,147,146,148,149,149,152,154,153,151,
153,156,154,154,156,156,156,157,158,158,160,162,163,161,161,162,159,154,155,153,149,151,155,155,155,154,149,144,146,150,151,151,149,141,137,137,140,140,140,142,143,141,136,133,136,138,135,136,138,137,135,138,139,135,130,133,137,133,130,133,133,127,121,117,115,116,117,119,117,113,111,113,114,116,116,115,117,119,119,118,113,108,107,111,114,112,111,113,115,115,115,114,113,109,107,108,108,110,115,121,124,120,115,112,110,107,108,111,112,113,115,115,113,111,108,107,109,107,110,116,115,113,114,115,110,106,111,115,114,114,112,110,112,112,113,115,115,115,113,114,116,118,121,122,118,
115,113,109,108,110,109,108,111,111,107,105,107,107,105,106,106,103,105,107,106,108,111,110,107,106,106,106,107,111,113,114,113,111,109,110,116,122,123,122,125,127,127,124,124,128};

//**************** Sax C3 to F3: (Band 2)
const PROGMEM uint8_t SaxC3StartChunk698[] = // recorded at low sample rate
{127,125,123,121,121,122,122,119,117,118,119,122,123,123,121,120,121,123,123,120,117,117,120,122,122,119,117,116,116,117,118,116,116,116,118,122,121,119,118,113,114,120,117,119,121,121,121,115,124,131,109,120,120,114,120,113,119,121,117,116,117,118,115,114,118,124,130,128,133,124,106,117,126,127,130,124,128,122,118,124,131,129,127,129,127,128,127,128,130,126,130,141,147,153,150,143,125,130,152,149,141,133,139,143,137,140,146,145,148,156,158,153,133,129,150,155,146,143,139,143,140,138,145,141,136,141,141,140,140,138,136,135,130,129,137,146,144,137,133,124,120,142,133,121,127, // [Sax] [C3 start chunk] [698 samples] (start at 34 in file)
121,124,120,117,123,124,119,122,127,119,117,116,113,113,111,114,115,119,127,138,138,126,98,98,139,131,118,118,106,105,105,121,134,119,104,108,113,114,112,109,104,107,110,116,123,114,114,114,111,126,133,132,120,112,107,109,108,117,125,118,116,112,117,126,114,116,126,118,118,120,119,115,110,105,115,121,117,122,127,129,121,120,126,121,122,118,126,130,132,132,137,129,119,135,145,136,126,130,127,135,138,143,151,147,147,150,145,136,139,148,150,145,138,140,142,143,142,145,147,147,151,155,159,155,148,148,151,147,147,149,142,139,144,147,141,143,144,135,139,140,138,138,130,133,139,140,
139,136,139,137,133,135,132,127,121,123,126,124,122,124,123,120,118,123,129,121,114,111,114,120,118,118,118,108,108,110,115,121,117,116,115,117,116,108,111,114,117,112,110,112,106,108,106,108,115,109,109,115,118,119,114,111,113,112,111,112,114,114,114,114,112,105,107,109,111,117,115,114,107,105,105,112,121,116,114,117,114,110,118,118,118,120,112,114,112,114,116,116,118,126,130,128,123,121,125,134,136,138,133,132,141,144,147,143,138,146,152,147,150,149,146,148,149,152,154,151,155,154,154,156,156,158,158,161,163,160,162,158,154,154,149,154,155,155,149,144,149,151,150,142,136,
140,140,141,143,139,133,137,136,136,138,135,139,137,130,136,133,131,134,125,118,115,116,119,115,111,113,116,116,116,119,119,115,107,109,114,111,112,115,115,114,111,107,108,109,115,123,121,114,111,107,110,112,113,115,113,109,107,108,109,116,113,115,113,106,113,115,114,111,111,113,114,115,114,114,117,121,121,116,113,108,110,109,110,111,105,108,106,106,105,104,108,106,111,109,106,106,107,111,114,113,111,109,116,123,122,126,127,124,126,131,132,135,139,137,141,143,147,155,155,155,157,157,158,160,161,164,162,164,168,173,173,171,168,164,166,164,164,166,166,162,165,165,164,168,157,
153,152,155,157,154,155,159,151,146,150,144,141,141,142,141,135,131,134,137,136,134,131,127,126,124,122,123,115,114,119,115,115,114,113,112,110,113,116,111,107,105,105,106,109,113,107,105,105,105,110,110,111,109,107,110,112,110,105,106,106,106,107,103,105,109,112,110,109,113,111,114,115,114,110,108,110,114,116,114,116,115,113,110,106,111,121,118,110,104,104,105,100,104,103,95,97,94,89,92,94,95,99,99,92,94,97,103,105,100,104,100,106,111,110,112,114,118};
const PROGMEM uint8_t SaxF3StartChunk698[] = // recorded at high sample rate
{128,128,128,128,128,127,125,123,125,127,126,125,125,122,122,124,125,125,124,124,122,119,120,121,121,123,125,123,121,119,118,118,119,120,121,120,121,121,120,120,120,119,120,120,119,118,119,121,122,122,120,120,118,115,118,123,124,123,123,122,121,125,121,119,129,127,116,122,127,121,125,126,123,126,127,125,122,123,125,125,126,125,126,129,132,134,133,136,135,125,121,130,136,133,135,136,133,134,131,127,130,134,135,132,132,132,130,131,131,131,135,136,132,129,135,142,147,148,142,139,130,121,129,141,140,136,132,127,134,135,129,129,132,134,134,137,144,144,142,134,119,119,134,140,135, // [Sax] [F#3 start chunk] [698 samples] (start at 30 in file)
132,129,126,128,126,123,126,132,130,129,128,126,125,125,125,126,124,124,124,121,123,129,134,135,130,126,123,115,111,124,127,116,119,124,119,118,117,114,118,124,124,118,118,121,118,114,118,121,117,114,111,112,114,116,120,124,129,132,131,123,106,102,123,134,120,118,125,117,109,109,112,125,134,126,113,112,116,117,118,120,121,114,113,119,121,123,128,126,121,123,125,123,132,140,139,133,123,122,127,128,124,121,130,134,132,134,135,134,140,140,129,128,139,140,136,136,133,134,134,133,128,128,137,139,136,139,146,148,145,138,135,139,140,139,137,135,138,140,139,138,140,145,141,133,137,
146,145,134,126,130,132,132,136,135,136,140,137,132,133,138,135,126,125,130,133,130,123,119,120,124,124,124,124,124,122,120,120,126,131,132,128,121,117,121,124,118,116,119,119,113,112,117,120,117,113,119,119,112,114,115,115,118,114,109,110,114,115,115,114,115,116,115,115,120,122,117,110,104,108,117,121,117,112,113,115,112,108,109,118,126,121,114,113,115,118,120,119,120,124,123,117,118,122,126,133,134,130,129,126,127,130,126,128,135,136,138,137,132,133,132,129,132,133,131,138,141,136,137,143,146,147,146,143,143,149,150,146,140,138,142,144,143,145,151,148,140,137,136,134,139,
143,146,146,142,136,133,135,143,145,146,146,145,144,140,136,140,139,135,137,135,133,136,133,127,124,124,124,126,132,135,130,124,122,121,122,126,129,130,128,120,119,125,124,121,123,119,114,119,125,120,114,115,113,111,113,112,113,117,117,112,111,112,107,107,108,106,107,108,110,110,110,108,109,109,104,106,108,108,109,109,108,108,105,106,112,112,110,108,103,102,105,107,108,112,113,111,111,116,116,116,124,125,119,121,125,122,125,133,129,126,131,130,129,131,131,130,132,132,128,129,135,141,143,141,142,148,147,143,141,142,149,150,152,156,154,150,148,145,144,149,153,152,149,153,154,
149,149,150,145,144,146,145,150,154,151,146,146,148,146,146,149,148,146,148,143,138,141,140,136,136,137,136,137,137,136,136,135,134,132,133,135,132,129,128,127,128,127,125,126,126,123,123,123,120,124,122,118,118,118,121,120,113,111,112,110,110,113,111,107,106,104,107,109,106,106,107,106,103,101,102,102,103,104,103,99,96,94,95,98,100,99,96,96,98,98,100,97,93,97,100,99,102,103,103,104,104,103,101,103,106,106,104,107,111,113,112,116,120,118,123,127,123,123};

//**************** Sax F#3 to B3: (Band 3)
const PROGMEM uint8_t SaxF3StartChunk630[] = // recorded at low sample rate
{127,127,127,127,127,127,127,127,127,127,128,128,127,126,127,129,128,128,128,128,127,128,131,131,130,129,127,126,126,128,128,128,128,126,123,126,126,125,123,122,125,125,124,123,120,120,121,124,124,120,118,118,120,121,121,121,120,120,120,120,119,118,121,122,121,119,117,116,124,122,124,120,125,119,127,124,118,126,122,126,123,128,124,123,124,126,124,127,131,134,134,135,122,128,135,134,135,133,132,127,133,134,132,132,130,131,132,137,131,131,141,149,144,138,124,129,142,136,130,131,135,127,133,133,137,144,143,133,116,132,139,133,128,126,126,123,132,130,128,126,125,125,125,124,123, // [Sax] [F#3 start chunk] [630 samples] (start at 0 in file)
122,129,136,131,125,117,113,128,117,121,121,117,115,118,126,118,120,119,115,120,118,112,112,114,119,124,131,131,117,100,128,125,119,123,110,110,121,134,117,112,118,117,121,116,113,121,123,128,122,124,124,132,142,132,123,125,128,121,130,133,134,134,138,139,127,138,139,135,134,134,133,127,136,138,137,146,147,138,137,140,138,135,138,140,138,143,142,133,143,145,129,130,132,134,135,138,138,131,138,133,124,130,133,124,119,123,124,124,124,121,120,128,132,127,118,121,122,116,120,115,112,120,117,115,119,112,115,116,117,109,111,115,115,114,116,115,118,122,112,105,112,121,115,112,115,
110,109,123,122,113,114,118,120,120,124,118,118,124,133,132,128,126,129,126,132,137,138,133,133,130,132,132,137,140,135,144,146,146,142,148,149,142,138,144,142,148,149,139,136,135,141,146,145,137,132,140,146,146,145,143,137,140,137,136,134,135,132,124,124,124,132,134,126,121,121,126,130,128,119,123,124,122,120,114,125,118,115,113,112,113,114,118,111,112,107,108,106,107,109,111,108,109,107,105,108,108,109,108,106,108,113,109,105,102,107,108,113,111,112,117,116,126,120,122,123,126,132,126,132,129,131,130,132,130,129,137,143,141,145,148,142,142,149,151,155,152,148,145,147,154,
149,153,152,149,149,144,145,148,154,149,146,148,145,149,147,147,141,140,140,135,137,137,138,136,136,133,133,134,131,128,127,127,125,126,124,123,121,123,120,117,119,120,112,112,110,111,112,106,105,107,108,106,107,103,101,102,103,103,100,95,95,98,100,96,97,98,100,94,98,99,101,103,104,105,101,103,107,104,108,113,113,119,119,126,124,124,127,129,136,135,146,140,140,145,144,151,149,153,151,153,161,158,158,160,160,163,160,160,154,161,161,161,161,158,158,156,160,161,156,152,152,154,157,153,149,147,146,146,145,142,139,140,135,133,135,133,131,130,126,126,128,124,125,122,119,118,117,
116,115,114,110,111,111,111,108,105,110,105,102,96,98,95,95,97,90,92,87,85,86,84,85,78,82,81,83,82,81,84,87,89,83,84,85,94,101,103,105,107,107,109,113,122,127,133,130};
const PROGMEM uint8_t SaxC4StartChunk630[] = // recorded at high sample rate
{128,128,125,125,128,126,125,125,126,128,125,131,133,124,128,128,125,126,124,125,127,125,126,129,128,124,122,123,127,129,129,129,122,115,121,128,128,127,124,125,124,121,122,123,122,125,126,124,122,118,119,120,120,125,130,134,135,130,119,111,122,134,131,125,121,123,122,120,124,130,131,133,137,136,127,114,117,131,131,129,131,131,129,124,124,129,130,130,132,131,129,128,127,128,128,129,133,138,136,133,131,130,125,131,140,129,126,129,130,130,128,131,136,135,130,132,132,128,129,129,126,124,125,128,131,134,139,141,137,126,116,133,141,128,126,124,117,117,123,134,132,119,119,125,125, // [Sax] [C3 start chunk] [630 samples] (start at 98 in file)
122,121,115,115,121,124,127,123,118,120,118,123,131,128,119,115,118,121,118,120,127,124,120,119,120,126,123,121,128,126,120,120,120,120,119,118,126,129,125,127,132,130,123,125,131,129,125,123,127,133,134,133,137,135,125,130,139,131,123,130,134,133,131,132,138,137,134,138,137,130,130,135,137,130,125,127,131,131,132,133,132,131,133,136,136,134,132,130,132,131,130,131,128,127,131,132,126,126,129,124,127,129,130,126,117,120,128,131,129,127,126,122,123,128,126,118,114,122,129,125,119,121,123,118,115,120,125,117,113,116,120,123,118,120,121,115,115,120,127,128,121,119,122,125,122,
119,121,124,128,126,125,124,122,122,120,123,125,123,128,134,136,135,128,129,134,133,130,131,135,133,130,133,131,126,128,131,135,138,138,134,130,130,134,141,141,137,136,136,133,135,138,136,138,136,134,135,132,128,128,129,132,135,132,128,127,129,131,132,131,127,125,130,133,130,127,122,121,126,124,120,121,121,121,119,120,123,119,116,116,114,115,116,116,115,115,119,117,117,120,117,117,115,117,119,119,117,115,119,122,121,117,116,119,121,121,124,125,123,125,126,127,133,132,130,131,128,131,132,129,133,134,131,130,131,133,133,132,136,140,140,137,138,139,139,135,136,140,141,142,143,
141,139,137,140,144,141,140,142,143,140,137,132,131,133,137,140,139,138,137,135,133,134,132,133,132,129,128,126,123,124,125,125,126,126,126,123,120,120,120,119,117,115,114,111,110,113,113,114,113,114,114,112,110,106,106,103,103,106,107,108,106,103,101,104,110,115,116,116,111,107,110,114,117,120,121,118,119,118,121,127,127,127,129,129,130,129,131,134,134,135,138,142,140,137,135,138,140,137,139,144,143,144,147,145,146,145,144,147,147,147,148,148,151,153,148,148,149,146,147,148,148,149,148,149,153,150,145,143,143,143,145,143,143,141,136,136,135,133,134,133,132,128,125,126,125,
122,119,118,117,117,116,114,109,105,104,103,102,98,97,95,95,96,95,91,92,95,94,93,93,92,92,94,97,96,99,98,97,98,99,101,103,106,111,114,112,112,114,119,123,122,124,128,127,127,127};

//**************** Sax C4 to F4: (Band 4)
const PROGMEM uint8_t SaxC4StartChunk501[] = // recorded at low sample rate
{129,129,129,129,129,129,129,129,129,129,128,129,129,128,128,128,127,128,128,128,129,128,127,127,128,129,131,130,126,129,130,132,130,129,129,129,131,133,132,130,127,128,132,134,129,124,122,126,133,137,133,129,130,130,127,125,125,128,122,127,126,128,134,125,128,126,123,124,126,125,130,126,120,124,128,130,126,112,123,130,125,124,122,119,121,122,126,124,117,116,118,121,132,137,134,112,114,137,127,119,121,118,124,132,135,141,130,109,124,134,129,133,128,123,129,131,132,133,128,128,128,130,136,141,133,133,125,139,135,125,131,130,130,138,136,132,133,127,130,124,124,128,134,141,146, // [Sax] [C4 start chunk] [501 samples] (start at 16 in file)
130,116,142,132,124,118,113,131,134,116,122,124,120,113,113,124,126,119,115,119,129,127,111,117,117,119,125,121,115,124,122,123,128,118,118,119,115,123,129,124,134,127,123,132,126,122,130,136,136,140,124,138,134,124,135,134,132,140,139,139,139,129,137,136,126,127,133,132,135,132,136,138,136,131,133,131,132,128,130,133,125,128,125,128,131,121,116,130,131,128,124,121,128,120,112,125,126,117,122,115,114,123,114,111,120,118,118,118,111,121,129,121,118,124,120,118,124,127,124,123,120,119,123,123,130,139,135,128,134,134,131,137,132,134,130,127,133,139,141,134,130,138,145,139,138,
135,138,140,140,137,136,132,128,130,134,135,127,129,132,134,127,127,133,130,123,121,126,120,119,120,118,120,119,113,113,112,115,112,115,116,116,117,114,114,117,117,114,117,121,116,114,119,121,124,123,125,127,134,132,130,130,132,131,135,133,131,134,134,135,142,141,139,143,138,138,142,144,145,143,139,145,145,143,146,143,136,132,135,142,141,140,136,134,134,134,131,128,125,122,125,124,127,124,120,119,119,115,113,109,109,111,111,111,112,109,105,101,98,101,105,103,99,97,108,113,114,107,106,112,117,119,118,116,121,127,127,129,130,129,134,136,137,143,142,136,140,140,140,146,146,149,
148,148,148,150,151,150,155,155,151,151,149,151,152,151,153,156,149,145,146,147,146,142,137,136,134,134,133,127,125,125,119,117,116,115,111,103,101,100,95,92,90,92,88,87,91,89,88,87,90,93,94,94,93,96,98,103,110,111,110,116,122,122,126,129,126};
const PROGMEM uint8_t SaxF4StartChunk501[] = // recorded at high sample rate
{126,125,124,125,124,123,123,123,124,125,125,123,125,126,125,123,121,124,126,123,123,124,127,127,131,127,125,126,125,125,127,129,129,130,130,128,125,125,129,134,136,136,131,132,132,128,125,127,130,132,134,133,131,129,130,131,131,130,127,128,132,132,131,130,124,128,133,128,124,126,125,121,123,125,127,130,130,124,120,126,125,120,120,124,123,123,126,124,121,123,123,121,123,124,123,125,129,130,127,124,123,124,130,127,126,128,128,127,129,129,127,129,132,131,131,133,132,129,131,134,136,138,136,132,126,136,140,134,134,131,128,132,141,135,126,129,130,129,128,129,132,133,131,126,126, // [Sax] [F#4 start chunk] [501 samples] (start at 12 in file)
128,131,135,131,125,122,122,120,123,126,124,123,122,121,117,121,125,123,122,120,117,114,114,118,120,124,125,120,117,120,121,122,123,124,124,124,126,121,121,129,125,122,129,129,126,129,132,131,136,137,131,132,134,130,126,129,134,135,136,139,136,135,137,137,134,135,137,136,136,133,129,130,131,129,132,132,131,130,130,128,125,128,129,125,124,125,123,125,126,120,120,125,123,118,119,122,120,120,122,116,113,117,122,121,121,120,118,121,127,129,125,122,122,122,121,125,127,128,130,129,125,126,128,134,137,136,135,136,133,132,137,138,134,133,137,139,139,137,134,136,139,142,141,136,131,
131,135,136,135,137,135,135,137,133,131,129,128,126,123,122,123,125,124,119,117,118,119,120,120,117,118,118,115,111,112,117,115,111,110,110,112,115,117,115,115,115,112,111,113,118,123,126,123,120,121,121,123,127,129,128,132,136,135,129,126,130,134,140,142,141,141,140,143,143,141,141,143,148,146,146,144,141,140,142,139,135,134,136,135,137,142,141,133,131,129,127,129,130,126,124,125,122,119,119,118,117,115,116,116,117,115,111,113,112,113,114,114,113,111,114,113,113,115,115,114,115,117,119,121,122,119,119,124,127,128,128,128,129,131,133,133,133,138,137,134,137,140,143,143,143,
145,148,146,144,144,143,147,149,148,147,144,144,143,144,143,140,140,139,137,134,133,130,129,130,131,127,121,119,117,114,116,114,112,111,110,108,105,106,107,106,102,103,103,100,102,105,105,104,104,105,104,107,110,109,110,113,114,114,115,118,119,121,124,125,126,130};

//**************** Sax F#4 to B4: (Band 5)
const PROGMEM uint8_t SaxF4StartChunk417[] = // recorded at low sample rate
{126,125,124,123,125,125,124,125,123,123,124,126,123,125,126,123,121,125,123,123,126,129,129,124,126,125,128,130,130,130,125,126,133,137,134,131,132,125,128,131,134,132,129,130,131,130,127,132,132,131,124,130,130,124,126,121,124,126,131,126,121,126,120,121,124,123,125,122,123,121,123,124,124,130,128,124,123,129,126,128,128,128,129,127,132,131,132,132,129,134,136,138,130,131,140,133,133,128,139,135,126,131,128,129,131,133,127,126,130,135,128,122,121,122,126,123,123,119,119,125,122,121,116,114,118,122,125,119,119,122,122,125,123,126,119,126,125,125,130,126,131,132,138,132,133, // [Sax] [F#4 start chunk] [417 samples] (start at 4 in file)
132,126,131,135,137,137,135,138,134,136,136,136,130,130,130,130,133,130,131,126,128,128,124,124,124,126,119,123,123,117,122,119,122,115,115,122,121,120,118,124,129,123,122,121,123,127,129,129,125,127,134,136,135,135,131,138,136,133,138,140,137,134,140,142,138,130,134,135,136,136,135,135,130,129,126,123,123,125,121,117,119,120,119,117,118,113,111,117,113,110,111,114,117,115,115,111,113,119,125,123,120,121,124,129,128,134,135,128,128,135,142,141,140,142,143,140,144,147,146,144,140,142,137,134,136,136,143,137,131,129,128,129,125,125,121,119,118,116,115,117,115,112,112,113,114,
113,112,114,113,115,114,115,118,121,121,119,125,128,128,128,131,133,133,137,135,136,142,143,143,147,146,144,144,147,149,147,144,143,144,141,140,138,134,132,129,130,129,122,119,114,115,113,111,109,106,106,107,103,103,101,102,105,104,105,104,108,110,110,114,114,116,119,121,125,126,130,132,137,138,139,143,150,152,153,154,153,152,155,156,155,157,155,152,151,151,148,145,139,138,134,133,129,127,125,119,116,113,108,110,108,108,105,102,100,101,102,100,101,101,101,103,109,111,108,107,111,112,114,120,126,128,129};
const PROGMEM uint8_t SaxC5StartChunk417[] = // recorded at high sample rate
{127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,128,127,127,127,127,127,127,128,128,129,130,132,132,130,129,128,128,130,129,130,131,132,132,130,129,129,129,130,129,127,126,126,127,127,127,126,124,122,126,126,124,122,123,124,123,122,122,122,123,125,126,126,126,127,127,121,123,131,131,127,128,127,126,128,131,134,133,128,128,132,130,130,132,131,131,131,130,130,129,129,129,128,128,129,128,126,124,126,127,124,123,122,122,122,122,123,123,120,119,119,118,120,123,127,128,125,118,122,126,123,123,123,126,131,128,126,130, // [Sax] [C5 start chunk] [417 samples] (start at 6 in file)
130,127,126,129,131,132,132,132,134,136,132,132,135,133,132,132,131,130,131,131,132,134,132,130,127,125,125,125,128,130,126,124,123,120,120,123,123,123,122,121,124,122,121,124,124,124,122,121,123,123,124,128,129,127,129,130,129,130,129,130,133,132,131,133,132,132,132,133,134,133,131,130,131,130,128,128,132,131,128,127,125,123,121,121,126,127,123,122,119,117,119,117,118,122,121,120,120,117,120,125,124,122,124,123,124,127,128,128,129,129,130,131,132,131,132,133,133,137,137,136,138,137,135,134,131,132,135,134,131,129,130,134,134,133,131,128,127,125,122,122,121,119,120,121,121,
117,116,118,120,119,118,119,120,118,117,122,122,122,123,124,126,128,128,129,130,130,130,131,131,130,132,134,135,137,139,136,134,134,135,133,133,135,135,132,131,130,131,131,130,127,128,127,124,123,120,118,118,117,116,117,115,116,115,114,116,119,121,121,119,118,119,121,123,125,127,127,127,129,131,130,130,133,136,138,139,137,136,136,136,137,138,140,140,139,138,137,137,138,137,136,135,132,130,129,127,126,125,122,120,119,118,115,114,117,116,112,111,110,112,112,112,113,113,116,118,119,122,125,124,124,126,127};

//**************** Sax C5 to F5: (Band 6)
const PROGMEM uint8_t SaxC5StartChunk407[] = // recorded at low sample rate
{127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,128,127,127,127,127,128,128,129,132,131,129,128,129,130,130,132,131,129,129,129,129,126,127,127,127,125,123,126,125,122,124,123,122,122,123,125,126,126,127,120,129,130,127,128,127,130,134,129,128,132,129,132,131,131,130,129,129,129,128,129,125,125,126,124,122,122,121,124,122,119,118,120,124,129,124,119,126,123,123,127,130,126,131,127,127,130,132,132,134,135,132,134,133,132,131,130,131,133,132,128,125,125,128,129,125,123,120,122,123,121,122,123,122,124,124,122,122,123,126,129,127,130,129,129,131,133, // [Sax] [C5 start chunk] [407 samples] (start at 0 in file)
131,133,131,132,133,133,130,131,129,128,132,130,126,124,121,123,127,124,120,117,118,118,122,120,119,117,124,123,123,123,125,128,128,130,130,131,131,132,133,137,136,137,136,134,131,135,134,129,130,134,133,130,127,124,122,121,119,121,120,116,118,120,118,120,118,119,122,122,124,126,128,129,130,130,131,130,132,134,136,139,134,134,134,133,135,133,131,130,132,128,128,126,123,120,118,117,117,116,115,115,115,120,121,119,118,122,123,127,127,129,131,130,133,137,138,137,136,136,138,140,140,138,137,137,137,136,133,130,128,126,123,120,119,115,115,116,112,110,112,112,113,114,118,119,124,
124,125,127,130,130,132,132,135,137,140,140,136,139,139,139,137,137,138,137,134,134,129,126,125,120,118,116,113,112,110,111,108,113,113,111,116,119,120,122,123,127,130,132,136,137,138,138,141,142,142,140,143,145,145,146,140,139,137,136,131,127,124,119,117,116,113,110,107,101,103,106,106,109,112,111,113,120,124,127,131,132,132,135,136,137,141,143,143,144,144,144,148,147,144,146,143,138,136,131,129,125,119,115,112,105,104,103,101,99,100,104,104,107,113,117,123,127};
const PROGMEM uint8_t SaxF5StartChunk407[] = // recorded at high sample rate
{127,127,127,127,126,126,127,126,125,125,125,124,124,125,126,127,126,126,125,126,129,129,127,127,128,130,130,128,128,129,130,130,130,130,128,127,130,128,126,126,126,126,127,126,123,124,125,125,122,121,122,124,125,126,124,124,123,125,127,127,127,129,129,127,128,129,130,131,131,130,131,132,131,129,130,131,130,130,132,130,127,125,126,129,125,124,123,123,123,124,123,123,124,126,126,123,122,124,126,128,129,129,129,129,128,128,130,130,130,131,132,131,131,129,129,129,129,130,128,126,124,122,125,126,126,123,120,120,121,124,122,121,122,123,124,125,126,126,127,129,130,129,129,130,129, // [Sax] [F#5 start chunk] [407 samples] (start at 0 in file)
131,133,134,134,132,133,132,132,132,131,129,130,132,129,126,124,124,123,122,123,122,121,120,121,123,121,120,122,123,125,125,125,126,128,130,130,130,130,130,132,134,134,133,132,132,132,133,132,130,128,129,129,127,126,124,124,122,120,120,119,118,118,119,121,122,123,121,123,125,127,128,129,129,129,131,134,135,135,133,133,135,136,136,135,134,134,135,133,130,129,128,125,124,123,121,120,119,119,117,116,117,117,117,117,118,121,124,125,125,127,129,130,131,132,134,135,135,136,135,136,136,134,134,135,136,134,132,131,130,126,124,123,121,119,117,117,115,115,115,115,116,116,118,121,122,
123,125,127,128,131,134,133,134,136,138,138,137,138,139,139,139,141,139,135,135,133,131,127,125,123,120,117,116,114,112,109,109,111,111,113,115,116,118,122,125,125,128,131,133,133,133,135,138,139,139,139,140,140,142,141,140,141,138,136,135,131,128,125,121,118,115,111,108,108,108,107,107,109,110,112,116,119,123,128,130,131,134,135,134,136,137,141,143,144,144,145,145,145,146,143,141,140,138,133,130,126,121,116,114,110,105,102,101,99,99,101,104,108,113,120,123,124};

//**************** Sax F#5 to B5: (Band 7)
const PROGMEM uint8_t SaxF5StartChunk321[] = // recorded at low sample rate
{127,127,127,126,127,126,125,125,124,124,126,126,126,125,128,129,126,128,130,129,128,130,130,131,128,128,129,126,127,126,127,124,123,125,123,122,123,125,125,124,124,127,127,128,129,127,129,130,131,130,132,131,129,130,130,130,131,126,126,128,125,123,123,123,123,123,126,125,122,124,127,129,129,129,128,129,130,130,132,131,130,129,129,130,128,125,123,126,126,122,119,122,123,121,122,124,125,126,127,129,130,130,130,130,134,134,133,132,132,132,129,131,130,126,124,123,122,122,120,120,123,120,121,123,125,125,127,130,130,130,130,133,134,133,132,132,132,130,128,129,127,125,124,121,120, // [Sax] [F#5 start chunk] [321 samples] (start at 0 in file)
119,118,119,121,123,121,124,127,129,129,129,133,135,134,133,135,136,135,134,135,133,129,127,125,124,121,120,119,116,117,117,117,119,123,125,125,128,130,132,134,135,136,135,136,134,135,136,134,131,129,125,123,120,118,116,115,115,115,116,119,122,123,126,127,132,133,134,137,139,137,139,139,140,139,135,134,130,126,124,119,116,114,110,109,111,112,115,117,122,125,127,131,133,133,136,139,139,139,140,141,141,140,137,135,130,126,120,116,111,108,109,107,108,110,114,119,125,129,131,135,135,136,140,143,144,145,145,146,143,140,138,131,127,119,115,109,103,101,99,100,105,110,119,123,126,
131,135,137,136,137,143,146,146,149,148,148,145,141,136,129,125,116,110,103,96,95,95,99,106,112,120,129,138,139,137,135};
const PROGMEM uint8_t SaxC6StartChunk321[] = // recorded at high sample rate
{127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127, // [Sax] [C6 start chunk] [321 samples] (start at 45 in file)
127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,126,126,126,126,125,126,126,126,127,129,129,131,132,132,132,133,133,133,134,133,132,132,131,130,129,127,123,118,113,107,103,100,101,107,115,125,135,141,
143,141,137,136,140,147,151,150,146,138,130,125,127,132,134,131,122,114,107,98,89,87,93,104,113,124,136,141,141,137,133};

//**************** Sax C6 to F6: (Band 8)
const PROGMEM uint8_t SaxC6StartChunk139[] = // recorded at low sample rate // same chunk used for all notes in band
{129,130,132,132,133,133,134,133,132,131,130,128,122,115,107,101,101,109,123,136,143,141,136,139,149,151,145,133,125,129,134,129,117,106,94,86,97,111,126,140,141,135,135,151,159,159,151,140,125,123,127,120,113,101,81,73,94,126,148,145,126,130,157,165,159,161,154,126,118,133,133,118,108,85,51,59,105,152,161,136,116,146,175,173,175,160,120,110,142,138,115,103,89,56,51,90,138,163,143,112,126,170,186,186,176,141,109,122,141,126,102,87,59,42,80,133,165,152,107,117,181,196,176,171,149,118,122,136,122,101,93,83,48,54,106,154,159,116,111};  // [Sax] [C6 start chunk] [139 samples] (start at 17 in file)


//**************** Violin F#3 to B3: (Band 3)
const PROGMEM uint8_t ViolinF3C4StartChunk1890[] = // recorded at high sample rate // same chunk used for all notes in band
{126,123,121,121,122,123,125,127,127,127,128,129,131,132,132,133,136,141,143,142,139,135,134,135,134,132,130,129,131,134,135,130,122,112,106,103,105,107,110,112,114,118,122,125,126,124,122,120,117,115,116,120,125,130,133,132,127,123,120,118,116,116,118,122,125,125,125,125,127,133,139,143,143,143,143,143,142,141,139,138,137,138,140,142,143,143,141,138,133,129,126,124,123,121,120,120,122,125,128,129,128,124,120,115,112,110,111,113,114,115,119,124,129,132,132,130,126,123,124,126,129,132,134,135,136,135,131,127,124,123,123,125,126,127,127,126,125,124,122,119,114,110,110,113,118, // [Violin] [C4 start chunk] [1890 samples] (start at 65 in file)
123,127,130,132,132,129,126,124,123,122,122,122,123,126,129,133,135,135,133,131,129,127,127,127,129,130,129,129,129,129,130,131,130,130,130,131,131,130,129,128,128,128,130,131,130,129,129,128,127,126,124,122,120,119,120,121,123,123,123,124,126,129,131,132,132,130,130,129,130,131,133,133,134,134,134,134,133,131,129,126,122,118,114,112,111,112,115,119,123,125,125,126,129,131,131,130,130,131,133,135,134,130,127,126,127,126,124,121,119,122,127,131,129,121,111,104,103,106,111,116,119,124,130,139,146,148,145,137,129,123,120,119,121,125,130,135,136,134,130,125,120,116,114,114,115,
116,116,117,122,128,133,138,140,139,139,138,139,140,141,141,140,138,139,140,143,145,145,145,143,140,137,134,130,125,119,114,113,116,120,125,130,131,128,124,118,112,107,105,104,104,107,112,119,127,133,135,131,126,122,119,119,118,117,117,120,123,127,130,129,127,126,125,127,129,131,130,129,129,130,130,127,121,116,112,112,116,122,128,132,136,139,140,139,135,130,125,122,120,121,124,128,133,137,140,141,139,135,131,128,126,125,124,124,125,127,130,133,135,135,133,131,131,130,130,129,127,126,127,129,130,130,128,126,125,124,124,123,120,118,115,114,115,115,115,116,118,118,120,121,121,
119,117,116,120,123,123,122,122,125,130,135,139,143,149,156,159,156,148,140,137,137,136,135,134,135,139,145,146,141,129,113,100,92,88,87,90,94,98,102,104,106,110,113,115,115,113,110,111,117,128,136,140,138,133,129,126,125,125,126,131,138,143,142,138,134,134,139,145,150,152,153,153,154,154,151,148,145,143,141,140,140,140,139,138,134,128,121,116,112,111,109,108,109,113,118,124,128,129,127,121,114,107,102,101,103,107,112,117,121,124,127,127,124,121,118,121,127,135,140,147,151,151,148,141,135,133,134,135,134,133,133,134,134,129,121,112,106,103,103,103,100,97,97,100,108,116,121,
124,128,134,141,146,147,142,133,126,122,122,125,129,133,138,143,147,150,148,142,130,115,104,101,108,120,131,136,137,138,142,143,141,134,127,124,126,132,139,142,142,141,139,136,131,124,117,112,110,112,115,118,118,117,117,118,122,129,135,140,142,142,140,138,135,133,129,125,123,124,126,130,133,135,134,130,126,121,116,112,108,108,109,113,119,125,129,131,129,124,120,118,117,118,119,120,122,125,126,127,126,123,119,114,111,113,120,128,134,136,136,135,134,133,131,128,125,125,130,137,144,149,148,144,139,135,133,131,129,126,126,126,127,127,127,126,125,124,122,120,117,115,116,120,124,
128,130,131,131,130,129,129,129,130,132,135,137,136,133,129,126,125,123,123,126,129,133,137,139,138,135,129,123,120,122,126,130,133,135,139,145,147,143,133,121,111,107,105,103,103,105,113,127,138,140,131,121,113,110,110,109,110,116,128,140,148,149,144,136,127,119,112,106,106,112,124,134,136,130,118,107,103,103,104,107,111,120,134,146,153,153,147,139,132,127,123,123,127,133,142,149,150,147,144,140,136,133,131,128,126,125,124,124,127,130,133,134,133,133,134,137,138,139,137,133,128,126,126,128,129,129,128,126,125,124,125,125,125,122,117,112,109,110,114,120,125,126,126,129,133,
134,129,117,104,94,95,103,115,124,128,130,134,136,135,128,118,109,113,129,149,160,161,155,153,155,155,146,131,117,110,112,120,126,125,120,111,103,96,89,85,87,94,106,117,124,129,136,142,145,141,131,122,121,127,137,145,149,147,145,142,139,137,132,129,130,134,136,132,124,118,116,121,129,136,141,143,143,145,146,148,148,147,142,136,131,128,129,132,135,135,133,130,128,128,129,126,119,110,102,101,105,112,120,124,125,123,121,117,113,110,107,105,106,111,118,126,132,133,131,125,119,115,114,115,119,127,134,141,147,152,152,149,145,141,140,140,137,132,128,126,127,128,125,117,107,97,92,
92,97,103,112,121,129,133,134,133,133,133,133,131,128,126,129,134,139,141,139,134,129,125,124,125,126,125,124,124,126,125,124,122,124,130,139,145,149,148,146,145,145,145,143,138,133,129,128,129,132,135,136,135,133,132,131,127,122,117,113,112,114,119,125,129,129,126,122,118,114,110,105,102,103,104,108,114,121,126,126,124,122,121,122,123,124,128,136,146,154,155,151,146,145,148,148,145,137,131,130,133,136,134,124,110,98,91,89,91,92,94,100,107,115,121,124,124,124,128,132,133,132,133,138,144,148,146,140,133,128,126,126,127,129,133,134,133,127,121,118,122,129,137,143,147,149,150,
151,150,146,142,139,137,136,134,133,134,136,137,137,133,128,124,121,118,115,112,109,111,117,124,129,131,129,124,118,113,111,110,109,109,110,112,117,122,127,131,133,131,129,130,133,135,135,137,140,145,149,147,142,135,132,136,141,139,131,124,120,123,126,124,116,103,89,81,81,83,86,93,101,109,116,121,122,124,126,128,127,126,125,128,135,143,150,152,149,144,140,138,139,140,141,140,139,138,135,131,129,131,137,145,149,150,148,145,140,137,135,133,134,135,135,136,134,133,132,130,125,119,113,111,111,112,113,114,114,115,119,124,127,127,125,124,122,121,119,115,112,111,112,112,114,119,
126,129,129,129,131,133,134,135,135,140,148,155,155,148,138,132,134,139,143,141,136,133,133,131,124,110,91,75,69,71,75,78,82,89,101,113,120,120,118,116,118,122,127,130,134,142,152,161,163,158,149,139,134,132,133,137,143,146,144,137,132,130,133,137,141,144,147,150,150,149,145,141,138,139,141,144,143,140,137,132,128,124,120,117,115,116,118,118,118,118,118,120,123,127,130,130,125,120,117,114,110,107,105,103,105,111,116,121,125,124,120,122,129,135,139,140,142,147,154,157,152,141,132,131,135,138,136,130,123,121,123,122,114,99,83,72,72,77,80,83,87,95,107,118,124,124,124,127,129,
129,127,126,133,145,158,165,165,159,151,143,138,136,134,136,141,145,142,133,125,124,130,137,142,145,147,149,150,150,148,145,144,147,149,149,146,142,139,135,128,121,115,112,112,114,116,114,110,108,109,113,117,122,127,128,125,123,120,114,110,108,108,106,107,111,117,122,123,121,119,122,128,134,136,137,143,153,159,157,145,134,132,136,139,139,134,129,130,132,130,121,104,85,72,66,67,71,75,80,88,101,115,125,129,127,127,129,130,130,131,134,143,155,165,167,163,156,148,141,136,136,139,145,149,147,140,132,128,132,141,147,150,150,151,152,152,149,146,143,143,145,146,144,138,133,128,123,
118,113,111,112,114,115,113,110,108,108,111,114,118,121,124,125,122,117,112,106,103,104,107,109,114,122,126,125,121,117,119,126,133,137,141,146,153,159,157,146,133,129,132,137,138,132,126,125,127,128,121,105,85,71,66,68,73,78,84,91,101,114,128,136,138,136,133,127,123,123,129,140,153,164,170,169,163,155,147,140,137,139,143,146,145,141,135,133,135,142,147,148,147,147,149,149,146,143,141,141,143,143,141,137,134,131,125,116,108,105,107,110,111,110,106,103,104,108,113,119,121,125,129,128,121,113,107,104,105,108,110,116,124,127,126,123,120,119,124,128,130,135,144,154,159,155,144,
136,135,138,141,139,132,127,128,130,128,118,99,80,70,69,70,74,78,85,96,106,117,127,133,135,135,133,129,125,126,132,144,157,166,168,165,159,152,145,140,139,141,145,147,145,139,133,131,134,141,148,150,150,150,151,149,145,141,140,142,145,146,145,139,133,127,120,112,105,102,103,105,106,105,104,104,107,111,115,119,121,124,129,128,120,111,103,99,101,106,112,121,131,134,129,123,119,119,124,130,133,139,149,159,163,156,145,140,143,146,146,142,135,131,130,129,123,110,89,72,66,68,72,75,79,85,94,102,110,119,127,132,135,134,128};

//**************** Violin C4 to F4: (Band 4)
const PROGMEM uint8_t ViolinC4StartChunk1343[] = // recorded at low sample rate
{126,123,121,122,124,127,127,127,129,132,132,133,139,143,141,135,135,135,132,129,130,135,132,120,108,103,106,110,113,116,122,126,125,122,118,115,118,124,132,133,127,121,118,116,118,123,125,125,126,131,140,143,143,143,143,141,138,137,138,141,143,142,137,131,126,124,122,120,121,125,128,128,124,117,112,110,112,114,117,124,130,132,129,124,124,128,131,134,136,135,129,124,123,124,126,127,126,125,123,118,111,110,114,122,128,131,132,128,124,123,122,122,124,128,133,135,133,130,127,127,129,130,129,129,129,131,130,130,131,131,129,128,128,130,130,129,129,127,126,123,120,119,121,123,123, // [Violin] [C4 start chunk] [1343 samples] (start at 46 in file)
124,127,131,132,131,130,129,131,133,134,134,134,133,131,127,121,116,112,111,114,120,125,125,127,130,131,130,131,134,135,130,126,127,126,122,120,125,131,126,113,103,104,111,117,123,133,144,148,141,130,122,119,122,128,135,136,131,124,117,114,115,116,116,120,128,135,139,139,138,139,141,141,139,138,141,144,145,144,141,137,132,125,117,113,116,123,130,130,125,117,109,105,104,106,114,125,134,134,127,121,119,118,117,119,125,129,129,126,125,128,131,130,129,130,129,123,115,112,116,124,132,137,140,138,133,126,121,121,124,131,137,141,140,135,129,126,124,124,125,129,133,135,134,131,131,
130,128,127,128,130,130,127,125,124,123,120,116,114,115,115,116,118,120,122,119,116,118,123,122,122,126,134,139,146,155,159,151,139,137,137,135,134,140,146,141,123,103,91,87,90,96,101,105,108,113,115,113,109,114,128,139,139,132,127,125,125,130,140,143,137,133,138,147,152,153,153,154,150,146,142,140,140,140,138,133,123,115,111,109,108,111,119,126,129,125,116,106,101,103,109,116,122,126,127,123,119,122,132,141,149,152,146,136,133,134,134,133,133,133,125,113,104,103,103,98,97,103,114,122,126,134,143,147,140,128,122,123,129,135,142,148,149,142,125,106,102,115,131,137,138,142,143,
134,125,125,133,141,142,140,136,129,120,112,111,115,118,118,117,120,128,137,142,142,139,135,131,126,123,125,130,134,134,129,123,116,110,108,110,117,125,130,129,123,118,117,118,120,123,126,127,125,119,112,113,122,132,136,136,134,132,129,125,127,136,146,149,143,137,133,130,127,126,127,127,126,125,124,121,117,115,119,125,130,131,131,130,128,129,133,136,136,132,127,124,123,125,131,136,140,136,129,121,121,127,132,135,141,147,141,125,110,106,104,103,110,128,140,133,118,111,110,109,116,132,147,148,139,127,116,107,106,119,134,134,120,106,102,105,108,119,137,152,153,143,132,125,123,
127,138,148,149,145,139,134,131,127,125,124,126,130,134,133,133,136,139,138,133,128,126,128,129,128,125,124,125,125,122,115,110,111,118,125,126,128,134,132,117,98,94,106,122,128,132,136,133,121,109,120,148,162,156,153,156,146,126,110,113,124,125,116,105,94,86,87,100,116,125,134,142,144,132,121,125,138,148,148,144,140,136,130,130,135,133,123,116,121,132,141,143,144,146,148,147,140,132,128,130,135,134,131,128,129,126,115,104,101,109,120,125,124,120,115,110,106,106,113,124,132,133,126,117,114,115,123,134,143,151,152,147,142,140,138,131,127,127,127,118,104,93,92,99,111,124,132,
134,133,133,132,129,126,131,139,141,136,128,124,125,126,124,124,126,124,122,129,141,148,148,145,145,144,140,133,128,129,133,136,135,133,131,126,118,113,113,119,127,129,125,119,114,107,102,103,107,115,124,126,123,121,122,123,128,139,152,155,148,145,148,146,135,130,133,136,126,107,92,89,91,94,102,113,121,124,124,129,133,132,135,144,148,142,132,126,126,128,132,134,129,121,119,127,139,146,149,150,150,145,139,137,135,133,135,137,136,130,124,120,116,111,109,117,126,131,129,120,113,111,109,109,111,117,124,130,133,129,130,134,135,138,144,149,144,135,133,140,138,127,120,124,125,113,
94,80,82,85,95,107,117,122,124,127,127,126,126,134,145,152,149,142,138,140,141,140,139,135,130,131,139,148,150,146,141,136,134,134,135,135,134,132,130,122,114,110,112,113,114,115,121,127,126,124,122,120,116,112,112,112,116,126,129,129,131,134,135,136,146,155,152,138,132,138,143,138,133,133,126,106,80,69,72,78,83,97,114,120,118,116,121,127,131,141,156,163,157,143,134,132,136,144,145,137,130,132,138,143,147,150,149,144,139,139,143,143,139,133,127,122,117,115,117,118,118,118,121,126,131,127,120,116,110,106,104,106,114,121,125,121,123,132,138,140,145,155,155,141,131,134,138,132,
123,122,123,111,89,72,74,80,84,92,108,122,124,125,128,129,126,132,150,164,164,154,143,137,135,137,144,142,129,124,131,141,145,148,150,150,146,145,148,149,145,140,134,124,115,111,114,116,111,108,111,116,124,128,125,122,115,109,108,106,109,117,123,121,119,125,134,136,141,154,159,145,132,135,140,137,129,131,131,117,91,71,66,70,77,85,102,121,129,127,129,130,130,134,147,163,167,159,148,139,135,140,148,147,136,128,134,146,150,150,152,151,147,143,144,146,142,134,128,121,113,111,113,115,111,108,109,114,118,123,125,120,112,105,103,107,111,121,126,122,117,123,133,138,144,154,159,146,
131,131,138,135,126,126,128,118,93,71,66,72,80,89,102,121,136,137,134,126,122,129,144,162,170,166,156,143,137,140,145,145,138,133,137,146,148,147,149,149,144,140,142,143,140,135,130,120,108,105,109,112,108,103,105,112,119,124,129,126,115,106,104,108,113,123,127,124,119,121,128,131,141,155,158,145,135,137,141,135,127,129,129,114,87,70,69,73,80,92,107,122,133,135,134,128,124,132,149,164,168,162,152,143,139,142,147,145,136,131,135,146,150,150,151,148,142,140,144,147,143,135,126,116,105,102,105,106,105,104,108,114,120,123,129,126,112,102,100,106,116,130,133,125,119,122,130,135,
147,160,160,145,140,145,146,138,131,130,125,106,77,66,70,75,80,91,103,114,126,134,135,128};
const PROGMEM uint8_t ViolinF4StartChunk1343[] = // recorded at high sample rate
{127,127,127,127,127,127,127,127,127,127,127,127,127,127,126,126,126,126,126,126,126,126,126,126,126,127,127,126,125,125,125,125,125,125,124,124,124,124,124,123,122,122,122,122,122,122,122,122,122,122,123,124,125,125,126,127,127,128,129,129,129,129,129,129,130,129,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,129,129,128,127,128,128,128,127,126,127,127,127,127,126,125,126,125,125,125,124,124,124,124,123,123,123,123,124,125,125,125,125,125,125,125,125,125,125,125,126,127,126,125,125,125,125,125,125,125,125,125,125,124,124,124,124,124,124,124,124,124,124,125,126, // [Violin] [F#4 start chunkE] [1343 samples] (start at 62 in file)
127,127,128,128,128,128,128,129,129,129,129,130,131,131,131,130,130,130,130,130,131,132,132,131,131,132,132,132,132,131,130,130,130,128,128,127,126,125,124,124,123,123,123,124,123,123,123,123,123,123,123,122,122,122,122,121,120,120,120,120,120,120,120,121,121,122,123,122,123,124,124,125,126,127,127,128,129,129,129,129,130,129,129,129,129,128,129,130,130,130,129,128,128,127,127,126,125,124,124,123,123,123,123,123,123,123,124,124,124,124,124,124,125,126,128,129,130,131,133,134,134,135,135,135,134,133,133,134,133,132,132,132,132,131,131,132,132,131,131,131,130,129,129,128,127,
127,126,126,125,124,122,122,121,121,119,119,119,119,119,118,119,121,121,121,122,123,123,122,123,124,123,122,122,121,121,120,120,120,120,120,122,123,123,123,123,124,125,125,125,125,125,126,126,127,129,132,134,135,136,137,137,137,137,136,136,136,136,135,134,133,133,132,132,132,131,131,131,131,130,129,129,129,130,131,130,130,130,130,130,130,130,128,127,126,126,125,125,125,125,125,124,124,123,123,123,123,122,121,121,120,119,119,119,119,119,118,118,117,117,116,116,116,116,116,117,118,118,118,118,119,119,120,120,120,122,125,128,132,135,138,138,138,137,137,138,139,140,140,138,137,
136,135,134,134,135,135,136,136,136,135,134,134,134,134,135,134,134,133,131,130,129,128,127,125,123,122,122,123,123,123,123,123,124,124,123,122,121,120,119,117,116,116,116,115,116,116,115,116,118,119,119,119,119,119,118,118,118,118,118,119,120,122,125,128,132,135,139,141,141,141,140,139,138,138,138,139,140,140,139,137,133,131,129,128,128,128,128,128,128,128,128,129,130,131,131,130,129,128,127,126,125,124,123,122,123,124,126,128,129,130,130,129,127,126,126,125,124,123,122,121,119,118,117,117,118,120,121,120,119,118,118,117,117,118,118,117,115,115,116,119,123,128,134,139,142,
144,143,143,141,140,139,137,137,137,137,137,137,137,137,136,136,135,134,132,131,130,132,134,136,138,137,136,134,131,129,126,125,125,123,120,119,117,117,119,121,124,126,127,126,124,121,119,118,117,115,113,110,108,105,104,104,107,110,112,114,114,114,114,114,115,116,117,117,116,115,116,119,124,130,136,143,148,149,148,146,145,144,143,143,143,144,146,148,148,147,146,144,142,141,138,136,133,132,132,133,136,138,140,139,139,137,135,131,127,126,124,122,120,118,118,118,120,123,125,127,127,125,122,120,118,116,115,113,111,108,106,105,105,109,113,117,119,118,116,113,111,110,109,108,106,
105,105,105,107,112,119,127,134,138,141,141,138,135,134,134,133,133,134,138,142,144,146,145,144,142,140,138,136,134,133,133,134,138,143,147,148,148,146,143,139,136,135,133,132,130,130,131,132,133,135,139,142,140,135,129,125,124,124,121,118,113,108,102,97,96,100,105,110,116,119,117,114,110,108,107,105,103,100,99,100,102,107,117,128,138,146,149,145,143,140,138,135,133,133,135,139,143,146,147,146,143,142,141,137,134,129,124,123,125,130,135,139,141,141,138,133,129,127,124,123,122,121,121,119,120,123,128,135,139,140,134,128,125,125,125,121,115,109,106,105,104,101,104,112,120,125,
127,126,122,118,115,113,111,108,110,113,114,117,125,134,145,152,155,153,148,142,140,138,131,126,126,129,138,145,150,151,147,144,140,139,136,128,121,118,119,125,132,138,140,139,139,139,135,128,123,120,119,120,120,120,122,125,132,137,142,144,140,131,123,122,122,120,114,107,102,99,98,96,94,97,106,115,118,116,113,112,111,110,106,102,99,99,103,107,112,124,138,151,160,161,154,147,144,142,142,139,134,136,142,150,156,154,153,152,152,151,146,140,133,127,126,129,136,142,145,147,146,144,140,135,128,122,119,115,112,113,117,122,125,130,136,138,137,128,118,113,114,118,117,109,101,96,95,
94,93,97,104,110,117,121,118,112,111,113,114,110,105,104,105,108,113,121,133,148,158,162,158,150,143,141,139,134,130,127,129,136,143,151,153,153,152,149,145,139,133,129,126,126,130,137,143,144,143,141,140,137,133,126,119,114,110,110,115,122,128,131,136,143,145,140,128,120,120,123,123,117,107,99,94,94,96,99,105,111,115,120,121,116,110,111,114,111,103,97,99,103,108,117,131,146,160,166,163,154,143,139,139,135,128,126,126,129,138,146,155,159,160,158,151,145,139,135,128,122,122,128,137,141,141,142,142,137,130,123,116,110,107,108,111,115,120,125,132,140,146,142,134,125,119,119,
118,115,108,99,93,92,94,95,98,105,115,122,125,122,113,108,114,117,112,104,101,105,109,116,129,144,159,170,173,169,160,152,149,146,137,130,129,132,139,148,156,161,160,159,153,146,139,132,125,118,116,121,130,136,137,138,138,136,132,126,119,112,105,102,104,108,114,121,127,134,142,143,135,124,115,114,115,113,107,98,90,87,89,92,98,108,116,122,123,123,117,108,108,114,114,107,103,105,108,112,122,140,158,173,181,180,170,157,150,148,143,135,133,134,137,142,147,154,157,159,158,152,146,140,134,125,120,122,130,138,142,143,142,141,135,126,120,115,109,103,101,102,106,113,121,126,135,140,
136,127,117,113,113,110,105,98,92,88,88,89,91,96,105,115,120,123,123,116,111,114,116,111,104,103,104,106,113,128};

//**************** Violin F#4 to B4: (Band 5)
const PROGMEM uint8_t ViolinF4StartChunk988[] = // recorded at low sample rate
{125,125,125,125,125,124,124,124,122,122,122,122,122,122,122,122,124,125,126,127,127,129,129,129,129,130,130,128,128,128,128,128,128,128,128,128,128,128,129,128,128,128,128,127,126,127,127,126,125,125,124,124,124,124,123,123,123,125,125,125,125,125,125,125,125,125,127,126,125,125,125,125,125,125,124,124,124,124,124,124,124,124,126,127,127,128,128,128,129,129,129,130,131,131,130,130,130,131,132,131,131,133,132,132,131,130,129,128,127,126,125,124,123,124,123,123,123,123,123,122,121,122,121,120,120,120,120,120,121,122,122,122,123,125,126,127,128,129,129,129,130,129,129,128,129, // [Violin] [F#4 start chunkE] [988 samples] (start at 64 in file)
130,130,129,128,128,127,125,124,123,122,123,123,123,123,123,123,124,124,125,127,129,131,133,134,135,135,134,133,134,133,132,132,132,131,132,132,132,131,129,129,128,127,126,125,123,122,121,120,119,119,119,118,120,121,121,122,122,122,123,123,122,121,120,120,120,120,121,123,123,123,125,125,125,125,126,126,130,133,135,137,138,138,137,136,136,136,134,133,133,132,132,131,131,130,129,129,131,131,130,130,130,130,129,127,126,125,124,125,124,124,123,123,123,122,121,119,118,119,119,118,117,116,116,115,115,116,117,117,117,118,119,119,120,123,127,133,137,139,138,138,139,140,140,138,137,
135,134,136,136,137,136,135,134,134,135,135,133,131,130,128,126,123,122,122,122,123,123,124,123,122,120,118,117,115,115,115,115,115,116,118,119,119,118,118,118,118,118,120,124,129,134,139,142,142,141,139,139,140,141,141,138,134,130,128,128,128,128,128,128,130,131,131,130,128,126,125,123,122,122,125,127,130,131,129,127,126,125,123,122,119,117,116,117,119,120,119,117,116,116,117,116,114,115,119,126,134,141,145,145,143,141,139,137,138,138,138,138,137,136,135,132,130,132,135,138,138,135,132,128,126,124,121,118,117,118,121,125,127,126,121,118,117,115,111,108,104,103,107,111,114,
114,114,115,116,117,116,115,116,121,129,139,147,148,146,145,144,142,143,145,147,147,145,143,141,138,134,132,132,135,138,139,139,136,132,127,125,123,120,118,119,122,126,127,126,122,119,117,115,112,109,106,106,112,117,119,117,114,111,109,108,106,106,107,112,122,132,138,140,137,134,134,132,133,138,143,145,144,142,139,137,134,133,133,138,144,148,147,143,138,135,133,131,129,131,132,135,140,140,134,126,124,123,119,112,105,98,99,105,113,119,118,112,109,107,104,101,100,102,111,125,140,148,145,141,138,134,133,135,141,146,147,144,142,139,134,128,123,125,133,139,141,139,133,128,124,
123,121,120,119,123,131,139,139,130,125,125,122,113,106,105,102,102,115,124,127,124,118,114,111,108,111,114,119,133,147,156,155,146,141,137,127,126,134,146,152,150,144,141,137,126,118,119,129,138,141,140,140,132,123,119,119,119,121,126,135,143,145,135,123,122,120,111,101,97,94,91,96,111,117,113,110,110,106,99,96,101,107,120,142,160,164,153,145,144,142,135,139,151,158,156,154,154,148,138,128,125,133,143,147,149,145,138,128,120,115,110,115,121,127,136,139,132,117,111,117,114,102,93,92,91,97,107,117,119,112,110,113,107,102,104,109,119,137,157,164,157,145,142,137,130,127,135,
147,154,154,152,147,137,130,125,127,138,144,144,142,139,133,123,114,109,112,122,129,135,145,143,128,118,123,122,109,96,92,94,100,109,115,121,116,109,113,109,97,97,103,114,134,157,168,162,146,139,137,128,126,129,141,153,161,161,153,143,137,127,120,128,139,142,143,140,130,120,111,106,109,115,122,130,143,145,133,121,118,118,110,97,90,93,95,102,115,124,122,110,111,116,106,101,105,114,132,154,171,174,162,151,148,137,129,132,142,155,162,161,154,144,135,124,116,120,133,137,138,138,132,124,113,103,102,107,117,126,137,144,135,119,113,114,109,97,86,87,93,104,117,123,123,113,106,114,
110,102,105,110,125,152,174,183,173,155,149,142,133,134,138,146,155,159,159,151,142,133,122,121,133,142,143,142,135,123,116,107,101,101,109,119,129,139,137,121,113,112,106,97,89,86,89,93,106,118,123,121,111,115,113,104,103,105,115,140,167,184,183,170,159,149,133,130,135,144,157,163,165,159,149,137,127,123,130,138,139,139,137,128,119,108,98,94,100,113,124,137,137,123,112,110,107,98,89,85,87,95,108,120,126,125,115,116,117,107,106,108,113};
const PROGMEM uint8_t ViolinC5StartChunk988[] = // recorded at high sample rate
{127,127,127,127,127,127,127,127,126,126,126,126,126,126,126,126,126,126,126,126,126,127,128,129,129,130,130,130,130,130,130,130,130,130,129,129,128,128,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,126,126,126,125,126,126,126,126,125,124,124,123,122,122,122,122,122,122,122,122,123,123,124,124,124,124,125,124,125,125,126,127,127,127,127,128,129,129,129,130,130,131,131,131,131,132,131,131,131,130,130,129,129,129,129,128,127,127,127,127,126,127,127,127,127,128,127,127,127,126,126,125,124,124,124,124,123,123,124,125,126,127,127,127,127,127,127,126,126,125,125,125, // [Violin] [C5 start chunk] [988 samples] (start at 177 in file)
125,125,126,126,127,128,129,129,129,129,129,129,129,130,130,129,130,129,128,128,127,126,126,126,126,126,126,126,126,127,127,127,128,128,127,126,126,125,125,125,124,123,123,122,122,121,122,122,123,124,125,125,125,125,125,126,126,126,127,127,127,128,128,129,130,131,133,134,134,134,134,133,132,131,130,129,129,129,129,129,129,129,129,129,129,129,128,127,127,126,126,126,127,127,127,127,127,126,125,125,124,123,122,121,120,119,118,119,119,121,122,123,124,124,124,124,124,124,125,125,124,125,125,126,127,128,129,130,132,133,134,134,134,134,134,134,133,133,133,133,133,132,131,131,129,
127,126,126,125,124,123,122,122,123,124,125,126,127,127,127,127,126,125,124,123,122,120,120,118,118,118,118,120,121,123,124,124,124,124,125,125,126,126,126,126,128,129,130,132,133,135,137,138,138,138,136,135,133,133,133,133,133,132,132,131,130,129,128,127,127,127,127,126,125,125,125,126,127,128,128,128,127,125,123,120,118,116,115,114,114,115,116,117,119,121,123,125,125,125,124,124,125,126,128,130,133,135,136,136,135,135,135,135,133,132,131,130,129,129,130,130,131,131,131,132,132,131,130,128,125,124,123,123,124,124,122,120,118,116,116,116,117,118,119,120,121,120,119,117,117,
118,118,119,119,119,121,124,128,131,135,139,142,144,145,143,140,141,141,141,141,140,138,137,136,135,133,131,130,130,131,133,133,133,132,131,130,132,132,131,130,127,123,118,113,109,108,108,108,109,110,112,115,115,116,118,121,121,120,118,116,113,113,116,121,126,130,134,137,139,141,140,139,139,137,135,134,133,132,134,135,137,137,137,138,139,138,137,135,131,128,124,124,127,129,129,128,124,118,114,111,112,114,114,115,116,117,118,116,115,115,115,117,117,115,115,115,117,121,126,131,137,143,148,149,149,148,144,145,144,141,139,137,134,134,134,134,133,133,135,136,137,137,136,132,132,
129,129,133,134,131,126,119,111,105,102,102,105,108,111,113,114,115,115,115,116,115,115,113,109,107,107,109,114,121,127,134,140,144,145,144,144,142,143,144,140,139,137,136,139,141,141,141,141,143,145,144,140,136,130,129,130,133,137,138,135,128,118,109,103,99,100,104,107,110,112,112,113,114,114,114,113,113,110,106,109,112,115,120,126,134,141,146,148,146,144,144,142,143,141,137,136,132,132,137,138,140,141,145,150,152,148,142,135,131,133,135,138,139,137,131,120,109,101,97,97,101,106,109,112,111,109,112,115,115,113,110,108,101,100,103,107,112,119,127,137,144,145,144,142,142,143,
141,141,138,135,134,134,137,140,141,144,149,154,157,154,145,139,132,133,140,142,143,142,138,127,114,104,97,94,96,101,106,111,113,113,115,119,119,117,111,106,98,93,98,104,110,117,127,137,146,148,145,144,144,146,144,142,139,135,134,132,135,139,140,142,147,152,156,155,146,136,128,127,135,141,142,142,140,130,117,107,102,98,101,106,109,112,113,112,112,119,122,120,114,106,100,94,98,103,106,113,124,136,144,146,141,139,140,144,142,138,135,128,128,127,131,136,138,141,149,156,161,160,149,137,132,130,138,145,142,141,139,130,112,100,93,91,94,101,107,113,116,116,116,125,130,126,119,109,
101,95,99,106,110,116,126,139,148,150,145,142,145,148,146,139,135,129,129,131,134,139,140,141,147,154,159,159,148,134,125,123,131,145,144,141,139,130,112,99,92,90,94,101,105,109,111,110,109,116,125,125,122,112,101,92,95,103,108,114,123,136,147,151,146,140,141,148,151,144,136,126,123,126,130,137,139,141,149,160,167,169,157,140,130,128,135,149,150,144,142,134,114,96,87,85,90,99,105,108,113,114,113,120,130,129,125,114,101,90,90,100,108,117};

//**************** Violin C5 to F5: (Band 6)
const PROGMEM uint8_t ViolinC5StartChunk680[] = // recorded at low sample rate
{128,129,130,130,130,130,130,130,130,129,128,128,127,127,127,127,127,127,127,127,127,127,126,126,125,126,126,125,125,124,123,122,122,122,122,122,123,123,124,124,125,124,125,126,127,127,128,129,129,130,130,131,131,132,132,131,131,130,129,129,129,128,127,127,126,127,127,127,127,127,127,126,125,124,124,124,123,124,126,127,127,127,127,126,125,125,125,125,126,126,127,129,129,129,129,129,130,129,130,129,127,127,126,126,126,126,126,127,127,128,128,126,126,125,125,124,123,122,121,122,122,124,125,125,125,125,126,126,127,127,128,129,131,133,134,134,134,133,131,130,129,129,129,129,129, // [Violin] [C5 start chunk] [680 samples] (start at 140 in file)
129,129,128,127,126,126,126,127,127,127,126,125,124,123,121,120,118,119,120,122,123,124,124,124,124,125,124,125,125,127,129,131,132,134,134,134,134,134,133,133,133,132,131,128,127,126,125,123,122,123,124,125,127,128,127,126,125,123,121,120,118,118,118,121,123,124,124,124,125,126,126,127,128,130,132,134,137,138,138,136,134,133,133,133,132,132,130,128,127,127,127,126,125,125,126,128,128,127,124,121,118,115,114,114,115,118,120,123,125,125,124,124,127,130,133,135,136,135,135,134,133,131,129,129,130,131,131,131,132,131,129,125,123,123,124,123,120,116,116,117,118,119,120,120,118,
117,118,119,119,120,124,130,135,140,144,144,141,140,141,141,139,137,136,134,131,129,131,133,133,132,130,132,132,130,126,120,113,109,108,108,110,113,115,116,120,121,119,116,113,115,121,128,134,137,140,140,139,138,134,133,132,134,137,137,138,139,138,134,129,124,125,129,129,126,118,112,111,114,115,116,118,116,115,116,117,116,114,116,121,128,136,145,149,149,146,145,143,140,136,133,134,134,133,135,137,137,134,131,129,132,134,128,118,107,102,103,108,111,114,115,115,115,115,114,109,107,109,117,126,135,143,145,144,143,144,141,138,136,139,142,141,142,145,144,138,131,129,132,138,136,
127,114,103,99,103,108,111,112,113,114,113,113,109,107,113,117,125,135,146,147,145,144,143,142,137,133,133,137,139,142,149,152,145,135,131,135,138,138,130,114,101,96,100,107,112,111,110,115,114,111,106,100,103,109,117,130,142,145,142,142,142,141,137,134,134,139,141,145,153,156,149,138,132,138,142,142,137,120,105,96,96,102,109,113,113,118,119,113,105,94,98,107,115,130,143,148,144,145,145,143,138,134,133,137,140,143,151,156,150,136,127,133,142,142,139,123,108,100,100,107,111,113,111,118,122,115,106,96,97,104,111,128,142,146,139,141,143,140,133,127,128,133,138,143,154,162,154,
138,130,136,145,141,139,121,100,91,93,103,111,117,115,123,130,121,108,96,99,109,114,129,145,149,143,145,148,141,133,129,131,137,140,143,152,160,153,134,123,129,145,142,138,121,99,91,93,103,107,112,109,115,125,123,111,95,95,106,112,126,144,151,142,141,150,147,133,124,125,134,139,143,157,168,163,140,128,134,150,146,142,125,97,86,88,101,107,114,113,120,130,127,113,93,90,104,115};
const PROGMEM uint8_t ViolinF5StartChunk680[] = // recorded at high sample rate
{127,127,126,126,126,126,126,126,126,127,128,128,128,128,128,127,127,128,128,129,130,130,130,131,131,130,129,129,128,127,127,127,128,128,129,130,130,130,130,129,128,127,126,125,125,125,125,125,125,125,125,126,126,127,127,126,126,125,124,124,124,124,124,124,125,126,126,126,127,127,127,126,126,127,127,128,129,130,131,131,130,130,129,129,128,127,127,126,125,124,123,122,121,121,121,121,120,121,121,122,124,125,125,126,126,126,127,129,129,131,132,133,133,132,132,132,131,131,130,129,129,129,130,129,130,129,128,127,126,125,125,124,125,125,126,127,127,127,126,126,126,125,124,124,124, // [Violin] [F#5 start chunk] [680 samples] (start at 264 in file)
124,125,125,126,127,130,132,134,135,135,135,134,134,133,134,133,132,129,127,124,123,123,122,121,121,121,121,122,123,123,123,122,121,120,120,120,121,122,122,123,122,121,121,121,122,123,122,122,123,125,129,131,132,132,133,133,133,133,133,132,131,129,127,125,125,125,126,127,128,129,128,126,126,125,127,129,131,132,133,136,136,138,139,139,139,137,137,136,135,134,132,129,126,126,126,127,126,124,121,120,119,118,119,120,120,122,121,120,119,117,114,113,112,115,118,122,127,133,136,137,136,134,133,131,127,125,123,120,120,122,122,123,127,129,130,127,122,119,117,117,118,119,122,123,125,
127,127,129,129,127,124,121,121,123,129,135,143,147,148,147,145,142,141,141,141,139,138,137,136,133,132,134,133,131,127,121,118,119,121,122,123,123,121,119,115,112,111,108,106,102,102,103,109,119,130,139,142,143,143,140,138,135,134,133,132,130,128,126,123,129,133,133,128,119,115,113,114,116,119,123,124,123,123,122,122,119,115,112,111,115,120,130,139,147,149,147,144,143,144,146,147,145,141,138,134,130,126,126,128,127,121,113,109,110,117,123,126,131,131,126,121,115,113,111,105,100,98,103,112,129,144,154,156,153,149,143,141,141,141,140,134,128,125,126,127,131,137,136,130,117,
107,103,107,113,118,126,130,127,124,122,121,119,112,103,97,99,105,120,141,154,156,154,153,148,144,147,148,144,140,133,128,124,125,126,134,135,129,115,103,100,104,113,118,122,123,119,114,113,116,116,108,102,97,102,110,129,149,157,158,156,153,149,148,151,151,148,141,136,131,128,131,134,141,144,134,117,103,99,105,112,114,120,123,118,114,113,116,115,107,100,93,97,108,125,149,159,158,153,152,146,146,148,149,147,140,133,127,120,127,133,138,140,129,112,100,97,103,113,117,123,125,119,115,113,116,115,105,96,91,97,108,129,154,164,161,156,154,147,145,151,153,150,144,141,137,129,133,
138,140,143,128,111,97,95,104,113,117,122,126,121,116,115,116,114,102,93,87,91,105,128,161,170,163,155,151,144,143,152,156,151,141,133,126,120,127,135,138,144,131,110,94,90,100,111,116,121,126,123,117,113,115,115,104,94,89,91,106,130,163,175,165,158,155,147,148,159,163,157,146,134,127,120,126,137,140,144,128,106,91,87,98,109,116,122,129,124,118,117,118,113,97,85,78,84,102};

//**************** Violin F#5 to B5: (Band 7)
const PROGMEM uint8_t ViolinF5StartChunk482[] = // recorded at low sample rate
{126,126,126,126,126,127,128,128,128,127,127,128,129,130,130,131,130,129,128,127,127,128,128,129,130,130,129,128,127,126,125,125,125,125,125,126,127,127,126,125,124,124,124,124,125,126,126,127,127,127,126,127,128,130,131,131,130,129,128,127,126,125,124,122,121,121,121,121,121,122,124,125,126,126,128,129,131,132,133,132,132,131,131,130,129,129,129,129,128,127,126,125,124,125,126,127,127,126,126,125,124,124,124,125,126,129,132,135,135,135,134,134,133,131,128,125,123,122,121,121,121,122,123,123,122,120,120,121,122,123,122,121,121,122,122,122,124,128,131,132,133,133,133,132, // [Violin] [F#5 start chunk] [482 samples] (start at 189 in file)
131,129,126,125,126,127,128,128,126,125,126,129,131,133,136,137,139,139,137,136,135,134,131,126,126,127,126,121,119,118,119,120,122,121,120,117,113,112,115,120,127,135,137,136,134,131,126,124,120,121,122,124,129,129,123,118,117,117,121,123,125,127,128,128,124,121,122,128,139,147,148,146,142,141,141,139,137,136,132,133,133,128,121,118,120,122,123,121,117,113,110,106,102,102,109,123,137,143,143,140,137,134,133,130,128,124,127,134,130,119,113,114,117,122,124,123,122,121,116,111,113,120,133,145,149,145,143,145,147,144,139,134,128,126,129,123,113,109,116,124,130,131,123,116,
112,107,99,100,112,135,152,156,151,143,141,141,139,130,125,126,130,138,132,116,104,106,114,124,130,125,122,121,115,102,97,105,128,152,155,154,148,145,148,143,136,127,124,126,135,132,113,100,104,114,121,123,116,113,116,111,100,99,110,138,156,157,154,149,149,152,146,138,131,129,134,143,139,116,100,105,113,118,123,115,114,116,110,97,95,106,135,159,157,152,147,146,150,145,136,126,123,132,139,134,111,97,103,114,121,125,116,114,116,109,94,94,107,139,164,160,155,147,147,154,148,143,135,130,137,142,134,109,94,103,115,120,126,117,115,116,106,90,89,103,142,171,161,153,144,146,156,
148,136,124,122,133,141,137,108,91,98,113,119,126,120,113,116,108,91,90,104,144,175,164,156,147,152,164,154,139,125,122,134,143,135,104,87,96,112,120,129,120,117,117,102,82,81,100};
const PROGMEM uint8_t ViolinC6StartChunk482[] = // recorded at high sample rate
{127,125,126,126,124,126,127,127,127,128,128,125,125,125,125,129,128,126,127,131,129,127,128,129,129,128,129,131,131,128,126,125,124,121,123,122,122,124,125,126,128,129,126,127,129,129,128,128,132,129,129,127,126,123,126,125,124,127,126,128,126,129,129,126,123,124,123,129,131,128,127,131,130,126,128,130,130,130,131,135,136,131,128,126,122,118,118,121,120,125,125,127,124,129,125,127,132,128,127,129,134,128,129,127,126,124,126,128,129,130,127,128,125,125,124,127,121,119,120,124,127,128,130,127,127,127,126,129,133,131,129,132,129,128,125,128,127,128,128,132,132,128,129,127, // [Violin] [C6 start chunk] [482 samples] (start at 129 in file)
125,115,120,119,125,124,129,128,128,132,129,129,132,133,126,128,131,131,131,130,127,120,121,121,125,124,129,127,126,119,122,125,125,124,124,125,129,134,132,136,136,129,128,129,130,127,127,127,122,119,123,129,130,127,123,121,123,124,124,128,132,131,126,130,136,141,134,132,129,123,119,120,126,120,120,121,120,121,130,131,125,127,123,126,121,124,127,124,127,129,125,135,135,133,132,138,130,126,131,132,134,129,131,131,128,126,132,124,121,118,119,119,119,126,125,117,125,130,129,137,138,138,134,137,131,128,128,125,122,120,131,135,129,131,127,117,114,114,119,118,117,121,120,115,
130,134,128,136,133,129,130,135,129,129,129,124,120,123,135,139,136,131,129,115,107,111,119,120,123,125,125,121,134,136,131,145,138,132,127,136,133,132,131,127,122,124,131,135,136,141,135,120,112,116,122,124,114,117,116,117,134,134,141,135,125,122,131,130,129,128,133,131,126,129,140,139,128,130,111,109,99,108,119,119,120,115,111,124,139,140,145,142,137,136,138,135,129,124,125,122,123,134,145,137,134,123,112,103,100,114,118,124,123,114,114,135,143,138,143,142,140,140,145,136,134,128,128,117,124,140,142,139,127,114,103,103,109,110,109,120,115,109,123,150,151,147,150,140,
133,128,131,125,132,130,126,123,130,138,140,130,119,108,98,110,112,111,116,118,102,102,124,149,144,153,155,150,139,137,136,132,134,127,119,130,141,142,137,127,102,85,96,106,110,110,132,112,104,120};

//**************** Violin C6 to F6: (Band 8)
const PROGMEM uint8_t ViolinC6F6StartChunk340[] = // recorded at low sample rate // same chunk used for all notes in band
{128,126,125,125,125,127,126,128,127,125,125,127,128,126,130,129,127,129,128,129,132,129,126,124,121,122,122,124,126,128,128,126,129,127,130,130,128,127,124,125,124,126,128,127,130,126,124,123,129,130,127,131,127,128,131,130,133,136,130,127,121,118,121,122,126,125,128,126,129,129,127,133,129,128,125,125,127,129,129,127,124,125,124,119,122,126,129,128,127,127,128,133,130,131,129,126,128,128,129,132,129,129,125,116,120,123,127,128,130,130,129,134,127,130,131,130,127,120,121,124,126,129,123,120,125,124,124,126,133,133,136,129,128,130,127,127,121,120,129,129,123,122,124,124, // [Violin] [C6 start chunk] [340 samples] (start at 90 in file)
131,131,127,135,139,132,130,120,120,124,119,121,120,129,129,125,126,122,125,125,127,127,131,135,132,137,127,129,134,131,131,129,127,130,120,119,118,120,126,119,127,130,136,139,135,135,128,128,121,123,133,131,129,120,112,118,117,120,118,121,134,130,136,128,133,131,129,126,119,130,140,134,128,113,108,120,121,127,121,131,134,138,141,127,135,132,132,126,122,131,135,140,132,113,117,123,117,115,119,132,139,135,123,129,131,128,133,127,131,140,132,123,108,101,113,121,118,111,127,142,143,141,135,138,130,125,123,123,139,141,129,117,100,107,118,125,116,118,142,139,143,139,142,140,
131,129,118,130,144,136,119,102,107,109,113,118,109,142,152,149,143,130,129,127,132,126,125,138,138,123,105,103,114,111,119,100,115,146,147,156,145,138,135,133,127,122,140,140,132,100,88,108,108,126,111,110};

//**************** Violin F#6 to B6: (Band 9)
const PROGMEM uint8_t ViolinF6C7StartChunk233[] = // recorded at low sample rate // same chunk used for all notes in band
{128,125,125,126,127,127,124,126,129,128,128,128,130,130,130,126,123,121,122,125,129,128,129,128,131,127,125,124,126,127,129,124,123,130,131,130,127,130,133,135,128,121,118,122,125,126,128,130,129,131,127,126,127,129,126,125,123,117,124,130,128,127,131,132,131,126,127,130,131,128,120,117,126,129,131,130,133,129,132,129,121,121,127,125,120,125,125,127,135,135,131,128,127,121,124,129,120,121,127,133,129,139,134,125,118,122,118,125,128,122,122,125,128,129,134,136,134,128,133,130,130,126,118,116,124,122,126,136,141,137,129,122,124,134,129,114,112,119,118,124,134,133,133,131, // [Violin] [F#6 start chunk] [233 samples] (start at 62 in file)
126,121,134,139,119,105,119,125,125,133,143,137,130,134,125,125,137,138,114,118,121,112,126,143,130,126,129,132,129,139,129,106,101,121,114,118,148,145,138,132,127,120,138,138,115,98,116,123,114,142,146,142,140,136,122,128,144,118,102,107,115,110,142,160,140,127,129,130,124,140,123,103,106,118,102,119,157,154,141,136,132,123,142,131,94,94,119,113,117};


// save an array of samples from 3 single cycles, taken from different parts of the note (note start, mid & end), for each instrument, into 3D array - for each band of 6 notes: - the notes will be reconstructed from the single cycles mixed together with each other, after the appropriate start chunk
// Band of notes from C2 to F2:
const PROGMEM uint8_t MsfNoteBand0[][6][304] = // [instrument] [cycle] [samples] // band 0
{
  { // instrument 1 (piano) here:
    {111,94,82,79,78,75,71,69,72,77,82,88,99,112,120,118,111,112,115,119,124,127,123,116,109,101,98,97,97,89,72,56,48,49,60,75,89,100,108,116,125,140,156,169,179,186,188,188,187,186,186,181,167,149,133,124,120,115,112,111,112,111,107,106,114,127,136,136,130,126,126,131,136,138,136,126,112,100,93,85,74,66,62,56,44,33,32,43,57,66,72,77,81,84,82,78,76,77,79,75,70,73,87,105,115,112,101,94,97,110,123,126,119,111,110,111,104,88,68,55,53,58,62,59,54,55,58,60,61,67,73,75,72,70,73,82,94,104,112,119,122,121,113,102,93,91,95,102,106,106,105,109,122,137,144,142,136,133,137,143,147,                 // [Piano] [start cycle] [304 samples for note C2] start at 872
    151,152,148,140,130,125,127,129,127,121,114,111,111,116,127,143,154,153,146,139,136,135,133,131,130,128,126,127,128,129,131,130,131,137,144,152,159,164,166,160,146,130,120,123,132,142,147,153,155,157,162,172,182,185,180,171,168,173,178,175,163,148,132,120,114,113,118,126,139,151,158,167,180,200,226,248,254,248,233,217,207,203,204,206,200,186,166,146,131,120,111,103,98,96,97,105,118,132,148,165,180,189,186,177,170,175,189,202,201,191,180,172,170,176,186,196,200,192,176,163,158,160,162,160,155,145,135,129,129,136,147,158,166,175,185,190,191,191,193,189,171,147,127},
    {120,104,95,97,105,113,116,115,111,106,103,104,111,121,132,143,148,144,131,114,98,86,80,82,87,93,95,92,84,72,63,60,65,76,92,108,119,121,117,112,107,102,98,93,88,88,93,97,95,84,72,64,62,62,62,59,53,42,27,12,3,4,15,36,60,82,99,106,108,106,107,113,120,122,120,116,110,102,93,83,78,76,75,71,63,51,39,27,16,8,6,10,18,27,37,46,53,56,54,48,42,39,39,39,36,34,36,44,50,52,51,49,47,48,50,53,56,59,58,53,45,40,42,50,59,66,74,83,92,100,106,112,118,126,135,141,142,138,130,122,114,105,100,100,104,111,118,123,127,130,130,128,123,121,124,128,131,134,141,151,159,161,157,153,150,146,                     // [Piano] [start cycle] [304 samples for note F#2] start at 872 (1409 from beginning)
    143,140,138,135,133,130,126,122,120,118,114,112,112,111,108,102,97,95,97,99,102,106,115,131,150,165,172,172,169,167,166,168,175,184,191,194,193,191,190,186,179,171,169,175,185,193,196,195,198,202,202,197,189,183,182,183,184,183,181,181,186,197,212,226,233,230,217,203,192,189,193,204,219,233,239,236,224,208,197,196,201,209,214,216,217,219,221,219,215,210,208,208,209,211,214,215,210,199,185,172,164,164,172,184,197,201,195,180,163,150,147,154,167,181,189,194,192,183,168,156,153,160,173,185,193,197,198,194,187,178,170,169,173,178,178,170,154,134},
    {123,113,107,104,101,99,95,90,87,88,90,93,95,94,93,93,97,101,107,114,118,119,120,120,120,121,122,122,120,119,120,121,122,123,121,117,112,106,100,96,94,92,90,86,80,73,71,73,79,85,90,91,87,82,78,76,79,84,86,84,80,77,77,80,84,86,87,89,90,92,95,100,102,101,98,92,87,86,88,90,89,85,78,70,68,71,78,87,94,99,99,97,92,87,85,85,83,81,81,82,82,84,87,92,97,102,104,106,109,114,116,116,114,108,102,98,97,97,99,101,102,104,109,114,118,124,133,141,148,154,157,158,159,157,156,157,158,156,149,140,131,124,118,115,114,116,122,128,134,141,149,156,163,170,175,177,178,180,183,184,183,178,                   // [Piano] [mid cycle] [304 samples for note C2] -6dB start at 16128
    172,168,166,163,158,152,146,140,133,126,119,117,119,123,127,133,139,141,142,143,144,147,151,154,155,157,158,160,162,165,166,167,171,176,180,182,179,173,169,171,177,183,187,191,191,188,185,182,177,172,167,162,160,161,161,158,155,149,141,138,138,140,141,141,139,135,132,132,132,134,140,147,154,160,165,166,166,166,164,163,166,169,169,167,163,157,150,145,138,132,128,126,124,120,116,111,106,105,110,119,128,136,140,139,139,139,140,143,147,151,154,159,164,166,164,160,152,143,136,131,124,119,113,106,101,99,103,110,120,130,137,143,149,154,158,161,162,158,152,145,137,130},
    {126,123,120,119,118,119,120,121,121,120,118,115,113,112,113,116,119,122,123,123,121,119,116,114,115,118,122,125,128,128,127,125,123,121,119,118,116,114,112,110,108,105,103,101,100,100,101,100,99,98,96,94,91,88,85,84,84,85,86,87,88,88,87,86,85,84,82,80,79,77,76,76,76,77,78,80,80,78,74,69,65,63,64,68,73,79,85,90,94,97,99,101,105,108,111,114,115,116,117,118,120,121,122,124,125,125,125,125,124,123,122,121,120,120,120,120,120,121,120,120,118,116,114,112,110,107,105,104,103,102,102,102,101,100,99,99,100,102,104,106,109,111,113,116,117,118,118,119,119,118,119,120,122,124,126,127,127,128, // [Piano] [mid cycle] [304 samples for note F#2] -6dB start at 16096
    127,127,126,125,124,125,126,128,129,132,135,139,144,149,153,155,156,155,152,148,143,139,136,134,135,137,140,142,144,145,147,148,150,152,153,153,152,151,151,151,152,154,157,161,164,167,168,167,165,163,160,158,157,158,160,162,164,165,164,161,156,151,147,143,142,143,143,143,143,143,144,147,152,157,161,163,164,163,162,162,163,166,169,171,172,170,166,161,157,154,153,153,153,155,156,157,158,158,157,155,152,149,148,146,146,146,147,149,152,155,159,162,163,163,161,159,156,152,149,146,145,145,146,148,148,147,145,142,139,135,132,130,131,133,135,137,137,136,133,130,128,127,127,128,129,130},
    {123,119,115,112,110,108,106,105,104,105,105,106,106,104,101,98,94,92,90,88,86,82,79,78,79,82,86,90,94,99,104,110,113,115,114,114,115,116,117,118,120,123,127,129,129,128,128,130,132,134,133,129,121,112,104,97,94,91,88,84,78,72,69,67,68,69,71,74,79,86,92,98,102,104,105,104,103,104,105,106,106,104,99,95,93,94,99,105,111,114,114,110,106,103,102,102,100,96,90,84,79,76,76,77,80,83,88,93,98,103,108,111,114,116,118,122,126,131,134,134,132,127,122,118,117,119,123,125,126,125,124,125,129,133,136,139,142,146,152,159,164,167,168,169,169,167,164,160,156,152,148,142,135,128,123,119,117,116,     // [Piano] [end cycle] [304 samples for note C2] -6dB start at 95506
    114,111,108,105,106,109,115,120,124,127,130,134,138,141,142,142,142,142,141,139,138,138,139,141,143,145,146,147,149,151,153,153,152,150,148,147,146,147,147,148,150,154,159,163,167,169,171,173,175,176,176,174,171,166,158,149,138,130,123,118,115,113,112,111,113,116,121,128,135,140,144,147,150,153,155,155,153,150,145,141,139,138,138,138,139,140,141,144,150,157,166,175,182,187,190,191,189,187,184,180,175,167,158,149,142,136,130,122,114,106,100,98,98,98,99,100,104,109,116,125,135,144,151,156,159,160,161,160,160,160,162,165,168,169,168,165,164,163,161,159,154,149,143,138,133,127},
    {126,125,124,122,121,119,118,117,116,115,115,115,116,116,115,114,113,111,108,105,101,97,92,87,82,77,73,71,70,70,72,73,75,76,77,77,77,77,76,76,76,76,76,76,76,77,78,80,82,84,86,88,89,89,89,88,87,85,84,84,83,82,80,78,76,75,73,72,71,71,71,72,73,75,77,79,81,84,86,88,89,89,88,86,84,81,78,75,73,72,73,74,77,80,84,88,92,97,101,105,108,111,113,115,116,116,115,114,112,109,106,104,101,98,96,94,92,90,89,89,89,90,91,92,94,95,97,98,99,100,101,101,102,103,104,106,109,112,117,122,128,133,137,139,141,141,140,137,134,129,125,121,119,119,120,124,128,134,140,145,150,155,158,161,162,162,162,160,158,     // [Piano] [end cycle] [304 samples for note F#2] -6dB start at  (174218 from beginning) (filtered less)
    156,153,150,148,145,143,142,142,143,145,148,151,154,158,161,164,167,169,171,172,173,172,172,170,168,165,161,157,154,150,148,147,147,149,153,158,164,170,175,178,180,181,180,179,176,174,171,168,166,165,164,165,166,168,171,174,177,179,181,182,183,183,183,184,183,183,182,180,178,175,173,172,172,174,176,179,183,186,189,190,191,190,188,185,181,177,174,170,167,164,162,159,158,156,156,157,159,161,162,163,163,163,161,159,156,152,148,144,140,135,132,129,127,126,126,127,129,132,135,137,140,141,142,142,142,142,142,141,139,138,136,134,132,130,128}
  },
  { // instrument 2 (guitar) out of limits (NoteBand1 used)
  },
  { // instrument 3 (marimba) out of limits (NoteBand1 used)
  },
  { // instrument 4 (trumpet) out of limits
  },
  { // instrument 5 (sax) here:
    {132,134,139,140,141,144,144,142,145,148,149,147,151,154,154,156,158,157,157,158,159,160,161,162,162,162,164,168,172,174,175,177,176,177,177,178,181,185,187,187,185,183,180,177,178,178,178,180,181,181,182,182,181,180,180,179,178,173,169,171,174,173,174,176,173,171,169,169,169,170,169,169,168,166,162,160,157,157,158,161,161,161,160,156,149,147,147,148,148,149,150,148,142,141,143,143,143,146,144,138,135,130,127,127,130,133,134,133,128,125,127,129,130,130,129,128,126,124,124,123,124,124,124,122,122,118,115,115,114,110,112,116,118,118,119,117,114,111,108,107,107,108,106,106,104,101, // [Sax] [start cycle] [304 samples for note C2] start at 1406 (1509 from beginning)
    101,104,103,98,95,97,97,98,101,103,101,97,96,94,90,89,92,95,98,100,104,104,101,98,98,103,105,102,102,104,102,100,101,101,100,100,101,99,98,100,102,105,106,107,106,104,103,104,104,104,106,109,109,112,112,109,109,108,105,105,107,109,109,109,109,109,109,112,113,112,109,109,110,111,108,110,112,114,116,114,112,111,112,112,114,115,115,116,120,119,117,117,118,116,116,114,114,109,104,105,106,104,105,108,108,107,106,106,104,101,96,91,90,93,94,92,93,93,91,91,92,88,85,85,84,82,87,87,87,88,87,84,86,86,86,89,91,91,91,93,93,90,90,93,93,92,98,104,104,104,105,107,109,115,122,123,122,124},
    {132,134,139,140,141,144,144,142,145,148,149,147,151,154,154,156,158,157,157,158,159,160,161,162,162,162,164,168,172,174,175,177,176,177,177,178,181,185,187,187,185,183,180,177,178,178,178,180,181,181,182,182,181,180,180,179,178,173,169,171,174,173,174,176,173,171,169,169,169,170,169,169,168,166,162,160,157,157,158,161,161,161,160,156,149,147,147,148,148,149,150,148,142,141,143,143,143,146,144,138,135,130,127,127,130,133,134,133,128,125,127,129,130,130,129,128,126,124,124,123,124,124,124,122,122,118,115,115,114,110,112,116,118,118,119,117,114,111,108,107,107,108,106,106,104,101, // [Sax] [start cycle] [304 samples for note F#2] (taken from C2) start at 1406 (1509 from beginning)
    101,104,103,98,95,97,97,98,101,103,101,97,96,94,90,89,92,95,98,100,104,104,101,98,98,103,105,102,102,104,102,100,101,101,100,100,101,99,98,100,102,105,106,107,106,104,103,104,104,104,106,109,109,112,112,109,109,108,105,105,107,109,109,109,109,109,109,112,113,112,109,109,110,111,108,110,112,114,116,114,112,111,112,112,114,115,115,116,120,119,117,117,118,116,116,114,114,109,104,105,106,104,105,108,108,107,106,106,104,101,96,91,90,93,94,92,93,93,91,91,92,88,85,85,84,82,87,87,87,88,87,84,86,86,86,89,91,91,91,93,93,90,90,93,93,92,98,104,104,104,105,107,109,115,122,123,122,124},
    {133,149,159,163,171,178,182,185,192,199,204,209,211,210,211,214,217,220,223,225,226,226,219,216,217,220,220,221,225,229,231,232,233,236,237,238,241,244,246,246,245,242,240,238,237,237,237,237,237,234,229,226,222,217,212,211,212,211,209,209,209,207,206,206,208,208,204,200,199,199,199,201,202,198,197,197,192,185,183,184,184,181,178,174,169,168,164,160,159,160,158,157,153,150,149,147,147,146,145,143,143,143,142,139,137,138,139,139,140,141,139,135,130,127,127,127,129,129,125,122,121,122,124,125,125,123,120,118,116,114,113,112,111,111,108,105,101,97,97,94,92,88,86,83,79,78,77,73,67, // [Sax] [mid cycle] [304 samples for note C2]  start at   (6694 from beginning)
    67,65,64,65,68,70,69,70,69,67,65,64,66,68,69,69,71,71,68,68,69,67,67,69,74,73,73,74,72,69,69,71,73,78,83,83,82,82,82,82,82,82,81,81,85,88,86,84,86,87,87,91,95,95,94,94,94,91,89,90,92,93,94,97,98,98,101,106,109,111,113,117,120,120,118,120,123,128,134,143,147,150,152,155,157,158,158,161,161,158,157,153,143,132,123,109,96,82,72,66,58,56,54,53,52,49,46,39,32,28,26,26,23,23,26,27,29,34,40,45,48,50,54,55,56,59,63,71,77,80,81,84,87,86,88,89,88,85,83,77,67,63,63,61,57,56,56,54,51,54,60,67,79,96,118},
    {137,154,168,178,185,188,189,192,196,203,212,218,220,222,221,219,221,224,227,228,225,224,226,227,230,230,228,226,227,229,228,228,231,232,232,233,236,239,241,237,233,230,229,228,227,226,225,222,218,213,207,203,200,195,189,187,188,188,187,187,186,186,185,185,188,191,192,191,189,188,187,188,187,184,183,183,181,174,168,163,159,156,152,147,145,145,146,145,144,143,141,139,138,137,139,140,140,142,144,145,146,145,141,138,138,137,136,136,139,141,138,132,128,125,124,122,121,122,122,121,121,121,119,115,112,110,109,108,107,107,107,109,111,110,109,108,106,103,102,102,105,106,105,104,101,95,  // [Sax] [mid cycle] [304 samples for note F#2] -1dB start at   (8805 from beginning)
    90,84,82,81,79,78,78,78,75,72,69,66,65,63,64,67,68,68,67,65,60,57,54,53,52,55,60,65,68,70,75,76,75,72,68,70,74,78,81,84,88,91,91,89,86,85,86,88,90,94,96,93,90,86,83,83,84,86,87,87,85,81,78,79,82,85,87,92,94,94,95,98,100,100,103,108,113,117,125,134,140,147,156,165,166,164,168,174,178,177,176,179,178,173,167,160,152,143,130,111,97,86,77,71,65,62,58,57,57,53,49,41,34,29,23,22,24,29,33,35,38,44,51,55,63,69,73,76,82,86,90,95,98,101,106,110,111,111,108,103,98,93,89,85,81,76,72,66,61,60,60,61,64,69,75,88,101,117},
    {139,154,163,167,173,176,181,185,189,196,202,204,206,207,207,210,213,216,221,226,230,234,236,237,237,238,237,235,236,236,234,232,231,231,231,231,233,235,233,232,230,226,224,225,227,228,228,226,225,220,217,215,212,210,208,209,209,209,208,208,207,206,204,203,204,204,205,205,203,201,198,195,191,187,184,178,174,173,169,167,164,162,160,158,160,160,159,160,160,159,157,157,157,159,161,161,161,160,158,156,155,152,148,147,145,140,137,137,136,133,128,125,120,115,116,116,116,116,118,119,119,119,121,122,123,125,127,126,127,127,125,125,123,120,117,112,105,101,96,93,90,89,85,79,75,73,69,64,   // [Sax] [end cycle] [304 samples for note C2] -1.2dB start at   (25852 from beginning)
    62,61,61,60,60,62,63,64,63,64,67,69,67,68,72,73,70,69,69,66,63,60,59,61,62,64,67,68,67,65,66,64,64,65,70,72,72,75,79,78,76,78,81,81,84,87,92,96,95,97,97,95,96,99,100,103,104,103,103,100,95,93,93,93,94,101,105,107,109,113,114,115,117,120,123,126,130,137,143,147,154,159,161,162,163,165,167,167,165,164,160,155,149,141,131,121,110,99,89,77,67,57,52,50,48,51,51,52,47,40,36,29,22,23,25,28,31,32,35,38,40,43,48,51,51,53,56,57,58,58,61,68,72,76,81,84,83,82,84,84,80,78,79,75,71,69,70,70,71,75,77,78,81,89,101,117},
    {143,160,176,184,192,200,204,210,216,221,227,231,232,235,233,232,233,235,238,236,234,233,231,226,223,221,219,215,214,216,218,220,220,222,223,225,226,227,228,231,233,230,228,230,232,231,229,227,224,220,217,214,209,203,198,195,193,191,194,195,191,188,186,187,187,186,184,183,180,175,174,174,173,171,168,165,161,157,154,151,148,146,144,141,139,138,138,137,137,137,136,135,135,137,140,138,136,137,138,137,135,135,135,133,129,126,125,123,122,123,121,116,112,108,105,103,100,101,99,95,94,94,96,97,97,98,101,100,98,99,99,100,100,100,101,103,106,108,109,111,112,111,108,105,101,97,91,87,85,   // [Sax] [end cycle] [304 samples for note F#2]  start at   (43166 from beginning)
    81,77,76,78,77,75,74,73,69,64,59,56,59,63,64,62,60,61,63,62,61,62,64,64,67,71,77,83,83,82,82,85,87,89,89,91,92,92,92,93,96,96,93,90,91,92,92,88,85,84,78,75,76,79,80,80,80,81,81,80,82,83,85,87,92,98,101,106,110,114,114,113,117,123,128,131,136,143,150,158,163,163,164,167,171,173,175,176,178,174,168,161,156,149,137,124,111,100,86,73,63,58,55,53,58,61,61,58,53,47,43,38,37,41,47,55,60,67,73,75,76,79,83,85,89,95,100,103,105,104,104,105,108,107,103,98,97,96,91,85,79,76,70,66,65,65,65,66,71,74,77,81,92,104,123},
  }
};
// Band of notes from F#2 to B2:
const PROGMEM uint8_t MsfNoteBand1[][6][215] = // [instrument] [cycle] [samples] // band 1
{
  { // instrument 1 (piano) here:
    {132,103,95,104,114,116,111,104,104,114,129,144,147,131,107,87,80,85,93,94,84,68,60,66,84,108,120,118,110,104,98,91,87,94,96,83,66,61,62,60,51,33,12,2,12,41,75,99,108,106,109,118,122,118,110,98,85,77,75,70,57,40,23,9,5,13,26,40,51,55,50,42,38,38,35,34,44,51,51,48,47,50,55,59,56,46,40,47,59,70,82,95,104,113,123,134,142,139,129,117,105,99,103,113,122,127,131,128,122,123,129,132,141,155,161,156,151,146,141,138,134,130,125,121,118,113,112,111,104,97,96,99,103,112,133,159,172,171,168,166,173,185,193,194,191,    // [Piano] [start cycle] [215 samples for note F#2] start at 672 (996 from beginning)
    188,178,170,175,189,196,196,200,203,194,184,182,185,183,180,185,202,223,234,224,204,190,192,207,228,240,232,211,196,199,210,216,218,221,220,214,209,208,210,214,215,204,184,167,164,176,194,202,188,163,147,152,171,187,194,189,170,154,157,175,190,198,197,190,176,169,174,179,172,149},
    {129,115,96,78,66,59,57,61,73,87,98,104,103,97,89,83,78,78,80,82,84,85,82,76,68,59,50,44,43,47,55,62,65,60,48,34,27,28,36,43,44,39,31,25,21,21,24,28,28,28,33,48,65,76,78,75,76,81,84,82,74,64,54,47,46,51,58,64,67,68,68,74,86,99,109,118,128,138,143,139,130,125,126,128,128,126,121,112,105,108,122,143,163,176,181,185,193,206,220,231,237,238,238,237,233,227,215,198,183,174,177,186,193,190,179,164,147,133,122,118,117,117,113,105,97,95,97,101,108,118,129,138,142,142,143,149,160,174,186,193,190,178,160,144,135,    // [Piano] [start cycle  pp = skipped 1 cycle] [215 samples for note C3] start at 672
    134,136,137,133,125,116,109,105,107,112,117,118,120,124,126,124,119,117,124,139,156,165,170,171,168,156,140,127,125,132,141,148,153,158,163,167,172,184,202,219,227,219,200,179,165,159,161,168,180,194,207,216,226,236,247,253,253,249,245,241,235,230,225,222,216,206,189,167,142,136},
    {125,122,121,123,124,122,119,117,117,119,120,119,116,113,112,115,119,123,126,126,126,127,127,125,121,117,115,116,118,119,118,114,107,101,96,94,94,96,97,95,92,88,85,84,83,84,86,87,87,85,82,80,78,74,70,66,64,63,64,68,73,76,79,81,85,90,96,102,107,111,114,115,117,118,120,122,123,123,123,123,122,122,121,120,121,121,120,117,113,110,106,103,100,98,97,97,99,103,106,107,105,104,104,106,110,114,119,124,127,126,123,120,117,115,115,117,120,125,133,143,150,150,145,140,137,138,141,143,145,145,144,143,143,142,141,141,    // [Piano] [mid cycle] [215 samples for note F#2] start at 11864
    143,147,153,157,159,158,156,154,154,158,163,168,171,171,168,165,161,158,156,155,153,150,147,144,141,141,143,148,153,158,163,167,170,168,164,159,156,157,162,168,172,170,164,156,149,147,149,151,153,154,154,153,154,156,159,161,162,160,157,154,155,157,158,156,152,147,143,141,141,142,142,140,138,138,139,138,136,131,127},
    {125,119,115,113,113,114,115,114,113,113,115,117,118,117,114,110,105,100,96,91,86,80,75,72,70,70,69,68,69,73,78,84,89,93,96,100,102,104,105,105,104,102,99,97,96,95,94,93,92,92,92,91,89,86,85,85,86,87,89,90,91,92,92,93,94,96,96,97,98,101,106,111,117,125,134,142,150,158,165,171,176,179,179,176,172,167,161,156,151,146,142,138,136,135,136,136,138,140,142,144,145,147,149,152,154,155,154,151,147,142,136,132,128,126,126,127,131,136,140,142,141,137,132,128,124,121,119,116,113,111,109,109,108,108,107,107,108,110,   // [Piano] [mid cycle] [215 samples for note C3] start at 11859
    111,112,112,113,114,116,116,116,116,114,113,112,111,109,107,103,98,94,94,96,100,104,109,116,124,133,138,142,143,145,147,150,153,156,159,159,159,161,163,167,169,170,171,175,180,186,189,191,191,190,189,188,186,183,179,174,169,165,163,162,161,160,159,161,164,168,170,169,167,163,159,155,151,146,140,134,130},
    {126,124,121,119,117,116,115,115,116,115,114,112,109,105,99,93,85,78,73,70,70,72,74,76,77,77,76,76,76,76,76,76,78,80,83,87,89,89,88,86,85,84,83,81,78,76,73,71,70,71,72,74,77,80,84,87,89,89,87,83,79,75,72,72,74,78,84,90,96,102,107,111,114,116,116,114,111,107,103,100,96,93,91,89,89,89,91,93,95,98,99,100,101,102,103,106,110,116,124,131,137,140,141,139,135,129,123,119,119,123,130,138,146,153,158,161,163,162,159,156,152,148,145,142,142,144,147,152,157,161,166,169,172,173,173,171,168,163,158,153,149,147,148,153, // [Piano] [end cycle] [215 samples for note F#2] start at (123186 from beginning) (filtered less)
    160,168,176,180,181,180,177,173,169,166,165,165,167,171,175,178,181,183,183,184,184,183,181,178,175,173,173,175,179,184,188,191,191,188,184,178,173,168,164,161,158,156,157,159,162,163,163,162,158,154,148,142,136,131,127,126,126,129,133,137,140,142,142,142,142,140,138,136,133,130,127},
    {128,117,107,96,85,77,71,69,69,72,76,83,92,102,110,115,115,111,104,97,90,85,81,79,77,75,72,69,66,64,64,65,68,70,72,71,68,65,63,64,67,71,75,77,77,76,74,73,72,74,79,86,94,103,113,123,133,141,148,153,156,155,152,147,142,138,137,137,137,136,134,130,125,120,117,117,120,126,134,144,153,162,168,171,174,176,176,174,170,164,155,146,139,136,137,142,150,159,169,178,185,189,189,187,182,177,172,169,166,165,165,166,168,172,176,180,183,185,186,185,184,182,178,174,169,164,158,152,145,137,130,123,118,114,110,106,103,101,   // [Piano] [end cycle] [215 samples for note C3] start at 66333
    102,105,110,116,120,123,124,122,119,114,109,104,100,99,101,104,105,105,104,102,102,102,104,107,110,114,119,124,130,135,140,141,142,141,141,139,137,135,135,137,142,148,154,159,163,167,169,172,173,173,170,163,153,142,132,124,118,115,115,118,122,126,130,132,135,138,141,146,151,155,157,156,155,151,147,141,133},
  },
  { // instrument 2 (guitar) here:
    {126,116,108,101,95,89,81,73,65,57,49,41,35,31,28,26,23,21,20,19,18,20,25,33,41,48,51,54,58,66,76,85,92,97,103,110,118,125,130,135,139,145,152,160,168,175,179,180,181,185,190,195,197,197,196,197,199,202,202,199,195,192,192,193,193,190,186,183,181,182,181,180,176,172,167,165,164,165,166,165,163,159,155,150,146,142,138,135,133,134,135,136,134,130,128,128,128,127,125,123,121,121,123,125,126,127,128,127,125,122,119,117,116,117,119,122,125,128,128,126,124,121,120,119,119,119,119,121,122,124,124,121,117,113,111,  // [Guitar] [start cycle] [215 samples for note F#2] start at 1931 (2217 from beginning of file)
    110,109,107,104,100,96,96,96,97,97,95,93,90,89,90,91,91,91,89,88,88,87,86,82,78,75,75,76,77,78,78,79,82,87,92,96,99,100,101,102,105,111,117,121,124,126,128,131,135,141,147,154,160,165,169,173,178,182,187,192,196,199,202,205,207,206,202,198,195,194,192,189,183,175,168,161,155,150,144,135},
    {125,118,111,106,103,104,105,106,103,97,87,77,68,63,61,61,61,59,53,44,32,22,16,14,18,26,34,39,39,35,28,21,18,18,24,32,41,48,52,54,55,57,61,65,68,71,73,75,79,85,93,100,106,109,111,113,116,122,129,137,144,149,149,146,142,139,140,144,151,159,165,167,165,162,159,157,158,160,163,164,165,164,164,164,165,166,167,167,168,169,170,172,174,175,175,174,171,166,161,157,155,157,161,167,172,175,175,173,170,168,167,169,171,172,173,173,172,171,171,170,169,168,168,170,172,174,175,172,167,159,150,144,140,138,138,139,139,139,  // [Guitar] [start cycle] [215 samples for note C3] start at 1931 (2349 from beginning of file)
    140,140,141,142,142,140,137,133,129,124,119,114,110,106,104,103,104,105,107,108,108,106,102,98,94,92,91,92,93,94,96,100,105,111,118,124,129,131,131,129,128,127,128,130,131,132,132,132,134,138,145,154,161,166,169,170,169,170,172,174,176,177,177,177,176,177,178,179,179,177,173,165,156,147,139,133,130},
    {123,116,110,105,101,97,94,91,87,84,81,78,76,74,72,72,71,69,66,64,63,64,67,70,73,75,77,80,84,88,93,98,102,107,111,116,120,125,129,133,136,139,143,147,150,153,156,159,163,166,170,172,173,174,175,175,176,177,177,177,175,174,172,172,172,172,171,169,167,165,164,162,159,156,153,150,148,146,143,141,140,138,137,135,134,133,132,130,128,127,126,126,126,125,124,123,122,122,121,121,120,119,120,121,123,126,128,130,132,133,132,131,130,128,127,127,127,127,127,127,127,126,124,123,121,120,120,119,119,119,120,121,122,122,   // [Guitar] [mid cycle] [215 samples for note F#2] start at  (11238 from beginning of file)
    121,119,118,115,113,111,110,110,110,110,111,111,111,111,111,111,112,112,112,113,113,114,115,117,119,120,119,118,116,114,113,112,111,111,112,113,115,116,116,116,117,118,120,122,123,124,125,127,129,132,135,138,141,142,143,144,146,148,149,151,153,154,156,158,158,159,160,161,162,162,162,160,157,154,151,148,146,144,141,136,130},
    {129,129,129,129,127,125,122,118,113,108,103,99,95,92,90,89,88,86,85,84,82,80,78,75,72,70,67,65,64,63,63,64,66,68,71,74,77,80,82,84,85,86,87,88,90,92,94,96,99,102,104,107,110,112,114,116,118,119,120,122,124,126,128,130,132,134,136,138,140,142,144,146,147,147,147,146,145,143,142,141,140,140,139,139,138,138,138,138,139,140,140,141,142,143,145,146,147,147,148,148,149,149,150,151,152,152,153,152,151,150,148,147,146,146,146,147,148,150,151,153,155,158,160,162,164,165,165,165,165,164,163,162,161,161,160,159,158,  // [Guitar] [mid cycle] [215 samples for note C3] start at  (16322 from beginning of file)
    157,155,154,152,150,148,145,143,141,140,139,138,138,138,139,139,140,140,139,138,136,133,130,127,123,121,119,117,117,117,117,117,117,117,116,115,114,114,113,113,113,113,113,115,116,119,122,125,128,130,132,134,134,134,133,132,132,132,133,134,136,138,141,143,146,148,149,148,147,145,142,139,136,134,132,131,130,129},
    {128,123,118,113,108,104,100,96,92,88,84,81,77,74,70,68,66,64,63,63,63,64,64,65,66,68,71,74,77,81,86,91,96,101,106,110,114,118,122,126,130,134,138,141,145,149,152,155,158,160,161,163,164,164,165,165,165,164,164,164,163,162,161,159,158,156,154,153,150,148,146,144,143,141,140,139,138,137,136,136,135,135,135,136,136,137,138,139,140,141,141,142,142,142,142,141,141,141,141,142,143,144,144,143,143,142,141,139,138,136,134,132,130,128,125,122,119,116,113,109,106,102,99,97,95,94,94,94,94,93,93,92,92,92,92,92,93,93,  // [Guitar] [end cycle] [215 samples for note F#2] start at  (43914 from beginning of file)
    95,96,97,98,100,101,103,105,107,109,111,113,116,120,123,126,129,131,132,133,134,134,134,133,133,133,132,132,132,132,132,132,132,132,132,133,133,133,134,134,135,136,138,139,141,142,143,144,145,147,148,150,152,153,155,157,159,161,163,164,166,166,167,167,167,166,165,164,163,162,160,156,153,148,143,138,133},
    {122,118,114,110,106,103,99,96,93,90,88,85,82,80,78,76,73,71,69,67,66,65,64,63,63,63,64,65,66,67,69,72,74,76,78,80,82,85,87,89,91,93,96,98,101,103,106,109,111,114,117,119,121,123,125,127,129,131,133,135,138,140,143,145,148,150,152,153,155,155,156,156,156,157,157,157,158,159,160,160,161,162,163,163,163,163,163,163,162,162,161,160,159,159,158,158,158,157,156,156,154,153,151,149,146,144,142,140,138,136,135,134,134,133,132,132,131,130,129,128,126,125,124,123,121,120,120,119,118,118,117,117,116,115,113,112,111,  // [Guitar] [end cycle] [215 samples for note C3] start at  (61057 from beginning of file)
    109,108,108,108,108,108,108,109,110,110,111,112,113,113,113,114,114,114,115,115,116,117,117,118,119,120,121,122,123,124,125,126,128,129,131,134,136,139,141,144,147,150,152,154,156,158,159,160,161,163,164,164,165,166,167,168,168,169,169,169,168,167,166,164,163,161,159,156,154,152,149,146,143,140,137,134,130,126},
  },
  { // instrument 3 (marimba) here:
    {125,120,115,110,105,100,95,90,86,82,78,74,71,68,65,62,60,58,56,55,53,52,51,51,50,50,49,49,49,49,49,49,49,48,48,48,47,47,46,45,44,43,41,40,38,36,34,32,30,28,26,24,22,20,18,16,14,12,11,10,9,8,7,7,7,8,9,10,11,13,15,17,20,23,26,29,33,37,41,45,49,53,58,62,66,71,75,79,83,87,91,94,98,101,104,107,110,112,114,116,118,120,121,123,124,125,127,128,129,130,131,133,134,135,137,139,141,143,145,147,150,153,156,159,163,166,170,174,178,182,186,191,195,199,204,208,212,216,220,224,227,230,233,236,239,241,243,244,246,247,247,   // [Marimba] [start cycle] [215 samples for note C3 instead of F#2] start at  (Created16 near end of file)
    247,247,247,246,246,244,243,241,240,238,236,234,232,229,227,225,223,221,219,217,215,213,212,210,209,208,207,206,205,205,204,204,204,204,204,203,203,203,203,202,202,201,200,199,198,197,195,193,191,189,186,183,180,176,173,169,164,160,155,151,146,141,135,130},
    {125,120,115,110,105,100,95,90,86,82,78,74,71,68,65,62,60,58,56,55,53,52,51,51,50,50,49,49,49,49,49,49,49,48,48,48,47,47,46,45,44,43,41,40,38,36,34,32,30,28,26,24,22,20,18,16,14,12,11,10,9,8,7,7,7,8,9,10,11,13,15,17,20,23,26,29,33,37,41,45,49,53,58,62,66,71,75,79,83,87,91,94,98,101,104,107,110,112,114,116,118,120,121,123,124,125,127,128,129,130,131,133,134,135,137,139,141,143,145,147,150,153,156,159,163,166,170,174,178,182,186,191,195,199,204,208,212,216,220,224,227,230,233,236,239,241,243,244,246,247,247,   // [Marimba] [start cycle] [215 samples for note C3 instead of F#2] start at  (Created16 near end of file)
    247,247,247,246,246,244,243,241,240,238,236,234,232,229,227,225,223,221,219,217,215,213,212,210,209,208,207,206,205,205,204,204,204,204,204,203,203,203,203,202,202,201,200,199,198,197,195,193,191,189,186,183,180,176,173,169,164,160,155,151,146,141,135,130},
    {}, // mid cycle not used
    {}, // mid cycle not used
    {126,124,122,120,119,117,115,113,111,109,108,106,104,102,101,99,97,96,94,93,91,89,88,86,85,84,82,81,80,78,77,76,75,74,73,72,71,70,69,69,68,67,67,66,65,65,65,64,64,64,63,63,63,63,63,63,63,63,64,64,64,65,65,66,66,67,67,68,69,70,71,72,72,73,75,76,77,78,79,80,82,83,84,86,87,89,90,92,93,95,97,98,100,102,103,105,107,109,110,112,114,116,118,120,121,123,125,127,129,131,133,134,136,138,140,142,144,145,147,149,151,152,154,156,157,159,161,162,164,165,167,168,170,171,172,174,175,176,177,178,180,181,182,182,183,184,     // [Marimba] [end cycle] [152 samples for note C3 instead of F#2] -6dB start at  (Sinewave near end of file)
    185,186,187,187,188,188,189,189,190,190,190,191,191,191,191,191,191,191,191,190,190,190,189,189,189,188,187,187,186,185,185,184,183,182,181,180,179,178,177,176,174,173,172,170,169,168,166,165,163,161,160,158,157,155,153,152,150,148,146,145,143,141,139,137,135,134,132,130,128},
    {126,124,122,120,119,117,115,113,111,109,108,106,104,102,101,99,97,96,94,93,91,89,88,86,85,84,82,81,80,78,77,76,75,74,73,72,71,70,69,69,68,67,67,66,65,65,65,64,64,64,63,63,63,63,63,63,63,63,64,64,64,65,65,66,66,67,67,68,69,70,71,72,72,73,75,76,77,78,79,80,82,83,84,86,87,89,90,92,93,95,97,98,100,102,103,105,107,109,110,112,114,116,118,120,121,123,125,127,129,131,133,134,136,138,140,142,144,145,147,149,151,152,154,156,157,159,161,162,164,165,167,168,170,171,172,174,175,176,177,178,180,181,182,182,183,184,     // [Marimba] [end cycle] [152 samples for note C3] -6dB start at  (Sinewave near end of file)
    185,186,187,187,188,188,189,189,190,190,190,191,191,191,191,191,191,191,191,190,190,190,189,189,189,188,187,187,186,185,185,184,183,182,181,180,179,178,177,176,174,173,172,170,169,168,166,165,163,161,160,158,157,155,153,152,150,148,146,145,143,141,139,137,135,134,132,130,128},
  },
  { // instrument 4 (trumpet) here:
    {122,115,109,104,99,97,95,95,96,97,100,103,106,110,113,116,117,118,119,119,119,118,117,117,117,116,116,116,116,116,116,117,117,117,118,118,119,119,120,121,121,122,122,123,124,124,124,124,124,125,125,125,125,125,125,125,125,126,126,126,126,126,125,125,125,125,125,125,124,124,124,124,124,123,123,123,123,123,123,123,123,123,123,123,123,123,123,123,124,124,125,125,126,126,127,128,128,129,129,130,130,130,130,130,130,130,131,131,131,131,130,130,130,129,129,128,127,126,126,125,125,124,124,124,123,123,123,123,123, // [Trumpet] [start cycle] [215 samples for note F#2] (taken from C3) start at 1697 (1733 from beginning)
    123,123,123,123,123,123,123,124,124,125,125,126,127,128,128,129,129,130,131,131,132,132,132,132,131,130,130,130,130,130,130,130,129,128,127,126,124,123,121,120,118,117,115,113,112,112,111,111,111,113,115,117,117,118,119,121,123,126,128,132,135,138,142,147,151,156,160,165,169,173,177,182,186,190,194,196,196,196,194,190,185,178,169,159,148,134},
    {122,115,109,104,99,97,95,95,96,97,100,103,106,110,113,116,117,118,119,119,119,118,117,117,117,116,116,116,116,116,116,117,117,117,118,118,119,119,120,121,121,122,122,123,124,124,124,124,124,125,125,125,125,125,125,125,125,126,126,126,126,126,125,125,125,125,125,125,124,124,124,124,124,123,123,123,123,123,123,123,123,123,123,123,123,123,123,123,124,124,125,125,126,126,127,128,128,129,129,130,130,130,130,130,130,130,131,131,131,131,130,130,130,129,129,128,127,126,126,125,125,124,124,124,123,123,123,123,123, // [Trumpet] [start cycle] [215 samples for note C3] (same as above)
    123,123,123,123,123,123,123,124,124,125,125,126,127,128,128,129,129,130,131,131,132,132,132,132,131,130,130,130,130,130,130,130,129,128,127,126,124,123,121,120,118,117,115,113,112,112,111,111,111,113,115,117,117,118,119,121,123,126,128,132,135,138,142,147,151,156,160,165,169,173,177,182,186,190,194,196,196,196,194,190,185,178,169,159,148,134},
    {117,91,71,59,52,51,54,60,69,79,92,104,114,120,123,122,118,112,106,102,100,100,102,105,110,115,121,125,128,130,129,125,122,118,114,111,109,108,109,111,114,117,120,122,125,126,126,126,125,124,125,125,127,129,131,134,137,139,141,142,142,141,140,138,135,131,127,123,119,116,114,113,112,113,114,116,117,119,119,120,119,118,117,116,115,115,115,116,117,118,120,121,123,124,125,126,126,126,126,126,125,125,125,126,127,128,130,133,135,136,137,138,138,138,136,134,132,130,127,125,124,123,123,124,124,125,126,126,127,127, // [Trumpet] [mid cycle] [215 samples for note F#2] 0.3dB start at  (5635 from beginning)
    128,127,127,126,126,126,128,130,134,138,142,145,148,149,150,149,147,144,139,134,129,124,120,117,116,117,117,119,120,122,124,125,126,127,127,126,125,124,122,120,118,118,118,118,118,118,119,118,118,117,116,114,112,111,110,110,111,113,116,119,123,127,131,135,139,143,145,147,149,152,155,159,165,173,184,196,211,227,241,250,250,237,212,179,144},
    {119,94,74,61,53,51,53,59,68,80,93,106,117,125,129,128,124,118,111,106,103,101,102,104,108,112,118,123,127,129,128,126,122,118,114,111,109,108,109,111,113,116,120,124,127,130,131,132,132,131,130,129,129,129,130,132,134,136,137,138,138,138,138,137,136,134,132,130,128,126,124,122,121,121,121,121,121,121,121,121,120,119,118,117,115,114,114,113,112,112,111,111,110,109,108,108,109,109,110,111,113,114,116,119,122,125,129,132,136,139,142,144,144,143,141,139,136,133,130,128,126,125,125,125,126,127,129,131,134,136, // [Trumpet] [mid cycle] [215 samples for note C3] 0.3dB start at  (8398 from beginning)
    138,139,139,138,135,132,128,123,118,114,111,110,109,109,110,113,115,118,121,123,124,123,121,119,118,117,117,120,124,130,135,141,145,149,151,152,151,148,143,137,131,126,122,118,115,114,114,115,116,117,117,117,116,114,111,108,106,105,105,107,111,117,125,133,141,149,156,161,165,167,169,171,173,178,185,195,209,225,240,250,250,237,212,180,147},
    {122,89,65,50,43,43,47,55,64,75,86,98,108,116,120,122,119,115,109,105,101,100,100,101,103,106,109,112,114,114,113,111,107,105,103,104,106,109,114,119,124,128,132,136,138,139,139,138,135,133,130,129,129,130,131,133,135,137,138,138,136,133,129,124,119,115,111,108,107,107,108,110,113,117,121,125,128,130,131,131,128,125,122,119,117,116,116,117,118,119,119,120,120,121,122,123,124,125,126,127,128,130,132,134,136,138,139,140,141,142,141,141,140,138,137,135,133,130,127,124,120,116,112,109,108,107,107,108,110,113,  // [Trumpet] [end cycle] [215 samples for note F#2] 0dB start at  (70326 from beginning)
    116,120,124,129,133,138,142,147,152,156,158,159,158,156,153,149,145,142,138,133,129,125,122,121,121,123,125,127,129,129,129,128,126,124,122,120,118,116,115,115,116,118,121,124,127,129,130,129,127,123,118,112,105,99,95,92,92,95,99,105,112,119,126,134,142,150,156,162,166,168,170,172,175,179,186,195,208,222,238,250,254,248,228,196,158},
    {117,87,65,50,44,44,50,60,72,88,103,118,130,136,137,133,124,113,103,95,91,90,93,99,106,115,123,129,132,133,131,127,122,117,113,109,106,105,106,108,111,114,118,120,122,123,122,121,120,120,121,123,126,130,134,138,141,145,147,148,148,146,143,138,133,128,124,121,119,118,118,118,118,118,118,118,118,118,117,117,117,117,118,118,118,118,118,118,118,118,118,118,117,117,116,115,114,114,114,114,115,118,120,124,127,131,134,136,138,140,140,140,140,139,137,136,135,133,131,130,128,128,127,128,128,129,130,131,132,133,133, // [Trumpet] [end cycle] [215 samples for note C3] 0dB start at  (102530 from beginning)
    132,131,130,129,128,127,127,127,127,128,128,128,128,128,128,127,126,125,123,120,118,116,114,112,111,111,111,112,114,117,121,125,129,132,134,134,133,131,129,127,126,125,125,125,125,126,126,125,124,122,119,116,113,110,108,108,109,112,116,121,127,134,141,147,152,157,160,163,165,166,169,172,178,187,200,215,232,246,254,252,237,210,175,139},
  },
  { // instrument 5 (sax) here:
    {132,135,137,139,138,143,146,146,146,145,145,153,156,152,155,159,157,157,160,161,160,163,166,166,165,168,166,161,166,171,167,174,177,173,172,170,167,173,175,170,166,165,165,165,167,169,168,164,162,161,158,155,159,158,158,158,154,151,151,149,150,152,150,148,149,152,153,152,149,146,144,140,135,132,131,132,133,128,128,130,128,127,124,120,121,119,118,120,119,117,116,115,115,116,115,119,119,112,109,109,110,116,116,115,107,100,103,107,102,98,102,104,100,102,103,100,105,106,104,106,110,111,108,106,104,102,106,   // [Sax] [start cycle] [215 samples for note F#2] start at 770 (838 from beginning) n
    108,108,111,112,105,102,101,96,95,100,101,104,110,109,109,106,106,106,103,106,110,112,113,116,116,116,108,106,112,115,114,114,112,113,116,115,112,110,111,108,107,108,111,114,115,110,107,106,106,102,100,100,103,104,105,101,99,99,98,99,102,101,98,97,93,96,98,98,95,91,90,93,97,98,101,103,101,101,105,106,106,111,114,113,112,116,118,123},
    {132,132,133,136,138,138,137,141,144,143,148,154,156,155,155,156,157,157,157,160,160,161,163,164,162,163,165,168,172,174,172,172,170,166,164,165,166,164,164,165,166,166,164,162,166,166,162,166,168,161,154,153,152,153,156,157,155,154,155,158,157,150,146,149,149,144,142,141,141,142,143,139,135,131,131,134,137,137,135,134,133,130,126,126,126,124,123,123,122,116,113,116,119,116,115,115,114,113,113,112,111,111,113,115,114,110,107,106,104,105,106,107,110,113,110,106,106,106,106,107,110,111,110,111,110,107,108,  // [Sax] [start cycle] [215 samples for note C3] start at 770 (819 from beginning) n
    109,111,112,110,108,107,108,109,110,108,106,108,110,111,113,111,110,111,113,112,112,114,115,114,114,111,109,109,110,113,116,115,114,115,116,115,114,113,110,106,106,113,120,121,115,110,106,103,104,105,102,100,104,105,99,95,97,97,92,90,90,93,94,95,97,100,100,95,91,94,96,98,104,106,102,101,104,102,101,107,111,110,110,112,113,114,118,124},
    {143,167,184,194,202,203,196,191,195,202,213,223,224,222,221,218,216,216,223,231,228,227,225,229,232,235,238,238,237,235,229,223,225,227,223,218,216,212,207,205,203,202,202,203,200,198,194,188,184,180,177,176,177,175,173,173,168,162,159,154,152,147,141,140,141,145,142,139,139,140,141,142,143,143,140,133,129,126,123,127,128,126,124,121,119,116,113,113,112,116,115,110,107,103,102,101,102,100,99,97,94,89,91,92,92,91,90,85,80,80,75,74,77,74,72,73,71,70,69,70,73,75,77,80,83,83,83,89,91,92,95,97,100,105,107,    // [Sax] [mid cycle] [215 samples for note F#2] start at   (2785 from beginning) n
    106,101,91,83,84,82,79,81,85,86,89,88,89,92,98,100,100,106,111,114,124,132,137,142,149,151,148,154,161,165,172,172,163,156,147,131,112,95,74,56,49,41,37,38,36,33,28,23,19,19,24,35,41,45,51,54,56,63,70,80,91,94,96,99,97,92,90,87,79,72,65,55,47,47,52,61,72,87,105,124},
    {134,156,172,183,190,192,189,184,188,199,208,216,217,215,214,213,209,208,212,219,220,221,223,223,224,226,230,232,230,228,226,222,220,218,217,215,212,209,211,210,206,202,200,198,199,199,195,192,190,189,186,184,183,180,176,175,172,167,162,160,155,150,147,146,147,149,148,147,148,146,143,142,142,142,142,142,138,132,130,132,135,135,133,129,126,124,123,122,120,119,119,116,111,104,102,102,101,99,96,91,87,84,84,84,84,86,88,87,83,80,77,76,79,83,85,84,82,82,83,85,90,94,97,99,98,97,99,103,104,104,106,110,111,111,    // [Sax] [mid cycle] [215 samples for note C3] start at   (2765 from beginning) n
    115,120,115,110,106,104,102,102,102,103,103,103,103,104,107,112,115,115,117,120,124,129,133,138,145,147,149,151,153,158,160,160,159,154,146,137,125,108,86,66,50,39,34,32,32,30,24,18,12,6,6,13,23,31,35,40,42,45,51,59,67,75,79,81,84,83,79,76,73,68,62,56,48,40,38,42,51,61,75,94,114},
    {138,165,182,192,203,213,216,222,228,225,224,228,226,227,232,231,229,229,225,218,216,219,217,218,224,228,228,231,230,229,228,229,229,227,227,221,217,215,207,200,195,191,188,184,182,182,183,184,180,176,175,174,172,174,172,161,154,149,142,139,141,141,137,135,132,132,133,135,138,138,138,138,136,134,129,124,124,124,122,119,113,103,98,95,92,92,100,103,103,105,102,96,96,98,98,100,106,104,101,104,106,107,106,102,91,81,79,76,76,77,74,68,62,57,58,62,67,67,65,64,61,64,68,68,72,79,80,80,86,89,86,92,96,95,95,92,89,   // [Sax] [end cycle] [215 samples for note F#2] start at   (34829 from beginning)n
    90,91,89,86,84,79,81,83,80,79,81,78,77,80,89,99,106,113,114,116,120,124,131,143,152,158,163,169,172,174,177,176,170,164,156,143,127,109,93,73,59,55,54,59,62,55,45,39,36,38,50,62,68,75,81,83,84,90,97,105,109,106,104,104,99,92,89,86,78,72,67,63,60,64,69,75,89,109},
    {134,146,154,160,167,174,182,188,190,192,195,199,204,208,208,207,206,204,199,197,198,196,196,199,201,204,209,211,212,212,214,216,217,216,212,206,201,196,193,192,191,189,186,183,181,182,180,177,175,172,170,169,168,167,163,160,157,153,149,146,147,146,144,144,144,145,147,148,148,147,145,142,140,141,143,143,143,141,135,130,127,128,129,127,124,123,124,125,127,126,124,124,124,124,124,122,121,120,119,119,116,112,109,106,104,101,98,97,96,93,91,89,87,87,90,92,91,90,92,92,94,98,104,109,112,110,108,106,106,108,109,  // [Sax] [end cycle] [215 samples for note C3] start at   (40171 from beginning) n -1.8dB
    110,111,110,110,111,113,116,116,116,115,116,116,115,111,108,104,101,101,104,106,108,109,109,110,112,115,119,123,128,131,133,134,135,139,142,144,145,142,138,131,122,109,93,79,66,56,50,47,44,40,33,27,23,24,27,33,37,39,44,50,55,60,66,72,77,82,86,88,91,90,86,82,77,71,67,64,61,59,58,61,65,75,92,113},
  }
};
const PROGMEM uint8_t MsfNoteBand2[][6][152] = // [instrument] [cycle] [samples] // band 2
{
  { // instrument 1 (piano) here:
    {125,97,81,78,82,90,97,96,90,83,84,94,102,95,75,60,59,65,61,56,61,65,60,51,46,47,56,59,49,36,33,34,29,23,26,39,58,75,87,88,84,80,77,76,77,70,56,49,57,72,81,82,87,103,123,133,131,128,129,125,114,105,109,123,135,141,143,155,182,214,232,231,219,209,205,206,210,209,200,188,178,170,165,157,141,119,103,104,112,112,104,101,105,114,130,148,159,160,157,156,154,151,143,128,109,98,101,110,114,115,110,97,91,101,124,143,151,149,144,143,145,144,134,120,116,128,146,170,193,203,200,194,183,171,162,159,163,178,201,221,229,226,226,230,230,233,241,239,220,187,152,130},                             // [Piano] [start cycle] [152 samples for note C3] start at 830
    {116,104,91,78,68,64,63,61,59,59,57,50,39,30,21,12,9,17,25,27,26,30,39,53,65,72,79,91,107,122,134,142,148,146,140,137,141,152,165,178,186,190,193,196,201,208,212,208,203,203,202,198,192,190,196,207,218,224,228,232,229,217,203,194,190,188,185,181,181,183,179,172,165,158,152,152,154,150,140,128,116,105,93,80,71,72,84,94,97,96,95,95,97,99,101,103,107,111,112,111,109,107,102,91,78,67,61,59,58,58,56,53,56,68,83,96,105,112,119,125,129,128,127,130,136,139,135,128,126,132,141,154,168,178,181,186,194,199,199,195,188,174,156,140,131,133,138,139,135,132,131,127},                           // [Piano] [start cycle] [152 samples for note F#3] start at 830 (new)
    {126,122,117,111,103,93,86,83,85,89,94,98,100,101,98,96,95,96,99,101,103,102,100,98,98,100,100,100,101,100,97,92,85,78,73,68,64,63,71,86,102,114,123,129,134,137,140,142,144,146,147,147,148,148,145,142,141,143,146,147,146,146,147,147,144,142,142,143,144,145,147,150,152,150,145,140,137,133,128,123,120,118,114,111,110,112,115,119,121,123,124,123,117,110,105,102,100,98,97,100,104,109,113,114,113,111,110,111,113,116,121,128,134,141,148,155,159,163,167,172,176,178,175,170,167,166,166,168,173,180,184,185,184,181,177,171,164,156,151,149,147,146,146,147,145,142,138,135,134,134,133,132}, // [Piano] [mid cycle] [152 samples for note C3] -6dB start at 9962
    {127,120,114,107,101,96,92,90,88,85,82,80,79,79,79,77,73,69,65,63,64,66,69,73,78,83,88,92,93,93,92,90,90,92,94,98,101,105,110,116,122,126,130,133,138,143,147,149,152,154,158,162,166,170,173,175,176,176,176,174,172,170,168,167,166,164,163,161,159,157,156,154,153,150,146,143,140,136,131,126,123,123,126,131,136,139,140,140,139,137,135,131,127,123,120,118,116,116,117,118,120,121,123,125,126,126,125,124,124,123,122,122,123,126,129,131,132,132,131,130,130,131,132,133,134,135,136,137,137,139,141,144,146,147,147,146,145,143,141,140,141,143,145,146,148,150,151,151,148,143,136,130},      // [Piano] [mid cycle] [152 samples for note F#3] -6dB start at 29398 (new) (29489 from beginning)
    {128,119,111,104,101,100,101,102,101,100,98,97,97,96,95,94,95,98,102,107,111,113,114,113,111,108,106,105,106,108,110,112,113,114,114,115,118,123,129,135,140,142,143,143,143,143,143,144,146,148,150,153,155,157,159,159,158,158,159,160,160,160,160,159,157,153,149,145,142,138,134,132,130,128,126,125,127,131,135,139,140,139,136,129,120,111,102,94,87,83,81,81,82,81,81,82,84,86,88,89,91,92,91,89,87,86,87,88,91,96,99,100,99,98,99,101,104,109,116,126,137,148,157,164,169,172,175,178,182,187,190,191,189,186,180,172,163,157,152,151,152,154,156,157,157,157,157,157,154,148,140,131},          // [Piano] [end cycle] [152 samples for note C3] start at 85360
    {123,119,115,111,107,104,102,99,98,96,95,93,92,90,89,88,87,86,85,85,86,87,89,90,92,93,93,93,92,91,91,90,90,90,91,93,94,97,99,101,102,104,105,106,107,108,109,111,113,115,117,119,120,122,123,123,122,121,120,120,119,118,118,117,117,116,115,114,112,110,107,104,102,99,97,94,93,93,93,94,97,99,102,106,108,111,113,114,115,115,116,116,117,117,118,120,122,126,129,133,138,142,146,149,153,156,159,162,165,168,172,175,178,181,184,186,188,189,190,190,190,191,191,191,191,191,190,190,189,188,186,184,181,178,174,171,168,165,162,160,158,156,155,153,151,149,147,144,140,136,132,128},                // [Piano] [end cycle] [152 samples for note F#3] start at ~161898 (1622062 from beginning) (LP filter)
  },  
  { // instrument 2 (guitar) here:
    {122,112,105,104,106,105,97,84,70,62,61,61,57,45,30,17,14,21,33,40,37,27,19,18,27,39,49,53,55,59,64,69,72,76,82,92,102,109,112,115,122,133,144,149,147,142,139,144,154,163,167,164,159,158,160,163,164,164,164,165,167,167,168,169,172,174,175,173,168,161,156,156,163,171,175,174,170,168,168,171,173,173,172,171,169,168,168,171,174,174,168,157,146,140,138,139,139,139,141,142,142,139,135,128,122,115,108,104,103,105,107,108,106,101,95,92,91,93,95,99,106,115,124,130,131,129,127,128,131,132,132,133,139,150,161,168,170,169,171,174,177,178,177,177,178,180,178,170,158,145,135,130},           // [Guitar] [start cycle] [152 samples for note C3] start at 1366 (1661 from beginning of file)
    {125,119,121,124,126,124,117,108,100,96,96,100,103,105,103,97,87,76,66,57,51,46,44,44,46,49,53,57,62,68,72,75,76,74,70,64,58,54,52,54,58,63,67,71,72,72,72,73,76,82,89,95,100,103,103,101,100,99,100,103,108,113,117,120,124,129,134,138,141,141,139,137,137,139,142,146,148,148,148,148,150,154,159,164,164,161,154,149,147,150,156,162,163,158,150,142,139,143,152,160,164,161,154,145,139,137,139,144,148,151,152,153,153,152,148,141,132,124,123,130,145,164,179,185,181,170,158,152,156,168,182,194,199,197,187,175,165,158,157,161,171,184,198,211,219,219,211,196,178,159,144,135},               // [Guitar] [start cycle] [152 samples for note F#3] start at 1366 (1749 from beginning of file)
    {128,124,119,112,105,99,94,91,89,87,85,83,81,77,73,69,66,64,63,64,66,70,74,79,82,84,86,87,89,91,95,98,102,106,110,113,116,118,120,122,125,128,130,133,136,139,142,145,146,147,146,145,143,141,140,139,138,138,138,138,139,141,142,143,145,146,147,148,149,150,151,152,153,152,150,148,147,146,146,148,150,152,155,158,162,164,165,165,164,163,162,161,160,159,157,155,153,150,146,144,141,139,138,138,139,140,140,139,137,133,128,124,120,118,117,117,117,117,116,115,114,113,113,113,114,116,119,123,128,131,133,134,133,132,132,133,134,137,141,145,147,149,148,145,140,136,133,131,130,129,129,128},  // [Guitar] [mid cycle] [152 samples for note C3] start at  (11542 from beginning of file)
    {121,115,112,111,111,112,113,113,112,110,106,102,98,94,91,89,88,87,87,85,83,80,78,76,76,77,78,79,80,80,79,77,76,74,73,72,70,69,69,69,71,74,78,81,84,87,90,93,96,99,102,105,108,109,111,112,114,117,120,125,129,134,140,147,154,162,170,176,181,184,185,185,183,182,180,178,176,174,174,175,178,182,186,189,191,190,188,185,180,175,169,161,153,144,135,128,122,119,118,118,118,120,121,122,122,122,120,117,113,109,105,101,100,99,100,102,104,108,112,118,125,132,139,146,151,155,158,160,161,161,161,161,160,160,160,161,161,162,162,162,161,159,158,157,156,155,153,149,144,138,132,126},              // [Guitar] [mid cycle] [152 samples for note F#3] start at  (15429 from beginning of file)
    {121,116,110,105,100,95,91,87,84,80,77,74,71,68,66,64,63,63,63,65,67,69,72,75,79,82,84,88,91,94,97,101,105,108,112,116,119,122,125,128,131,134,137,140,144,147,150,153,155,156,156,156,157,157,158,160,161,162,163,163,163,163,162,161,160,159,158,158,157,157,155,153,150,147,144,141,138,136,135,134,133,132,131,129,127,126,124,122,121,120,119,118,117,116,114,112,110,109,108,108,108,108,109,110,111,112,113,114,114,115,115,116,117,118,120,121,122,124,125,127,129,132,136,139,143,147,151,154,156,159,160,162,163,165,166,167,168,169,169,168,167,165,163,160,157,153,150,146,141,137,132,127}, // [Guitar] [end cycle] [152 samples for note C3] start at  (43166 from beginning of file)
    {126,121,116,111,107,104,100,97,94,91,87,84,81,78,75,73,71,70,69,68,67,66,65,64,63,63,63,64,65,66,68,68,69,70,72,73,75,77,79,81,83,86,88,90,91,93,96,98,101,105,108,111,114,117,119,122,125,128,131,134,137,140,142,144,147,149,151,154,156,158,160,162,164,166,167,168,169,169,170,171,172,172,172,172,172,171,170,169,168,166,164,162,160,158,156,154,152,150,148,146,143,141,139,138,136,135,133,132,130,129,128,127,126,126,126,127,128,130,131,133,135,137,139,142,144,147,150,153,156,158,160,162,163,163,164,165,166,166,167,167,166,164,161,158,155,153,150,147,144,141,136,132},                // [Guitar] [end cycle] [152 samples for note F#3] start at  (56797 from beginning of file)
  },
  { // instrument 3 (marimba) here:
    {122,115,109,103,97,91,86,82,78,75,73,70,68,67,65,64,62,61,59,58,56,54,51,49,47,45,43,41,39,36,33,30,27,24,21,18,15,12,9,7,6,5,5,5,7,8,10,12,14,17,20,24,28,32,36,41,46,51,57,62,67,73,78,83,87,91,94,97,100,103,105,108,110,113,115,117,120,123,126,129,133,136,140,143,147,152,156,162,167,173,179,185,191,197,202,208,213,218,222,226,229,232,234,235,236,237,238,238,239,238,237,236,235,234,232,231,229,228,226,225,223,223,222,222,222,222,222,222,222,221,220,219,218,216,214,211,208,205,202,198,194,189,185,180,174,168,162,155,149,142,136,129},                                               // [Marimba] [start cycle] [152 samples for note F#3 instead of C3] start at 2348 (3087 from beginning of file)
    {122,115,109,103,97,91,86,82,78,75,73,70,68,67,65,64,62,61,59,58,56,54,51,49,47,45,43,41,39,36,33,30,27,24,21,18,15,12,9,7,6,5,5,5,7,8,10,12,14,17,20,24,28,32,36,41,46,51,57,62,67,73,78,83,87,91,94,97,100,103,105,108,110,113,115,117,120,123,126,129,133,136,140,143,147,152,156,162,167,173,179,185,191,197,202,208,213,218,222,226,229,232,234,235,236,237,238,238,239,238,237,236,235,234,232,231,229,228,226,225,223,223,222,222,222,222,222,222,222,221,220,219,218,216,214,211,208,205,202,198,194,189,185,180,174,168,162,155,149,142,136,129},                                               // [Marimba] [start cycle] [152 samples for note F#3] start at 2348 (3087 from beginning of file)
    {}, // mid cycle not used
    {}, // mid cycle not used
    {124,122,119,116,114,111,109,106,104,101,99,97,94,92,90,88,86,84,82,80,78,77,75,74,72,71,70,68,67,67,66,65,64,64,64,63,63,63,63,63,64,64,64,65,66,67,67,68,70,71,72,74,75,77,78,80,82,84,86,88,90,92,94,97,99,101,104,106,109,111,114,116,119,122,124,127,130,132,135,138,140,143,145,148,150,153,155,157,160,162,164,166,168,170,172,174,176,177,179,180,182,183,184,186,187,187,188,189,190,190,190,191,191,191,191,191,190,190,190,189,188,187,187,186,184,183,182,180,179,177,176,174,172,170,168,166,164,162,160,157,155,153,150,148,145,143,140,138,135,132,130,127},                              // [Marimba] [end cycle] [152 samples for note F#3 instead of C3] -6dB start at  (Sinewave near end of file)
    {124,122,119,116,114,111,109,106,104,101,99,97,94,92,90,88,86,84,82,80,78,77,75,74,72,71,70,68,67,67,66,65,64,64,64,63,63,63,63,63,64,64,64,65,66,67,67,68,70,71,72,74,75,77,78,80,82,84,86,88,90,92,94,97,99,101,104,106,109,111,114,116,119,122,124,127,130,132,135,138,140,143,145,148,150,153,155,157,160,162,164,166,168,170,172,174,176,177,179,180,182,183,184,186,187,187,188,189,190,190,190,191,191,191,191,191,190,190,190,189,188,187,187,186,184,183,182,180,179,177,176,174,172,170,168,166,164,162,160,157,155,153,150,148,145,143,140,138,135,132,130,127},                              // [Marimba] [end cycle] [152 samples for note F#3] -6dB start at  (Sinewave near end of file)
  },
  { // instrument 4 (trumpet) here:
    {117,108,101,97,95,96,98,102,107,111,115,118,119,119,118,117,117,116,116,116,116,116,117,117,118,119,120,121,121,122,123,124,124,124,125,125,125,125,125,125,126,126,126,125,125,125,125,125,124,124,124,123,123,123,123,123,123,123,123,123,123,123,124,124,125,126,127,128,129,130,130,130,130,130,130,131,131,131,130,130,129,128,127,126,125,124,124,123,123,123,123,123,123,123,123,123,124,125,126,127,128,129,130,131,131,132,132,132,131,130,130,130,130,129,128,127,125,122,120,118,116,114,112,111,111,112,115,117,118,120,122,126,130,134,140,146,152,158,165,171,177,182,189,194,196,196,193,186,177,164,149,129}, // [Trumpet] [start cycle] [152 samples for note C3] start at 1208 (1226 from beginning)
    {122,116,111,107,103,100,98,97,98,99,100,102,105,107,108,110,112,113,114,114,115,115,115,116,116,116,116,115,115,115,115,117,118,119,120,121,122,122,122,123,123,122,122,121,120,119,119,119,119,120,121,121,122,122,122,122,122,122,122,122,122,122,121,121,121,121,121,121,121,120,120,119,119,118,118,118,119,120,120,121,122,123,125,126,127,128,129,129,129,130,130,130,131,132,132,132,132,132,132,132,130,129,128,127,126,124,123,121,119,117,116,113,111,108,107,106,106,106,105,106,109,112,114,117,119,121,125,129,135,141,148,155,162,168,173,180,186,191,196,200,204,206,206,205,201,196,188,179,169,159,147,136}, // [Trumpet] [start cycle] [152 samples for note F#3] start at 1208 (1334 from beginning)
    {98,69,53,48,52,63,81,101,118,128,128,122,112,105,101,101,104,111,119,125,129,127,123,117,111,108,108,110,114,119,124,129,131,132,131,130,129,130,131,134,137,138,139,138,137,136,133,129,126,123,121,120,120,121,121,121,120,118,116,115,113,112,111,111,110,109,108,108,109,110,112,114,117,122,127,132,137,142,144,145,142,138,134,130,127,125,125,126,128,131,134,137,139,139,136,132,125,118,112,109,108,109,112,116,121,123,123,121,118,116,118,123,131,139,146,151,153,151,146,137,129,122,116,113,113,115,116,117,116,112,108,105,104,106,112,122,134,147,157,164,169,171,174,180,192,210,234,253,254,230,185,136},    // [Trumpet] [mid cycle] [152 samples for note C3] 0dB start at  (5938 from beginning)
    {107,77,59,46,42,45,52,60,69,80,93,106,117,126,132,135,135,132,130,128,127,126,125,125,125,124,124,123,120,117,114,110,106,102,99,96,94,92,91,91,93,95,99,104,110,114,118,120,122,123,124,124,125,127,129,130,130,129,130,131,133,135,138,142,146,149,151,152,151,149,145,141,138,135,133,130,127,125,122,119,117,115,114,113,114,114,114,115,116,118,121,124,128,129,130,130,129,128,126,125,123,122,121,120,119,119,119,119,120,121,121,120,119,118,117,116,115,114,113,113,114,116,119,123,127,131,135,139,143,148,151,154,156,158,160,162,164,167,168,170,172,177,183,192,206,223,239,250,249,228,191,147},                // [Trumpet] [mid cycle] [152 samples for note F#3] 0.3dB start at  (5157 from beginning)
    {109,71,50,44,49,62,79,100,118,130,133,127,118,111,108,110,115,122,127,130,130,125,116,105,95,87,84,85,91,100,109,118,124,129,132,135,138,140,143,146,149,151,151,148,142,135,128,122,118,116,115,116,119,122,124,126,126,124,122,121,120,120,120,119,120,122,124,124,122,118,112,108,108,112,120,128,134,138,139,137,134,130,125,122,122,124,129,134,138,139,137,133,128,123,118,115,113,112,114,118,124,130,135,139,139,137,133,128,123,119,118,119,124,129,135,139,139,137,134,130,127,124,121,119,118,119,120,120,117,112,107,103,102,105,111,121,132,143,152,159,163,167,171,178,190,206,228,247,254,238,198,145},        // [Trumpet] [end cycle] [152 samples for note C3] 0dB start at  (74779 from beginning)
    {103,67,44,31,30,35,47,61,75,91,106,121,133,142,147,146,142,136,131,127,125,125,126,129,132,134,136,136,133,129,123,116,109,102,96,91,89,89,91,94,97,100,102,104,106,108,110,113,117,119,121,122,124,125,127,129,132,134,137,139,141,143,145,147,149,150,149,147,141,133,124,116,110,106,104,104,104,105,107,108,110,113,116,120,122,124,126,127,129,131,133,135,137,138,138,138,137,135,132,130,127,125,124,122,121,121,121,123,125,127,128,128,125,122,118,114,111,108,107,108,109,112,116,120,125,129,134,140,146,151,155,156,155,154,152,151,151,153,155,158,161,166,174,184,198,214,230,241,239,222,188,145},             // [Trumpet] [end cycle] [152 samples for note F#3] -1dB start at  (102463 from beginning)
  },
  { // instrument 5 (sax) here:
    {126,136,137,139,144,146,149,150,155,160,158,157,158,156,163,171,171,176,182,185,189,186,181,179,178,181,184,183,178,178,178,181,181,177,172,170,167,165,168,169,164,157,157,158,156,157,150,146,142,141,146,144,137,129,135,137,130,132,130,124,126,125,128,126,119,118,117,116,118,116,109,107,105,109,111,102,99,96,96,104,103,100,94,92,97,103,101,97,100,100,102,103,104,101,97,104,106,106,106,107,107,107,107,107,105,104,107,107,110,110,108,109,113,113,109,111,114,114,114,114,116,117,117,111,107,104,103,102,99,96,88,86,88,90,89,86,84,84,80,81,86,87,84,84,87,90,94,99,104,105,106,114},   // [Sax] [start cycle] [152 samples for note C3] start at 698 (732 from beginning) n
    {126,126,128,133,136,135,142,147,139,138,145,144,144,149,150,150,151,153,151,152,159,161,158,158,159,160,160,162,162,159,160,158,154,158,163,160,160,162,159,158,158,157,157,159,162,160,157,154,152,152,153,155,158,154,150,149,147,146,146,146,146,144,141,140,140,140,136,132,134,135,134,132,131,130,128,126,126,128,126,123,125,124,121,120,118,117,117,116,115,114,115,112,109,111,112,111,111,109,106,105,110,108,103,102,97,96,99,96,93,96,98,93,90,92,89,85,86,86,84,84,86,81,78,82,82,80,84,83,80,83,84,85,89,88,83,83,84,85,90,97,101,102,104,106,107,106,107,109,111,116,123,125},           // [Sax] [start cycle] [152 samples for note F#3] start at 698 (728 from beginning) n
    {128,158,179,190,191,184,190,205,216,216,214,213,208,213,220,221,223,224,227,231,230,227,223,219,217,215,210,210,209,203,200,198,199,194,190,189,185,183,178,175,171,164,159,152,147,146,148,148,148,146,142,142,142,142,137,131,132,135,134,129,124,123,121,120,118,113,104,102,101,98,93,86,84,84,85,88,86,81,77,77,82,85,83,82,84,90,95,98,98,98,103,104,106,111,110,116,118,110,105,102,102,103,103,103,104,109,115,115,118,124,130,137,145,148,150,155,160,160,156,146,133,110,80,54,38,32,32,28,19,11,5,13,27,35,40,44,51,63,74,80,84,83,77,73,66,57,46,38,43,55,74,100},                          // [Sax] [mid cycle] [152 samples for note C3] start at   (1956 from beginning) n
    {124,159,192,207,208,194,173,168,179,200,217,224,220,219,215,208,205,207,212,211,211,213,213,212,206,198,191,191,194,205,222,230,229,226,220,214,210,214,213,203,193,189,190,185,185,186,178,175,179,175,171,170,169,167,163,158,155,153,152,147,140,141,138,129,127,124,121,121,119,114,110,106,104,104,103,99,101,105,106,103,100,98,102,104,104,103,102,105,112,116,115,114,114,117,118,117,111,108,107,104,99,97,99,97,94,93,88,82,78,74,69,65,62,57,50,47,44,39,39,38,31,29,27,23,21,21,18,19,23,22,24,31,36,39,46,53,61,70,76,77,82,89,93,96,103,105,102,95,82,73,80,98},                          // [Sax] [mid cycle] [152 samples for note F#3] start at   (1800 from beginning) n
    {144,156,165,175,184,190,192,195,201,205,204,204,200,197,197,194,198,204,210,210,211,213,213,212,205,199,195,192,189,185,181,180,180,177,174,170,168,165,159,154,150,146,146,144,143,142,145,146,146,143,140,139,141,142,139,133,128,128,128,124,124,126,126,124,124,124,124,123,120,119,117,112,108,104,100,98,96,92,89,88,92,93,93,94,95,102,108,113,110,106,107,108,111,111,110,109,113,115,116,118,117,115,109,105,104,108,109,110,107,108,113,118,124,131,134,138,144,146,145,138,128,113,91,71,55,48,45,40,31,24,26,34,41,48,56,61,68,76,82,87,90,89,83,77,71,66,63,60,61,70,90,119},              // [Sax] [end cycle] [152 samples for note C3] start at   (28554 from beginning) n
    {139,166,184,190,185,168,159,165,183,203,214,213,210,207,199,195,194,199,202,201,201,201,199,193,187,182,179,182,190,202,211,213,211,207,203,197,197,204,201,193,187,185,181,175,175,174,168,168,166,160,158,157,155,155,153,150,147,144,140,133,129,129,127,125,123,123,122,121,119,115,109,107,106,106,104,101,101,100,96,89,87,89,96,101,103,104,105,106,107,108,108,109,112,117,119,117,115,113,109,105,101,100,100,98,95,91,88,84,80,78,78,78,79,77,74,72,69,66,63,59,55,52,50,49,47,45,42,42,43,44,48,51,55,61,67,72,75,80,84,88,93,101,105,107,107,104,98,91,85,84,92,113},                       // [Sax] [end cycle] [152 samples for note F#3] start at   (27782 from beginning) n
  }
};
const PROGMEM uint8_t MsfNoteBand3[][6][107] = // [instrument] [cycle] [samples] // band 3
{
  { // instrument 1 (piano) here:
    {122,105,87,70,64,62,59,59,51,36,23,11,12,25,27,28,43,61,73,85,106,127,141,148,142,137,145,164,181,189,194,198,208,211,204,203,200,192,192,207,220,227,232,222,203,192,189,184,180,183,175,165,155,152,154,144,127,110,94,76,71,86,97,96,94,96,100,102,108,112,111,108,103,88,70,61,58,58,55,54,70,92,105,115,124,129,127,131,139,135,127,130,144,163,178,183,193,200,196,186,164,140,131,137,139,132,131},              // [Piano] [start cycle] [107 samples for note F#3] start at 591 (new)
    {101,85,68,59,61,68,73,71,63,53,44,37,28,18,7,0,0,10,28,48,65,72,73,75,87,110,134,151,160,160,153,144,139,139,140,142,143,149,159,169,172,167,158,150,146,145,146,150,155,161,170,179,186,189,191,192,190,188,188,191,190,183,173,164,156,153,153,155,155,151,144,140,140,146,157,170,181,183,175,161,147,136,130,131,140,155,173,188,195,191,177,159,143,136,138,146,152,154,153,152,154,157,159,155,145,132,122},      // [Piano] [start cycle] [107 samples for note C4] start at 591 (new)
    {122,113,105,97,93,92,93,89,83,77,71,66,63,66,71,77,81,84,85,87,87,88,90,94,98,104,110,118,124,128,131,136,143,149,154,161,167,171,172,172,174,175,173,169,165,162,162,163,164,160,155,147,139,132,129,128,130,133,137,139,139,137,135,131,126,121,117,117,119,123,125,124,122,121,120,121,122,125,125,124,126,129,131,130,129,130,132,133,134,137,143,147,146,143,140,139,140,141,143,147,151,151,151,150,147,139,129}, // [Piano] [mid cycle] [107 samples for note F#3] -6dB start at 20882 (new) (20973 from beginning)
    {119,112,106,99,93,88,83,79,75,72,69,65,63,63,65,68,71,73,74,74,73,72,72,72,73,74,76,78,81,84,88,91,94,96,98,99,100,100,101,103,106,110,115,122,130,138,145,150,154,156,158,159,160,161,161,162,162,162,161,159,156,154,153,153,155,156,158,158,157,156,156,156,158,159,162,164,167,169,168,166,164,161,159,157,155,153,151,149,147,148,150,152,154,156,157,158,159,160,160,159,157,154,149,144,139,134,127},            // [Piano] [mid cycle] [107 samples for note C4] -6dB start at 19309 (new) (19366 from beginning)
    {122,116,111,107,103,100,98,96,93,91,88,87,86,87,89,91,93,94,93,92,91,90,90,91,93,96,99,101,103,104,105,107,109,112,116,119,121,122,122,122,121,120,120,119,119,118,116,114,111,107,103,99,95,93,93,95,98,102,106,110,113,114,115,115,115,116,119,122,127,132,137,143,147,152,156,161,166,171,176,180,183,186,188,189,190,190,190,191,191,190,189,186,182,178,173,169,165,162,159,157,155,152,149,144,139,133,127},      // [Piano] [end cycle] [107 samples for note F#3] -6dB start at ~114073 (114164 from beginning) (LP filter)
    {124,120,116,111,106,101,95,90,86,82,79,76,74,71,69,67,66,65,65,64,63,63,63,64,66,68,69,71,73,76,79,81,84,86,87,88,89,90,91,93,95,97,99,102,106,110,114,119,124,129,133,137,140,142,144,146,147,149,152,154,156,158,159,161,162,163,164,164,164,165,165,167,168,171,173,176,178,180,181,182,182,181,180,179,178,176,175,174,174,173,173,171,170,168,166,165,163,161,158,155,151,147,143,139,136,132,129},                // [Piano] [end cycle] [107 samples for note C4] -6dB start at 49989 (LP filter)
  },
  { // instrument 2 (guitar) here:
    {123,113,104,100,102,106,107,101,88,72,59,51,48,48,50,53,59,67,72,74,70,63,56,52,54,60,66,69,69,69,72,79,89,97,100,99,97,99,103,107,111,117,125,132,136,137,137,138,141,142,142,142,144,152,161,163,157,149,149,157,164,161,151,142,143,152,159,159,153,146,143,144,146,147,151,157,158,147,130,120,129,152,174,180,170,158,156,168,185,197,196,184,169,158,158,170,189,209,219,215,197,173,153,142,139,137,130},         // [Guitar] [start cycle] [107 samples for note F#3] start at 1145 (1341 from beginning of file)
    {122,106,92,82,75,71,68,67,69,74,80,86,88,84,75,64,56,55,61,70,77,80,79,77,77,82,89,96,99,99,96,96,101,112,125,135,137,133,125,120,122,131,144,153,155,150,142,139,142,150,156,156,149,138,129,126,133,145,154,155,144,126,111,106,115,129,141,143,136,125,117,116,122,127,127,121,114,114,127,151,176,192,192,176,155,141,142,156,176,193,204,209,210,211,210,204,190,170,148,132,125,127,135,143,145,141,130},          // [Guitar] [start cycle] [107 samples for note C4] start at 1145 (1455 from beginning of file)
    {123,115,111,111,113,113,112,108,102,96,91,88,87,86,84,80,77,76,77,79,80,79,77,75,73,71,69,68,70,73,78,83,88,92,96,100,105,108,110,112,115,120,126,133,141,151,163,173,181,185,185,184,181,178,175,174,176,181,187,191,191,187,182,174,164,153,140,129,121,118,118,119,121,122,122,119,114,108,103,100,99,101,105,110,118,128,138,147,154,158,160,161,161,160,160,161,162,162,162,160,159,157,156,153,148,139,130},       // [Guitar] [mid cycle] [107 samples for note F#3] start at  (10861 from beginning of file)
    {128,124,121,117,114,111,109,107,103,98,93,88,85,85,88,90,92,92,90,87,86,85,86,86,87,87,88,90,94,98,102,104,105,104,102,101,101,102,104,106,108,110,112,115,119,123,127,129,130,130,128,127,126,127,129,131,134,137,138,139,139,138,136,133,130,126,122,120,119,120,122,124,126,128,131,135,140,143,146,148,149,151,155,161,167,173,178,182,185,188,190,191,189,185,179,173,169,167,167,167,166,162,155,147,140,134,130}, // [Guitar] [mid cycle] [107 samples for note C4] start at  (12042 from beginning of file)
    {126,119,113,107,102,97,93,88,83,79,75,72,70,69,68,66,65,63,63,64,65,67,69,70,71,73,76,79,83,86,89,91,94,98,102,107,111,115,119,123,127,132,136,139,143,146,150,153,156,159,162,164,167,168,169,170,171,172,172,172,171,169,168,165,163,159,157,154,151,148,145,142,139,137,135,133,131,129,127,126,126,127,129,131,134,136,139,143,147,151,155,158,161,163,164,165,166,167,167,165,162,157,153,150,145,141,134},         // [Guitar] [end cycle] [107 samples for note F#3] start at  (39982 from beginning of file)
    {127,121,115,110,106,101,96,90,85,80,75,72,70,68,68,67,66,65,64,63,63,63,64,66,68,69,71,74,76,80,83,86,89,92,95,97,100,102,105,108,111,115,119,124,129,133,136,138,139,140,143,147,151,155,158,160,161,161,161,160,158,156,154,153,152,152,152,150,148,144,139,135,132,131,133,136,139,142,144,146,148,150,153,155,158,162,166,171,176,181,184,186,186,184,182,180,179,178,176,173,169,164,158,152,146,140,133},          // [Guitar] [end cycle] [107 samples for note C4] start at  (55932 from beginning of file)
  },
  { // instrument 3 (marimba) here:
    {122,113,104,96,88,82,77,73,70,67,65,63,61,59,56,53,50,47,44,42,38,34,30,26,21,16,12,9,6,5,5,6,9,11,15,19,24,30,36,43,50,57,65,73,80,86,91,96,100,104,107,111,114,118,122,126,131,136,141,146,152,159,167,175,184,192,200,208,216,222,227,231,234,236,237,238,239,238,237,235,233,231,229,226,224,223,222,222,222,222,222,221,220,218,215,212,207,203,197,191,185,178,170,161,151,142,133},                               // [Marimba] [start cycle] [107 samples for note F#3] start at  (2173 from beginning of file) file a
    {122,113,104,96,88,82,77,73,70,67,65,63,61,59,56,53,50,47,44,42,38,34,30,26,21,16,12,9,6,5,5,6,9,11,15,19,24,30,36,43,50,57,65,73,80,86,91,96,100,104,107,111,114,118,122,126,131,136,141,146,152,159,167,175,184,192,200,208,216,222,227,231,234,236,237,238,239,238,237,235,233,231,229,226,224,223,222,222,222,222,222,221,220,218,215,212,207,203,197,191,185,178,170,161,151,142,133},                               // [Marimba] [start cycle] [107 samples for note F#3 instead of C4] start at 1759 (2173 from beginning of file) file a
    {}, // mid cycle not used
    {}, // mid cycle not used
    {125,121,118,114,110,107,103,100,96,93,90,87,84,81,79,77,74,72,70,69,67,66,65,64,64,63,63,63,63,64,65,66,67,68,70,71,73,75,78,80,83,86,89,92,95,98,101,105,109,112,116,120,123,127,131,134,138,142,145,149,153,156,159,162,165,168,171,174,176,179,181,183,184,186,187,188,189,190,191,191,191,191,190,190,189,188,187,185,184,182,180,177,175,173,170,167,164,161,158,154,151,147,144,140,136,133,129},                  // [Marimba] [end cycle] [107 samples for note F#3] start at  (Sinewave near end of file)
    {125,121,118,114,110,107,103,100,96,93,90,87,84,81,79,77,74,72,70,69,67,66,65,64,64,63,63,63,63,64,65,66,67,68,70,71,73,75,78,80,83,86,89,92,95,98,101,105,109,112,116,120,123,127,131,134,138,142,145,149,153,156,159,162,165,168,171,174,176,179,181,183,184,186,187,188,189,190,191,191,191,191,190,190,189,188,187,185,184,182,180,177,175,173,170,167,164,161,158,154,151,147,144,140,136,133,129},                  // [Marimba] [end cycle] [107 samples for note F#3 instead of C4] start at  (Sinewave near end of file)
  },
  { // instrument 4 (trumpet) here:
    {123,114,108,103,99,97,98,100,103,106,109,111,113,114,115,115,116,116,116,115,115,115,117,119,120,121,122,122,123,122,122,120,119,119,119,120,122,122,122,122,122,122,122,122,121,121,121,121,121,120,119,119,118,118,119,120,122,123,125,127,128,129,129,130,130,131,132,132,132,132,131,129,128,126,124,122,119,117,114,110,108,106,106,105,107,111,115,118,121,126,133,142,152,162,170,179,187,194,200,205,206,203,197,186,173,158,142},    // [Trumpet] [start cycle] [107 samples for note F#3] start at 847 (939 from beginning of file)
    {119,114,110,106,104,104,105,107,110,112,113,115,116,116,117,117,118,118,117,117,117,118,119,121,122,123,123,124,124,123,122,121,121,121,121,122,123,123,123,123,122,122,122,122,122,121,121,121,121,121,120,120,120,121,122,123,124,126,127,128,129,129,129,129,129,129,130,130,130,129,128,127,126,125,124,122,120,118,115,112,111,110,109,109,112,115,118,120,124,129,136,144,152,159,166,173,179,185,190,192,191,187,180,169,157,144,130}, // [Trumpet] [start cycle] [107 samples for note C4] start at 847 (938 from beginning of file)
    {89,59,44,43,54,67,84,104,122,133,138,137,133,129,127,127,127,126,125,123,119,113,108,103,99,95,92,91,93,98,105,112,118,122,124,125,126,128,130,131,129,130,132,135,140,145,150,152,150,144,137,131,127,123,119,117,115,114,115,116,117,117,117,120,124,128,130,130,129,127,126,124,123,120,117,116,115,115,115,114,112,111,111,113,116,119,123,128,134,139,142,146,149,150,152,154,157,159,162,168,178,196,220,244,250,221,161},              // [Trumpet] [mid cycle] [107 samples for note F#3] -0.3dB start at  (4166 from beginning of file)
    {84,59,50,50,59,72,85,103,119,128,130,125,119,117,118,121,124,126,128,127,124,120,117,114,112,110,109,107,106,106,106,107,107,107,107,108,112,116,121,125,130,134,138,141,143,145,145,141,136,130,128,129,129,126,121,117,114,113,114,116,116,118,121,125,128,128,126,124,122,120,119,119,119,119,119,121,123,124,123,121,119,117,117,117,118,120,122,125,128,132,137,142,146,149,152,157,161,165,170,179,192,212,235,254,250,204,137},        // [Trumpet] [mid cycle] [107 samples for note C4] -0dB start at  (5986 from beginning of file)
    {79,42,29,34,51,71,93,115,133,145,147,141,132,127,124,126,130,134,136,135,129,120,110,100,92,89,90,94,98,102,105,107,111,115,119,121,123,125,128,132,135,139,142,145,147,149,149,145,135,122,112,106,104,104,106,108,111,115,120,123,126,128,130,133,136,138,138,137,134,131,127,125,122,121,121,123,127,128,127,122,117,112,108,107,109,114,119,126,132,140,149,154,156,154,152,151,153,156,161,168,181,200,223,241,235,196,135},             // [Trumpet] [end cycle] [107 samples for note F#3] -1dB start at  (72129 from beginning of file)
    {106,58,29,22,32,52,77,103,127,144,152,151,144,137,133,133,138,143,146,145,137,124,111,98,88,81,78,79,83,89,94,97,98,96,94,95,98,106,115,127,138,147,154,158,159,158,156,152,145,138,131,125,120,117,116,116,116,117,118,118,117,118,121,126,133,139,143,146,148,149,147,143,136,127,119,114,112,113,116,118,120,121,120,118,115,112,110,110,112,117,126,137,147,156,162,165,167,169,170,172,177,189,207,228,234,213,164},                     // [Trumpet] [end cycle] [107 samples for note C4] -1.5dB start at  (99223 from beginning of file)
  },
  { // instrument 5 (sax) here:
    {133,133,144,159,162,162,155,152,156,165,173,170,168,167,168,175,178,176,172,172,174,174,173,174,173,175,176,168,165,168,170,169,161,157,157,150,155,151,150,148,145,146,139,143,140,138,134,131,129,129,129,124,124,115,115,121,121,119,117,118,114,112,115,114,112,114,108,105,108,105,99,94,93,89,88,89,85,82,76,73,69,69,72,68,64,64,63,62,65,65,67,68,69,71,72,72,75,80,89,93,95,98,103,113,119,127,129},       // [Sax] [start cycle] [107 samples for note F#3] start at 630 (630 from beginning) n
    {130,133,130,131,137,137,141,142,140,141,143,144,146,146,145,150,150,152,153,151,153,153,156,159,156,155,155,157,159,162,161,159,159,159,160,162,161,159,156,154,155,155,156,156,153,154,156,154,151,147,145,145,144,142,140,135,130,129,129,129,126,118,110,105,101,100,102,99,93,86,80,76,79,84,83,81,80,78,76,74,71,71,78,80,75,76,76,75,79,81,86,90,91,96,100,100,101,103,107,113,116,120,123,122,125,125,126},  // [Sax] [start cycle] [107 samples for note C4] start at 630 (728 from beginning) n
    {119,169,204,207,182,167,187,215,224,219,215,206,207,212,211,213,213,206,194,191,196,217,231,227,221,211,213,212,196,190,187,185,184,175,178,172,170,169,163,157,153,152,142,141,135,127,124,121,119,111,107,104,103,99,103,106,102,99,103,105,103,103,111,116,114,115,118,117,109,107,102,97,98,95,92,86,78,73,65,63,53,48,42,39,36,29,27,22,22,18,22,22,28,36,41,52,63,75,77,86,92,99,105,102,88,73,86},           // [Sax] [mid cycle] [107 samples for note F#3] start at   (1275 from beginning) n
    {129,141,149,148,145,141,137,131,129,126,131,139,142,142,145,151,160,168,172,178,182,184,185,187,188,189,192,191,189,188,189,192,193,196,198,200,202,201,197,194,191,192,195,194,187,178,172,168,168,164,161,157,159,166,175,177,177,178,185,194,196,194,184,166,148,129,104,78,54,42,42,48,55,61,59,48,31,20,18,30,49,59,61,59,57,57,57,56,56,54,51,47,42,36,29,17,9,6,13,27,50,77,98,110,118,119,121},             // [Sax] [mid cycle] [107 samples for note C4] start at   (1694 from beginning) n
    {117,166,203,207,180,160,179,208,218,214,207,197,203,210,209,211,215,207,197,190,192,210,222,221,211,197,194,198,187,182,179,175,177,167,162,157,157,161,152,142,138,138,133,128,120,107,106,108,110,112,108,111,109,105,106,109,105,105,110,111,112,116,127,129,125,122,130,132,126,121,115,111,111,111,105,97,89,82,78,75,68,59,53,49,44,36,31,33,34,30,23,21,27,38,49,56,57,61,67,81,94,101,105,101,88,74,84},    // [Sax] [end cycle] [107 samples for note F#3] start at   (17540 from beginning) n
    {130,134,135,135,142,154,156,156,154,151,147,142,135,134,137,138,140,147,155,165,174,179,181,181,180,185,193,200,206,208,203,197,190,187,186,184,183,184,185,185,182,180,179,176,174,179,181,179,177,175,170,165,159,153,149,149,155,160,160,157,157,162,171,176,180,177,163,146,131,110,89,66,49,44,48,53,61,64,56,42,28,23,31,48,63,68,66,67,66,66,63,61,58,55,52,48,46,43,34,24,19,21,32,55,82,106,123},          // [Sax] [end cycle] [107 samples for note C4] start at   (37437 from beginning) n
  },
  { // instrument 6 (violin) here:
    {125,128,137,149,160,165,163,157,151,147,143,141,142,144,145,143,139,134,131,131,136,143,149,151,151,151,151,149,144,141,142,146,150,152,150,146,140,133,123,112,103,100,102,104,104,103,101,102,106,112,119,122,121,120,122,121,115,107,101,98,101,108,116,125,134,136,130,124,121,122,125,129,133,142,153,161,160,151,140,138,143,148,147,142,136,131,129,126,118,103,85,70,66,70,72,73,76,84,95,104,112,118,126,132,135,133,130}, // [Violin] [start cycle] [107 samples for note C4] start at 1890 (1955 from beginning)
    {125,128,137,149,160,165,163,157,151,147,143,141,142,144,145,143,139,134,131,131,136,143,149,151,151,151,151,149,144,141,142,146,150,152,150,146,140,133,123,112,103,100,102,104,104,103,101,102,106,112,119,122,121,120,122,121,115,107,101,98,101,108,116,125,134,136,130,124,121,122,125,129,133,142,153,161,160,151,140,138,143,148,147,142,136,131,129,126,118,103,85,70,66,70,72,73,76,84,95,104,112,118,126,132,135,133,130}, // [Violin] [start cycle] [107 samples for note C4] start at 1890 (1955 from beginning) same samples used for all notes in band
    {144,152,166,181,196,208,213,207,190,171,154,141,135,137,146,154,151,138,126,124,137,155,167,168,166,169,175,180,178,171,167,166,168,171,173,171,164,153,141,125,105,82,63,52,53,60,65,67,69,78,96,114,126,128,123,117,105,85,70,64,68,80,96,116,137,156,163,157,148,143,139,132,127,128,141,159,172,174,165,154,145,142,141,136,126,117,113,114,109,91,59,25,7,13,35,57,73,81,86,90,95,101,110,121,132,140,143},                    // [Violin] [mid cycle] [107 samples for note C4] -0.5dB start at   (5044 from beginning)
    {144,152,166,181,196,208,213,207,190,171,154,141,135,137,146,154,151,138,126,124,137,155,167,168,166,169,175,180,178,171,167,166,168,171,173,171,164,153,141,125,105,82,63,52,53,60,65,67,69,78,96,114,126,128,123,117,105,85,70,64,68,80,96,116,137,156,163,157,148,143,139,132,127,128,141,159,172,174,165,154,145,142,141,136,126,117,113,114,109,91,59,25,7,13,35,57,73,81,86,90,95,101,110,121,132,140,143},                    // [Violin] [mid cycle] [107 samples for note C4] -0.5dB start at   (5044 from beginning) same samples used for all notes in band
    {141,149,166,186,205,220,229,228,215,198,179,161,146,137,135,139,142,140,132,126,127,134,141,142,139,140,145,152,155,150,143,140,142,147,149,149,149,147,145,139,126,108,90,76,71,71,71,70,71,80,98,122,140,147,144,137,128,111,93,81,78,82,92,106,120,134,144,144,135,127,125,125,126,129,139,156,172,179,172,159,147,141,143,145,143,137,131,127,122,109,85,52,22,7,12,27,43,57,70,82,93,101,110,122,133,141,144},                 // [Violin] [end cycle] [107 samples for note C4] -0.5dB start at   (29965 from beginning)
    {141,149,166,186,205,220,229,228,215,198,179,161,146,137,135,139,142,140,132,126,127,134,141,142,139,140,145,152,155,150,143,140,142,147,149,149,149,147,145,139,126,108,90,76,71,71,71,70,71,80,98,122,140,147,144,137,128,111,93,81,78,82,92,106,120,134,144,144,135,127,125,125,126,129,139,156,172,179,172,159,147,141,143,145,143,137,131,127,122,109,85,52,22,7,12,27,43,57,70,82,93,101,110,122,133,141,144},                 // [Violin] [end cycle] [107 samples for note C4] -0.5dB start at   (29965 from beginning) same samples used for all notes in band
  }
};
const PROGMEM uint8_t MsfNoteBand4[][6][76] = // [instrument] [cycle] [samples] // band 4
{
  { // instrument 1 (piano) here:
    {113,94,87,91,97,99,92,76,58,43,31,24,19,10,1,0,12,32,55,81,104,120,124,122,121,125,136,145,147,145,144,145,152,159,159,155,157,165,171,178,186,190,191,188,187,196,209,211,193,167,153,161,181,194,196,190,182,174,161,150,155,174,184,177,165,160,159,150,133,122,128,144,157,157,144,127},   // [Piano] [start cycle] [76 samples for note C4] start at 728 (new)
    {124,111,90,67,51,44,36,27,18,12,8,7,8,10,21,33,37,31,22,21,26,35,46,57,65,69,70,67,63,60,63,72,83,101,122,141,152,157,165,172,177,176,172,166,166,172,175,176,178,190,208,226,236,238,239,241,239,232,224,217,216,221,233,239,235,226,215,204,191,177,163,155,154,154,149,136},                // [Piano] [start cycle] [76 samples for note F#4] start at 728
    {115,106,97,89,82,76,71,67,63,63,67,71,74,74,73,71,72,73,75,79,83,89,93,96,98,99,100,102,106,112,121,132,142,150,155,158,160,161,162,162,162,161,157,154,153,155,157,158,157,156,157,159,161,165,168,169,165,161,159,156,153,150,148,148,152,155,157,158,159,160,159,155,149,142,135,126},      // [Piano] [mid cycle] [76 samples for note C4] -6dB start at 13724 (new) (13756 from beginning)
    {127,122,116,111,105,100,96,92,90,88,88,88,88,88,87,85,84,83,83,83,83,83,81,79,77,76,77,79,83,87,90,94,97,101,104,108,112,116,121,125,129,132,137,143,150,157,163,168,173,178,183,187,190,191,191,189,187,185,181,177,172,169,166,163,161,159,158,158,158,157,155,152,148,143,138,133},         // [Piano] [mid cycle] [76 samples for note F#4] -6dB start at 14852
    {125,120,114,107,99,91,85,80,76,73,70,67,66,65,64,63,64,65,68,70,73,77,80,84,86,88,89,91,93,96,99,104,109,115,122,129,135,139,143,145,147,151,154,157,159,161,163,164,164,164,165,167,170,174,177,180,181,182,181,179,177,175,174,173,173,171,169,166,164,161,157,152,147,141,136,131},         // [Piano] [end cycle] [76 samples for note C4] start at 35474 (LP filter)
    {125,121,117,114,111,108,105,101,96,91,85,80,76,75,74,73,71,67,65,63,64,67,71,76,82,89,97,105,113,119,123,126,128,130,131,132,134,136,138,140,142,145,148,154,161,167,173,176,179,180,182,183,183,181,178,174,168,162,156,152,150,150,151,151,150,148,145,143,142,142,141,140,138,136,132,128}, // [Piano] [end cycle] [76 samples for note F#4] -6dB start at 69612
  },
  { // instrument 2 (guitar) here:
    {119,103,90,78,69,69,79,88,88,79,70,65,68,71,71,70,74,84,92,95,94,96,104,115,122,122,120,124,134,144,146,140,136,140,149,152,143,133,133,145,154,148,129,115,119,132,140,133,123,123,130,130,117,105,112,141,171,182,169,152,148,159,174,187,199,210,214,206,187,163,143,135,140,146,144,134},        // [Guitar] [start cycle] [76 samples for note C4] start at 1038 (1185 from beginning of file)
    {81,49,41,59,96,140,173,184,170,141,110,89,87,104,129,152,161,150,125,95,73,65,74,92,110,119,119,110,100,95,96,102,109,113,112,106,101,98,99,104,108,110,110,109,111,116,123,128,129,126,122,118,117,122,130,138,144,144,141,141,149,168,192,214,224,218,199,176,157,150,156,170,183,183,165,129},    // [Guitar] [start cycle] [76 samples for note F#4] start at 1038 (1496 from beginning of file)
    {124,121,115,108,100,91,81,75,76,84,93,95,90,82,76,75,78,83,89,97,105,111,111,107,103,103,107,113,119,124,128,131,133,132,130,126,125,128,134,141,144,143,139,135,131,128,126,125,125,125,125,126,129,135,141,145,149,154,162,171,178,182,185,189,191,188,182,176,173,172,168,158,146,135,130,129},   // [Guitar] [mid cycle] [76 samples for note C4] -6dB start at  (7870 from beginning of file)
    {119,97,83,78,81,92,104,116,122,123,121,120,121,125,132,138,141,138,131,121,111,103,97,95,95,96,96,96,97,99,103,109,115,120,125,127,127,124,122,119,119,120,122,126,127,127,124,119,114,110,110,113,120,126,129,128,124,120,118,124,136,154,172,185,191,189,182,173,168,168,174,180,183,179,165,144}, // [Guitar] [mid cycle] [76 samples for note F#4] -6dB start at  (7502 from beginning of file)
    {121,117,113,110,105,97,88,81,76,74,74,75,76,77,77,77,78,81,85,90,96,103,108,112,115,116,118,120,122,125,127,128,128,126,124,124,127,130,132,132,130,128,126,125,126,128,130,131,129,125,123,124,129,136,143,148,152,156,161,167,173,181,187,191,191,187,182,179,178,177,173,167,158,148,137,128},    // [Guitar] [end cycle] [76 samples for note C4] -6dB start at  (34330 from beginning of file)
    {126,124,125,127,129,130,131,130,128,126,124,123,124,125,125,124,120,115,109,104,101,100,101,101,99,96,91,87,84,84,87,91,95,97,98,98,98,99,102,105,108,110,110,109,109,110,113,116,119,122,125,129,134,141,150,158,166,172,176,180,183,187,190,191,188,182,174,166,158,154,150,147,144,139,133,128},  // [Guitar] [end cycle] [76 samples for note F#4] -6dB start at  (50905 from beginning of file)
 //   {126,124,123,122,121,118,115,111,107,104,101,100,98,97,95,92,89,85,83,81,81,84,87,91,96,100,103,106,109,112,116,121,126,130,133,134,134,134,134,136,139,144,150,156,162,167,171,175,179,183,187,190,191,189,185,178,168,157,146,137,130,125,122,119,116,113,111,110,111,114,118,123,126,128,128,128},  // [Guitar] [end cycle] [76 samples for note F#4] -6dB start at  new (48026 from beginning of file)
  },
  { // instrument 3 (marimba) here:
    {120,107,96,86,78,72,67,63,59,55,50,45,41,37,33,28,22,15,9,5,2,2,4,7,12,19,27,36,47,58,70,80,89,96,101,107,111,116,122,128,134,141,150,160,172,184,197,210,221,229,235,240,242,243,243,241,239,235,232,228,226,225,225,225,224,222,218,214,208,201,193,184,172,159,146,132},                          // [Marimba] [start cycle] [76 samples for note C4] start at 1478 (1481 from beginning of file)
    {120,107,96,86,78,72,67,63,59,55,50,45,41,37,33,28,22,15,9,5,2,2,4,7,12,19,27,36,47,58,70,80,89,96,101,107,111,116,122,128,134,141,150,160,172,184,197,210,221,229,235,240,242,243,243,241,239,235,232,228,226,225,225,225,224,222,218,214,208,201,193,184,172,159,146,132},                          // [Marimba] [start cycle] [76 samples for note C4 instead of F#4] start at 1478 (1481 from beginning of file)
    {}, // mid cycle not used
    {}, // mid cycle not used
    {122,117,111,106,101,97,92,88,84,80,77,74,71,69,67,65,64,63,63,63,64,65,67,68,71,73,77,80,84,88,92,97,101,106,111,116,122,127,132,137,143,148,153,157,162,166,170,174,177,180,183,185,187,189,190,191,191,191,190,189,187,186,183,181,177,174,170,166,162,157,153,148,143,138,132,127},               // [Marimba] [end cycle] [107 samples for note C4] start at  (Sinewave near end of file)
    {122,117,111,106,101,97,92,88,84,80,77,74,71,69,67,65,64,63,63,63,64,65,67,68,71,73,77,80,84,88,92,97,101,106,111,116,122,127,132,137,143,148,153,157,162,166,170,174,177,180,183,185,187,189,190,191,191,191,190,189,187,186,183,181,177,174,170,166,162,157,153,148,143,138,132,127},               // [Marimba] [end cycle] [107 samples for note C4 instead of F#4] start at  (Sinewave near end of file)
  },
  { // instrument 4 (trumpet) here:
    {122,114,108,104,104,107,110,113,115,116,117,117,118,117,117,118,120,121,123,124,124,123,121,121,121,122,123,123,123,122,122,122,121,121,121,121,120,120,121,122,124,126,128,129,129,129,129,130,130,130,128,127,125,123,121,118,114,111,110,109,112,117,120,125,134,145,156,166,176,184,191,192,187,176,159,141}, // [Trumpet] [start cycle] [76 samples for note C4] start at  (666 from beginning of file)
    {114,102,95,92,93,95,100,105,111,117,120,122,123,124,125,126,127,128,127,127,125,123,122,121,120,119,118,118,118,118,118,119,120,120,122,122,122,123,123,123,122,122,122,121,122,122,122,122,123,122,121,121,120,120,121,123,125,128,131,134,139,143,146,148,149,151,152,154,157,162,169,181,194,201,189,151},     // [Trumpet] [start cycle] [76 samples for note F#4] start at  (695 from beginning of file)
    {76,52,50,65,83,108,126,130,122,117,119,123,127,127,124,119,114,112,109,107,106,106,107,107,107,110,116,122,128,135,139,142,145,143,135,129,129,128,123,116,113,114,116,118,122,127,128,125,122,120,119,119,119,120,123,123,121,118,117,117,120,123,126,132,139,145,149,155,161,167,176,196,226,254,235,148},      // [Trumpet] [mid cycle] [76 samples for note C4] 0dB start at  (4252 from beginning of file)
    {74,54,44,41,58,71,81,101,114,118,123,125,124,125,126,129,130,131,133,133,129,124,119,117,116,114,115,116,116,118,122,125,125,122,122,125,127,123,115,111,110,111,113,113,112,115,124,131,131,131,131,127,124,122,122,122,122,124,128,132,135,133,135,139,147,154,162,169,175,184,194,208,234,254,226,139},        // [Trumpet] [mid cycle] [76 samples for note F#4] 0dB start at  (3965 from beginning of file)
    {75,28,23,46,81,118,145,153,146,136,133,138,146,145,133,115,97,83,77,79,87,94,97,96,94,97,107,122,138,151,158,160,158,152,142,132,123,118,116,115,117,118,117,118,123,132,141,146,149,148,143,132,120,113,112,115,119,121,120,116,112,109,111,119,133,149,160,166,169,170,173,183,208,234,222,155},                // [Trumpet] [end cycle] [76 samples for note C4] -1.5dB start at  (70478 from beginning of file)
    {109,61,34,32,47,66,84,100,111,117,119,117,114,112,113,117,124,132,139,144,144,140,134,129,124,119,116,114,111,110,112,117,122,125,127,131,135,138,138,136,132,126,121,115,107,99,94,91,93,100,112,124,135,142,144,143,139,136,133,132,131,129,127,127,131,140,151,163,177,190,202,214,229,234,210,155},           // [Trumpet] [end cycle] [76 samples for note F#4] -1.5dB start at  (90645 from beginning of file)
  },
  { // instrument 5 (sax) here:
    {130,132,131,137,141,143,141,144,147,148,148,152,155,156,155,156,161,160,158,159,163,166,163,162,163,166,164,160,158,158,160,157,157,158,154,149,147,146,143,137,130,129,129,123,110,101,97,99,93,83,74,73,80,77,76,72,70,64,70,75,70,71,71,75,82,87,92,98,98,101,108,115,121,122,125,125},                       // [Sax] [start cycle] [76 samples for note C4] start at 501 (517 from beginning)
    {131,132,137,138,138,139,142,146,150,152,152,154,155,153,152,153,154,156,155,155,157,156,154,152,150,151,151,148,148,144,140,138,137,135,133,133,129,127,126,124,120,116,116,112,109,108,111,108,107,108,105,103,100,100,101,102,102,99,101,101,102,101,101,104,109,111,109,108,107,109,112,112,113,116,120,125}, // [Sax] [start cycle] [76 samples for note F#4] start at 501 (513 from beginning)
    {120,122,125,141,148,141,137,131,125,125,133,136,143,152,161,168,173,176,181,182,185,187,186,185,186,188,193,196,196,195,193,190,190,188,180,170,166,164,158,161,172,178,178,182,195,197,189,168,143,111,73,48,44,53,63,62,42,24,24,47,62,64,62,60,58,55,52,50,41,33,16,9,21,43,82,110},                          // [Sax] [mid cycle] [76 samples for note C4] start at   (1049 from beginning)
    {136,168,184,181,163,142,137,154,184,215,230,225,205,194,199,204,213,217,210,197,185,173,168,170,175,179,182,182,175,160,154,155,155,154,150,138,128,118,106,97,92,87,85,83,73,64,59,56,52,49,52,55,58,62,64,67,72,78,83,94,100,101,101,100,101,102,104,106,99,86,75,62,53,58,74,100},                            // [Sax] [mid cycle] [76 samples for note F#4] start at   (1516 from beginning)
    {112,116,121,137,145,146,149,149,146,146,149,149,153,161,168,168,167,168,172,181,194,205,208,206,200,196,198,199,194,188,183,181,185,185,178,167,160,152,143,139,149,156,155,157,166,172,173,158,138,113,80,56,53,61,69,65,42,21,18,39,56,62,63,69,72,72,73,70,63,52,34,20,22,42,70,96},                          // [Sax] [end cycle] [76 samples for note C4] start at   (28636 from beginning)
    {151,180,188,175,149,132,139,166,202,227,232,222,205,207,217,225,232,231,221,206,193,180,176,177,178,179,178,174,160,147,146,146,148,146,136,123,115,105,99,99,98,94,94,89,78,69,60,51,43,42,47,49,51,51,47,45,50,56,67,80,88,92,94,98,103,105,110,112,105,96,84,67,59,64,85,119},                                // [Sax] [end cycle] [76 samples for note F#4] start at   (50047 from beginning)
  },
  { // instrument 6 (violin) here:
    {125,136,153,164,162,154,147,142,142,145,144,139,132,131,137,147,151,151,151,148,142,143,148,152,149,142,132,117,103,100,103,104,102,102,108,118,122,120,122,119,108,100,99,108,120,133,134,125,121,123,129,136,150,161,156,140,139,148,147,138,131,129,120,99,74,66,71,73,78,91,105,115,125,133,134,129},        // [Violin] [start cycle] [76 samples for note C4] start at 1343 (1389 from beginning)
    {147,165,178,183,179,170,162,156,149,137,130,130,133,139,146,155,161,163,164,160,154,146,138,130,124,123,127,134,138,139,138,139,137,131,124,118,111,102,97,95,98,106,115,122,131,139,137,128,117,112,110,110,106,99,93,88,86,88,91,98,107,117,123,126,127,120,114,116,118,114,108,106,107,108,112,123},          // [Violin] [start cycle] [76 samples for note F#4] start at 1343 (1405 from beginning)
    {146,163,185,204,213,201,174,150,136,137,151,152,134,123,139,163,168,166,173,180,175,167,167,171,173,168,154,135,109,77,54,52,63,66,70,89,115,128,124,114,91,68,65,78,103,131,158,161,149,142,134,126,134,159,175,168,151,143,141,132,118,113,111,88,41,7,19,51,74,84,90,96,108,123,137,143},                     // [Violin] [mid cycle] [76 samples for note C4] -0.5dB start at   (3583 from beginning)
    {134,169,208,230,229,221,215,208,190,170,155,150,158,175,194,204,203,202,203,201,198,194,184,161,138,124,120,125,131,133,136,141,141,134,123,106,93,84,77,71,74,86,102,117,131,137,131,121,109,98,89,80,67,50,29,20,26,37,44,48,60,80,103,123,133,122,114,115,115,104,92,85,86,87,93,109},                        // [Violin] [mid cycle] [76 samples for note F#4] -1.5dB start at   (4370 from beginning)
    {141,156,184,209,227,227,207,181,156,139,135,141,140,129,126,136,142,139,141,151,154,146,140,144,149,149,148,145,134,110,85,72,71,70,70,84,116,142,146,138,122,97,79,79,92,111,131,145,139,127,125,126,131,150,173,177,161,144,142,145,140,131,126,113,80,34,7,17,39,59,77,93,104,119,135,143},                   // [Violin] [end cycle] [76 samples for note C4] -0.5dB start at   (21284 from beginning)
    {137,177,216,234,232,225,220,211,189,167,156,157,167,181,189,188,187,191,196,196,193,186,170,145,126,121,126,134,142,153,166,174,170,158,141,121,102,87,76,73,77,88,105,120,131,133,124,109,94,81,71,64,55,40,28,29,38,46,49,56,70,89,106,113,106,96,95,101,101,95,90,89,87,86,95,116},                           // [Violin] [end cycle] [76 samples for note F#4] -1.5dB start at   (41392 from beginning)
  }
};
const PROGMEM uint8_t MsfNoteBand5[][6][54] = // [instrument] [cycle] [samples] // band 5
{
  { // instrument 1 (piano) here:
    {119,93,62,45,35,22,12,7,8,12,28,37,27,21,27,41,57,67,70,66,61,64,76,98,127,149,158,168,176,175,167,167,175,176,181,202,227,237,239,241,234,222,216,221,237,237,223,209,191,171,156,155,152,135},              // [Piano] [start cycle] [54 samples for note F#4] start at 674 (n) or 1060 (e)
    {113,94,74,53,34,21,16,17,19,24,35,48,58,61,61,63,71,84,98,110,117,120,119,118,119,122,130,150,182,214,230,229,214,198,190,188,185,174,166,165,171,181,191,196,195,192,192,190,184,175,165,155,143,130},       // [Piano] [start cycle] [54 samples for note C5] start at 1060 (qhf quiet hammer & LP filtered)
    {125,118,114,111,110,108,105,103,102,100,96,90,84,80,78,77,80,85,88,89,90,92,93,94,95,96,98,101,105,108,111,118,129,142,155,166,174,179,183,186,187,188,190,191,190,187,181,176,171,164,157,148,139,130},      // [Piano] [mid cycle] [54 samples for note F#4] -6dB start at 7418
    {126,117,109,102,95,86,79,76,78,84,89,93,95,96,96,98,102,108,115,121,125,128,129,129,129,129,129,130,132,135,139,142,145,146,147,149,153,160,166,170,170,169,167,166,165,166,165,161,157,154,151,145,138,130}, // [Piano] [mid cycle] [54 samples for note C5] -8dB start at 6619 (6897 from beginning) (qhf quiet hammer & LP filtered)
    {127,124,118,112,106,101,95,89,84,79,76,75,74,74,73,73,73,74,75,76,79,84,89,95,101,109,118,128,137,144,151,158,163,168,173,177,180,182,183,186,189,191,189,185,180,175,169,162,154,146,139,134,132,130},       // [Piano] [end cycle] [54 samples for note F#4] -6dB start at 35530
    {126,120,114,109,105,101,98,96,94,91,89,88,86,85,84,85,85,86,87,87,87,88,90,92,97,103,110,119,127,135,143,149,155,161,166,170,173,176,177,178,178,178,177,177,176,174,172,168,164,158,151,144,137,130},        // [Piano] [end cycle] [54 samples for note C5] -8dB start at 50863 (51141 from beginning) extra LP filtering
  },
  { // instrument 2 (guitar) here:
    {77,42,57,110,166,183,156,111,86,98,134,159,150,113,76,66,85,110,120,112,98,95,103,112,112,105,98,100,106,110,109,111,117,126,129,125,118,117,126,138,144,142,142,160,193,220,220,194,162,150,162,181,180,141},    // [Guitar] [start cycle] [54 samples for note F#4] start at 709 (1063 from beginning of file)
    {112,67,32,22,48,102,161,193,184,145,99,76,89,129,176,204,195,154,102,65,62,86,118,138,137,116,90,75,78,96,117,130,129,118,107,103,109,123,138,149,154,157,165,178,190,196,191,177,163,154,155,160,160,146},       // [Guitar] [start cycle] [54 samples for note C5] start at 709 (1063 from beginning of file)
    {98,80,80,94,112,122,123,120,122,130,139,140,132,118,105,97,95,96,96,97,100,106,115,122,126,126,123,119,119,122,126,127,124,117,110,110,117,126,129,125,119,121,138,163,184,191,184,172,167,173,182,180,161,130},  // [Guitar] [mid cycle] [54 samples for note F#4] -6dB start at  (5331 from beginning of file)
    {114,109,113,124,136,141,139,132,126,126,131,139,145,144,136,122,108,97,92,92,96,100,104,105,105,104,104,103,103,103,104,103,101,97,93,90,91,98,108,122,135,147,157,164,168,173,179,186,191,191,184,169,150,131},  // [Guitar] [mid cycle] [54 samples for note C5] -6dB start at  (16561 from beginning of file)
    {125,124,127,130,131,129,126,124,123,124,125,121,114,106,101,100,101,99,93,86,83,86,92,96,98,97,98,102,107,110,109,109,110,115,119,123,128,136,147,160,170,176,181,187,191,190,183,171,160,153,149,144,136,128},   // [Guitar] [end cycle] [54 samples for note F#4] -6dB start at  (36169 from beginning of file)
    {121,116,114,113,113,112,110,109,110,112,115,117,117,113,106,100,94,91,91,93,95,97,99,99,101,103,106,109,111,113,113,113,113,114,117,123,130,138,147,155,162,170,177,183,188,191,191,188,182,173,162,150,139,128}, // [Guitar] [end cycle] [54 samples for note C5] -6dB start at  (49441 from beginning of file)
  },
  { // instrument 3 (marimba) here:
    {120,104,89,77,68,61,54,47,39,32,25,18,10,2,0,0,2,9,18,30,44,59,73,85,95,104,112,122,132,144,157,173,190,206,221,233,240,244,245,245,243,239,236,233,232,231,228,222,214,203,191,176,158,139},                     // [Marimba] [start cycle] [54 samples for note C4 instead of F#4] start at 1050 (1058 from beginning of file)
    {120,104,89,77,68,61,54,47,39,32,25,18,10,2,0,0,2,9,18,30,44,59,73,85,95,104,112,122,132,144,157,173,190,206,221,233,240,244,245,245,243,239,236,233,232,231,228,222,214,203,191,176,158,139},                     // [Marimba] [start cycle] [54 samples for note C4] start at 1050 (1058 from beginning of file)
    {}, // mid cycle not used
    {}, // mid cycle not used
    {120,113,105,99,92,86,81,76,72,68,66,64,63,63,64,66,68,72,76,80,86,92,98,105,112,119,127,134,141,149,155,162,168,173,178,182,186,188,190,191,191,190,188,186,182,178,174,168,162,156,149,142,135,127},             // [Marimba] [end cycle] [107 samples for note C4 instead of F#4] start at  (Sinewave near end of file)
    {120,113,105,99,92,86,81,76,72,68,66,64,63,63,64,66,68,72,76,80,86,92,98,105,112,119,127,134,141,149,155,162,168,173,178,182,186,188,190,191,191,190,188,186,182,178,174,168,162,156,149,142,135,127},             // [Marimba] [end cycle] [107 samples for note C4] start at  (Sinewave near end of file)
  },
  { // instrument 4 (trumpet) here:
    {110,97,92,94,99,107,115,120,123,124,125,127,128,127,125,123,121,119,118,118,118,118,119,120,122,122,123,123,122,122,122,122,122,122,122,121,121,120,121,124,128,132,138,144,147,149,151,154,159,166,182,199,193,138}, // [Trumpet] [start cycle] [54 samples for note F#4] start at  (494 from beginning of file)
    {110,97,92,94,99,107,115,120,123,124,125,127,128,127,125,123,121,119,118,118,118,118,119,120,122,122,123,123,122,122,122,122,122,122,122,121,121,120,121,124,128,132,138,144,147,149,151,154,159,166,182,199,193,138}, // [Trumpet] [start cycle] [54 samples for note C5]  (same as F#4 above)
    {70,48,41,64,78,104,117,123,125,124,127,130,131,133,131,122,118,115,115,116,117,120,126,123,122,126,124,114,111,111,113,112,117,128,132,131,129,123,122,121,123,127,134,134,135,142,153,163,174,184,201,230,254,164},  // [Trumpet] [mid cycle] [54 samples for note F#4] 0dB start at  (2817 from beginning of file)
    {64,41,40,62,79,102,116,126,132,130,130,130,131,132,129,120,115,113,113,115,116,121,128,128,127,132,131,122,120,122,128,126,121,120,118,117,117,115,117,120,121,121,127,132,138,148,159,168,176,185,200,227,247,163},  // [Trumpet] [mid cycle] [54 samples for note C5] -0.5dB start at  (4129 from beginning of file)
    {82,33,33,58,85,106,114,113,110,109,115,127,140,148,146,136,126,119,115,111,108,113,122,126,129,135,140,138,132,124,116,106,96,91,95,109,126,139,143,140,135,132,130,127,125,129,142,161,182,201,218,238,226,149},     // [Trumpet] [end cycle] [54 samples for note F#4] -1.2dB start at  (64351 from beginning of file)
    {94,49,33,39,62,90,114,128,132,130,128,127,129,133,135,133,128,122,117,118,125,132,136,138,141,139,132,122,114,108,106,105,107,109,112,112,106,96,88,88,99,117,134,148,160,169,176,179,183,194,221,241,214,154},       // [Trumpet] [end cycle] [54 samples for note C5] -1dB start at  (64808 from beginning of file)
  },
  { // instrument 5 (sax) here:
    {131,136,139,146,154,153,151,152,156,158,159,162,162,160,158,157,154,152,152,150,145,137,134,132,125,124,119,114,110,104,104,100,101,96,90,90,91,91,91,89,91,95,95,98,99,102,105,108,109,111,113,111,117,125},              // [Sax] [start cycle] [76 samples for note F#4] start at 417 (421 from beginning)
    {130,131,130,132,132,133,136,137,138,140,141,137,137,140,139,139,139,137,137,137,137,137,136,134,134,132,128,127,126,124,121,118,118,116,113,113,111,109,111,110,108,111,113,113,111,113,117,119,120,120,122,123,124,128},  // [Sax] [start cycle] [76 samples for note C5] start at 417 (423 from beginning)
    {56,86,131,175,187,161,135,151,197,230,221,198,198,205,212,206,189,170,167,175,179,184,175,158,153,154,151,135,122,106,94,89,85,78,65,58,50,50,58,61,63,67,73,81,90,100,99,100,104,107,105,89,72,56},                       // [Sax] [mid cycle] [76 samples for note F#4] start at   (1021 from beginning)
    {75,122,159,170,166,151,143,146,168,201,210,203,187,168,148,133,128,133,141,151,155,149,144,147,155,145,135,132,121,114,115,104,85,66,61,59,53,49,42,35,45,57,67,76,95,132,170,205,224,206,166,122,69,50},                  // [Sax] [mid cycle] [76 samples for note C5] start at   (1187 from beginning)
    {69,99,143,185,196,170,143,152,191,225,223,202,203,211,218,216,206,192,183,182,183,185,179,159,149,150,149,133,116,100,90,86,81,71,55,53,51,50,55,59,59,55,55,63,78,93,92,91,96,97,99,89,79,67},                            // [Sax] [end cycle] [76 samples for note F#4] start at   (16911 from beginning)
    {50,93,140,156,156,146,131,127,135,162,183,187,178,159,137,117,112,120,135,153,167,166,159,155,163,161,150,151,151,143,142,135,117,95,79,76,70,68,69,64,57,59,58,61,71,100,141,185,218,219,184,136,85,45},                  // [Sax] [end cycle] [76 samples for note C5] start at   (29859 from beginning)
  },
  { // instrument 6 (violin) here:
    {133,159,180,184,173,162,153,138,130,134,139,149,160,168,164,153,142,128,117,122,132,136,140,141,129,119,110,99,96,99,110,121,138,141,128,116,110,105,96,89,84,84,89,99,111,119,123,115,115,118,109,106,109,113},          // [Violin] [start cycle] [54 samples for note F#4] start at 988 (1052 from beginning)
    {130,146,155,155,150,144,145,150,151,141,132,125,121,126,132,139,139,140,150,161,166,164,151,130,120,118,125,143,148,142,142,138,117,95,83,79,83,95,105,110,113,113,109,116,130,130,125,116,102,90,89,99,104,111},         // [Violin] [start cycle] [54 samples for note C5] start at 988 (1165 from beginning)
    {136,187,228,226,215,204,177,154,152,171,196,205,204,205,200,195,173,139,122,123,130,133,138,135,122,102,88,78,73,85,105,127,135,126,108,95,82,65,37,20,30,42,49,71,103,129,126,116,120,108,91,88,91,105},                 // [Violin] [mid cycle] [54 samples for note F#4] -1.5dB start at   (3267 from beginning)
    {138,175,188,188,185,193,206,210,204,184,157,133,106,104,110,107,110,134,176,216,232,207,164,126,116,131,157,170,176,174,150,116,75,35,13,14,22,36,55,77,95,110,127,144,155,156,130,90,60,51,57,65,73,99},                 // [Violin] [mid cycle] [54 samples for note C5] -1.0dB start at   (3976 from beginning)
    {144,188,223,219,199,186,158,131,125,141,171,188,196,206,209,206,188,159,149,165,178,180,180,167,135,100,78,65,58,65,91,128,150,147,132,115,97,78,51,27,24,23,20,37,74,108,109,95,94,92,87,97,109,121},                    // [Violin] [end cycle] [54 samples for note F#4] -1.5dB start at   (14714 from beginning)
    {147,194,218,212,185,166,170,185,189,177,159,146,132,130,145,150,149,154,174,195,205,190,151,103,74,78,107,142,165,178,169,133,83,38,13,17,38,61,83,97,99,95,96,107,119,127,125,111,88,72,67,71,79,101},                   // [Violin] [end cycle] [54 samples for note C5] -1.0dB start at   (20661 from beginning)
  }
};
const PROGMEM uint8_t MsfNoteBand6[][6][38] = // [instrument] [cycle] [samples] // band 6
{
  { // instrument 1 (piano) here:
    {101,73,44,22,16,18,26,43,58,61,62,74,94,111,119,119,118,121,133,169,215,232,216,194,189,183,169,165,175,190,196,193,192,188,175,161,146,127},        // [Piano] [start cycle] [38 samples for note C5] start at 1314 (1322 from beginning) (qhf)
    {114,104,90,73,57,46,44,45,46,45,41,39,47,59,75,96,114,130,143,154,165,175,178,181,187,194,198,201,207,213,219,221,220,213,195,171,148,133},          // [Piano] [start cycle] [38 samples for note F#5] start at 1314 (1412 from beginning) (n1)
    {120,107,102,100,93,84,78,76,79,87,96,102,104,108,116,122,123,124,124,125,128,133,138,145,150,156,165,170,168,163,162,166,169,168,167,162,150,133},   // [Piano] [mid cycle] [38 samples for note C5] -8dB start at  (5613 from beginning) (qhf)
    {126,119,113,108,102,97,93,90,88,87,87,89,94,99,104,109,114,119,125,130,133,137,142,147,152,155,157,159,160,161,161,161,160,155,150,144,138,132},     // [Piano] [mid cycle] [38 samples for note F#5] -10dB start at  (4537 from beginning) (n1)
    {123,117,111,107,102,97,90,84,79,76,77,79,85,92,99,106,112,118,124,130,138,145,151,155,156,157,159,163,167,170,171,170,168,164,158,151,142,134},      // [Piano] [end cycle] [38 samples for note C5] -8dB start at  (29413 from beginning) (qhff)
    {118,112,106,101,98,95,94,93,92,91,92,95,98,103,108,113,118,124,129,134,140,145,151,155,160,164,166,167,167,165,162,159,155,151,147,142,135,128},     // [Piano] [end cycle] [38 samples for note F#5] -10dB start at  (19979 from beginning) (n1)
  },
  { // instrument 2 (guitar) here:
    {103,43,23,71,155,195,157,94,79,129,191,200,141,74,64,103,138,131,96,74,88,118,132,120,105,106,125,145,154,160,175,193,194,176,157,155,161,151},     // [Guitar] [start cycle] [38 samples for note C5] start at 499 (789 from beginning)
    {122,92,56,34,39,75,129,174,188,168,132,102,96,111,137,152,145,117,85,67,72,95,122,141,147,144,141,149,166,185,196,191,174,153,137,132,134,135},     // [Guitar] [start cycle] [38 samples for note F#5] start at 499 (931 from beginning)
    {116,109,120,136,141,133,125,129,141,145,135,115,98,92,95,101,105,105,104,103,103,104,103,99,93,90,96,111,130,148,160,168,174,184,191,188,169,142},  // [Guitar] [mid cycle] [38 samples for note C5] -6dB start at  (11654 from beginning)
    {118,109,108,111,114,112,107,102,101,106,115,122,123,117,106,95,85,81,81,84,90,96,102,107,114,124,136,149,163,174,183,189,191,189,183,171,154,135},  // [Guitar] [mid cycle] [38 samples for note F#5] -6dB start at  (12033 from beginning)
    {122,115,113,113,111,109,111,115,117,114,106,96,91,91,95,98,99,101,104,108,112,113,113,114,117,126,137,149,160,170,180,187,191,189,181,166,150,133}, // [Guitar] [end cycle] [38 samples for note C5] -6dB start at  (34792 from beginning)
    {126,111,99,91,87,88,92,99,105,109,112,112,112,112,113,112,109,105,100,98,99,103,111,121,130,139,146,154,163,172,181,188,191,189,181,170,157,141},   // [Guitar] [end cycle] [38 samples for note F#5] -6dB start at  (38058 from beginning)
  },
  { // instrument 3 (marimba) here:
    {118,103,88,75,65,55,43,30,21,15,13,15,25,40,56,69,83,97,108,120,136,157,178,196,212,224,228,227,226,226,225,222,219,212,199,180,159,140},           // [Marimba] [start cycle] [38 samples for note F#5 instead of C5] start at 1006 (1034 from beginning of file)
    {118,103,88,75,65,55,43,30,21,15,13,15,25,40,56,69,83,97,108,120,136,157,178,196,212,224,228,227,226,226,225,222,219,212,199,180,159,140},           // [Marimba] [start cycle] [38 samples for note F#5] start at 1006 (1034 from beginning of file)
    {}, // mid cycle not used
    {}, // mid cycle not used
    {116,106,96,87,80,73,68,65,63,63,65,69,74,80,88,97,107,117,127,138,148,158,167,174,181,186,189,191,191,189,185,180,174,166,157,147,137,127},               // [Marimba] [end cycle] [38 samples for note F#5 instead of C5] start at  (Sinewave near end of file)
    {116,106,96,87,80,73,68,65,63,63,65,69,74,80,88,97,107,117,127,138,148,158,167,174,181,186,189,191,191,189,185,180,174,166,157,147,137,127},               // [Marimba] [end cycle] [38 samples for note F#5] start at  (Sinewave near end of file)
  },
  { // instrument 4 (trumpet) here:
    {116,98,93,97,108,118,122,125,127,128,125,122,118,117,117,117,118,121,122,122,121,121,121,122,122,120,119,120,126,132,141,148,151,154,160,174,198,186}, // [Trumpet] [start cycle] [38 samples for note C5] start at  (357 from beginning of file)
    {116,98,93,97,108,118,122,125,127,128,125,122,118,117,117,117,118,121,122,122,121,121,121,122,122,120,119,120,126,132,141,148,151,154,160,174,198,186}, // [Trumpet] [start cycle] [38 samples for note F#5]   (same as C5 above)
    {49,44,62,96,116,130,130,131,129,131,122,114,113,115,117,125,126,128,133,121,122,128,124,119,119,117,118,118,120,124,136,140,156,165,181,194,247,179},  // [Trumpet] [mid cycle] [38 samples for note C5] -0.5dB start at  (2677 from beginning of file)
    {52,38,53,86,106,119,120,127,137,144,134,123,115,113,114,123,125,128,133,123,122,128,128,121,117,113,113,115,121,125,136,143,161,171,186,198,247,180},  // [Trumpet] [mid cycle] [38 samples for note F#5] -0.5dB start at  (3298 from beginning of file)
    {91,37,36,70,108,129,131,128,127,133,135,130,121,117,124,134,138,141,136,122,110,105,105,108,112,110,95,86,96,122,144,162,174,180,186,216,241,180},     // [Trumpet] [end cycle] [38 samples for note C5] -1dB start at  (45605 from beginning of file)
    {88,39,27,43,77,112,137,147,146,136,121,111,107,109,119,127,131,132,129,125,121,115,112,113,112,108,105,108,120,134,147,160,175,192,211,241,230,161},   // [Trumpet] [end cycle] [38 samples for note F#5] -1dB start at  (99665 from beginning of file)
  },
  { // instrument 5 (sax) here:
    {132,133,134,136,137,139,141,145,147,148,149,153,151,154,152,148,147,141,138,136,130,126,120,113,108,105,100,96,93,90,91,95,95,101,109,117,122,126},    // [Sax] [start cycle] [38 samples for note C5] start at 407 (407 from beginning)
    {127,130,133,136,137,136,136,138,143,146,146,146,149,149,147,148,147,144,142,138,133,129,126,120,115,111,106,99,96,95,95,96,98,103,109,112,117,124},    // [Sax] [start cycle] [38 samples for note F#5] start at 407 (407 from beginning)
    {109,162,169,152,141,164,205,206,185,157,134,129,140,152,152,143,152,147,134,125,114,112,83,64,59,53,44,37,51,67,80,123,177,221,205,146,75,52},         // [Sax] [mid cycle] [38 samples for note C5] start at   (831 from beginning)
    {98,165,172,152,139,156,200,209,184,156,132,126,139,150,151,141,145,149,133,124,115,110,89,66,62,60,50,42,52,72,82,118,171,217,211,149,81,49},          // [Sax] [mid cycle] [38 samples for note F#5] -3dB start at  (793 from beginning)
    {85,145,159,144,127,132,168,190,177,146,117,113,133,159,171,158,158,161,152,152,145,138,114,85,75,70,69,62,57,61,61,93,153,209,216,159,87,39},          // [Sax] [end cycle] [38 samples for note C5] start at   (21046 from beginning)
    {87,150,164,143,129,146,194,213,193,165,138,127,136,148,151,141,144,154,145,140,135,126,99,66,55,54,47,43,55,80,95,122,163,200,200,153,88,43},          // [Sax] [end cycle] [38 samples for note F#5] start at  (42402 from beginning)
  },
  { // instrument 6 (violin) here:
    {135,154,156,147,145,152,144,130,122,125,137,139,143,159,168,159,131,117,124,146,145,142,129,95,80,79,97,107,113,109,115,131,126,113,93,88,101,109},    // [Violin] [start cycle] [38 samples for note C5] start at 680 (820 from beginning)
    {126,160,171,165,158,156,153,154,163,168,161,148,137,127,121,129,142,143,149,133,106,91,90,102,108,113,117,120,116,111,113,115,114,101,86,76,78,97},    // [Violin] [start cycle] [38 samples for note F#5] start at 680 (944 from beginning)
    {141,181,182,185,201,209,191,161,123,105,110,106,136,195,226,187,130,118,149,171,177,156,103,47,16,24,38,66,91,115,139,157,144,91,54,56,66,89},         // [Violin] [mid cycle] [38 samples for note C5] -1.2dB start at   (2912 from beginning)
    {143,206,217,192,178,158,140,171,211,223,203,190,173,129,99,114,143,167,161,106,62,49,72,105,135,149,148,137,123,108,96,85,59,28,16,18,38,74},          // [Violin] [mid cycle] [38 samples for note F#5] -1.2dB start at  (3560 from beginning)
    {165,224,222,189,175,185,180,154,134,125,146,159,170,193,207,173,110,81,108,144,161,147,87,35,16,32,58,94,111,105,106,125,132,97,57,52,70,97},          // [Violin] [end cycle] [38 samples for note C5] -1.2dB start at   (14768 from beginning)
    {159,226,243,212,183,166,149,150,172,178,158,140,133,125,119,136,159,162,141,108,83,77,86,110,140,145,128,113,101,95,93,93,72,42,35,43,63,96},          // [Violin] [end cycle] [38 samples for note F#5] -0.8dB start at  (21071 from beginning)
  }
};
const PROGMEM uint8_t MsfNoteBand7[][6][27] = // [instrument] [cycle] [samples] // band 7
{
  { // instrument 1 (piano) here:
    {109,90,66,48,44,46,45,40,45,64,90,117,138,153,170,178,182,192,198,203,212,220,220,212,183,150,128},        // [Piano] [start cycle] [27 samples for note F#5] -0dB start at 900 (950 from beginning)
    {111,94,76,59,48,45,51,59,67,77,92,107,122,133,143,154,167,179,188,193,192,187,180,171,159,144,129},        // [Piano] [start cycle] [27 samples for note C6] -0dB start at 900 (936 from beginning)
    {125,119,113,106,98,91,87,88,93,96,100,106,116,124,131,139,148,156,162,164,161,159,155,148,142,137,130},    // [Piano] [mid cycle] [27 samples for note F#5] -10dB start at 4831 (4881 from beginning)
    {121,114,108,103,101,99,98,99,101,106,111,115,121,128,136,143,149,153,157,159,159,156,151,145,140,133,126}, // [Piano] [mid cycle] [27 samples for note C6] -12dB start at 4827 (4863 from beginning)
    {122,115,106,98,90,87,88,92,96,101,109,113,122,130,139,146,153,157,161,164,166,165,159,153,145,137,130},    // [Piano] [end cycle] [27 samples for note F#5] -10dB start at 47891 (47941 from beginning)
    {119,112,105,100,97,96,97,99,103,108,114,121,128,136,143,149,154,157,159,159,158,157,153,148,142,134,126},  // [Piano] [end cycle] [27 samples for note C6] -12dB start at 11821 (11857 from beginning)
  },
  { // instrument 2 (guitar) here:
    {102,53,32,72,147,188,163,113,95,121,150,141,97,67,80,117,144,145,141,156,183,196,180,151,134,134,133},     // [Guitar] [start cycle] [27 samples for note F#5] start at 355 (662 from beginning)
    {83,40,47,107,165,165,116,75,85,131,159,138,96,78,97,131,150,149,151,166,185,186,170,151,146,147,128},      // [Guitar] [start cycle] [27 samples for note C6] start at 355 (641 from beginning)
    {112,107,112,113,106,100,105,117,123,115,99,85,80,83,90,99,107,118,134,153,171,184,191,191,181,160,133},    // [Guitar] [mid cycle] [27 samples for note F#5] start at  (8550 from beginning)
    {120,90,82,96,116,127,124,119,125,138,139,121,96,79,80,93,107,116,121,129,141,157,172,184,191,185,161},     // [Guitar] [mid cycle] [27 samples for note C6] start at  (8769 from beginning)
    {122,103,90,87,91,100,108,112,112,112,112,110,104,98,98,105,118,132,143,155,167,180,189,191,182,165,144},   // [Guitar] [end cycle] [27 samples for note F#5] start at  (27041 from beginning)
    {121,111,101,92,88,88,89,89,88,87,88,91,97,106,119,135,155,175,188,191,186,177,170,163,154,143,131},        // [Guitar] [end cycle] [27 samples for note C6] start at  (31991 from beginning)
  },
  { // instrument 3 (marimba) here:
    {111,90,72,59,42,25,16,13,22,42,63,82,101,117,140,169,196,218,228,227,226,225,221,214,195,167,139},         // [Marimba] [start cycle] [27 samples for note F#5] start at 715 (735 from beginning of file)
    {111,90,72,59,42,25,16,13,22,42,63,82,101,117,140,169,196,218,228,227,226,225,221,214,195,167,139},         // [Marimba] [start cycle] [27 samples for note F#5 instead of C6] start at 715 (735 from beginning of file)
    {}, // mid cycle not used
    {}, // mid cycle not used
    {119,105,91,80,71,66,63,64,69,76,86,99,113,128,142,156,169,179,186,190,191,188,182,173,162,148,134},        // [Marimba] [end cycle] [27 samples for note F#5] start at  (Sinewave near end of file)
    {119,105,91,80,71,66,63,64,69,76,86,99,113,128,142,156,169,179,186,190,191,188,182,173,162,148,134},        // [Marimba] [end cycle] [27 samples for note F#5 instead of C6] start at  (Sinewave near end of file)
  },
  { // instrument 4 (trumpet) here:
    {78,72,99,122,125,123,122,123,122,125,128,128,125,124,123,123,121,118,115,115,117,129,141,159,184,221,169}, // [Trumpet] [start cycle] [27 samples for note F#5] start at  (243 from beginning of file)
    {78,72,99,122,125,123,122,123,122,125,128,128,125,124,123,123,121,118,115,115,117,129,141,159,184,221,169}, // [Trumpet] [start cycle] [27 samples for note C6]   (same as F#5 above)
    {36,43,80,113,116,128,141,133,118,119,119,125,128,130,121,128,117,116,111,120,121,136,145,175,180,234,199}, // [Trumpet] [mid cycle] [27 samples for note F#5] -1.5dB start at  (2964 from beginning of file)
    {44,35,81,126,137,140,132,120,111,111,117,123,127,126,123,126,122,118,111,108,113,140,158,184,190,234,190}, // [Trumpet] [mid cycle] [27 samples for note C6] -1.5dB start at  (847 from beginning of file)
    {54,25,53,105,140,148,136,116,107,111,125,132,131,126,119,112,112,110,105,111,131,148,169,191,224,241,150}, // [Trumpet] [end cycle] [27 samples for note F#5] -1dB start at  (70814 from beginning of file)
    {64,40,63,100,129,139,136,130,122,117,119,123,119,106,97,93,100,112,120,122,129,145,170,193,234,241,168},   // [Trumpet] [end cycle] [27 samples for note C6] -1dB start at  (98860 from beginning of file)
  },
  { // instrument 5 (sax) here:
    {137,146,153,152,150,152,152,149,143,139,133,125,117,108,99,90,86,89,94,99,107,117,131,138,139,137,133},    // [Sax] [start cycle] [27 samples for note F#5] start at 321 (321 from beginning)
    {136,148,156,159,159,155,148,140,128,122,124,128,124,118,113,105,94,79,72,80,100,122,142,150,146,132,123},  // [Sax] [start cycle] [27 samples for note C6] start at 321 (366 from beginning)
    {85,162,156,131,163,210,179,138,123,140,146,135,144,136,120,112,87,66,64,52,57,80,106,176,219,169,76},      // [Sax] [mid cycle] [27 samples for note F#5] start at   (1130 from beginning)
    {122,167,210,209,180,164,169,170,146,114,105,117,119,105,95,94,87,69,48,44,68,104,136,160,163,142,117},     // [Sax] [mid cycle] [27 samples for note C6] start at   (774 from beginning)
    {91,157,153,139,169,194,167,137,124,142,143,128,125,121,121,121,99,81,67,45,63,99,131,191,213,152,65},      // [Sax] [end cycle] [27 samples for note F#5] start at   (29258 from beginning)
    {145,182,210,202,179,176,188,185,148,110,107,118,108,83,76,89,94,75,47,36,55,86,120,149,165,159,140},       // [Sax] [end cycle] [27 samples for note C6] start at   (37055 from beginning)
  },
  { // instrument 6 (violin) here:
    {139,168,160,154,151,155,165,155,140,126,124,138,145,139,106,92,102,112,117,121,114,114,117,107,87,80,98},   // [Violin] [start cycle] [27 samples for note F#5] start at 482 (671 from beginning)
    {141,148,146,157,149,144,133,138,131,132,120,115,122,141,152,146,140,121,95,88,103,109,100,121,122,102,115}, // [Violin] [start cycle] [27 samples for note C6] start at 482 (611 from beginning)
    {187,218,186,157,148,203,223,201,169,110,102,146,174,115,53,64,108,144,145,130,107,95,78,37,16,31,85},       // [Violin] [mid cycle] [27 samples for note F#5] -1.2dB start at  (2314 from beginning)
    {166,159,173,221,234,201,176,162,154,132,111,113,127,164,174,174,129,86,71,58,63,54,70,76,35,51,96},         // [Violin] [mid cycle] [27 samples for note C6] -1.5dB start at  (3770 from beginning)
    {192,234,209,182,148,149,164,151,150,132,122,152,158,109,75,79,108,142,144,120,103,92,81,38,30,58,101},      // [Violin] [end cycle] [27 samples for note F#5] -1.5dB start at  (15053 from beginning)
    {147,151,188,225,234,192,162,153,145,111,96,114,163,201,208,186,126,79,54,60,72,69,70,31,21,59,113},         // [Violin] [end cycle] [27 samples for note C6] -1.5dB start at  (20151 from beginning)
  }
};
const PROGMEM uint8_t MsfNoteBand8[][3][19] = // [instrument] [cycle] [samples] // band 8 (not mixed with 2nd note)
{
  { // instrument 1 (piano) here:
    {101,76,51,41,48,60,74,97,119,136,152,170,187,196,194,185,171,151,128},      // [Piano] [start cycle] [19 samples for note C6] -0dB start at 625 (661 from beginning)
    {123,113,105,100,99,99,103,110,117,125,135,146,153,157,159,156,149,141,132}, // [Piano] [mid cycle] [19 samples for note C6] -12dB start at 3367 (3403 from beginning)
    {117,107,100,96,97,101,106,115,126,136,146,153,158,159,158,155,149,139,127}, // [Piano] [end cycle] [19 samples for note C6] -12dB start at 8308 (8344 from beginning)
  },
  { // instrument 2 (guitar) here:
    {90,35,88,169,140,75,102,158,128,80,101,143,150,154,181,184,157,146,142},   // [Guitar] [start cycle] [19 samples for note C6] start at 250 (451 from beginning)
    {111,81,97,124,124,120,135,137,105,79,85,106,118,126,144,166,184,191,166},  // [Guitar] [mid cycle] [19 samples for note C6] start at  (6171 from beginning)
    {126,111,97,88,87,89,87,87,90,99,115,138,167,189,191,180,169,158,142},      // [Guitar] [end cycle] [19 samples for note C6] start at  (22512 from beginning)
  },
  { // instrument 3 (marimba) here:
    {118,91,64,37,19,17,35,60,84,108,144,184,212,222,225,227,216,187,151},      // [Marimba] [start cycle] [19 samples for note F#6] start at 656 (658 from beginning of file)
    {}, // mid cycle not used
    {115,96,79,68,63,65,74,89,107,128,149,167,181,189,191,185,173,156,136},     // [Marimba] [end cycle] [19 samples for note F#6] start at  (Sinewave near end of file)
  },
  { // instrument 4 (trumpet) here:
    {80,81,120,125,122,122,123,128,126,124,123,123,117,116,115,134,150,197,203}, // [Trumpet] [start cycle] [19 samples for note C6] start at  (171 from beginning of file)
    {53,36,120,138,138,117,110,114,126,125,124,123,117,106,112,145,179,207,234}, // [Trumpet] [mid cycle] [19 samples for note C6] -1.5dB start at  (596 from beginning of file)
    {60,44,89,134,139,124,114,118,126,123,109,100,101,113,122,155,195,241,200},  // [Trumpet] [end cycle] [19 samples for note C6] -1dB start at  (69511 from beginning of file)
  },
  { // instrument 5 (sax) here:
    {180,218,179,164,161,125,108,126,119,106,98,86,60,51,88,144,167,126,101},    // [Sax] [start cycle] [19 samples for note C6] start at 139 (156 from beginning)
    {172,215,182,165,172,139,104,118,113,96,93,75,45,55,104,148,165,134,114},    // [Sax] [mid cycle] [19 samples for note C6] start at   (347 from beginning)
    {127,192,187,159,180,173,119,122,130,99,110,118,71,37,59,102,149,159,120},   // [Sax] [end cycle] [19 samples for note C6] start at   (24065 from beginning)
  },
  { // instrument 6 (violin) here:
    {143,146,155,148,137,135,132,121,116,138,152,142,119,86,104,103,115,119,104}, // [Violin] [start cycle] [19 samples for note C6] start at 340 (430 from beginning)
    {169,165,229,234,184,168,140,108,115,162,188,145,77,59,49,54,66,27,80},       // [Violin] [mid cycle] [19 samples for note C6] -1.5dB start at  (2805 from beginning)
    {151,193,234,190,155,145,104,107,169,208,182,105,58,65,74,64,23,57,130},      // [Violin] [end cycle] [19 samples for note C6] -1.5dB start at  (14181 from beginning)
  }
};
const PROGMEM uint8_t MsfNoteBand9[][3][13] = // [instrument] [cycle] [samples] // band 9 (not mixed with 2nd note)
{
  { // instrument 1 (piano) here:
    {122,82,47,44,61,86,120,144,170,193,195,181,156},      // [Piano] [start cycle] [13 samples for note F#6] -0dB start at 428 (24 from beginning)
    {130,115,104,96,99,104,115,129,143,156,159,153,143},   // [Piano] [mid cycle] [13 samples for note F#6] -12dB start at 2383 (2407 from beginning)
    {129,114,103,95,95,102,116,131,143,152,157,155,145},   // [Piano] [end cycle] [13 samples for note F#6] -12dB start at 5880 (5904 from beginning)
  },
  { // instrument 2 (guitar) out of limits
  },
  { // instrument 3 (marimba) here:
    {87,48,18,24,60,94,140,196,221,226,220,180,129},      // [Marimba] [start cycle] [13 samples for note F#6] start at 450 (451 from beginning of file)
    {}, // mid cycle not used
    {112,85,67,64,74,97,127,156,179,190,187,170,143},     // [Marimba] [end cycle] [13 samples for note F#6] start at  (Sinewave near end of file)
  },
  { // instrument 4 (trumpet) out of limits
  },
  { // instrument 5 (sax) out of limits
  },
  { // instrument 6 (violin) here:
    {155,158,143,137,129,114,145,149,104,87,109,110,115}, // [Violin] [start cycle] [13 samples for note F#6] start at 233 (295 from beginning)
    {177,234,209,161,162,110,168,125,43,60,79,34,85},     // [Violin] [mid cycle] [13 samples for note F#6] -1.5dB start at  (8392 from beginning)
    {164,210,234,185,180,118,153,151,55,32,72,34,66},     // [Violin] [end cycle] [13 samples for note F#6] -1.5dB start at  (10418 from beginning)
  }
};

/***********************************************************************************************/ 

void Setup1_DAWG()
{
  analogReadResolution(12);
  analogWriteResolution(12);
  Serial.begin (115200);
  Serial.setTimeout(50);
  Serial.println("\n   ************** Due Arbitrary Waveform Generator **************\n\n                Type and enter '?' for help.\n\n");
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
  pinMode(20, INPUT_PULLUP); // modulation on / off
  pinMode(43, INPUT_PULLUP); // Save current settings as Start-up Default switch
  pinMode(52, INPUT_PULLUP); // Save Preset switch - press after entering number with keypad - Press twice to replace existing Preset data (LED will light up if it exists) or press clear to cancel
  pinMode(53, INPUT_PULLUP); // Load Preset switch - press after entering number with keypad  
  pinMode(59, INPUT_PULLUP); // music mode on / off
  pinMode(60, INPUT_PULLUP); // Save Startup Tune switch - press after entering tune number with keypad - add 100 to tune number to stay in music mode after playing. To cancel playing at startup, make tune number zero
  pinMode(61, INPUT_PULLUP); // Play Tune switch - press after entering number with keypad - will stay in music mode after playing. Press clear while playing to stop
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
  pinMode(25, OUTPUT); // indicates: Unsync'ed square wave: Pot controls FREQ or PERIOD
  pinMode(27, OUTPUT); // indicates: Unsync'ed square wave: Pot controls FREQ or PERIOD
  pinMode(29, OUTPUT); // indicates: Sync'ed wave: Pot controls FREQ or PERIOD
  pinMode(31, OUTPUT); // indicates: Sync'ed wave: Pot controls FREQ or PERIOD
  pinMode(33, OUTPUT); // indicates: Unsync'ed square wave: Pot controls DUTY CYCLE or PULSE WIDTH
  pinMode(35, OUTPUT); // indicates: Unsync'ed square wave: Pot controls DUTY CYCLE or PULSE WIDTH
  pinMode(37, OUTPUT); // indicates: Sync'ed wave: Pot controls DUTY CYCLE or PULSE WIDTH
  pinMode(39, OUTPUT); // indicates: Sync'ed wave: Pot controls DUTY CYCLE or PULSE WIDTH
  pinMode(41, OUTPUT); // indicates: Exact Freq Mode ON 
  pinMode(45, OUTPUT); // indicates: Square Wave Sync ON
  pinMode(47, OUTPUT); // indicates: Analogue wave is being controlled
  pinMode(49, OUTPUT); // indicates: Unsynch'ed Square wave is being controlled
  pinMode(48, OUTPUT); // indicates: key pressed
  pinMode(50, OUTPUT); // lights up if confirmation needed to replace existing Preset data when saving Preset
  pinMode(51, OUTPUT); // indicates: switches enabled
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
    if (interruptMode == 0 && InterruptMode > 0) MusicEnterExit(0); // exit modulation mode or music mode first
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
      if (interruptMode > 0) MusicEnterExit(interruptMode); // enter modulation mode or music mode last
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

void SaveMusicToFlash(int tune) // tune = tune - 1 // save music tune
{
  int address; // Preset info up to 226720. Tunes start at: Start up tune: 226748 & 226749. 226750 = start address for Tunes config data: 150, + Tune names: 1450 (50 * 29), + Envelope data: 1750, + MusicNotes: 6400 (25 * 256) + 25600 (25 * 1024). Tunes total of 35352 = Grand total: 262100 used. Max memory available: 262144
  // save music config data at 226750, just after 226720 used by: start default + 50 presets + 50 arbitrary waves: Presets = 12240 (51 * 240) + (11 * 8196) + 4 (space) + (40 * 3108) (11 + 40 arbitrary waves)
  dueFlashStorage.write(tune + 226750, LinkedPreset);            // 255 = tune empty // 0 = indicates not empty & not linked to preset // 1 to 50 = preset number linked to
  dueFlashStorage.write(tune + 226800, Tempo);                   // 0 - 255 (plus 15 added later = 15 - 270)
  dueFlashStorage.write(tune + 226850, Instrument);              // 0 = wave, 1 = piano, 2 = guitar, 3 = marimba, 4 = trumpet, 5 = sax, 6 = violin
  // Tune name data starts at 226900: 1450 ((tune - 1) * 29) or (50 * 29) + 226900 = 228350  (later in sketch)
  // Envelope data - stores 5 envelope settings of each instrument for each tune:
  for (byte i = 0; i < 7; i++) // 7 (instruments) * 50 (tunes) * 5 (settings) = 1750 + 228350 (Envelope data start point) = 230100 (Tune notes data start point)
  {
    for (byte ii = 0; ii < 5; ii++) // 5 settings:
    {
      dueFlashStorage.write(228350 + (tune * 35) + (i * 5) + ii, Envelope[i][ii]); // save 5 settings of each instrument (up to 35) for selected tune: attack rate, decay delay time, decay rate, sustain level, release rate
    }
  }
  uint16_t limit = 256;
  if (tune >= 25) limit = 1024;
  byte musicNotes[min(NotesCount, limit)]; // create array the correct (limited) size
  for (uint16_t i = 0; i < min(NotesCount, limit); i++) // transfer music notes into correct sized array:
  {
    musicNotes[i] = MusicNotes[i]; // address = 250 = length of 1st 25 short tunes or address = 1000 = length of last 25 long tunes
//    Serial.print("musicNotes[i] "); Serial.print(musicNotes[i]); Serial.print("  MusicNotes[i] "); Serial.println(MusicNotes[i]); //;
  }
  if (tune < 25) address = (tune * 256) + 230100;  // save Notes data for 1st 25 tunes (25 * 256) + 230100 = 236500
  else   address = ((tune - 25) * 1024) + 236500; // save Notes data for last 25 tunes (25 * 1024) + 236500 = 262100
  dueFlashStorage.write(address, musicNotes, min(NotesCount, limit)); // address = 256 = length of 1st 25 short tunes or address = 1024 = length of last 25 long tunes // write data in an array for fast saving
  if (NotesCount < limit) dueFlashStorage.write(address + NotesCount, 255); // if tune length is less than limit, 255 indicates end
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
      if (PotsEnabled == 1 || PotsEnabled == 3) digitalWrite(51, HIGH); // Switches enabled LED
      else digitalWrite(51, LOW);
      if (PotsEnabled >= 2) digitalWrite(23, HIGH); // Pots enabled LED
      else digitalWrite(23, LOW);
      if (PotsEnabled == 0)
      {
        digitalWrite(4, LOW);
        digitalWrite(6, LOW);
        digitalWrite(25, LOW);
        digitalWrite(27, LOW);
        digitalWrite(29, LOW);
        digitalWrite(31, LOW);
        digitalWrite(33, LOW);
        digitalWrite(35, LOW);
        digitalWrite(37, LOW);
        digitalWrite(39, LOW);
        digitalWrite(41, LOW);
        digitalWrite(45, LOW);
        digitalWrite(47, LOW);
        digitalWrite(49, LOW);        
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
      else if (!digitalRead(20)) // Toggle Modulation Mode ON / OFF
      {
        if (InterruptMode == 1) MusicEnterExit(0); // exit mod mode
        else                    MusicEnterExit(1); // enter mod mode
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(59)) // Toggle Music Mode ON / OFF
      {
        if (InterruptMode > 1) MusicEnterExit(0); // exit music mode
        else                   MusicEnterExit(2); // enter music mode
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(60)) // Save Startup Tune
      {
        if       (UserInput > 100 && dueFlashStorage.read(226748) != 100) dueFlashStorage.write(226748, 100); // stay in music mode after playing startup tune and keep startup tune wave settings
        else if (UserInput <= 100 && dueFlashStorage.read(226748) != 255) dueFlashStorage.write(226748, 255); // load startup default settings after playing startup tune, may require exiting Music Mode
        byte tuneNum = byte(int(UserInput) % 100);
        if (dueFlashStorage.read(226749) != tuneNum) dueFlashStorage.write(226749, tuneNum); // start up tune number // only write to flash if necessary
        Serial.print("   Saved start up Tune "); Serial.println(tuneNum);
        SwitchPressedTime = millis();
      }
      else if (!digitalRead(61)) // Play Tune
      {
        keyPressed = 1; // causes LED (on pin 48) to light until finished playing
        PlayTune(UserInput, 'l'); // 'l' (L) means load Linked Preset before playing if not already loaded
        UserInput = 0;
        SwitchPressedTime = millis();
      }      
      else if (!digitalRead(43)) // Save Start-up Defaults
      {
        keyPressed = 1; // causes LED (on pin 48) to light
        if (ClearPreset == 0)
        {
          SaveToFlash(ClearPreset);
          if (UsingGUI) { Serial.print("Preset "); Serial.print(ClearPreset); Serial.println(" saved"); }
          else
          {
            Serial.print("   Current Settings have been saved as Start-up Default ");
            if (dueFlashStorage.read(3) == 11)  Serial.println(" - including Arbitrary wave!\n"); // if Arbitrary wave included
            else Serial.println(" - without Arbitrary wave!\n"); // if Arbitrary wave not included
          }
          ClearPreset = 255; // reset
          digitalWrite(50, LOW);
        }
        else // need confirmation
        {
          if (!UsingGUI) { Serial.println("   Are you sure you want to replace the start-up default settings?  Type Y or N  (the N must be upper case)\n"); }
          ClearPreset = 0;
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
          digitalWrite(25, PotPeriodMode[0]);
          digitalWrite(27, !PotPeriodMode[0]);
          digitalWrite(29, PotPeriodMode[1]);
          digitalWrite(31, !PotPeriodMode[1]);
          digitalWrite(33, !PotPulseWidth[0]);
          digitalWrite(35, PotPulseWidth[0]);
          digitalWrite(37, !PotPulseWidth[1]);
          digitalWrite(39, PotPulseWidth[1]);
        }
        if (PotsEnabled != 2) // non - pot related switches:
        {
          digitalWrite(4, min(1, SweepMode));
          digitalWrite(6, min(1, TimerMode));
          digitalWrite(41, ExactFreqMode);
          digitalWrite(45, SquareWaveSync);
          if (Control > 0) digitalWrite(47, HIGH);
          else digitalWrite(47, LOW);
          if (Control != 1) digitalWrite(49, HIGH);
          else digitalWrite(49, LOW);   
        }
        LEDupdateTime = millis();
      }
      else if (PotsEnabled == 0) // if PotsEnabled just switched off
      {
        digitalWrite(4, LOW);
        digitalWrite(6, LOW);
        digitalWrite(25, LOW);
        digitalWrite(27, LOW);
        digitalWrite(29, LOW);
        digitalWrite(31, LOW);
        digitalWrite(33, LOW);
        digitalWrite(35, LOW);
        digitalWrite(37, LOW);
        digitalWrite(39, LOW);
        digitalWrite(41, LOW);
        digitalWrite(45, LOW);
        digitalWrite(47, LOW);
        digitalWrite(49, LOW);
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
          delay(1); // short delay to ensure next char is ready to be read correctly by Serial.peek()
          if (UserChars[0] == 'm' && Serial.peek() >= '0' && Serial.peek() <= '9') // if received "m" plus a number - Music command
          {
            MusicEnterExit(Serial.read() - '0');
          }
          else SetFreqPeriod(); // FREQ or PERIOD ADJUSTMENT: in Hertz or Milliseconds
          break;
        case 'd': // if (UserChars[0] == 'd' || UserChars[0] == 'u') // DUTY CYCLE or PULSE WIDTH ADJUSTMENT: in % or microseconds OR Update command
        case 'u':
          delay(1); // short delay to ensure next char is ready to be read correctly by Serial.peek()
          if (UserChars[0] == 'u' && Serial.peek() == 'u') // if received "uu" - Update command
          {
            byte remInterruptMode = InterruptMode;
            if (InterruptMode > 2) MusicEnterExit(2); // if mini-soundfont instruments are enabled, temporarily switch to WAVE instrument for update
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
            if (remInterruptMode > 2) MusicEnterExit(remInterruptMode); // if mini-soundfont instruments were enabled before update, switch back to them
            else
            {
              Serial.read(); // remove 2nd 'u' so Pulse Width command won't be detected
              if (!UsingGUI) Serial.println("   Noise doesn't need updating.\n");
            }
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
                  SinAmp = UserInput / 100.0;
                  if (!UsingGUI) {
                    Serial.print("   Sine Wave Amplitude is ");
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
                    Serial.println(  "   s0a - for Amplitude      (eg: 100 means 100%, which is default = 100s0a)");
                    Serial.println(  "   s0v - for Vertical shift                              (default =  50s0v)");
                    Serial.println(  "   s0p - for Phase shift relative to sync'ed square wave (default = 0.5s0p)");
                    Serial.println(  "   s0f - for 2nd sine wave Frequency mulptile              (default = 8s0f)");
                    Serial.println(  "   s0+ - to Add waves      - mix: 0 to 100     (50 = both) (default = 0s0+)");
                    Serial.println(  "   s0* - to Multiply waves - mix: 0 to 100     (50 = both) (default = 0s0*)");
                    Serial.println(  "   Hint: 50s0* = ring modulation. 76s0* = amplitude mod. 100s0* = 2nd wave");
                    Serial.println(  "   Current values: ");
                    Serial.print(    "   Amplitude = "); Serial.print(SinAmp * 100, 0); Serial.print(", Bias = "); Serial.print(SinVshift * 100, 0); Serial.print(", Phase = "); Serial.println(SinPhase);
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
          if (Serial.peek() == 'T') // "PT" - Play Tune from flash memory command
          {
            Serial.read(); // clear 'T'
            int tune = Serial.parseInt();
            char ch = Serial.read(); // "PTXl" (X is the tune number) means Play Tune and load linked preset if not currently loaded. "PTXr" means reload linked preset anyway
            PlayTune(tune, ch);
          }
          else
          {
            if (PotsEnabled < 3) PotsEnabled++;
            else PotsEnabled = 0;
            if (PotsEnabled == 1 || PotsEnabled == 3) digitalWrite(51, HIGH); // Switches enabled LED
            else digitalWrite(51, LOW);
            if (PotsEnabled >= 2) digitalWrite(23, HIGH); // Pots enabled LED
            else digitalWrite(23, LOW);
            if      (PotsEnabled == 0) Serial.println("   Pots & Switches Disabled\n");
            else if (PotsEnabled == 1) Serial.println("   Switches Only Enabled\n");
            else if (PotsEnabled == 2) Serial.println("   Pots Only Enabled\n");
            else if (PotsEnabled == 3) Serial.println("   Pots & Switches Enabled\n");
          }
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
        case '>': // if Enter pressed (ie: if UserChars[0] default char '>' was not replaced by serial data)
          if (millis() - 500 < TouchedTime && TouchedTime > 0) // if Enter pressed twice - display status
          {
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
          }
          else TouchedTime = millis();
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
          else if (Serial.peek() == 't') // if received "lt" - list tune usage command
          {
            for (byte i = 0; i < 50; i++) // send List of Tune usage info. i = Tune Number - 1
            {
              Serial.print("   Tune "); Serial.print(i + 1);
              byte usage = dueFlashStorage.read(226750 + i); // read Tune Usage info from flash
              if (usage > 150) Serial.print(" is empty");
              else // read name, etc
              {
                Serial.print(" ");
                for (byte ii = 0; ii < 29 ; ii++) // Read Name from flash memory, 1 character at a time
                {
                  char ch = dueFlashStorage.read((i * 29) + 226900 + ii); // read Name from flash
                  if (ch == '\n')
                  {
                    if (ii == 0) Serial.print("has no name");
                    break;
                  }
                  Serial.print(ch);
                }
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
        case 'z':
          TimingCount = 65000; // will be set to zero at start of note playing
          if (UsingGUI) ReceiveNotes(-1); // if tune >= 0 read from flash memory, else read from Serial
          break;
        case 'Z': // play notes:
          delay(1); // short delay to ensure next char is ready to be read correctly by Serial.peek()
          if (Serial.peek() >= '0' && Serial.peek() <= '9')
          {
            uint16_t startPoint =  Serial.parseInt();
            if (InterruptMode < 2) MusicEnterExit(2);
            PlayNotes(startPoint); // 0 = play from start
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
            if (InterruptMode > 0) MusicEnterExit(0);
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
            else if (Serial.peek() == 'T') // link tune x to preset x // LTxPx
            {
              Serial.read(); // clear 'T'
              int tune = Serial.parseInt();
              if (tune < 1 || tune > 50)
              {
                Serial.print("   Tune "); Serial.print(tune); Serial.println(" does not exist!\n");
                break;
              }
              delay(1); // short delay to ensure next char is ready to be read correctly by Serial.peek()
              if (Serial.peek() == 'P') // link tune x to preset x // LTxPx
              {
                int preset = Serial.parseInt();
                if (preset < 1 || preset > 50)
                {
                  Serial.print("   Preset "); Serial.print(preset); Serial.println(" does not exist!\n");
                  break;
                }
                dueFlashStorage.write(tune - 1 + 226750, preset);   // 255 = empty // 0 = indicates tune not empty & not linked to preset // 1 to 50 = preset number linked to
                Serial.print("   Linked Tune "); Serial.print(tune); Serial.print(" to Preset "); Serial.println(preset);
              }            
            }          
            break;
          }
          else if (UserChars[0] == 'S' && (Serial.peek() == 'D' || Serial.peek() == 'N' || Serial.peek() == 'P' || Serial.peek() == 'T' || Serial.peek() == 'U')) // if received "SD", "SP", "SPN", "ST", "STN" or "SUT" - Save Defaults, Save Preset, Save Preset Name, Save Tune, Save Tune Name or Start Up Tune command
          {
            if (Serial.peek() == 'D')
            {
              Serial.read(); // clear 'D'
              if (!UsingGUI) { Serial.println("   Are you sure you want to replace the start-up default settings?  Type Y or N  (the N must be upper case)\n"); }
              ClearPreset = 0;
            }
            else if (Serial.peek() == 'P') // Save Preset
            {
              Serial.read(); // clear 'P'
              if (Serial.peek() == 'N') // receive Preset Name
              {
                Serial.read(); // clear 'N'
                int preset = 0;
                delayMicroseconds(100); // allow time to ensure next serial character has arrived
                if (Serial.peek() >= '0' && Serial.peek() <= '9')  preset = (preset * 10) + (Serial.read() - '0'); // the char '0' is ASCII 48, so '0'(48) minus '0'(48) = int 0 (and '9'(57) minus '0'(48) = int 9 etc)
                else break;
                delayMicroseconds(100); // allow time to ensure next serial character has arrived
                if (Serial.peek() >= '0' && Serial.peek() <= '9')  preset = (preset * 10) + (Serial.read() - '0'); // read only 2 digits - the following characters consist of the name being received
                else break;
                if (preset < 1 || preset > 50)
                {
                  Serial.print("   Preset "); Serial.print(preset); Serial.println(" does not exist!\n");
                  break;
                }
                else // if preset number is valid
                {
                  char ch;
                  for (byte i = 0; i < 22; i++) // receive / Save Preset Name:
                  {
                    unsigned long del = millis();
                    while (Serial.available() == 0 && millis() - 2 < del) delayMicroseconds(10); // if next serial char not arrived yet, wait up to 2 mSecs for serial to become available. if still no serial, continue in case it was the last char sent
                    ch = Serial.read();
                    dueFlashStorage.write((preset * 240) + 221 + i, ch);
                    if (ch == '\n') break; // if end of name, '\n' will have been written to flash memory indicating end of name
                  }
                  Serial.println("   Name saved");
                }
              }
              else // if (Serial.peek() != 'N') // Save Preset:
              {
                int preset = Serial.parseInt();
                if (preset < 1 || preset > 50)
                {
                  Serial.print("   Preset "); Serial.print(preset); Serial.println(" does not exist!\n");
                  break;
                }
                else if (dueFlashStorage.read((preset * 240) + 3) <= 11) // if Preset not empty
                {
                  if (!UsingGUI) { Serial.print("   Preset "); Serial.print(preset); Serial.println(" is not empty!\n   Do you want to replace it?  Type Y or N  (the N must be upper case)\n"); }
                  ClearPreset = preset;
                }
                else // if Preset is empty
                {
                  SaveToFlash(preset);
                  if (UsingGUI) { Serial.print("Preset "); Serial.print(preset); Serial.println(" saved"); }
                  else
                  {
                    Serial.print("   Current Settings have been saved as Preset "); Serial.print(preset);
                    if (dueFlashStorage.read((preset * 240) + 3) == 11 && preset < 26)  Serial.println(" - including Arbitrary wave!\n"); // if Arbitrary wave included
                    else Serial.println(" - without Arbitrary wave!\n"); // if Arbitrary wave not included
                  }
                }
              }
            }
            else if (Serial.peek() == 'T') // Save Tune / Name
            {
              Serial.read(); // clear 'T'
              if (Serial.peek() == 'N') // receive Tune Name
              {
                Serial.read(); // clear 'N'
                int tune = 0;
                delayMicroseconds(100); // allow time to ensure next serial character has arrived
                if (Serial.peek() >= '0' && Serial.peek() <= '9')  tune = (tune * 10) + (Serial.read() - '0'); // the char '0' is ASCII 48, so '0'(48) minus '0'(48) = int 0 (and '9'(57) minus '0'(48) = int 9 etc)
                else break;
                delayMicroseconds(100); // allow time to ensure next serial character has arrived
                if (Serial.peek() >= '0' && Serial.peek() <= '9')  tune = (tune * 10) + (Serial.read() - '0'); // read only 2 digits - the following characters consist of the name being received
                else break;
                if (tune < 1 || tune > 50)
                {
                  Serial.print("   Tune "); Serial.print(tune); Serial.println(" does not exist!\n");
                  break;
                }
                else // if tune number is valid
                {
                  char ch;
                  for (byte i = 0; i < 28; i++) // receive & Save Tune Name:
                  {
                    unsigned long del = millis();
                    while (Serial.available() == 0 && millis() - 2 < del) delayMicroseconds(10); // if next serial char not arrived yet, wait up to 2 mSecs for serial to become available. if still no serial, continue in case it was the last char sent
                    ch = Serial.read();
                    dueFlashStorage.write(((tune - 1) * 29) + 226900 + i, ch);
                    if (ch == '\n') break; // if end of name, '\n' will have been written to flash memory indicating end of name
                    else if (i == 27 && ch != '\n') dueFlashStorage.write(((tune - 1) * 29) + 226900 + i + 1, '\n'); // if max number of characters received, save '\n' to last allocated flash memory location to indicate end of name
                  }
                  Serial.println("   Name saved");
                }
              }
              else // if (Serial.peek() != 'N') // Save Tune: ("ST")
              {
                int tune = Serial.parseInt();
                if (tune < 1 || tune > 50 || (tune > 25 && NotesCount > 1024) || (tune <= 25 && NotesCount > 256))
                {
                  if (tune < 1 || tune > 50) { Serial.print("   Tune "); Serial.print(tune); Serial.println(" does not exist!\n"); }
                  else { Serial.print("   Tune is too long: "); Serial.print(NotesCount); Serial.println(" 'notes'\n"); }
                  break;
                }
                else // if Tune is not too long // empty:
                {
                  SaveMusicToFlash(tune - 1);
                  Serial.print("Saved tune "); Serial.println(tune);
                }
              }
            }
            else if (Serial.peek() == 'U') // save Start Up Tune number ("SUT")
            {
              Serial.read(); // clear 'U'
              delayMicroseconds(100); // allow time to ensure next serial character has arrived
              if (Serial.peek() == 'T') // Start Up Tune: ("SUT")
              {
                Serial.read(); // clear 'T'
                uint8_t tune = Serial.parseInt();
                if       (tune > 100 && dueFlashStorage.read(226748) != 100) dueFlashStorage.write(226748, 100); // stay in music mode after playing startup tune and keep startup tune wave settings
                else if (tune <= 100 && dueFlashStorage.read(226748) != 255) dueFlashStorage.write(226748, 255); // load startup default settings after playing startup tune, may require exiting Music Mode
                byte tuneNum = byte(tune % 100);
                if (dueFlashStorage.read(226749) != tuneNum) dueFlashStorage.write(226749, tuneNum); // start up tune number // only write to flash if necessary
                Serial.print("   Saved start up Tune "); Serial.print(tuneNum);
              }
            }
            break;
          } // else go to default
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
              if (InterruptMode > 0) MusicEnterExit(0);
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
              if (InterruptMode > 0) MusicEnterExit(0);
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
                  // if (UserChars[0] == '?') // Help
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
                  Serial.println(  "   Type:  SD   to Save current settings as Start-up Default settings.");
                  Serial.println(  "   Type:  LD   to Load start-up Default settings.");
                  Serial.println(  "   Type:  FD   to load Factory start-up Default settings.");
                  Serial.println(  "   Type:  SPx  to Save settings to a Preset. The x is the Preset number: 1 to 50.");
                  Serial.println(  "   Type:  LPx  to Load Preset settings. The x is the Preset number: 1 to 50.");
                  Serial.println(  "   Type:  CPx  to Clear Preset settings. The x is the Preset number: 1 to 50.");
                  Serial.println(  "   Type: SPNxn to Save Preset Name. The x must be 2 digits: 01 to 50. The n is the Name up to 22 chars.");
                  Serial.println(  "   Type:  lp   to view a List of Presets with their names.");
                  Serial.println(  "   Type:  m1   to enter Modulation mode. Input A2 modulates the composite wave, set to any mix of waveshapes.");
                  Serial.println(  "   Type:  m2   or m3 to enter Music mode. Normal waveform stops, then play previously saved tunes. See PTx below.");
                  Serial.println(  "   Type:  m0   to enter normal waveform mode with no Modulation. Only this mode works at full sample rate.");
                  Serial.println(  "   Type:  lt   to view a List of Tunes with their names.");
                  Serial.println(  "   Type: PTx   to Play a Tune you saved in flash memory (using GUI). The x is the Tune number: 1 to 50.");
                  Serial.println(  "   Type: PTxl  as above, but first Load the preset you Linked to the tune, if not loaded, to play with chosen waveshape. (see LTxPx below)");
                  Serial.println(  "   Type: PTxr  as above, but first Reload the preset you linked to the tune, even if it's already loaded.");
                  Serial.println(  "   Type: LTxPx to Link a Tune to a Preset, so it plays with the chosen waveshape. (the tune's instrument must be 'wave')");
                  Serial.println(  "   Type: STNxn to Save Tune Name. The x must be 2 digits: 01 to 50. The n is the Name up to 28 chars.");
                  Serial.println(  "   Type: SUTx  to select a Start Up Tune to play when Arduino starts. The x is the Tune number: 1 to 50.");
                  Serial.println(  "               (add 100 to Start Up Tune number to not load defaults after playing. ie: SUT105 for tune 5)");
// Only used by GUI  Serial.println( " Type:  STx  to Save the Tune in the music score window. The x is the Tune number: 1 to 50. Only used by GUI");
                  Serial.println(  "   Press enter twice to display the current status.\n\n");
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
  if (StartupTune)
  {
    uint8_t tune = dueFlashStorage.read(226749); // start up tune number
    PlayTune(tune, 'l'); // 'l' (L) means load Linked Preset before playing if not already loaded
    StartupTune = 0; // prevents this code running again
  }
} // end of void loop()

void MusicEnterExit(byte interruptMode) // 0 = Exit Modulation mode & Music mode. 1 = Enter Modulation mode. 2 or more = Music mode
{
  interruptMode = min(3, interruptMode); // limit to voice 1 for now
  if (interruptMode >= 1 && interruptMode <= 9) // if Modulation or Music Mode enabled
  {
    if (WaveShape != 3)
    {
      UserChars[1] = '3'; // change WaveShape to 3 (composite)
      ChangeWaveShape(1); // 1 = sent from serial ( not switch)
      delay(50);
    }
    if (!ExactFreqMode) ToggleExactFreqMode(); //.
    if (interruptMode >= 3 && interruptMode <= 6) TC_setup2c(); // set voice 1 pitch
    else
    {
      TC_setup2b(); // set up analogue slow mode timing at quarter of normal sample rate to speed up CreateWaveFull(3) below
      Increment[0] = Increment[0] * 4; // compensate for quarter of normal sample rate
      Increment[1] = Increment[1] * 4;
    }
  }
  if (interruptMode >= 2 && interruptMode <= 9) // if Music Mode enabled
  {
    NVIC_DisableIRQ(TC0_IRQn); //.
    TimingCount = 65000; // will be set to zero to start a note playing
    TC_setup6(); // set up tempo
    TC_setup8(); // set up timing for envelope, etc
    Modulation = 0; // amplitude off
    NoteReadTotal = 65000; // will be set to zero at start of note playing
    TempoCount = 15000; // will be set to zero at start of note playing
  }
  else // if interruptMode = 1 or 0 // if Modulation mode or normal mode enabled
  {
    NVIC_DisableIRQ(TC4_IRQn); // disable tempo timing
    NVIC_DisableIRQ(TC5_IRQn); // disable envelope timing
  }
//  if (interruptMode >= 6 && interruptMode <= 9) TC_setup7(); // set up voice 2 pitch
//  else NVIC_DisableIRQ(TC3_IRQn); // disable voice 2 pitch
  InterruptMode = interruptMode; // set InterruptMode after above setup
  if (InterruptMode == 0)
  {
    TC_setup2(); // EXIT modulation / music mode (re-enables IRQ(TC0_IRQn) at normal sample rate)
    TC_setup5(); //
  }
  if (InterruptMode < 3) // if wave (not minisoundfonts)
  {
    CreateWaveFull(WaveShape);
    SetWaveFreq(0);
  }
}

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

void ReceiveNotes(int8_t tune) // tune = tune - 1  // if tune >= 0 read from flash memory
{
  int address;
  byte oldTempo = Tempo;
  NotesCount = 0;
  if (tune >= 0) // if tune > 0 read from flash memory
  {
    //                        LinkedPreset data starts at 226750   // read earlier in sketch // 255 = empty // 0 = indicates not empty & not linked to preset // 1 to 50 = preset number linked to
    Tempo                   = dueFlashStorage.read(tune + 226800); // , Tempo);                          // 0 - 255 (plus 15 added later = 15 - 270)
    Instrument              = dueFlashStorage.read(tune + 226850); // , Instrument                       // 0 = wave, 1 = piano, 2 = guitar, etc
    // Tune name data starts at 226900: 1450 ((tune - 1) * 29) or (50 * 29) + 226900 = 228350 (later in sketch)
    // read Envelope data - 5 envelope settings of each instrument for each tune stored:
    for (byte i = 0; i < 7; i++) // 7 (instruments) * 50 (tunes) * 5 (settings) = 1750 + 228350 (Envelope data start point) = 230100 (Tune notes data start point)
    {
      for (byte ii = 0; ii < 5; ii++) // 5 settings:
      {
        Envelope[i][ii] = dueFlashStorage.read(228350 + (tune * 35) + (i * 5) + ii); // (tune * 35 is for 7 instruments * 5 settings) // read 5 settings of each instrument for selected tune: attack rate, decay delay time, decay rate, sustain level, release rate
      }
    }
    uint16_t limit = 256;
    if (tune >= 25) limit = 1024;
    if (tune < 25) address = (tune * 256) + 230100;  // saved Notes data for 1st 25 tunes (25 * 256) + 230100 = 236500
    else   address = ((tune - 25) * 1024) + 236500; // saved Notes data for last 25 tunes (25 * 1024) + 236500 = 262100
    for (uint16_t i = 0; i < limit; i++) // read music notes:
    {
      uint8_t musicByt = dueFlashStorage.read(address + i);
      if (musicByt == 255) break; // if end of tune
      MusicNotes[i] = musicByt;
 //     Serial.print("musicByt "); Serial.print(musicByt); Serial.print("  MusicNotes[i] "); Serial.println(MusicNotes[i]); //;
      NotesCount = i + 1;
    }
  }
  else // if (tune == -1) read Serial from GUI
  {
    unsigned long del = millis();
    while (Serial.available() == 0 && millis() - 10 < del) delayMicroseconds(10); // if next serial char not arrived yet, wait up to 10 mSecs for serial to become available. if still no serial, continue in case it was the last char sent
    Tempo = Serial.read(); // Tempo was sent as a 0-255 byte representing 15-270 (15 is added back later)
    del = millis();
    while (Serial.available() == 0 && millis() - 10 < del) delayMicroseconds(10); // if next serial char not arrived yet, wait up to 10 mSecs for serial to become available. if still no serial, continue in case it was the last char sent
    LinkedPreset = Serial.read(); // 255 = tune empty // 0 = indicates not empty & not linked to preset // 1 to 50 = preset number linked to
    while (Serial.available() == 0 && millis() - 10 < del) delayMicroseconds(10); // if next serial char not arrived yet, wait up to 10 mSecs for serial to become available. if still no serial, continue in case it was the last char sent
    Instrument = Serial.read(); // 0 = wave, 1 = piano, 2 = guitar, etc  
    // read Envelope data - 5 envelope settings of each instrument for each tune stored:
    for (byte i = 0; i < 7; i++) // 7 (instruments) * 50 (tunes) * 5 (settings) = 1750
    {
      for (byte ii = 0; ii < 5; ii++) // 5 settings:
      {
        del = millis();
        while (Serial.available() == 0 && millis() - 10 < del) delayMicroseconds(10); // if next serial char not arrived yet, wait up to 10 mSecs for serial to become available. if still no serial, continue in case it was the last char sent
        Envelope[i][ii] = Serial.read(); // read 5 settings of each instrument for selected tune: attack rate, decay delay time, decay rate, sustain level, release rate
      }
    }
    del = millis();
    while (Serial.available() == 0 && millis() - 10 < del) delayMicroseconds(10); // if next serial char not arrived yet, wait up to 10 mSecs for serial to become available. if still no serial, continue in case it was the last char sent
    while (Serial.available() > 0)
    {
      MusicNotes[NotesCount] = Serial.read();
      NotesCount++;
      del = millis();
      while (Serial.available() == 0 && millis() - 10 < del) delayMicroseconds(10); // if next serial char not arrived yet, wait up to 10 mSecs for serial to become available. if still no serial, continue in case it was the last char sent
    }
    del = millis();
    while (Serial.available() == 0 && millis() - 10 < del) delayMicroseconds(10); // if next serial char not arrived yet, wait up to 10 mSecs for serial to become available. if still no serial, continue in case it was the last char sent
    if (Serial.available() > 0) Serial.readString(); // clear serial
  }
//  for (int i = 0; i < NotesCount; i++) { Serial.print("  MusicNotes[i] "); Serial.println(MusicNotes[i]); } //;
  LoadedTune = tune + 1;
  if (Tempo != oldTempo) TC_setup6();
  for (byte instrument = 0; instrument < 7; instrument++) { AttackRate[instrument] = pow(1.17, Envelope[instrument][0]); }
  DecayDelay = Envelope[Instrument][1] * 200;
  if (Instrument == 0)
  {
    TC_setup2a();
    InterruptMode = 2; // wave
    DecayRate = Envelope[Instrument][2];
    SetWaveFreq(0);
  }
  else
  {
    TC_setup2c(); // set voice 1 pitch
//    if (Instrument >= 6 && Instrument <= 9) TC_setup7(); // set up voice 2 pitch // not used // if (Instrument + 3 >= 4 && Instrument + 3 <= 9) TC_setup7();
//    else NVIC_DisableIRQ(TC3_IRQn); // disable voice 2 pitch
    InterruptMode = 3; // use minisoundfonts
  }
  NVIC_EnableIRQ(TC0_IRQn); // ensure interrupts running (to play the tune)
// received bytes:
// note or rest length 3 - 16, when it changes.
// velocity 154 - 159.
// legato: 149. slide: 150. end: 151 before note.
// staccato dot: 153 before note.
// loud: 161.
// midi notes 5 octaves: 36 - 61. FULL range: 17 - 124.
// rest 250.
//  Serial.print("Instrument "); Serial.println(Instrument);
//  Serial.print("Tempo: "); Serial.println(Tempo);
//  Serial.print("Envelope[Instrument][0] (attack) = "); Serial.println(Envelope[Instrument][0]);
//  Serial.print("Envelope[Instrument][1] (delay) = "); Serial.println(Envelope[Instrument][1]);
//  Serial.print("Envelope[Instrument][2] (decay) = "); Serial.println(Envelope[Instrument][2]);
//  Serial.print("Envelope[Instrument][3] (sustain) = "); Serial.println(Envelope[Instrument][3]);
//  Serial.print("Envelope[Instrument][4] (release) = "); Serial.println(Envelope[Instrument][4]);
//  Serial.print("InterruptMode "); Serial.println(InterruptMode);
}

void PlayTune(byte tune, char ch) // playing saved tune (not score) // tune = tune number, if char = 'l' then load Linked Preset first if not already loaded, if char = 'r' then load Linked Preset first regardless
{
  TimingCount = 65000; // will be set to zero at start of note playing
  if (PotsEnabled >= 2) PotsEnabled -= 2; // if pots enabled, then enabled switches and disable pots
  bool sMM = 1; // don't exit music mode
  if (StartupTune) { if (dueFlashStorage.read(226748) != 100) sMM = 0; } // stay in music mode after playing startup tune
  if (tune < 1 || tune > 50)
  {
    Serial.print("   Tune "); Serial.print(tune); Serial.println(" does not exist!\n");
    return;
  }
  if (InterruptMode < 2) MusicEnterExit(2); // enter music mode if not already enabled
  uint8_t linkedPreset = dueFlashStorage.read(tune - 1 + 226750); // 255 = tune empty // 0 = indicates tune not empty & not linked to a preset // 1 to 50 = preset number linked to
  if (dueFlashStorage.read((linkedPreset * 240) + 3) > 11) linkedPreset = 0; // if linked Preset is empty don't use it
  if (linkedPreset <= 50) // if Tune not empty
  {
    if (linkedPreset > 0 && ((LoadedPreset != linkedPreset && ch == 'l') || ch == 'r')) // load linked preset before playing if Linked Preset has changed & char = 'l' or if ch = 'r' for reload regardless
    {
      UserChars[2] = '!'; // indicates loading Defaults or Preset when changing WaveShape
      NVIC_DisableIRQ(TC0_IRQn); // stop interrupts running (to speed up loading settings)
      Settings(1, linkedPreset, sMM); // if Preset linked, read it from flash in Settings() // send to GUI if staying in music mode or not playing startup tune
      CreateWaveFull(10);
      UserChars[2] = ' '; // return to default
      LoadedPreset = linkedPreset; // remember last loaded Preset
      ReceiveNotes(tune - 1);
    }
    else if (tune != LoadedTune) ReceiveNotes(tune - 1);
    else Instrument = dueFlashStorage.read(tune + 226849); // (tune - 1) + 226850 = Instrument at start of tune // 0 = wave, 1 = piano, 2 = guitar, etc
    PlayNotes(0); // 0 = play from start
  }
  else { Serial.print("   Tune "); Serial.print(tune); Serial.println(" is empty!\n"); }
}

void PlayNotes(uint16_t playPos) // starting Play Position // playing score or saved tune
{
  if (UsingGUI) { Serial.print("Playing "); Serial.println(Instrument); delay(150); } // delay to improve sync with PlayProgress display in GUI
  else Serial.println("   Playing");
  Play = 1;
  PeakLevel = 65535; // reset
  Velocity = 50; // mf = 50 / 100 default start level
  Legato = 0; // reset
  FadeOut = 1;
  SlurFilter = 0;
  SlurStrength = 10000; // strength of low pass filtering at any moment for slurred notes. 10000 = no filtering
  byte     staccato = 1; // 2 or 3 = staccato on, Resets to 1 after note
//  NoteLength = 24; // 96 = whole note, 48 = 1/2 note, 24 = 1/4 note, 12 = 1/8 note, 6 = 1/16 note
  uint16_t addToNoteLength = 0; // add lengths if Tied note
  byte     note = 0; // what pitch in the scale the read note is: 0 = C, 1 = D --- 6 = A, 7 = B
  int8_t   legatoVel = 0;
  int16_t  cresLen = -1; // Crescendo length
  byte     cresStartVel = 0; // crescendo start Velocity
  CresEndVel = 0; // reset crescendo end Velocity
  uint16_t cresStartPos = 5000; // crescendo start PlayPos
  uint16_t tieStartPos = 5000; // tie start PlayPos
  byte     remCresStNoteLength = 0; // remember NoteLength at crescendo start
  unsigned long slideEndTime[100]; // slide note segment time
  unsigned long cresStartTime = 0;
  unsigned long cresEndTime = 0;
  unsigned long cresDelay = 0; // if crescendo starts during tied note, delay start of cres after start of note
  int16_t  slideStartPos = 0; // slide start PlayPos
  byte     lastSlideMusicByt = 0;
  uint16_t slideDivisor[100];
  slideDivisor[0] = 0;
  float    slideWaveFreq[100];
  slideWaveFreq[0] = 0;
  byte     slideEnd = 0;
  byte     slideSeg = 0; // Slide segment number
  uint16_t slidePlayPos[100]; // Slide length
  uint16_t slideLen[100]; // Slide length
  float    slideFreqIncr[100]; // slide wave freq increment
  int16_t  slideEndPos = 0;
  bool     readNoteLen = 0; //
  byte     remNoteLen = 0; // remember NoteLength at slide end
  byte     comp = 0; // LP filter compensation at low Velocity piano
  byte     instrument = Instrument;
  byte     oldInstrument = Instrument;
  if (InterruptMode == 3 && Instrument == 1) comp = 3; // compensate for LP filter used at low velocity piano
  MusicByt = 0;
  byte lastNote = 0; // previous note
  byte loud = 0;
  byte velocity = Velocity;
  byte remVelocity = Velocity;
  bool skipMid = 0;
  bool sMM = 1; // Stay in Music Mode
  bool sync = 0; // sync GUI play progress
  uint16_t i = 0;
  if (playPos == 0) i = 1; // if playing from the start
  if (StartupTune) { if (dueFlashStorage.read(226748) != 100) sMM = 0; } // stay in music mode after playing startup tune
  Serial.print("   'Note' Count = "); Serial.println(NotesCount);
  while (Play && playPos < NotesCount) // while playing tune
  {
 //   Serial.print("p "); Serial.print(playPos); Serial.print(" M "); Serial.println(MusicNotes[playPos]);
    if (i > 0) MusicByt = MusicNotes[playPos]; // read byte at current play position if playing from the start or after finding latest note length, etc (next line)
    else // if (i == 0 && playPos > 0) // if not playing from the start, find latest note length, instrument, velocity and legato state
    {
      byte leg = 0;
      byte sli = 0;
      for (i = 0; i <= playPos; i++)
      {
        if (Legato == 3) Legato = 0; // none
        if      (MusicNotes[i] <= 16) MusicByt = MusicNotes[i]; // set latest note length
        else if (MusicNotes[i] >= 140 && MusicNotes[i] <= 147) instrument = MusicNotes[i] - 140; // set new instrument
        else if (MusicNotes[i] >= 154 && MusicNotes[i] <= 159) velocity = 100 * pow(0.71, 159 - MusicNotes[i]); // set latest velocity
        else if (MusicNotes[i] == 149) leg = 1; // set latest Legato
        else if (MusicNotes[i] == 150) sli = 1; // set latest slide
        else if (MusicNotes[i] == 151) { leg = 3; sli = 0; } // set latest Legato
        else if (MusicNotes[i] >= 17 && MusicNotes[i] <= 124) // if MusicByt is a note // (17 - 124 is FULL range)
        {
          if      (leg == 1) Legato = min(2, Legato + 1 + sli); // 1st or following slurred notes (before last one)
          else if (leg == 3 && Legato > 0) { Legato = 3; leg = 0; } // last slurred note
          else if (sli == 1) Legato = 0; // slide
        }
      }
      playPos--; // go back 1 position to read the note length value in switch/case code below
    }
    switch (MusicByt)
    {
      case 3: // 0 - 2 reserved for 1/32 notes (but NoteLength would have to be based on whole note equal to 192 TempoCounts instead of 96)
        NoteLength = 4; // 1/16 triplet
        break;
      case 4:
        NoteLength = 6; // 1/16
        break;
      case 5:
        NoteLength = 8; // 1/8 triplet
        break;
      case 6:
        NoteLength = 9; // 1/16 50% extra
        break;
      case 7:
        NoteLength = 12; // 1/8
        break;
      case 8:
        NoteLength = 16; // 1/4 triplet
        break;
      case 9:
        NoteLength = 18; // 1/8 50% extra
        break;
      case 10:
        NoteLength = 24; // 1/4
        break;
      case 11:
        NoteLength = 32; // 1/2 triplet
        break;
      case 12:
        NoteLength = 36; // 1/4 50% extra
        break;
      case 13:
        NoteLength = 48; // 1/2
        break;
      case 14:
        NoteLength = 72; // 1/2 50% extra
        break;
      case 15:
        NoteLength = 96; // whole
        break;
      case 16:
        NoteLength = 144; // whole 50% extra
        break;
      case 154:
        if (CresEndVel == 0) velocity = 18 + comp; // pp = 18 / 100 // -15dB // comp = compensation for LP filter used at low velocity if piano // if de/cresendo not in progress
        break;
      case 155:
        if (CresEndVel == 0) velocity = 25 + comp; // p = 25 / 100 // -12dB // comp = compensation for LP filter used at low velocity if piano
        break;
      case 156:
        if (CresEndVel == 0) velocity = 35 + comp; // mp = 35 / 100 // -9dB // comp = compensation for LP filter used at low velocity if piano
        break;
      case 157:
        if (CresEndVel == 0) velocity = 50 + min(1, comp); // mf = 50 / 100 // -6dB  default start level // comp = compensation for LP filter used at low velocity if piano
        break;
      case 158:
        if (CresEndVel == 0) velocity = 71 + min(1, comp); // f = 71 / 100 // -3dB // comp = compensation for LP filter used at low velocity if piano
        break;
      case 159:
        if (CresEndVel == 0) velocity = 100; // ff = 100 / 100 // 0dB
        break;
    }
    if (cresLen >= 0 || MusicByt == 160 || MusicByt == 162) // if start of crescendo / decrescendo find its length before proceeding to play it:
    {
      playPos++; // move to next PlayPos
      if (cresStartVel == 0)
      {
        if (cresStartPos == 5000)
        {
          cresLen = 0; // -1 when reset
          cresStartPos = playPos - 1; // remember PlayPos at de/crescendo start
        }
        else if (MusicByt <= 124) // the note length data or note where de/crescendo starts
        {
          remCresStNoteLength = NoteLength; // remember NoteLength at crescendo start
          cresStartVel = velocity; // remember velocity at de/crescendo start - used to calculate velocity while playing crescendo or decrescendo below
          velocity = 0; // until velocity MusicByt is read at the end of the de/crescendo
          cresLen += NoteLength; // add note or rest length to de/crescendo length
          if (MusicByt <= 16) readNoteLen = 1; // if note length data
          else readNoteLen = 0; // if note
        }
        continue;
      }
      else if (MusicByt <= 124 || MusicByt == 250) // if note length, note or rest
      {
        if (velocity == 0) // before end of crescendo
        {
          if (MusicByt <= 16) // if note length
          {
            cresLen += NoteLength; // add note or rest length to de/crescendo length
            readNoteLen = 1;
          }
          else // if note or rest
          {
            if (readNoteLen == 0) cresLen += NoteLength; // add note or rest length to de/crescendo length, if note or rest & if note length not read since last note or rest
            readNoteLen = 0;
          }
        }
        else // if velocity MusicByt was just read, this is where the de/crescendo ends
        {
 //         Serial.print("cresLen = "); Serial.println(cresLen);
          NoteLength = remCresStNoteLength; // restore NoteLength at start of crescendo
          CresEndVel = velocity; // used to calculate velocity while playing crescendo or decrescendo below
          velocity = cresStartVel; // restore // cresStartVel used to calculate velocity while playing crescendo or decrescendo below
          playPos = cresStartPos; // return to next PlayPos after de/crescendo start - ready to play it
          cresEndTime = (cresLen * 2500 / (Tempo + 15)) - 100; // ends 100mS before next note so another crescendo can follow on its heels
          cresLen = -1; // reset
        }
        continue;
      }
      else continue; // don't pass this point until length of crescendo / decrescendo is found
    }
    if (MusicByt <= 16)
    {
      if (addToNoteLength > 0)
      {
        for (uint16_t i = playPos; playPos > 2 && (MusicNotes[i - 1] == 160 || MusicNotes[i - 1] == 162 || MusicNotes[i - 1] == 151); i--) // check if crescendo starts part way thru tied note
        {
          if (MusicNotes[i - 1] == 160 || MusicNotes[i - 1] == 162) // if crescendo starts part way thru tied note
          {
            cresDelay = addToNoteLength * 2500 / (Tempo + 15); // delay start of cresendo after start of tied note
            if (cresStartVel > 0) remVelocity = velocity;
            break;
          }
        }
        if (MusicNotes[playPos - 1] == 160 || MusicNotes[playPos - 1] == 162) // if crescendo starts part way thru tied note
        {
          cresDelay = addToNoteLength * 2500 / (Tempo + 15); // delay start of cresendo after start of tied note
          if (cresStartVel > 0) remVelocity = velocity;
        }
        NoteLength += addToNoteLength;
        addToNoteLength = 0;
        if (cresStartPos < 5000 && tieStartPos < cresStartPos) velocity = remVelocity; // needed if cres is inside tied note
      }
      if (MusicNotes[playPos] <= 16) // && (MusicNotes[playPos + 1] <= 16 || (((MusicNotes[playPos + 1] >= 154 && MusicNotes[playPos + 1] <= 162) && (MusicNotes[playPos + 2] <= 16 || (MusicNotes[playPos + 2 == 151] && MusicNotes[playPos + 3] <= 16)))))) // if current MusicByt and next MusicByt are both note length data, possibly with velocity or cresendo start & legato end between them
      {
        for (uint16_t i = playPos; MusicNotes[i + 1] <= 16 || MusicNotes[i + 1] == 151 || (MusicNotes[i + 1] >= 154 && MusicNotes[i + 1] <= 162); i++)
        {
          if (MusicNotes[i + 1] <= 16)
          {
            if (tieStartPos == 5000) tieStartPos = playPos; // tieStartPos is reset to 5000 just before note is played
            addToNoteLength = NoteLength;
            break;
          }
        }
      }
    }
    if (slideStartPos > 0 || MusicByt == 150) // slide start (Legato = 3) // find length of slide:
    {
      playPos++; // move to next PlayPos
      if (slideDivisor[0] == 0 && slideWaveFreq[0] == 0)
      {
        if (slideStartPos == 0) slideStartPos = playPos; // remember next PlayPos after slide start
        if (MusicByt >= 17 && MusicByt <= 124) // the note where slide starts
        {
          remVelocity = velocity;
          slideLen[1] = slideLen[2] = NoteLength; // set 1st note length to slide length
          slidePlayPos[1] = playPos; // used if crescendo starts after 1st slide note
          slideSeg = 2; // end of 1st slide segment & 2nd play position
          if (instrument == 0) slideWaveFreq[0] = slideWaveFreq[1] = FindNoteWaveFreq(MusicByt); // if Wave selected - used to calculate TargetWaveFreq while playing
          else // if (InterruptMode == 3)
          {
            lastSlideMusicByt = MusicByt;
            slideDivisor[0] = slideDivisor[1] = FindNoteDivisor(MusicByt, instrument); // if minisoundfont instrument selected - used to calculate NoteDivisor while playing
          }
        }
        continue;
      }
      else if (MusicByt <= 16) // if note length data
      {
        slideLen[slideSeg] += NoteLength; // add note length to slide length
        slidePlayPos[slideSeg] = playPos;
        continue;
      }
      else if (MusicByt == 149 || MusicByt == 151) // if 149 Legato = 1 || if 151 Legato = 3 or 4 (slide is 150)
      {
        if (MusicByt == 149) slideEnd = 2; // if 149 Legato = 1 || if 151 Legato = 3 or 4 (slide is 150)
        else slideEnd = 4;
        continue;
      }
      else if (MusicByt >= 17 && MusicByt <= 124) // if note
      {
        if (instrument == 0) slideWaveFreq[slideSeg] = FindNoteWaveFreq(MusicByt); // if Wave selected - used to calculate TargetWaveFreq while slide is playing
        else // if (instrument >= 1) // if minisoundfont instrument selected - used to calculate NoteDivisor while slide is playing
        {
          slideDivisor[slideSeg] = slideDivisor[slideSeg - 1];
          if (MusicByt != lastSlideMusicByt) // if pitch changed
          {
            if (MusicByt > lastSlideMusicByt) // reduce noteDivisor faster than using "power of" with a float: 0.943877551 ^ 12 = 0.500020586
            {
              byte finish = MusicByt - lastSlideMusicByt;
              for (int i = 0; i < finish; i++) { slideDivisor[slideSeg] = slideDivisor[slideSeg] * 185 / 196; }
            }
            else if (MusicByt < lastSlideMusicByt) // increase noteDivisor // 196 / 185 = 1.059459459, 1.059459459 ^ 12 = 1.99991765
            {
              byte finish = lastSlideMusicByt - MusicByt;
              for (int i = 0; i < finish; i++) { slideDivisor[slideSeg] = slideDivisor[slideSeg] * 196 / 185; }
            }    
            lastSlideMusicByt = MusicByt;
          }
        }
 //;;;       byte ts = 24; // time signature
 //;;;       if (TimeSig > 6) ts = 12;
        if      (instrument == 0) slideFreqIncr[slideSeg] = (slideWaveFreq[slideSeg] - slideWaveFreq[slideSeg - 1]) / (slideLen[slideSeg] * 4000 / (Tempo + 15) / 24); //;;; / ts): // 4000 for 15mS updates (60 Sec / 15mS)
        else if (instrument >= 1) slideEndTime[slideSeg] = slideLen[slideSeg] * 2500 / (Tempo + 15); // calculate note 'end' time
        if (slideSeg > 2) slideLen[slideSeg] += slideLen[slideSeg - 1]; // (2 = 1st segment) // add previous segment to produce running total for next segment's calculation
        slidePlayPos[slideSeg] = playPos;
        if (slideEnd > 0) // if slideEnd, the slide finishes at the end of this note
        {
          remNoteLen = NoteLength; // remember NoteLength
          NoteLength = slideLen[slideSeg]; // all segments play in 1 note
          slideLen[0] = slideSeg; // remember number of segments
          slideSeg = 1; // start playing from slide segment 1
          slideEndPos = playPos - 1;
          playPos = slideStartPos; // return to next PlayPos after slide start - ready to play it
          slideStartPos = -1; // -1 indicates 1st slide note
          slideDivisor[0] = slideWaveFreq[0] = 0;
          velocity = remVelocity; // restore velocity from start of slide in case it changed
        }
        else if (slideSeg < 100) // if less than max number of segments
        {
          slideSeg++; // new slide segment
          slideLen[slideSeg] = 0;
        }
        continue;
      }
      else continue; // don't pass this point until length of slide is found
    }
    switch (MusicByt)
    {
      case 140:
      case 141:
      case 142:
      case 143:
      case 144:
      case 145:
      case 146:
      case 147:
        instrument = MusicByt - 140; // set new instrument
        break;
      case 149:
        Legato = 1; // legato start (1 = 1st slurred note)
        break;
  //    case 150: // read later
  //      break;
      case 151:
        Legato = 3; // legato / slide end - add 2 before last legato / slide note. So 3 = last legato note, 4 = last slide note (freq slides reach the last note's freq at the end of the last note). Reset to 0 after next (last) note
        break;
      case 153:
        staccato = 2; // staccato
        break;
      case 161: // loudness
        loud = min(100 - Velocity, velocity * 36 / 100); // increases volume by 2.5dB when added to Velocity (42 / 100 = 3dB)
        Velocity = velocity + loud; // Velocity used in interrupt handler
        break;
      case 250: // rest
        Rest = 1; // rest length determined by latest NoteLength
        MusicByt = lastNote;
        TempoCount = 0;  // prepare to start 'playing' rest
        while (TempoCount < NoteLength)
        {
   //       delay(1); // wait while rest time passes. Current note is released immediately (in interrupts)
          if (!digitalRead(8) || Serial.available() > 1) // stop playing:
          {
            if (!digitalRead(8)) { TempoCount = NoteLength; Play = 0; } // stop playing
            else
            {
              byte ch = Serial.read();
              if (ch == '<') { TempoCount = NoteLength; Play = 0; } // stop playing
              else if (ch == 'm' && Serial.read() == '2') sMM = 1; // if Music Window opened while Start-up tune playing, Stay in Music Mode
            }
          }
        }
        for (i = playPos + 1; i < NotesCount; i++)
        {
          if (MusicNotes[i] == 250) break; // if next 'note' is a rest, continue with normal note release
          else if (MusicNotes[i] >= 17 && MusicNotes[i] <= 124) // full range
          {
            FadeOut = 1; // prevents note from ending abruptly (before next note) creating a click when NoteDivisor & NoteFreqBand set at start of next note
            break;
          }
        }
        Rest = 0; // reset
        playPos++;
        if (cresEndTime > 0 && millis() >= cresEndTime)
        {
          velocity = CresEndVel;
          cresStartPos = 5000; // reset
          cresStartVel = CresEndVel = cresStartTime = cresEndTime = cresDelay = 0; // reset
        }
        continue;
    } // end of switch (MusicByt)
    if (UsingGUI && playPos % 30 == 0) sync = 1;
    if (MusicByt >= 17 && MusicByt <= 124) // if MusicByt is a note // (17 - 124 is FULL range)
    {
      TempoCount = 0; // prepare to start playing note // start counting before following short delay to compensate for it
      if (sync)
      {
        Serial.println("^"); // sync play progress in GUI
        sync = 0;
      }
      tieStartPos = 5000;
      skipMid = 0;
      note = MusicByt % 12; // calculate note from note number (A, B, C, etc - C = 0, C# = 1, D = 2, etc)
      bool withinLimits = 0;
      if (instrument == 1) // if minisoundfont instrument piano selected
      {
        if (MusicByt >= 36 && MusicByt <= 96) withinLimits = 1; // C2 to C7
      }
      else if (instrument == 2) // if minisoundfont instrument guitar selected
      {
        if (MusicByt >= 40 && MusicByt <= 88) withinLimits = 1; // E2 to E6
      }
      else if (instrument == 3) // if minisoundfont instrument marimba selected
      {
        skipMid = 1;
        if (MusicByt >= 40 && MusicByt <= 96) withinLimits = 1; // E2 to C7
      }
      else if (instrument >= 4) // if minisoundfont instrument trumpet, sax or violin selected
      {
        if      (instrument == 4) { if (MusicByt >= 42 && MusicByt <= 89) withinLimits = 1; } // trumpet: F#2 to F6
        else if (instrument == 5) { if (MusicByt >= 36 && MusicByt <= 89) withinLimits = 1; } // sax: C2 to F6
        else if (                       MusicByt >= 54 && MusicByt <= 96) withinLimits = 1;   // violin: F#3 to C7
        if (withinLimits) Vibrato = 1;
      }
//      TempoCount = 0; // prepare to start playing note // start counting before following short delay to compensate for it
      int slurMax = 0; // max slur filter strength for end of last note
      uint8_t fade = 0; // slur filter fade-in time
      int slurMax2 = 0; // max slur filter strength for start of current note
      uint8_t fade2 = 0; // slur filter fade-out time
      uint8_t slurDiffIns = 0; // slur filter difference for different instruments - 0 is for sax & violin // a higher figure gives less filtering
      uint16_t slurIncr = 0; // increments to reach max slur filter strength
      uint16_t filtStren = 0; // increases slur filter strength if note pitches are further aapart
      if (SlurFilter) // && Instrument != 2)
      {
        if      (Instrument == 0) slurDiffIns = 20; // wave needs less slur filtering
        else if (Instrument == 1) slurDiffIns = 50; // piano needs very little slur filtering
        else if (Instrument <= 4) slurDiffIns = 10; // guitar, marimba & trumpet need slightly less slur filtering
        filtStren = 200 - slurDiffIns + min(12, abs(lastNote - MusicByt) * 2); // increase slur filter strength if note pitches are further apart
  //      slurMax = (pow(1.05, filtStren - lastNote) + 50) * 200; // max slur filter strength for PREVIOUS slurred note which is ending now // pow(1.059464, filtStren - lastNote) + 50) * 200;
  //      fade = constrain(pow(1.05, 165 - lastNote), 1, 120); // slur filter fade-in time for PREVIOUS slurred note which is ending now
  //      slurMax2 = (pow(1.05, filtStren - MusicByt) + 50) * 200; // max slur filter strength for next / CURRENT note. Global variable SlurMax will be set at note start (below)
  //      fade2 = constrain(pow(1.05, 165 - MusicByt), 10, 120); // slur filter fade-in time for next / CURRENT slurred note
  //    instead of using "pow(" code as above, use the for loop code below which is MUCH quicker
        uint8_t lastFilterStrength = filtStren - lastNote;
        uint8_t lastFilterSpeed = 165 - lastNote;
        uint8_t nextFilterStrength = filtStren - MusicByt;
        uint8_t nextFilterSpeed = 165 - MusicByt;
        uint8_t inc = 105; // use (int) 105 instead of (float) 1.05 then divide by 100 in for loop code below which is MUCH quicker
        int cal = inc;
        for (uint8_t i = 0; i < 200; i++) // instead of using "pow(" code as above, this for loop code is MUCH quicker
        {
          cal = round(cal * inc / 100); // increase cal by the power of i
          if (i == lastFilterStrength) { slurMax   = ((cal / 100) + 50) * 200; if (fade > 0 && slurMax2 > 0 && fade2 > 0) break; } // calculate max slur filter strength for PREVIOUS slurred note which is ending now - break if other 3 calculations have finished
          if (i == lastFilterSpeed)    { fade  = constrain(cal / 100, 1, 120); if (slurMax > 0 && slurMax2 > 0 && fade2 > 0) break; } // calculate slur filter fade-in time for PREVIOUS slurred note which is ending now - used to calculate fade-in increment "slurIncr"
          if (i == nextFilterStrength) { slurMax2  = ((cal / 100) + 50) * 200; if (slurMax > 0 && fade > 0 && fade2 > 0) break; } // calculate max slur filter strength for next / CURRENT slurred note. Global variable SlurMax will be set at start of note below
          if (i == nextFilterSpeed)    { fade2 = constrain(cal / 100, 1, 120); if (slurMax > 0 && fade > 0 && slurMax2 > 0) break; } // calculate slur filter fade-out time for next / CURRENT slurred note - used to calculate fade-out increment "slurIncr." Global variable SlurIncr will be set at start of note below
        }
        SlurStrength = 10000; // start point with no filtering
        slurIncr = (slurMax - 10000) / fade; // number of increments to reach max slur filter strength at 16mSecs in following for loop
      }
      for (uint8_t i = 0; i < 125; i++) // 25mSecs // allow time for FadeOut between notes, to prevent clicks; or for filtering if between slurred notes
      {
        unsigned long del = micros();
        while (micros() - 200 < del) ; // delayMicroseconds(10); // 200 microseconds * 125 = 25mSecs
        if (SlurFilter && i > 124 - fade) // if end of slurred note increase low pass filtering:
        {
          SlurStrength = constrain(SlurStrength + slurIncr, 10000, slurMax); // if end of slurred note increase low pass filtering:
          if (Instrument == 6) // violin
          {
            SlurVioFade = 1;
            if (i % 5 == 0) Modulation = map(SlurStrength, 10000, slurMax, ModulationCalc / 16, ModulationCalc / 32); //' if end of slurred violin note fade modulation by 50% to match quiet start of next note
          }
        }
      }
      if (SlurFilter) // && Instrument >= 4) // calculate slur increment value for next note, which will be updated just before it starts
      {
        slurIncr = (slurMax2 - 10000) / fade2 / 4; // up to 500 increments to reduce slur filter strength to normal using larger numbers to calculate slur filter strength using TimingCount
        if      (instrument == 1) slurIncr *= 4; // if piano reduce filter strength much quicker
        else if (instrument == 3) slurIncr *= 8; // if marimba or violin reduce filter strength MUCH quicker
      }
      lastNote = MusicByt; // allows note to continue (while being released) during Rest, if Rest follows current note and other MusicByts are in between - also used for slurred notes
      if (instrument >= 1 && withinLimits) // if minisoundfont instrument selected and note is within the instrument's limits - could be changing to new instrument
      {
  //      if (instrument == 1 || instrument == 2) DecayRate = min(100, Envelope[instrument][2] + min(50, (pow(1.09, MusicByt - 36) / 5))); // if piano or guitar // makes higher notes decay faster (increase 1.09 & 5 for more effect) practise in calculator with: Envelope[Instrument][2] + (1.09^(60 - 36)) ÷ 5 (where 60 - 36 is the note number between 0 & 60)
  //      else DecayRate = Envelope[instrument][2];
        NoteFreqBand = min(9, (MusicByt - 36) / 6);
        NoteDivisor = FindNoteDivisor(MusicByt, instrument);
        slideStartPos = 0; // -1 indicates 1st slide note
        uint16_t cycleLen = CycleLen;
    //  read config data: [Instrument - 1] [note freq band] [config data: 0 = number of samples in start chunk, 1 = number of samples in single cycle, 2 = number of samples from startCycle beginning to midCycle beginning, 3 = number of samples from midCycle beginning to endCycle beginning]
        StartBegin = pgm_read_word_near(&MsfConfig[instrument - 1][NoteFreqBand][0]); // read start chunk length / startCycle begin
        CycleLen   = pgm_read_word_near(&MsfConfig[instrument - 1][NoteFreqBand][1]); // read cycle length
        MidBegin   = pgm_read_word_near(&MsfConfig[instrument - 1][NoteFreqBand][2]); // midCycle begin
        EndBegin   = pgm_read_word_near(&MsfConfig[instrument - 1][NoteFreqBand][3]); // endCycle begin
        TC_setup2c(); // set sample rate with NoteDivisor
        if (instrument != oldInstrument || InterruptMode != 3) // change to new instrument OR back to old instrument after temporarily being out of limits
        {
//          if (instrument >= 6 && instrument <= 9) TC_setup7(); // set up voice 2 pitch // not used // if (instrument + 3 >= 4 && instrument + 3 <= 9) TC_setup7();
//          else NVIC_DisableIRQ(TC3_IRQn); // disable voice 2 pitch
          InterruptMode = 3; // use minisoundfonts
          DecayDelay = Envelope[instrument][1] * 200;
        }
        if ((instrument == 2 || instrument >= 4) && Legato > 1) NoteReadCycle = NoteReadCycle * CycleLen / cycleLen; // start reading next slurred note from same proportion of the way through the wave cycle
      }
      else if (instrument == 0 || !withinLimits) // if instrument Wave selected
      {
        TargetWaveFreq = FindNoteWaveFreq(MusicByt);
        slideStartPos = 0; // -1 indicates 1st slide note
        SetWaveFreq(0); // 0 = don't display freq
        if (instrument != oldInstrument || InterruptMode != 2) // change to new instrument
        {
          TC_setup2a(); // set up analogue slow mode timing at half of normal sample rate (skip CreateWaveFull(3) at quarter sample rate)
          InterruptMode = 2; // wave
          DecayDelay = Envelope[instrument][1] * 200;
          DecayRate = Envelope[instrument][2];
          WaveBit = 2147483648; // start wave from zero crossing point (4294967296 ÷ 2)
        }
      }
      if (instrument > 0)
      {
        if (instrument <= 2) DecayRate = min(100, Envelope[instrument][2] + min(50, (pow(1.09, MusicByt - 36) / 5))); // if piano or guitar (& possibly out of limits) // makes higher notes decay faster (increase 1.09 & 5 for more effect) practise in calculator with: Envelope[Instrument][2] + (1.09^(60 - 36)) ÷ 5 (where 60 - 36 is the note number between 0 & 60)
        else DecayRate = Envelope[instrument][2];
      }
      oldInstrument = Instrument = instrument;
      if (SlurFilter) // && Instrument != 2)
      {
        SlurMax = slurMax2; // update max slur filter strength for new note
        SlurStrength = slurMax2; // * mult; // set SlurStrength to max slur filter strength for new note
        SlurIncr = slurIncr; // update increments to reach min slur filter strength, starting from max - for new note
      }
      else if (Instrument != 2 || Legato < 2) ModulationCalc = 0; // if not between Legato notes
      if (loud == 0)  Velocity = velocity; // if note is set to normal loudness
      if (Instrument < 4 && Legato > 0) // 1 = 1st slurred note, 2 = following slurred notes, 3 = last slurred note
      {
        legatoVel = min(100 - Velocity - loud, velocity * 10 / 100); // ~1dB of velocity
        if (Legato >= 2)
        {
          legatoVel = -legatoVel; // if not 1st legato note reduce velocity instead of increasing it. If last slurred note reduce volume by ~1dB
        }
        if (Legato == 2) legatoVel /= 3; // if 2nd or following slurred notes reduce volume by less: ~0.3dB
      }
      else legatoVel = 0;
      NotePlayLen = NoteLength / staccato;
      Velocity = velocity + legatoVel + loud; // Velocity used in interrupt handler // increases or decreases volume
      /*if (Legato < 2 || Instrument != 2)*/ PeakLevel = 65535 * Velocity / 100; //;;
      int8_t vibDeviation = 0;
      int8_t vibDirection = 5; // vibrato direction (up or down)
      uint16_t noteDivisor = NoteDivisor;
      uint16_t incDiv = 350; // reduce number for faster vibrato
      if (Instrument == 5) incDiv = 380; // slower vibrato for sax
      uint8_t increment = NoteDivisor / incDiv;
      uint8_t limit = NoteDivisor / 100; // vibrato deviation limit - adjustable later in sketch
      unsigned long updateTime = millis();
      if (Instrument >= 4 && slideSeg == 0) updateTime = millis() + 200;
      unsigned long vibTime = millis() + 200; // vibrato
      unsigned long slideStartTime = millis();
      slideEndTime[2] += millis();
      unsigned long cresUpdateTime = cresDelay +  millis(); // cresDelay is delay from start of note, if crescendo starts during a tied note
      slideEndTime[1] = millis();
      if (Legato < 2 || Instrument < 2 || Instrument == 3) NoteReadTotal = NoteReadCycle = 0; // set to zero at start of note playing if not Legato; or if Legato & wave, piano or marimba
      else if (Instrument == 2) NoteReadTotal = StartBegin + (NoteReadCycle * int(MidBegin / NoteReadCycle / 5)); // StartBegin + NoteReadCycle; // if Legato >= 2 & guitar
      if (Instrument < 4) Vibrato = 0;
      if (SlurFilter) // && Instrument != 2)
      {
        if (Instrument == 6) NoteReadTotal = StartBegin + NoteReadCycle; // if instrument is violin
        else if (Instrument >= 4) NoteReadTotal = StartBegin + MidBegin; // if minisoundfont instrument trumpet or sax selected & we are between slurred notes
      }
 //     TimingCount = 0; // start playing note
      EnvelopeDivisor = 2100; // set envelope clock to 20kHz for attack period
      TC_setup8(); // set envelope clock to 20kHz for attack period
      SkipMid = skipMid;
      FadeOut = 0; // disable when new note starts // prevents note from ending abruptly (between notes) creating a click when NoteDivisor & NoteFreqBand set at start of next note
      TimingCount = 0; // start playing note
      while (TempoCount < NoteLength) // while note plays (played by interrupts)
      {
        if (slideSeg > 0) // if sliding note
        {
          if (millis() - 15 >= updateTime) // if sliding note & time to update freq
          {
            updateTime = millis();
            if (InterruptMode == 2)
            {
              if (TempoCount < slideLen[max(2, slideSeg)]) TargetWaveFreq += slideFreqIncr[max(2, slideSeg)];
              else TargetWaveFreq = slideWaveFreq[max(2, slideSeg)];
              SetWaveFreq(0); // 0 = don't display freq
            }
            else if (InterruptMode == 3)
            {              
              uint16_t nDiv = map(millis(), slideEndTime[max(1, slideSeg - 1)], slideEndTime[max(2, slideSeg)], slideDivisor[max(1, slideSeg - 1)], slideDivisor[max(2, slideSeg)]);
              NoteDivisor = constrain(nDiv, min(slideDivisor[max(1, slideSeg - 1)], slideDivisor[max(2, slideSeg)]),max(slideDivisor[max(1, slideSeg - 1)], slideDivisor[max(2, slideSeg)]));
              NoteDivSync = 1; // flag to trigger synchronous update of NoteDivisor / sample rate
            }
            if (TempoCount >= slideLen[slideSeg])
            {
              if (slideSeg < slideLen[0]) // if not finished slide // slideLen[0] = number of slide segments
              {
                playPos = slidePlayPos[slideSeg];
                slideSeg++;
                if (slideSeg > 2) 
                {
                  slideEndTime[slideSeg - 1] = millis();
                  slideEndTime[slideSeg] += millis();
                }
              }
     //         else // if (slideSeg >= slideLen[0]) // if slide finished // unlikely to be reached due to 25mS update interval, so code moved to just after note
     //         {
     //           playPos = slideEndPos;
     //           Serial.print("epp "); Serial.println(playPos);
     //           slideLen[0] = slideSeg = 0; // if finished slide // slideLen[0] = number of slide segments
     //         }
            }
          }
        }
        else if (Vibrato) // if not sliding note (sounds bad if sliding)
        {
          if (millis() - 12 > updateTime) // updates every 12 milliseconds
          {
            updateTime = millis();
            if      (vibDeviation >  limit) vibDirection = -increment;
            else if (vibDeviation < -limit) vibDirection =  increment;
            vibDeviation += vibDirection;
            uint16_t vibStrength = min(millis() - vibTime, 600); // gradually increases vibrato for 600 mSecs
            NoteDivisor = noteDivisor + (vibDeviation * vibStrength / 600); // reduce number for stronger vibrato
            NoteDivSync = 1; // flag to trigger synchronous update of NoteDivisor / sample rate
          }
        }
        if (playPos >= cresStartPos && millis() - 33 >= cresUpdateTime) // while playing crescendo or decrescendo:
        {
          if (cresStartTime == 0)
          {
            cresEndTime += millis();
            cresStartTime = millis();
          }     
          cresUpdateTime = millis();
          byte vel = map(millis(), cresStartTime, cresEndTime, cresStartVel, CresEndVel);
          velocity = constrain(vel, min(cresStartVel, CresEndVel),max(cresStartVel, CresEndVel));
          Velocity = min(100, velocity + legatoVel + loud); // Velocity used in interrupt handler // increases or decreases volume
          PeakLevel = 65535 * Velocity / 100;
          if (millis() >= cresEndTime)
          {
            velocity = CresEndVel;
            cresStartPos = 5000; // reset
            cresStartVel = CresEndVel = cresStartTime = cresEndTime = cresDelay = 0; // reset
          }
        }
        if (!digitalRead(8) || Serial.available() > 1) // stop playing:
        {
          if (!digitalRead(8)) { TempoCount = NoteLength; Play = 0; } // stop playing
          else
          {
            byte ch = Serial.read();
            if (ch == '<') { TempoCount = NoteLength; Play = 0; } // stop playing
            else if (ch == 'm' && Serial.read() == '2') sMM = 1; // if Music Window opened while Start-up tune playing, Stay in Music Mode
          }
        }
      } // end of note playing
      if (slideSeg > 0) // if sliding, finish it:
      {
        playPos = slideEndPos;
        slideLen[0] = slideSeg = 0; // if finished slide // slideLen[0] = number of slide segments
      }
      if (slideEnd > 0) // if finished note was a slide
      {
        NoteLength = remNoteLen;
        if (slideEnd == 2) Legato = 2;
        else Legato = 0; // slideEnd will be either 2: continue with slur, or 4: end legato
        slideEnd = slideLen[0] = slideSeg = 0; // if finished slide // slideLen[0] = number of slide segments
      }
      if (Legato > 0)
      {
        if (Legato > 2) Legato = 0; // reset after last legato note
        else if (Legato == 1) Legato = 2; // if 1st slurred note
        if (Legato == 2 && Instrument != 2) SlurFilter = 1; // begin tapering low pass filter (to run between adjacent slurred notes in interrupt handler)
      }
      if (Legato == 0)
      {
        for (i = playPos + 1; i < NotesCount; i++)
        {
          if (MusicNotes[i] == 250) break; // if next 'note' is a rest, continue with normal note release
          else if (MusicNotes[i] >= 17 && MusicNotes[i] <= 124) // full range
          {
            if (Instrument == 0) PeakLevel = ModulationCalc = Modulation * 16;
            FadeOut = 1; // prevents note from ending abruptly between notes, creating a click when NoteDivisor & NoteFreqBand set at start of next note
            break;
          }
        }
      }
      if (loud > 0) loud = 0; // if note was loud, return to normal loudness
      staccato = 1; // reset
    }
    playPos++; // when note has finished playing, move to next note
  }
  Play = 0;
  FadeOut = 0;
  if (!sMM) // exit music mode after playing startup tune:
  {
    while (Modulation > 50) { delay(100); } // wait for last note to fade out
    UserChars[2] = '!'; // indicates loading Defaults or Preset when changing WaveShape
    Settings(1, 0, 0); // reload start up default settings & don't send to GUI
    UserChars[2] = ' '; // return to default
    LoadedPreset = 0;
  }
  if (UsingGUI) { Serial.print("Stopped "); Serial.println(Instrument); }
  else Serial.println("   Stopped\n");
}

float FindNoteWaveFreq(byte musicByt)
{
  float targetWaveFreq;
  byte note = musicByt % 12;
  byte oct = (musicByt / 12) - 1; // calculate octave from note number
  if      (note ==  0) targetWaveFreq = 16.35 * pow(2, oct); //
  else if (note ==  1) targetWaveFreq = 17.32 * pow(2, oct); //
  else if (note ==  2) targetWaveFreq = 18.35 * pow(2, oct); //
  else if (note ==  3) targetWaveFreq = 19.45 * pow(2, oct); //
  else if (note ==  4) targetWaveFreq = 20.60 * pow(2, oct); //
  else if (note ==  5) targetWaveFreq = 21.83 * pow(2, oct); //
  else if (note ==  6) targetWaveFreq = 23.12 * pow(2, oct); //
  else if (note ==  7) targetWaveFreq = 24.50 * pow(2, oct); //
  else if (note ==  8) targetWaveFreq = 25.96 * pow(2, oct); //
  else if (note ==  9) targetWaveFreq = 27.50 * pow(2, oct); //
  else if (note == 10) targetWaveFreq = 29.14 * pow(2, oct); //
  else if (note == 11) targetWaveFreq = 30.87 * pow(2, oct); //
  return targetWaveFreq;
}
        
int FindNoteDivisor(byte musicByt, byte instrument)
{
  int noteDivisor;
  switch (musicByt)
  {
    case 36: noteDivisor = 2112; break; // play at recorded sample rate C2 42000000÷304÷65.41 = 2112.183071959 (recorded at sample rate: 19884.64 samples per sec. 304 samples per cycle - 65.41Hz * 304) 
//  NoteFreqBand 0
    case 37: noteDivisor = 1994; break; // play at faster sample rate C#2 42000000÷304÷69.30 = 1993.620414673 (mix 5/6th C2 with 1/6th F#2 recorded at sample rate: 28120 samples per sec. 304 samples per cycle - 92.5Hz * 304)
    case 38: noteDivisor = 1882; break; // play at faster sample rate D2  42000000÷304÷73.42 = 1881.747408565 (mix 4/6th C2 with 2/6th F#2 recorded at sample rate: 28120 samples per sec. 304 samples per cycle - 92.5Hz * 304)
    case 39: noteDivisor = 1776; break; // play at faster sample rate D#2 42000000÷304÷77.78 = 1776.265039044 (mix 3/6th C2 with 3/6th F#2 recorded at sample rate: 28120 samples per sec. 304 samples per cycle - 92.5Hz * 304)
    case 40: noteDivisor = 1676; break; // play at faster sample rate E2  42000000÷304÷82.41 = 1676.470024716 (mix 2/6th C2 with 4/6th F#2 recorded at sample rate: 28120 samples per sec. 304 samples per cycle - 92.5Hz * 304)
    case 41: noteDivisor = 1582; break; // play at faster sample rate F2  42000000÷304÷87.31 = 1582.383400949 (mix 1/6th C2 with 5/6th F#2 recorded at sample rate: 28120 samples per sec. 304 samples per cycle - 92.5Hz * 304)
    //                                                                      42000000÷304÷92.5 = 1493.59886202
    case 42: noteDivisor = 2112; break; // play at recorded sample rate F#2 42000000÷215÷92.5 = 2111.879321182 (recorded at sample rate: 19887.5 samples per sec. 215 samples per cycle - 92.5Hz * 215)
//  NoteFreqBand 1
    case 43: noteDivisor = 1993; break; // play at faster sample rate G2  42000000÷215÷98.000 = 1993.355481728 (mix 5/6th F#2 with 1/6th C3 recorded at sample rate: 28124.15 samples per sec. 215 samples per cycle - 130.81Hz * 215)
    case 44: noteDivisor = 1881; break; // play at faster sample rate G#2 42000000÷215÷103.83 = 1881.429617734 (mix 4/6th F#2 with 2/6th C3 recorded at sample rate: 28124.15 samples per sec. 215 samples per cycle - 130.81Hz * 215)
    case 45: noteDivisor = 1776; break; // play at faster sample rate A2  42000000÷215÷110.00 = 1775.898520085 (mix 3/6th F#2 with 3/6th C3 recorded at sample rate: 28124.15 samples per sec. 215 samples per cycle - 130.81Hz * 215)
    case 46: noteDivisor = 1676; break; // play at faster sample rate A#2 42000000÷215÷116.54 = 1676.238520759 (mix 2/6th F#2 with 4/6th C3 recorded at sample rate: 28124.15 samples per sec. 215 samples per cycle - 130.81Hz * 215)
    case 47: noteDivisor = 1582; break; // play at faster sample rate B2  42000000÷215÷123.47 = 1582.156290672 (mix 1/6th F#2 with 5/6th C3 recorded at sample rate: 28124.15 samples per sec. 215 samples per cycle - 130.81Hz * 215)
    //                                                                     42000000÷215÷130.81 = 1493.378466549
    case 48: noteDivisor = 2112; break; // play at recorded sample rate C3 42000000÷152÷130.81 = 2112.344541501 (recorded at sample rate: 19883.12 samples per sec. 152 samples per cycle - 130.81Hz * 152)
//  NoteFreqBand 2
    case 49: noteDivisor = 1994; break; // play at faster sample rate C#3 42000000÷152÷138.59 = 1993.764264909 (mix 5/6th C3 with 1/6th F#3 recorded at sample rate: 28120 samples per sec. 152 samples per cycle - 185.00Hz * 152)
    case 50: noteDivisor = 1882; break; // play at faster sample rate D3  42000000÷152÷146.83 = 1881.875566803 (mix 4/6th C3 with 2/6th F#3 recorded at sample rate: 28120 samples per sec. 152 samples per cycle - 185.00Hz * 152)
    case 51: noteDivisor = 1776; break; // play at faster sample rate D#3 42000000÷152÷155.56 = 1776.265039044 (mix 3/6th C3 with 3/6th F#3 recorded at sample rate: 28120 samples per sec. 152 samples per cycle - 185.00Hz * 152)
    case 52: noteDivisor = 1677; break; // play at faster sample rate E3  42000000÷152÷164.81 = 1676.571746094 (mix 2/6th C3 with 4/6th F#3 recorded at sample rate: 28120 samples per sec. 152 samples per cycle - 185.00Hz * 152)
    case 53: noteDivisor = 1582; break; // play at faster sample rate F3  42000000÷152÷174.61 = 1582.474024819 (mix 1/6th C3 with 5/6th F#3 recorded at sample rate: 28120 samples per sec. 152 samples per cycle - 185.00Hz * 152)
    //                                                                      42000000÷152÷185.00 = 1493.59886202
    case 54: noteDivisor = 2122; break; // play at recorded sample rate F#3 42000000÷107÷185.00 = 2121.74791614 (recorded at sample rate: 19795.00 samples per sec. 107 samples per cycle - 185.00Hz * 107)
//  NoteFreqBand 3
    case 55: noteDivisor = 2003; break; // play at faster sample rate G3  42000000÷107÷196.00 = 2002.670226969 (mix 5/6th F#3 with 1/6th C4 recorded at sample rate: 27994.41 samples per sec. 107 samples per cycle - 261.63Hz * 107)
    case 56: noteDivisor = 1890; break; // play at faster sample rate G#3 42000000÷107÷207.65 = 1890.31237412  (mix 4/6th F#3 with 2/6th C4 recorded at sample rate: 27994.41 samples per sec. 107 samples per cycle - 261.63Hz * 107)
    case 57: noteDivisor = 1784; break; // play at faster sample rate A3  42000000÷107÷220.00 = 1784.1971113   (mix 3/6th F#3 with 3/6th C4 recorded at sample rate: 27994.41 samples per sec. 107 samples per cycle - 261.63Hz * 107)
    case 58: noteDivisor = 1684; break; // play at faster sample rate A#3 42000000÷107÷233.08 = 1684.071411043 (mix 2/6th F#3 with 4/6th C4 recorded at sample rate: 27994.41 samples per sec. 107 samples per cycle - 261.63Hz * 107)
    case 59: noteDivisor = 1590; break; // play at faster sample rate B3  42000000÷107÷246.94 = 1589.549544367 (mix 1/6th F#3 with 5/6th C4 recorded at sample rate: 27994.41 samples per sec. 107 samples per cycle - 261.63Hz * 107)
    //                                                                    42000000÷107÷261.63 = 1500.299524084
    case 60: noteDivisor = 2112; break; // play at recorded sample rate C4 42000000÷76÷261.63 = 2112.263803644 (recorded at sample rate: 19883.88 samples per sec. 76 samples per cycle - 261.63Hz * 76)
//  NoteFreqBand 4
    case 61: noteDivisor = 1994; break; // play at faster sample rate C#4 42000000÷76÷277.18 = 1993.764264909 (mix 5/6th C4 with 1/6th F#4 recorded at sample rate: 28119.24 samples per sec. 76 samples per cycle - 369.99Hz * 76)
    case 62: noteDivisor = 1882; break; // play at faster sample rate D4  42000000÷76÷293.66 = 1881.875566803 (mix 4/6th C4 with 2/6th F#4 recorded at sample rate: 28119.24 samples per sec. 76 samples per cycle - 369.99Hz * 76)
    case 63: noteDivisor = 1776; break; // play at faster sample rate D#4 42000000÷76÷311.13 = 1776.207948277 (mix 3/6th C4 with 3/6th F#4 recorded at sample rate: 28119.24 samples per sec. 76 samples per cycle - 369.99Hz * 76)
    case 64: noteDivisor = 1677; break; // play at faster sample rate E4  42000000÷76÷329.63 = 1676.520883862 (mix 2/6th C4 with 4/6th F#4 recorded at sample rate: 28119.24 samples per sec. 76 samples per cycle - 369.99Hz * 76)
    case 65: noteDivisor = 1582; break; // play at faster sample rate F4  42000000÷76÷349.23 = 1582.428711587 (mix 1/6th C4 with 5/6th F#4 recorded at sample rate: 28119.24 samples per sec. 76 samples per cycle - 369.99Hz * 76)
    //                                                                      42000000÷76÷369.99 = 1493.639230648
    case 66: noteDivisor = 2102; break; // play at recorded sample rate F#4 42000000÷54÷369.99 = 2102.158917208 (recorded at sample rate: 19979.46 samples per sec. 54 samples per cycle - 369.99Hz * 54)
//  NoteFreqBand 5
    case 67: noteDivisor = 1984; break; // play at faster sample rate G4  42000000÷54÷392.00 = 1984.126984127 (mix 5/6th F#4 with 1/6th C5 recorded at sample rate: 28255.5 samples per sec. 54 samples per cycle - 523.25Hz * 54)
    case 68: noteDivisor = 1873; break; // play at faster sample rate G#4 42000000÷54÷415.30 = 1872.809481767 (mix 4/6th F#4 with 2/6th C5 recorded at sample rate: 28255.5 samples per sec. 54 samples per cycle - 523.25Hz * 54)
    case 69: noteDivisor = 1768; break; // play at faster sample rate A4  42000000÷54÷440.00 = 1767.676767677 (mix 3/6th F#4 with 3/6th C5 recorded at sample rate: 28255.5 samples per sec. 54 samples per cycle - 523.25Hz * 54)
    case 70: noteDivisor = 1668; break; // play at faster sample rate A#4 42000000÷54÷466.16 = 1668.478157237 (mix 2/6th F#4 with 4/6th C5 recorded at sample rate: 28255.5 samples per sec. 54 samples per cycle - 523.25Hz * 54)
    case 71: noteDivisor = 1575; break; // play at faster sample rate B4  42000000÷54÷493.88 = 1574.83149303  (mix 1/6th F#4 with 5/6th C5 recorded at sample rate: 28255.5 samples per sec. 54 samples per cycle - 523.25Hz * 54)
    //                                                                     42000000÷54÷523.25 = 1486.436269045
    case 72: noteDivisor = 2112; break; // play at recorded sample rate C5 42000000÷38÷523.25 = 2112.304171801 (recorded at sample rate: 19883.5 samples per sec. 38 samples per cycle - 523.25Hz * 38)
//  NoteFreqBand 6
    case 73: noteDivisor = 1994; break; // play at faster sample rate C#5 42000000÷38÷554.37 = 1993.764264909 (mix 5/6th C5 with 1/6th F#5 recorded at sample rate: 28119.24 samples per sec. 38 samples per cycle - 739.99Hz * 38)
    case 74: noteDivisor = 1882; break; // play at faster sample rate D5  42000000÷38÷587.33 = 1881.875566803 (mix 4/6th C5 with 2/6th F#5 recorded at sample rate: 28119.24 samples per sec. 38 samples per cycle - 739.99Hz * 38)
    case 75: noteDivisor = 1776; break; // play at faster sample rate D#5 42000000÷38÷622.25 = 1776.207948277 (mix 3/6th C5 with 3/6th F#5 recorded at sample rate: 28119.24 samples per sec. 38 samples per cycle - 739.99Hz * 38)
    case 76: noteDivisor = 1677; break; // play at faster sample rate E5  42000000÷38÷659.25 = 1676.520883862 (mix 2/6th C5 with 4/6th F#5 recorded at sample rate: 28119.24 samples per sec. 38 samples per cycle - 739.99Hz * 38)
    case 77: noteDivisor = 1582; break; // play at faster sample rate F5  42000000÷38÷698.46 = 1582.428711587 (mix 1/6th C5 with 5/6th F#5 recorded at sample rate: 28119.24 samples per sec. 38 samples per cycle - 739.99Hz * 38)
    //                                                                      42000000÷38÷739.99 = 1493.619046061
    case 78: noteDivisor = 2102; break; // play at recorded sample rate F#5 42000000÷27÷739.99 = 2102.158917208 (recorded at sample rate: 19979.46 samples per sec. 27 samples per cycle - 739.99Hz * 27)
//  NoteFreqBand 7
    case 79: noteDivisor = 1984; break; // play at faster sample rate G5  42000000÷27÷783.99 = 1984.126984127 (mix 5/6th F#5 with 1/6th C6 recorded at sample rate: 28255.5 samples per sec. 27 samples per cycle - 1046.50Hz * 27)
    case 80: noteDivisor = 1873; break; // play at faster sample rate G#5 42000000÷27÷830.61 = 1872.809481767 (mix 4/6th F#5 with 2/6th C6 recorded at sample rate: 28255.5 samples per sec. 27 samples per cycle - 1046.50Hz * 27)
    case 81: noteDivisor = 1768; break; // play at faster sample rate A5  42000000÷27÷880.00 = 1767.676767677 (mix 3/6th F#5 with 3/6th C6 recorded at sample rate: 28255.5 samples per sec. 27 samples per cycle - 1046.50Hz * 27)
    case 82: noteDivisor = 1668; break; // play at faster sample rate A#5 42000000÷27÷932.33 = 1668.478157237 (mix 2/6th F#5 with 4/6th C6 recorded at sample rate: 28255.5 samples per sec. 27 samples per cycle - 1046.50Hz * 27)
    case 83: noteDivisor = 1575; break; // play at faster sample rate B5  42000000÷27÷987.77 = 1574.83149303  (mix 1/6th F#5 with 5/6th C6 recorded at sample rate: 28255.5 samples per sec. 27 samples per cycle - 1046.50Hz * 27)
    //                                                                     42000000÷27÷1046.50 = 1486.436269045
    case 84: noteDivisor = 2112; break; // play at recorded sample rate C6 42000000÷19÷1046.50 = 2112.304171801 (recorded at sample rate: 19883.5 samples per sec. 19 samples per cycle - 1046.50Hz * 19)
//  NoteFreqBand 8
    case 85: noteDivisor = 1994; break; // play at faster sample rate C#6 42000000÷19÷1108.73 = 1993.746282494 (Not mix 5/6th C6 with 1/6th F#6 recorded at sample rate: 28119.24 samples per sec. 19 samples per cycle - 1479.98Hz * 19)
    case 86: noteDivisor = 1882; break; // play at faster sample rate D6  42000000÷19÷1174.66 = 1881.843525607 (Not mix 4/6th C6 with 2/6th F#6 recorded at sample rate: 28119.24 samples per sec. 19 samples per cycle - 1479.98Hz * 19)
    case 87: noteDivisor = 1776; break; // play at faster sample rate D#6 42000000÷19÷1244.51 = 1776.222220625 (Not mix 3/6th C6 with 3/6th F#6 recorded at sample rate: 28119.24 samples per sec. 19 samples per cycle - 1479.98Hz * 19)
    case 88: noteDivisor = 1677; break; // play at faster sample rate E6  42000000÷19÷1318.51 = 1676.53359913  (Not mix 2/6th C6 with 4/6th F#6 recorded at sample rate: 28119.24 samples per sec. 19 samples per cycle - 1479.98Hz * 19)
    case 89: noteDivisor = 1582; break; // play at faster sample rate F6  42000000÷19÷1396.91 = 1582.440039651 (Not mix 1/6th C6 with 5/6th F#6 recorded at sample rate: 28119.24 samples per sec. 19 samples per cycle - 1479.98Hz * 19)
    //                                                                      42000000÷19÷1479.98 = 1493.619046061
    case 90: noteDivisor = 2183; break; // play at recorded sample rate F#6 42000000÷13÷1479.98 = 2182.981682705 (recorded at sample rate: 19239.74 samples per sec. 13 samples per cycle - 1479.98Hz * 13)
//  NoteFreqBand 9
    case 91: noteDivisor = 2060; break; // play at faster sample rate G6  42000000÷13÷1567.98 = 2060.465841892 (Not mix 5/6th F#6 with 1/6th C7 recorded at sample rate: 27209 samples per sec. 13 samples per cycle - 2093.00Hz * 13)
    case 92: noteDivisor = 1945; break; // play at faster sample rate G#6 42000000÷13÷1661.22 = 1944.817201075 (Not mix 4/6th F#6 with 2/6th C7 recorded at sample rate: 27209 samples per sec. 13 samples per cycle - 2093.00Hz * 13)
    case 93: noteDivisor = 1836; break; // play at faster sample rate A6  42000000÷13÷1760.00 = 1835.664335664 (Not mix 3/6th F#6 with 3/6th C7 recorded at sample rate: 27209 samples per sec. 13 samples per cycle - 2093.00Hz * 13)
    case 94: noteDivisor = 1733; break; // play at faster sample rate A#6 42000000÷13÷1864.66 = 1732.631809965 (Not mix 2/6th F#6 with 4/6th C7 recorded at sample rate: 27209 samples per sec. 13 samples per cycle - 2093.00Hz * 13)
    case 95: noteDivisor = 1635; break; // play at faster sample rate B6  42000000÷13÷1975.53 = 1635.393656775 (Not mix 1/6th F#6 with 5/6th C7 recorded at sample rate: 27209 samples per sec. 13 samples per cycle - 2093.00Hz * 13)
    
    case 96: noteDivisor = 1544; break; // play at faster sample rate C7  42000000÷13÷2093.00 = 1543.606894777 (                             C7 recorded at sample rate: 27209 samples per sec. 13 samples per cycle - 2093.00Hz * 13)
  }
  if (instrument == 2 || instrument == 3) // if guitar or marimba
  {
    if (musicByt <= 40) noteDivisor = 2371; // 2112 * 4437 / 3953; // for E2 & F2, lowest 2 notes - but still use NoteFreqBand 1 for the 2 notes below F#2 and reduce noteDivisor further instead of using NoteFreqBand 0 - saves memory
    if (musicByt == 41) noteDivisor = 2238; // 2112 * 4188 / 3953;
  }
  return noteDivisor;
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
        if (!WaveHalf) DACC->DACC_CDR = WaveFull2[WaveBit / 1048576]; // if displaying 2nd wave half only (just passed end), write to DAC (1048576 = 4294967296 / 4096)
        else           DACC->DACC_CDR = WaveFull[WaveBit / 1048576]; // if displaying 1st wave half only (just passed end), write to DAC
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
          DACC->DACC_CDR = WaveFull2[WaveBit / 1048576]; // if 2nd wave half (just passed end of 1st half), write to DAC (1048576 = 4294967296 / 4096)
          if (SquareWaveSync)
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR; // set TIOA (sq. wave on pin 3) LOW when triggered (on next line)
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // the counter is reset and the clock is started
          }
        }
        else
        {
          DACC->DACC_CDR = WaveFull[WaveBit / 1048576]; // if 1st wave half (just passed end of 2nd half), write to DAC (1048576 = 4294967296 / 4096)
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
      if (!WaveHalf) DACC->DACC_CDR = WaveFull2[WaveBit / 1048576]; // if 2nd wave half, write to DAC (1048576 = 4294967296 / 4096)
      else           DACC->DACC_CDR = WaveFull[WaveBit / 1048576]; // if 1st wave half, write to DAC (1048576 = 4294967296 / 4096)
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
        if (!WaveHalf) DACC->DACC_CDR = constrain(WaveFull2[WaveBit / 1048576], 0, WAVERESOL-1); // if displaying 2nd wave half only (just passed end), write to DAC (1048576 = 4294967296 / 4096)
        else           DACC->DACC_CDR = constrain(WaveFull[WaveBit / 1048576], 0, WAVERESOL-1); // if displaying 1st wave half only (just passed end), write to DAC
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
          DACC->DACC_CDR = constrain(WaveFull2[WaveBit / 1048576], 0, WAVERESOL-1); // if 2nd wave half (just passed end of 1st half), write to DAC (1048576 = 4294967296 / 4096)
          if (SquareWaveSync)
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR; // set TIOA (sq. wave on pin 3) LOW when triggered (on next line)
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // the counter is reset and the clock is started
          }
        }
        else
        {
          DACC->DACC_CDR = constrain(WaveFull[WaveBit / 1048576], 0, WAVERESOL-1); // if 1st wave half (just passed end of 2nd half), write to DAC (1048576 = 4294967296 / 4096)
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
      if (!WaveHalf) DACC->DACC_CDR = constrain(WaveFull2[WaveBit / 1048576], 0, WAVERESOL-1); // if 2nd wave half, write to DAC (1048576 = 4294967296 / 4096)
      else           DACC->DACC_CDR = constrain(WaveFull[WaveBit / 1048576], 0, WAVERESOL-1); // if 1st wave half, write to DAC (1048576 = 4294967296 / 4096)
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
        if (!WaveHalf) DACC->DACC_CDR = constrain(((WaveFull2[WaveBit / 1048576] * Modulation) / WAVERESOL) + HALFRESOL, 0, WAVERESOL-1); // if displaying 2nd wave half only (just passed end), write to DAC (1048576 = 4294967296 / 4096)
        else           DACC->DACC_CDR = constrain(((WaveFull[WaveBit / 1048576] * Modulation) / WAVERESOL) + HALFRESOL, 0, WAVERESOL-1); // if displaying 1st wave half only (just passed end), write to DAC
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
          DACC->DACC_CDR = constrain(((WaveFull2[WaveBit / 1048576] * Modulation) / WAVERESOL) + HALFRESOL, 0, WAVERESOL-1); // if 2nd wave half (just passed end of 1st half), write to DAC (1048576 = 4294967296 / 4096)
          if (SquareWaveSync)
          {
            TC2->TC_CHANNEL[1].TC_CMR = TC_CMR_WAVE | TC_CMR_ASWTRG_CLEAR; // set TIOA (sq. wave on pin 3) LOW when triggered (on next line)
            TC2->TC_CHANNEL[1].TC_CCR = TC_CCR_SWTRG; // the counter is reset and the clock is started
          }
        }
        else
        {
          DACC->DACC_CDR = constrain(((WaveFull[WaveBit / 1048576] * Modulation) / WAVERESOL) + HALFRESOL, 0, WAVERESOL-1); // if 1st wave half (just passed end of 2nd half), write to DAC (1048576 = 4294967296 / 4096)
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
      if (!WaveHalf) DACC->DACC_CDR = constrain(((WaveFull2[WaveBit / 1048576] * Modulation) / WAVERESOL) + HALFRESOL, 0, WAVERESOL-1); // if 2nd wave half, write to DAC (1048576 = 4294967296 / 4096)
      else           DACC->DACC_CDR = constrain(((WaveFull[WaveBit / 1048576] * Modulation) / WAVERESOL) + HALFRESOL, 0, WAVERESOL-1); // if 1st wave half, write to DAC (1048576 = 4294967296 / 4096)
    }
  }
  else if (InterruptMode == 2) // Modulate Wave with envelope as it feeds to DAC, producing a musical note - constrained 
  {
    if (TimingCount < 65000)
    {
      int16_t tempSample = 0;
      if (WaveBit < Increment[!WaveHalf]) // if rolled over. If end of wave half (if 4294967295 exceeded - highest number for 32 bit int before roll-over to zero)
      {
        if (ExactFreqDutyNot50) // if in exact freq mode and not at 50% duty-cycle
          WaveBit = (WaveBit / 1000) * DutyMultiplier[WaveHalf]; // if not 50% duty-cycle TRY to maintain freq
        if (MinOrMaxWaveDuty) // if duty set to 0 or 100
        {
          if (!WaveHalf) tempSample = constrain(WaveCom2[WaveBit / 1048576], -2047, 2047); // if displaying 2nd wave half only (just passed end), write to DAC (1048576 = 4294967296 / 4096)
          else           tempSample = constrain(WaveCom[WaveBit / 1048576], -2047, 2047); // if displaying 1st wave half only (just passed end), write to DAC
        }
        else // if duty not set to 0 or 100
        {
          if (WaveHalf) tempSample = constrain(WaveCom2[WaveBit / 1048576], -2047, 2047); // if 2nd wave half (just passed end of 1st half), write to DAC (1048576 = 4294967296 / 4096)
          else          tempSample = constrain(WaveCom[WaveBit / 1048576], -2047, 2047); // if 1st wave half (just passed end of 2nd half), write to DAC (1048576 = 4294967296 / 4096)
          WaveHalf = !WaveHalf; // change to other wave half
        }
      }
      else // if not end of wave half
      {
        if (!WaveHalf) tempSample = constrain(WaveCom2[WaveBit / 1048576], -2047, 2047); // if 2nd wave half, write to DAC (1048576 = 4294967296 / 4096)
        else           tempSample = constrain(WaveCom[WaveBit / 1048576], -2047, 2047); // if 1st wave half, write to DAC (1048576 = 4294967296 / 4096)
      }
      if (SlurFilter)
      {
        NoteFilter += (tempSample - NoteFilter) * 1000 / (SlurStrength - 9000); // apply low pass filter between adjacent slurred notes (use 1000 instead of 10000 to increase strength 10 fold because updating 10 times more often than with mini-soundfonts)
        tempSample = NoteFilter;
      }
      DACC->DACC_CDR = ((tempSample * Modulation) / WAVERESOL) + HALFRESOL;
      if (FadeOut)
      {
        if (ModulationCalc > 0) ModulationCalc = ModulationCalc - max(1, ModulationCalc / 1500); // ModulationCalc * 999 / 1000;
        Modulation = ModulationCalc / 16;
      }
    }
    else DACC->DACC_CDR = HALFRESOL - 1; // silence when no note
  }
  else if (InterruptMode == 3) // read & mix Instrument minisoundfont data to re-create its sound, then feed to DAC, modulated by envelope, to produce music note:
  {
    if (NoteDivSync)
    {
      TC_setup2c(); // set sample rate with NoteDivisor
      NoteDivSync = 0;
    }
    RunInterrupt3(); // too much code to run here
  }
}

void RunInterrupt3()
{
  if (TimingCount < 65000)
  {
    int16_t tempSample = 0;
    uint8_t comp = 0; // amplitude boost compensation for when different notes are mixed and partially cancel each other out due to conflicting waveshapes
    int16_t noteDivisor = NoteDivisor * 10; // use larger numbers to allow smaller compensation increments
    if (Instrument == 1) // piano
    {
      if      (NoteFreqBand == 0) comp = (min(165, min(noteDivisor - 14930, 21120 - noteDivisor))) / 7; // cap the boost // 2112 - 1493 = 619
      else if (NoteFreqBand == 1) comp = (min(noteDivisor - 14930, 21120 - noteDivisor)) / 60; // the larger the final divisor the less the boost
      else if (NoteFreqBand == 2) comp = (min(206, min(noteDivisor - 14940, 21120 - noteDivisor))) / 4; // cap the boost
      else if (NoteFreqBand == 3) comp = (min(noteDivisor - 15000, 21220 - noteDivisor)) / 50;
  //    else if (NoteFreqBand == 4) comp = (min(noteDivisor - 14940, 21120 - noteDivisor)) / 50;
      else if (NoteFreqBand == 5) comp = (min(noteDivisor - 14860, 21020 - noteDivisor)) / 90;
  //    else if (NoteFreqBand == 6) comp = (min(noteDivisor - 14940, 21120 - noteDivisor)) / 60;
   //   else if (NoteFreqBand == 7) comp = (min(noteDivisor - 14860, 21020 - noteDivisor)) / 50;
    }
    else if (Instrument == 2) // guitar
    {
 //     if      (NoteFreqBand == 0) comp = (min(noteDivisor - 14930, 21120 - noteDivisor)) / 40; // 2112 - 1493 = 619
      if      (NoteFreqBand == 1) comp = (min(noteDivisor - 14930, 21120 - noteDivisor)) / 60; // the larger the final divisor the less the boost
      else if (NoteFreqBand == 2) comp = (min(noteDivisor - 14940, 21120 - noteDivisor)) / 50;
      else if (NoteFreqBand == 3) comp = (min(noteDivisor - 15000, 21220 - noteDivisor)) / 40;
      else if (NoteFreqBand == 4) comp = (min(noteDivisor - 14940, 21120 - noteDivisor)) / 40;
   //   else if (NoteFreqBand == 5) comp = (min(noteDivisor - 14860, 21020 - noteDivisor)) / 90;
   //   else if (NoteFreqBand == 6) comp = (min(noteDivisor - 14940, 21120 - noteDivisor)) / 90;
   //   else if (NoteFreqBand == 7) comp = (min(noteDivisor - 14860, 21020 - noteDivisor)) / 40;
    }
    else if (Instrument == 5) // sax
    {
      if      (NoteFreqBand == 4) comp = (min(noteDivisor - 14940, 21120 - noteDivisor)) / 40;
      else if (NoteFreqBand == 7) comp = (min(noteDivisor - 14860, 21020 - noteDivisor)) / 40;
    }
    else if (Instrument == 6) // violin
    {
      if      (NoteFreqBand == 4) comp = (min(noteDivisor - 14940, 21120 - noteDivisor)) / 26; // 23;
      else if (NoteFreqBand == 5) comp = (min(noteDivisor - 14860, 21020 - noteDivisor)) / 25;
      else if (NoteFreqBand == 6) comp = (min(noteDivisor - 14940, (21120 - noteDivisor) * 5)) / 45; // most boost at low end of band
      else if (NoteFreqBand == 7) comp = (min(noteDivisor - 14860, 21020 - noteDivisor)) / 35;
    }
    comp = max(0, comp);
    if (Instrument == 1 && NoteReadTotal < StartBegin) // read from start chunk: (each instrument will have a unique number of bytes, so they can't be together in an array)
    {
      int8_t startChunk = 0;
      if (NoteFreqBand == 0) // (MusicByt < 42) // 42 = 1494
      {
        if (NoteDivisor <= 1493) startChunk = pgm_read_word_near(PianoF2StartChunk872 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(PianoC2StartChunk872 + NoteReadTotal) - 127; // read from start chunk note C2
          if (NoteDivisor < 2112) // (MusicByt > 36) // 36 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(PianoF2StartChunk872 + NoteReadTotal) - 128; // 2112 - 1493 = 619
            startChunk = ((startChunk * (NoteDivisor - 1493)) + (startChunk2 * (2112 - NoteDivisor))) / (619 - comp); // 6; // read from start chunk if notes C#2 to F2 - mix C2 with F#2
          }
        }
      }
      else if (NoteFreqBand == 1) // (MusicByt < 48) // 48 = 1493
      {
        if (NoteDivisor <= 1493) startChunk = pgm_read_word_near(PianoC3StartChunk672 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(PianoF2StartChunk672 + NoteReadTotal) - 127; // read from start chunk note F#2
          if (NoteDivisor < 2112) // (MusicByt > 42) // 42 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(PianoC3StartChunk672 + NoteReadTotal) - 127; // 2112 - 1493 = 619 // 619 * 6 = 3714
            startChunk = ((startChunk * (NoteDivisor - 1493)) + (startChunk2 * (2112 - NoteDivisor))) / (619 - comp); // (36 - comp); // read from start chunk if notes G2 to B2 - mix F#2 with C3
          }
        }
      }
      else if (NoteFreqBand == 2) // (MusicByt < 54) // 54 = 1494
      {
        if (NoteDivisor <= 1494) startChunk = pgm_read_word_near(PianoF3StartChunk830 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(PianoC3StartChunk830 + NoteReadTotal) - 127; // read from start chunk note C3
          if (NoteDivisor < 2112) // (MusicByt > 48) // 42 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(PianoF3StartChunk830 + NoteReadTotal) - 127; // 2112 - 1494 = 618 // 618 * 3 = 1854
            startChunk = ((startChunk * (NoteDivisor - 1494)) + (startChunk2 * (2112 - NoteDivisor))) / (618 - comp); // (18 - comp); // read from start chunk if notes C#3 to F3 - mix C3 with F#3
          }
        }
      }
      else if (NoteFreqBand == 3) // (MusicByt < 60) // 60 = 1500
      {
        if (NoteDivisor <= 1500) startChunk = pgm_read_word_near(PianoC4StartChunk591 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(PianoF3StartChunk591 + NoteReadTotal) - 127; // read from start chunk note F#3
          if (NoteDivisor < 2122) // (MusicByt > 54) // 54 = 2122
          {
            int8_t startChunk2 = pgm_read_word_near(PianoC4StartChunk591 + NoteReadTotal) - 127; // 2122 - 1500 = 622 // 622 * 5 = 3110
            startChunk = ((startChunk * (NoteDivisor - 1500)) + (startChunk2 * (2122 - NoteDivisor))) / (622 - comp); // (30 - comp); // read from start chunk if notes G3 to B3 - mix F#3 with C4
          }
        }
      }
      else if (NoteFreqBand == 4) // (MusicByt < 66) // 66 = 1494
      {
        if (NoteDivisor <= 1494) startChunk = pgm_read_word_near(PianoF4StartChunk728 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(PianoC4StartChunk728 + NoteReadTotal) - 127; // read from start chunk note C4
          if (NoteDivisor < 2112) // (MusicByt > 60) // 60 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(PianoF4StartChunk728 + NoteReadTotal) - 127; // 2112 - 1494 = 618
            startChunk = ((startChunk * (NoteDivisor - 1494)) + (startChunk2 * (2112 - NoteDivisor))) / (618 - comp); // read from start chunk if notes C#4 to F4 - mix C4 with F#4
          }
        }
      }
      else if (NoteFreqBand == 5) // (MusicByt < 72) // 72 = 1486
      {
        if (NoteDivisor <= 1486) startChunk = pgm_read_word_near(PianoC5StartChunk1060 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(PianoF4StartChunk1060 + NoteReadTotal) - 127; // read from start chunk note F#4
          if (NoteDivisor < 2102) // (MusicByt > 66) // 66 = 2102
          {
            int8_t startChunk2 = pgm_read_word_near(PianoC5StartChunk1060 + NoteReadTotal) - 127; // 2102 - 1486 = 616 // 616 * 5 = 3080
            startChunk = ((startChunk * (NoteDivisor - 1486)) + (startChunk2 * (2102 - NoteDivisor))) / (616 - comp); // (30 - comp); // read from start chunk if notes G4 to B4 - mix F#4 with C5
          }
        }
      }
      else if (NoteFreqBand == 6) // (MusicByt < 78) // 78 = 1494
      {
        if (NoteDivisor <= 1494) startChunk = pgm_read_word_near(PianoF5StartChunk1314 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(PianoC5StartChunk1314 + NoteReadTotal) - 127; // read from start chunk note C5
          if (NoteDivisor < 2112) // (MusicByt > 72) // 72 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(PianoF5StartChunk1314 + NoteReadTotal) - 127; // 2112 - 1494 = 618 // 618 * 5 = 3090
            startChunk = ((startChunk * (NoteDivisor - 1494)) + (startChunk2 * (2112 - NoteDivisor))) / (618 - comp); // (30 - comp); // read from start chunk if notes C#5 to F5 - mix C5 with F#5
          }
        }
      }
      else if (NoteFreqBand == 7) // (MusicByt < 84) // 84 = 1486
      {
        if (NoteDivisor <= 1486) startChunk = pgm_read_word_near(PianoC6StartChunk900 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(PianoF5StartChunk900 + NoteReadTotal) - 127; // read from start chunk note F#5
          if (NoteDivisor < 2102) // (MusicByt > 78) // 78 = 2102
          {
            int8_t startChunk2 = pgm_read_word_near(PianoC6StartChunk900 + NoteReadTotal) - 127; // 2102 - 1486 = 616
            startChunk = ((startChunk * (NoteDivisor - 1486)) + (startChunk2 * (2102 - NoteDivisor))) / (616 - comp); // 6; // read from start chunk if notes G5 to B5 - mix F#5 with C6
          }
        }
      }
      else if (NoteFreqBand == 8) startChunk = pgm_read_word_near(PianoC6StartChunk625 + NoteReadTotal) - 127; // read from start chunk if note C6 to F6
      else if (NoteFreqBand == 9) startChunk = pgm_read_word_near(PianoF6StartChunk428 + NoteReadTotal) - 127; // read from start chunk if note F#6 to C7
      tempSample = startChunk * 16; //newReading;
      NoteReadTotal++;
    }
    else if (Instrument == 2 && NoteReadTotal < StartBegin) // if guitar note start // read from start chunk: (each instrument will have a unique number of bytes, so they can't be together in an array)
    {
      int8_t startChunk;
      if (NoteFreqBand <= 1) // (MusicByt < 48) // 48 = 1493
      {
        if (NoteDivisor <= 1493) startChunk = pgm_read_word_near(GuitarC3StartChunk1931 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(GuitarF2StartChunk1931 + NoteReadTotal) - 127; // read from start chunk note F#2
          if (NoteDivisor < 2112) // (MusicByt > 42) // 42 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(GuitarC3StartChunk1931 + NoteReadTotal) - 127; // 2112 - 1493 = 619 // 619 * 6 = 3714
            startChunk = ((startChunk * (NoteDivisor - 1493)) + (startChunk2 * (2112 - NoteDivisor))) / (619 - comp); // (36 - comp); // read from start chunk if notes G2 to B2 - mix F#2 with C3
          }
        }
      }
      else if (NoteFreqBand == 2) // (MusicByt < 54) // 54 = 1494
      {
        if (NoteDivisor <= 1494) startChunk = pgm_read_word_near(GuitarF3StartChunk1366 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(GuitarC3StartChunk1366 + NoteReadTotal) - 127; // read from start chunk note C3
          if (NoteDivisor < 2112) // (MusicByt > 48) // 42 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(GuitarF3StartChunk1366 + NoteReadTotal) - 127; // 2112 - 1494 = 618 // 618 * 3 = 1854
            startChunk = ((startChunk * (NoteDivisor - 1494)) + (startChunk2 * (2112 - NoteDivisor))) / (618 - comp); // (18 - comp); // read from start chunk if notes C#3 to F3 - mix C3 with F#3
          }
        }
      }
      else if (NoteFreqBand == 3) // (MusicByt < 60) // 60 = 1500
      {
        if (NoteDivisor <= 1500) startChunk = pgm_read_word_near(GuitarC4StartChunk1145 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(GuitarF3StartChunk1145 + NoteReadTotal) - 127; // read from start chunk note F#3
          if (NoteDivisor < 2122) // (MusicByt > 54) // 54 = 2122
          {
            int8_t startChunk2 = pgm_read_word_near(GuitarC4StartChunk1145 + NoteReadTotal) - 127; // 2122 - 1500 = 622 // 622 * 5 = 3110
            startChunk = ((startChunk * (NoteDivisor - 1500)) + (startChunk2 * (2122 - NoteDivisor))) / (622 - comp); // (30 - comp); // read from start chunk if notes G3 to B3 - mix F#3 with C4
          }
        }
      }
      else if (NoteFreqBand == 4) // (MusicByt < 66) // 66 = 1494
      {
        if (NoteDivisor <= 1494) startChunk = pgm_read_word_near(GuitarF4StartChunk1038 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(GuitarC4StartChunk1038 + NoteReadTotal) - 127; // read from start chunk note C4
          if (NoteDivisor < 2112) // (MusicByt > 60) // 60 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(GuitarF4StartChunk1038 + NoteReadTotal) - 127; // 2112 - 1494 = 618
            startChunk = ((startChunk * (NoteDivisor - 1494)) + (startChunk2 * (2112 - NoteDivisor))) / (618 - comp); // read from start chunk if notes C#4 to F4 - mix C4 with F#4
          }
        }
      }
      else if (NoteFreqBand == 5) // (MusicByt < 72) // 72 = 1486
      {
        if (NoteDivisor <= 1486) startChunk = pgm_read_word_near(GuitarC5StartChunk709 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(GuitarF4StartChunk709 + NoteReadTotal) - 127; // read from start chunk note F#4
          if (NoteDivisor < 2102) // (MusicByt > 66) // 66 = 2102
          {
            int8_t startChunk2 = pgm_read_word_near(GuitarC5StartChunk709 + NoteReadTotal) - 127; // 2102 - 1486 = 616 // 616 * 5 = 3080
            startChunk = ((startChunk * (NoteDivisor - 1486)) + (startChunk2 * (2102 - NoteDivisor))) / (616 - comp); // (30 - comp); // read from start chunk if notes G4 to B4 - mix F#4 with C5
          }
        }
      }
      else if (NoteFreqBand == 6) // (MusicByt < 78) // 78 = 1494
      {
        if (NoteDivisor <= 1494) startChunk = pgm_read_word_near(GuitarF5StartChunk499 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(GuitarC5StartChunk499 + NoteReadTotal) - 127; // read from start chunk note C5
          if (NoteDivisor < 2112) // (MusicByt > 72) // 72 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(GuitarF5StartChunk499 + NoteReadTotal) - 127; // 2112 - 1494 = 618 // 618 * 5 = 3090
            startChunk = ((startChunk * (NoteDivisor - 1494)) + (startChunk2 * (2112 - NoteDivisor))) / (618 - comp); // (30 - comp); // read from start chunk if notes C#5 to F5 - mix C5 with F#5
          }
        }
      }
      else if (NoteFreqBand == 7) // (MusicByt < 84) // 84 = 1486
      {
        if (NoteDivisor <= 1486) startChunk = pgm_read_word_near(GuitarC6StartChunk355 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(GuitarF5StartChunk355 + NoteReadTotal) - 127; // read from start chunk note F#5
          if (NoteDivisor < 2102) // (MusicByt > 78) // 78 = 2102
          {
            int8_t startChunk2 = pgm_read_word_near(GuitarC6StartChunk355 + NoteReadTotal) - 127; // 2102 - 1486 = 616
            startChunk = ((startChunk * (NoteDivisor - 1486)) + (startChunk2 * (2102 - NoteDivisor))) / (616 - comp); // 6; // read from start chunk if notes G5 to B5 - mix F#5 with C6
          }
        }
      }
      else if (NoteFreqBand == 8) startChunk = pgm_read_word_near(GuitarC6StartChunk250 + NoteReadTotal) - 127; // read from start chunk if note C6 to F6
      tempSample = startChunk * 16; //newReading;
      NoteReadTotal++;
    }    
    else if (Instrument == 3 && NoteReadTotal < StartBegin) // if marimba note start // read from start chunk: (each instrument will have a unique number of bytes, so they can't be together in an array)
    {
      int8_t startChunk;
      if      (NoteFreqBand <= 1) startChunk = pgm_read_word_near(MarimbaC3StartChunk3528 + NoteReadTotal) - 127; // read from same start chunk // note F#2
      else if (NoteFreqBand == 2) startChunk = pgm_read_word_near(MarimbaF3StartChunk2348 + NoteReadTotal) - 127; // read from start chunk note // F#3 instead of C3 - saves memory
      else if (NoteFreqBand == 3) startChunk = pgm_read_word_near(MarimbaF3StartChunk1759 + NoteReadTotal) - 127; // read from same start chunk // read from start chunk note F#3
      else if (NoteFreqBand == 4) startChunk = pgm_read_word_near(MarimbaC4StartChunk1478 + NoteReadTotal) - 127; // read from same start chunk // read from start chunk note C4
      else if (NoteFreqBand == 5) startChunk = pgm_read_word_near(MarimbaC5StartChunk1050 + NoteReadTotal) - 127; // read from same start chunk // note F#4
      else if (NoteFreqBand == 6) startChunk = pgm_read_word_near(MarimbaF5StartChunk1006 + NoteReadTotal) - 127; // read from same start chunk // note C5
      else if (NoteFreqBand == 7) startChunk = pgm_read_word_near(MarimbaF5StartChunk715 + NoteReadTotal) - 127; // read from same start chunk // note F#5
      else if (NoteFreqBand == 8) startChunk = pgm_read_word_near(MarimbaF6StartChunk656 + NoteReadTotal) - 127; // read from start chunk if note C6 to F6
      else if (NoteFreqBand == 9) startChunk = pgm_read_word_near(MarimbaF6StartChunk450 + NoteReadTotal) - 127; // read from start chunk if note F#6 to C7
      int16_t newReading = (startChunk * 16);
      tempSample = newReading;
      NoteReadTotal++;
    }    
    else if (Instrument == 4 && NoteReadTotal < StartBegin) // if trumpet note start // read from start chunk: (each instrument will have a unique number of bytes, so they can't be together in an array)
    {
      int8_t startChunk;
      if (NoteFreqBand <= 1) // (MusicByt < 48) // 48 = 1493
      {
        startChunk = pgm_read_word_near(TrumpetF2C3StartChunk1697 + NoteReadTotal) - 127; // read from same start chunk // note F#2
      }
      else if (NoteFreqBand == 2) // (MusicByt < 54) // 54 = 1494
      {
        if (NoteDivisor <= 1494) startChunk = pgm_read_word_near(TrumpetF3StartChunk1208 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(TrumpetC3StartChunk1208 + NoteReadTotal) - 127; // read from start chunk note C3
          if (NoteDivisor < 2112) // (MusicByt > 48) // 42 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(TrumpetF3StartChunk1208 + NoteReadTotal) - 127; // 2112 - 1494 = 618 // 618 * 3 = 1854
            startChunk = ((startChunk * (NoteDivisor - 1494)) + (startChunk2 * (2112 - NoteDivisor))) / (618 - comp); // (18 - comp); // read from start chunk if notes C#3 to F3 - mix C3 with F#3
          }
        }
      }
      else if (NoteFreqBand == 3) // (MusicByt < 60) // 60 = 1500
      {
        if (NoteDivisor <= 1500) startChunk = pgm_read_word_near(TrumpetC4StartChunk847 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(TrumpetF3StartChunk847 + NoteReadTotal) - 127; // read from start chunk note F#3
          if (NoteDivisor < 2122) // (MusicByt > 54) // 54 = 2122
          {
            int8_t startChunk2 = pgm_read_word_near(TrumpetC4StartChunk847 + NoteReadTotal) - 127; // 2122 - 1500 = 622 // 622 * 5 = 3110
            startChunk = ((startChunk * (NoteDivisor - 1500)) + (startChunk2 * (2122 - NoteDivisor))) / (622 - comp); // (30 - comp); // read from start chunk if notes G3 to B3 - mix F#3 with C4
          }
        }
      }
      else if (NoteFreqBand == 4) // (MusicByt < 66) // 66 = 1494
      {
        if (NoteDivisor <= 1494) startChunk = pgm_read_word_near(TrumpetF4StartChunk600 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(TrumpetC4StartChunk600 + NoteReadTotal) - 127; // read from start chunk note C4
          if (NoteDivisor < 2112) // (MusicByt > 60) // 60 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(TrumpetF4StartChunk600 + NoteReadTotal) - 127; // 2112 - 1494 = 618
            startChunk = ((startChunk * (NoteDivisor - 1494)) + (startChunk2 * (2112 - NoteDivisor))) / (618 - comp); // read from start chunk if notes C#4 to F4 - mix C4 with F#4
          }
        }
      }
      else if (NoteFreqBand == 5) // (MusicByt < 72) // 72 = 1486
      {
        startChunk = pgm_read_word_near(TrumpetF4C5StartChunk426 + NoteReadTotal) - 127; // read from same start chunk // note F#4
      }
      else if (NoteFreqBand == 6) // (MusicByt < 78) // 78 = 1494
      {
        startChunk = pgm_read_word_near(TrumpetC5F4StartChunk300 + NoteReadTotal) - 127; // read from same start chunk // note C5
      }
      else if (NoteFreqBand == 7) // (MusicByt < 84) // 84 = 1486
      {
        startChunk = pgm_read_word_near(TrumpetF5C6StartChunk217 + NoteReadTotal) - 127; // read from same start chunk // note F#5
      }
      else if (NoteFreqBand == 8) startChunk = pgm_read_word_near(TrumpetC6StartChunk152 + NoteReadTotal) - 127; // read from start chunk if note C6 to F6
      tempSample = startChunk * 16;
      NoteReadTotal++;
    } 
    else if (Instrument == 5 && NoteReadTotal < StartBegin) // if sax note start // read from start chunk: (each instrument will have a unique number of bytes, so they can't be together in an array)
    {
      int8_t startChunk;
      if (NoteFreqBand == 0) // (MusicByt < 48) // 48 = 1493 //temp
      {
        startChunk = pgm_read_word_near(SaxC2F2StartChunk1406 + NoteReadTotal) - 127; // read from same start chunk // note C2
      }
      else if (NoteFreqBand == 1) // (MusicByt < 48) // 48 = 1493
      {
        if (NoteDivisor <= 1493) startChunk = pgm_read_word_near(SaxC3StartChunk770 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(SaxF2StartChunk770 + NoteReadTotal) - 127; // read from start chunk note F#2
          if (NoteDivisor < 2112) // (MusicByt > 42) // 42 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(SaxC3StartChunk770 + NoteReadTotal) - 127; // 2112 - 1493 = 619 // 619 * 6 = 3714
            startChunk = ((startChunk * (NoteDivisor - 1493)) + (startChunk2 * (2112 - NoteDivisor))) / (619 - comp); // (36 - comp); // read from start chunk if notes G2 to B2 - mix F#2 with C3
          }
        }
      }
      else if (NoteFreqBand == 2) // (MusicByt < 54) // 54 = 1494
      {
        if (NoteDivisor <= 1494) startChunk = pgm_read_word_near(SaxF3StartChunk698 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(SaxC3StartChunk698 + NoteReadTotal) - 127; // read from start chunk note C3
          if (NoteDivisor < 2112) // (MusicByt > 48) // 42 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(SaxF3StartChunk698 + NoteReadTotal) - 127; // 2112 - 1494 = 618 // 618 * 3 = 1854
            startChunk = ((startChunk * (NoteDivisor - 1494)) + (startChunk2 * (2112 - NoteDivisor))) / (618 - comp); // (18 - comp); // read from start chunk if notes C#3 to F3 - mix C3 with F#3
          }
        }
      }
      else if (NoteFreqBand == 3) // (MusicByt < 60) // 60 = 1500
      {
        if (NoteDivisor <= 1500) startChunk = pgm_read_word_near(SaxC4StartChunk630 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(SaxF3StartChunk630 + NoteReadTotal) - 127; // read from start chunk note F#3
          if (NoteDivisor < 2122) // (MusicByt > 54) // 54 = 2122
          {
            int8_t startChunk2 = pgm_read_word_near(SaxC4StartChunk630 + NoteReadTotal) - 127; // 2122 - 1500 = 622 // 622 * 5 = 3110
            startChunk = ((startChunk * (NoteDivisor - 1500)) + (startChunk2 * (2122 - NoteDivisor))) / (622 - comp); // (30 - comp); // read from start chunk if notes G3 to B3 - mix F#3 with C4
          }
        }
      }
      else if (NoteFreqBand == 4) // (MusicByt < 66) // 66 = 1494
      {
        if (NoteDivisor <= 1494) startChunk = pgm_read_word_near(SaxF4StartChunk501 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(SaxC4StartChunk501 + NoteReadTotal) - 127; // read from start chunk note C4
          if (NoteDivisor < 2112) // (MusicByt > 60) // 60 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(SaxF4StartChunk501 + NoteReadTotal) - 127; // 2112 - 1494 = 618
            startChunk = ((startChunk * (NoteDivisor - 1494)) + (startChunk2 * (2112 - NoteDivisor))) / (618 - comp); // read from start chunk if notes C#4 to F4 - mix C4 with F#4
          }
        }
      }
      else if (NoteFreqBand == 5) // (MusicByt < 72) // 72 = 1486
      {
        if (NoteDivisor <= 1486) startChunk = pgm_read_word_near(SaxC5StartChunk417 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(SaxF4StartChunk417 + NoteReadTotal) - 127; // read from start chunk note F#4
          if (NoteDivisor < 2102) // (MusicByt > 66) // 66 = 2102
          {
            int8_t startChunk2 = pgm_read_word_near(SaxC5StartChunk417 + NoteReadTotal) - 127; // 2102 - 1486 = 616 // 616 * 5 = 3080
            startChunk = ((startChunk * (NoteDivisor - 1486)) + (startChunk2 * (2102 - NoteDivisor))) / (616 - comp); // (30 - comp); // read from start chunk if notes G4 to B4 - mix F#4 with C5
          }
        }
      }
      else if (NoteFreqBand == 6) // (MusicByt < 78) // 78 = 1494
      {
        if (NoteDivisor <= 1494) startChunk = pgm_read_word_near(SaxF5StartChunk407 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(SaxC5StartChunk407 + NoteReadTotal) - 127; // read from start chunk note C5
          if (NoteDivisor < 2112) // (MusicByt > 72) // 72 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(SaxF5StartChunk407 + NoteReadTotal) - 127; // 2112 - 1494 = 618 // 618 * 5 = 3090
            startChunk = ((startChunk * (NoteDivisor - 1494)) + (startChunk2 * (2112 - NoteDivisor))) / (618 - comp); // (30 - comp); // read from start chunk if notes C#5 to F5 - mix C5 with F#5
          }
        }
      }
      else if (NoteFreqBand == 7) // (MusicByt < 84) // 84 = 1486
      {
        if (NoteDivisor <= 1486) startChunk = pgm_read_word_near(SaxC6StartChunk321 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(SaxF5StartChunk321 + NoteReadTotal) - 127; // read from start chunk note F#5
          if (NoteDivisor < 2102) // (MusicByt > 78) // 78 = 2102
          {
            int8_t startChunk2 = pgm_read_word_near(SaxC6StartChunk321 + NoteReadTotal) - 127; // 2102 - 1486 = 616
            startChunk = ((startChunk * (NoteDivisor - 1486)) + (startChunk2 * (2102 - NoteDivisor))) / (616 - comp); // 6; // read from start chunk if notes G5 to B5 - mix F#5 with C6
          }
        }
      }
      else if (NoteFreqBand == 8) startChunk = pgm_read_word_near(SaxC6StartChunk139 + NoteReadTotal) - 127; // read from start chunk if note C6 to F6
      tempSample = startChunk * 16;
      NoteReadTotal++;
    }
    else if (Instrument == 6 && NoteReadTotal < StartBegin) // if violin note start // read from start chunk: (each instrument will have a unique number of bytes, so they can't be together in an array)
    {
      int8_t startChunk;
      if (NoteFreqBand == 3) // (MusicByt < 60) // 60 = 1500
      {
        startChunk = pgm_read_word_near(ViolinF3C4StartChunk1890 + NoteReadTotal) - 127; // for all notes, read from start chunk note C4
      }
      else if (NoteFreqBand == 4) // (MusicByt < 66) // 66 = 1494
      {
        if (NoteDivisor <= 1494) startChunk = pgm_read_word_near(ViolinF4StartChunk1343 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(ViolinC4StartChunk1343 + NoteReadTotal) - 127; // read from start chunk note C4
          if (NoteDivisor < 2112) // (MusicByt > 60) // 60 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(ViolinF4StartChunk1343 + NoteReadTotal) - 127; // 2112 - 1494 = 618
            startChunk = ((startChunk * (NoteDivisor - 1494)) + (startChunk2 * (2112 - NoteDivisor))) / (618 - comp); // read from start chunk if notes C#4 to F4 - mix C4 with F#4
          }
        }
      }
      else if (NoteFreqBand == 5) // (MusicByt < 72) // 72 = 1486
      {
        if (NoteDivisor <= 1486) startChunk = pgm_read_word_near(ViolinC5StartChunk988 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(ViolinF4StartChunk988 + NoteReadTotal) - 127; // read from start chunk note F#4
          if (NoteDivisor < 2102) // (MusicByt > 66) // 66 = 2102
          {
            int8_t startChunk2 = pgm_read_word_near(ViolinC5StartChunk988 + NoteReadTotal) - 127; // 2102 - 1486 = 616 // 616 * 5 = 3080
            startChunk = ((startChunk * (NoteDivisor - 1486)) + (startChunk2 * (2102 - NoteDivisor))) / (616 - comp); // (30 - comp); // read from start chunk if notes G4 to B4 - mix F#4 with C5
          }
        }
      }
      else if (NoteFreqBand == 6) // (MusicByt < 78) // 78 = 1494
      {
        if (NoteDivisor <= 1494) startChunk = pgm_read_word_near(ViolinF5StartChunk680 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(ViolinC5StartChunk680 + NoteReadTotal) - 127; // read from start chunk note C5
          if (NoteDivisor < 2112) // (MusicByt > 72) // 72 = 2112
          {
            int8_t startChunk2 = pgm_read_word_near(ViolinF5StartChunk680 + NoteReadTotal) - 127; // 2112 - 1494 = 618 // 618 * 5 = 3090
            startChunk = ((startChunk * (NoteDivisor - 1494)) + (startChunk2 * (2112 - NoteDivisor))) / (618 - comp); // (30 - comp); // read from start chunk if notes C#5 to F5 - mix C5 with F#5
          }
        }
      }
      else if (NoteFreqBand == 7) // (MusicByt < 84) // 84 = 1486
      {
        if (NoteDivisor <= 1486) startChunk = pgm_read_word_near(ViolinC6StartChunk482 + NoteReadTotal) - 127;
        else
        {
          startChunk = pgm_read_word_near(ViolinF5StartChunk482 + NoteReadTotal) - 127; // read from start chunk note F#5
          if (NoteDivisor < 2102) // (MusicByt > 78) // 78 = 2102
          {
            int8_t startChunk2 = pgm_read_word_near(ViolinC6StartChunk482 + NoteReadTotal) - 127; // 2102 - 1486 = 616
            startChunk = ((startChunk * (NoteDivisor - 1486)) + (startChunk2 * (2102 - NoteDivisor))) / (616 - comp); // 6; // read from start chunk if notes G5 to B5 - mix F#5 with C6
          }
        }
      }
      else if (NoteFreqBand == 8) startChunk = pgm_read_word_near(ViolinC6F6StartChunk340 + NoteReadTotal) - 127; // read from start chunk if note C6 to F6
      else if (NoteFreqBand == 9) startChunk = pgm_read_word_near(ViolinF6C7StartChunk233 + NoteReadTotal) - 127; // read from start chunk if note F6 to B6
      tempSample = startChunk * 16;
      NoteReadTotal++;
    }
    else if (NoteReadTotal >= StartBegin) // if (NoteReadTotal < StartBegin + MidBegin + EndBegin) // read cycle data: [Instrument - 1] [start cycle] [cycle samples]
    {
      int16_t startCycle;
      int8_t startSample;
      int8_t midSample;
      int8_t endSample;
      if (NoteReadCycle == CycleLen) NoteReadCycle = 0;
      if (NoteReadTotal < StartBegin + MidBegin)
      {
        if (NoteFreqBand == 0) // (MusicByt < 42) // 42 = 1494
        {
          if (NoteDivisor <= 1493) startSample = pgm_read_word_near(&MsfNoteBand0[Instrument - 1][1][NoteReadCycle]) - 127;
          else
          {
            if (Instrument == 2 || Instrument == 3) startSample = pgm_read_word_near(&MsfNoteBand1[Instrument - 1][0][NoteReadCycle]) - 127; // lowest 2 notes
            else
            {
              startSample = pgm_read_word_near(&MsfNoteBand0[Instrument - 1][0][NoteReadCycle]) - 127; // read note C2
              if (NoteDivisor < 2112) // (MusicByt > 36) // 36 = 2112
              {
                int8_t startSample2 = pgm_read_word_near(&MsfNoteBand0[Instrument - 1][1][NoteReadCycle]) - 127;
                startSample = ((startSample * (NoteDivisor - 1493)) + (startSample2 * (2112 - NoteDivisor))) / (618 - comp); // 6; // if notes C#2 to F2 - mix C2 with F#2
              }
            }
          }
        }
        else if (NoteFreqBand == 1) // (MusicByt < 48) // 48 = 1493
        {
          if (NoteDivisor <= 1493) startSample = pgm_read_word_near(&MsfNoteBand1[Instrument - 1][1][NoteReadCycle]) - 127;
          else
          {
            startSample = pgm_read_word_near(&MsfNoteBand1[Instrument - 1][0][NoteReadCycle]) - 127; // read note F#2
            if (NoteDivisor < 2112) // (MusicByt > 42) // 42 = 2112
            {
              int8_t startSample2 = pgm_read_word_near(&MsfNoteBand1[Instrument - 1][1][NoteReadCycle]) - 127;
              startSample = ((startSample * (NoteDivisor - 1493)) + (startSample2 * (2112 - NoteDivisor))) / (619 - comp); // (36 - comp); // if notes G2 to B2 - mix F#2 with C3
            }
          }
        }
        else if (NoteFreqBand == 2) // (MusicByt < 54) // 54 = 1494
        {
          if (NoteDivisor <= 1494) startSample = pgm_read_word_near(&MsfNoteBand2[Instrument - 1][1][NoteReadCycle]) - 127;
          else
          {
            startSample = pgm_read_word_near(&MsfNoteBand2[Instrument - 1][0][NoteReadCycle]) - 127; // read note C3
            if (NoteDivisor < 2112) // (MusicByt > 48) // 42 = 2112
            {
              int8_t startSample2 = pgm_read_word_near(&MsfNoteBand2[Instrument - 1][1][NoteReadCycle]) - 127;
              startSample = ((startSample * (NoteDivisor - 1494)) + (startSample2 * (2112 - NoteDivisor))) / (618 - comp); // (18 - comp); // if notes C#3 to F3 - mix C3 with F#3
            }
          }
        }
        else if (NoteFreqBand == 3) // (MusicByt < 60) // 60 = 1500
        {
          if (NoteDivisor <= 1500) startSample = pgm_read_word_near(&MsfNoteBand3[Instrument - 1][1][NoteReadCycle]) - 127;
          else
          {
            startSample = pgm_read_word_near(&MsfNoteBand3[Instrument - 1][0][NoteReadCycle]) - 127; // read note F#3
            if (NoteDivisor < 2122) // (MusicByt > 54) // 54 = 2122
            {
              int8_t startSample2 = pgm_read_word_near(&MsfNoteBand3[Instrument - 1][1][NoteReadCycle]) - 127;
              startSample = ((startSample * (NoteDivisor - 1500)) + (startSample2 * (2122 - NoteDivisor))) / (622 - comp); // (30 - comp); // if notes G3 to B3 - mix F#3 with C4
            }
          }
        }
        else if (NoteFreqBand == 4) // (MusicByt < 66) // 66 = 1494
        {
          if (NoteDivisor <= 1494) startSample = pgm_read_word_near(&MsfNoteBand4[Instrument - 1][1][NoteReadCycle]) - 127;
          else
          {
            startSample = pgm_read_word_near(&MsfNoteBand4[Instrument - 1][0][NoteReadCycle]) - 127; // read note C4
            if (NoteDivisor < 2112) // (MusicByt > 60) // 60 = 2112
            {
              int8_t startSample2 = pgm_read_word_near(&MsfNoteBand4[Instrument - 1][1][NoteReadCycle]) - 127; // 2112 - 1494 = 618
              startSample = ((startSample * (NoteDivisor - 1494)) + (startSample2 * (2112 - NoteDivisor))) / (618 - comp); // 6; // if notes C#4 to F4 - mix C4 with F#4
            }
          }
        }
        else if (NoteFreqBand == 5) // (MusicByt < 72) // 72 = 1486
        {
          if (NoteDivisor <= 1486) startSample = pgm_read_word_near(&MsfNoteBand5[Instrument - 1][1][NoteReadCycle]) - 127;
          else
          {
            startSample = pgm_read_word_near(&MsfNoteBand5[Instrument - 1][0][NoteReadCycle]) - 127; // read note F#4
            if (NoteDivisor < 2102) //(MusicByt > 66) // 66 = 2102
            {
              int8_t startSample2 = pgm_read_word_near(&MsfNoteBand5[Instrument - 1][1][NoteReadCycle]) - 127; // 2102 - 1486 = 616
              startSample = ((startSample * (NoteDivisor - 1486)) + (startSample2 * (2102 - NoteDivisor))) / (616 - comp); // (30 - comp); // if notes G4 to B4 - mix F#4 with C5
            }
          }
        }
        else if (NoteFreqBand == 6) // (MusicByt < 78) // 78 = 1494
        {
          if (NoteDivisor <= 1494) startSample = pgm_read_word_near(&MsfNoteBand6[Instrument - 1][1][NoteReadCycle]) - 127;
          else
          {
            startSample = pgm_read_word_near(&MsfNoteBand6[Instrument - 1][0][NoteReadCycle]) - 127; // read note C5
            if (NoteDivisor < 2112) // (MusicByt > 72) // 72 = 2112
            {
              int8_t startSample2 = pgm_read_word_near(&MsfNoteBand6[Instrument - 1][1][NoteReadCycle]) - 127;
              startSample = ((startSample * (NoteDivisor - 1494)) + (startSample2 * (2112 - NoteDivisor))) / (618 - comp); // (30 - comp); // if notes C#5 to F5 - mix C5 with F#5
            }
          }
        }
        else if (NoteFreqBand == 7) // (MusicByt < 84)
        {
          if (NoteDivisor <= 1486) startSample = pgm_read_word_near(&MsfNoteBand7[Instrument - 1][1][NoteReadCycle]) - 127;
          else
          {
            startSample = pgm_read_word_near(&MsfNoteBand7[Instrument - 1][0][NoteReadCycle]) - 127; // read note F#5
            if (NoteDivisor < 2102) // (MusicByt > 78) // 78 = 2102
            {
              int8_t startSample2 = pgm_read_word_near(&MsfNoteBand7[Instrument - 1][1][NoteReadCycle]) - 127;
              startSample = ((startSample * (NoteDivisor - 1486)) + (startSample2 * (2102 - NoteDivisor))) / (616 - comp); // 6; // if notes G5 to B5 - mix F#5 with C6
            }
          }
        }
        else if (NoteFreqBand == 8) startSample = pgm_read_word_near(&MsfNoteBand8[Instrument - 1][0][NoteReadCycle]) - 127; // if notes C6 to F6
        else if (NoteFreqBand == 9) startSample = pgm_read_word_near(&MsfNoteBand9[Instrument - 1][0][NoteReadCycle]) - 127; // if notes F#6 to C7
      }
      if (!SkipMid && NoteReadTotal >= StartBegin + CycleLen && NoteReadTotal < StartBegin + MidBegin + EndBegin) // if skipping mid cycle, mix from start cycle straight to end cycle (for future use)
      {
        if (NoteFreqBand == 0) // (MusicByt < 42) // 42 = 1494
        {
          if (NoteDivisor <= 1493) midSample = pgm_read_word_near(&MsfNoteBand0[Instrument - 1][3][NoteReadCycle]) - 127;
          else
          {
            if (Instrument == 2 || Instrument == 3) midSample = pgm_read_word_near(&MsfNoteBand1[Instrument - 1][2][NoteReadCycle]) - 127; //  lowest 2 notes
            else
            {
              midSample = pgm_read_word_near(&MsfNoteBand0[Instrument - 1][2][NoteReadCycle]) - 127; // read note C2
              if (NoteDivisor < 2112) // (MusicByt > 36) // 36 = 2112
              {
                int8_t midSample2 = pgm_read_word_near(&MsfNoteBand0[Instrument - 1][3][NoteReadCycle]) - 127;
                midSample = ((midSample * (NoteDivisor - 1493)) + (midSample2 * (2112 - NoteDivisor))) / (619 - comp); // 6; // if notes C#2 to F2 - mix C2 with F#2
              }
            }
          }
        }
        else if (NoteFreqBand == 1) // (MusicByt < 48) // 48 = 1493
        {
          if (NoteDivisor <= 1493) midSample = pgm_read_word_near(&MsfNoteBand1[Instrument - 1][3][NoteReadCycle]) - 127;
          else
          {
            midSample = pgm_read_word_near(&MsfNoteBand1[Instrument - 1][2][NoteReadCycle]) - 127; // read note F#2
            if (NoteDivisor < 2112) // (MusicByt > 42) // 42 = 2112
            {
              int8_t midSample2 = pgm_read_word_near(&MsfNoteBand1[Instrument - 1][3][NoteReadCycle]) - 127;
              midSample = ((midSample * (NoteDivisor - 1493)) + (midSample2 * (2112 - NoteDivisor))) / (619 - comp); // (36 - comp);  // if notes G2 to B2 - mix F#2 with C3
            }
          }
        }
        else if (NoteFreqBand == 2) // (MusicByt < 54) // 54 = 1494
        {
          if (NoteDivisor <= 1494) midSample = pgm_read_word_near(&MsfNoteBand2[Instrument - 1][3][NoteReadCycle]) - 127;
          else
          {
            midSample = pgm_read_word_near(&MsfNoteBand2[Instrument - 1][2][NoteReadCycle]) - 127; // read note C3
            if (NoteDivisor < 2112) // (MusicByt > 48) // 42 = 2112
            {
              int8_t midSample2 = pgm_read_word_near(&MsfNoteBand2[Instrument - 1][3][NoteReadCycle]) - 127;
              midSample = ((midSample * (NoteDivisor - 1494)) + (midSample2 * (2112 - NoteDivisor))) / (618 - comp); // (18 - comp); // if notes C#3 to F3 - mix C3 with F#3
            }
          }
        }
        else if (NoteFreqBand == 3) // (MusicByt < 60) // 60 = 1500
        {
          if (NoteDivisor <= 1500) midSample = pgm_read_word_near(&MsfNoteBand3[Instrument - 1][3][NoteReadCycle]) - 127;
          else
          {
            midSample = pgm_read_word_near(&MsfNoteBand3[Instrument - 1][2][NoteReadCycle]) - 127; // read note F#3
            if (NoteDivisor < 2122) // (MusicByt > 54) // 54 = 2122
            {
              int8_t midSample2 = pgm_read_word_near(&MsfNoteBand3[Instrument - 1][3][NoteReadCycle]) - 127;
              midSample = ((midSample * (NoteDivisor - 1500)) + (midSample2 * (2122 - NoteDivisor))) / (622 - comp); // (30 - comp);  // if notes G3 to B3 - mix F#3 with C4
            }
          }
        }
        else if (NoteFreqBand == 4) // (MusicByt < 66) // 66 = 1494
        {
          if (NoteDivisor <= 1494) midSample = pgm_read_word_near(&MsfNoteBand4[Instrument - 1][3][NoteReadCycle]) - 127;
          else
          {
            midSample = pgm_read_word_near(&MsfNoteBand4[Instrument - 1][2][NoteReadCycle]) - 127; // read note C4
            if (NoteDivisor < 2112) // (MusicByt > 60) // 60 = 2112
            {
              int8_t midSample2 = pgm_read_word_near(&MsfNoteBand4[Instrument - 1][3][NoteReadCycle]) - 127; // 2112 - 1494 = 618
              midSample = ((midSample * (NoteDivisor - 1494)) + (midSample2 * (2112 - NoteDivisor))) / (618 - comp); // 6; // if notes C#4 to F4 - mix C4 with F#4
            }
          }
        }
        else if (NoteFreqBand == 5) // (MusicByt < 72) // 72 = 1486
        {
          if (NoteDivisor <= 1486) midSample = pgm_read_word_near(&MsfNoteBand5[Instrument - 1][3][NoteReadCycle]) - 127;
          else
          {
            midSample = pgm_read_word_near(&MsfNoteBand5[Instrument - 1][2][NoteReadCycle]) - 127; // read note F#4
            if (NoteDivisor < 2102) //(MusicByt > 66) // 66 = 2102
            {
              int8_t midSample2 = pgm_read_word_near(&MsfNoteBand5[Instrument - 1][3][NoteReadCycle]) - 127; // 2102 - 1486 = 616
              midSample = ((midSample * (NoteDivisor - 1486)) + (midSample2 * (2102 - NoteDivisor))) / (616 - comp); // (30 - comp);  // if notes G4 to B4 - mix F#4 with C5
            }
          }
        }
        else if (NoteFreqBand == 6) // (MusicByt < 78) // 78 = 1494
        {
          if (NoteDivisor <= 1494) midSample = pgm_read_word_near(&MsfNoteBand6[Instrument - 1][3][NoteReadCycle]) - 127;
          else
          {
            midSample = pgm_read_word_near(&MsfNoteBand6[Instrument - 1][2][NoteReadCycle]) - 127; // read note C5
            if (NoteDivisor < 2112) // (MusicByt > 72) // 72 = 2112
            {
              int8_t midSample2 = pgm_read_word_near(&MsfNoteBand6[Instrument - 1][3][NoteReadCycle]) - 127;
              midSample = ((midSample * (NoteDivisor - 1494)) + (midSample2 * (2112 - NoteDivisor))) / (618 - comp); // (30 - comp); // if notes C#5 to F5 - mix C5 with F#5
            }
          }
        }
        else if (NoteFreqBand == 7) // (MusicByt < 84)
        {
          if (NoteDivisor <= 1486) midSample = pgm_read_word_near(&MsfNoteBand7[Instrument - 1][3][NoteReadCycle]) - 127;
          else
          {
            midSample = pgm_read_word_near(&MsfNoteBand7[Instrument - 1][2][NoteReadCycle]) - 127; // read note F#5
            if (NoteDivisor < 2102) // (MusicByt > 78) // 78 = 2102
            {
              int8_t midSample2 = pgm_read_word_near(&MsfNoteBand7[Instrument - 1][3][NoteReadCycle]) - 127;
              midSample = ((midSample * (NoteDivisor - 1486)) + (midSample2 * (2102 - NoteDivisor))) / (616 - comp); // 6; // if notes G5 to B5 - mix F#5 with C6
            }
          } 
        }
        else if (NoteFreqBand == 8) midSample = pgm_read_word_near(&MsfNoteBand8[Instrument - 1][1][NoteReadCycle]) - 127; // if notes C6 to F6
        else if (NoteFreqBand == 9) midSample = pgm_read_word_near(&MsfNoteBand9[Instrument - 1][1][NoteReadCycle]) - 127; // if notes F#6 to C7
      }
      uint16_t sp = 0;
      if (SkipMid) sp = StartBegin + CycleLen; // if skipping mid cycle, mix from start cycle straight to end cycle (for future use)
      else sp = StartBegin + MidBegin + CycleLen;
      if (NoteReadTotal >= sp)
      {
        if (NoteFreqBand == 0) // (MusicByt < 42) // 42 = 1494
        {
          if (NoteDivisor <= 1493) endSample = pgm_read_word_near(&MsfNoteBand0[Instrument - 1][5][NoteReadCycle]) - 127;
          else
          {
            if (Instrument == 2 || Instrument == 3) endSample = pgm_read_word_near(&MsfNoteBand1[Instrument - 1][4][NoteReadCycle]) - 127; // lowest 2 notes 
            else
            {
              endSample = pgm_read_word_near(&MsfNoteBand0[Instrument - 1][4][NoteReadCycle]) - 127;  // read note C2
              if (NoteDivisor < 2112) // (MusicByt > 36) // 36 = 2112
              {
                int8_t endSample2 = pgm_read_word_near(&MsfNoteBand0[Instrument - 1][5][NoteReadCycle]) - 127;
                endSample = ((endSample * (NoteDivisor - 1493)) + (endSample2 * (2112 - NoteDivisor))) / (618 - comp); // 6; // if notes C#2 to F2 - mix C2 with F#2
              }
            }
          }
        }
        else if (NoteFreqBand == 1) // (MusicByt < 48) // 48 = 1493
        {
          if (NoteDivisor <= 1493) endSample = pgm_read_word_near(&MsfNoteBand1[Instrument - 1][5][NoteReadCycle]) - 127;
          else
          {
            endSample = pgm_read_word_near(&MsfNoteBand1[Instrument - 1][4][NoteReadCycle]) - 127; // read note F#2
            if (NoteDivisor < 2112) // (MusicByt > 42) // 42 = 2112
            {
              int8_t endSample2 = pgm_read_word_near(&MsfNoteBand1[Instrument - 1][5][NoteReadCycle]) - 127;
              endSample = ((endSample * (NoteDivisor - 1493)) + (endSample2 * (2112 - NoteDivisor))) / (619 - comp); // (36 - comp);  // if notes G2 to B2 - mix F#2 with C3
            }
          }
        }
        else if (NoteFreqBand == 2) // (MusicByt < 54) // 54 = 1494
        {
          if (NoteDivisor <= 1494) endSample = pgm_read_word_near(&MsfNoteBand2[Instrument - 1][5][NoteReadCycle]) - 127;
          else
          {
            endSample = pgm_read_word_near(&MsfNoteBand2[Instrument - 1][4][NoteReadCycle]) - 127;  // read note C3
            if (NoteDivisor < 2112) // (MusicByt > 48) // 42 = 2112
            {
              int8_t endSample2 = pgm_read_word_near(&MsfNoteBand2[Instrument - 1][5][NoteReadCycle]) - 127;
              endSample = ((endSample * (NoteDivisor - 1494)) + (endSample2 * (2112 - NoteDivisor))) / (618 - comp); // (18 - comp); // if notes C#3 to F3 - mix C2 with F#3
            }
          } 
        }
        else if (NoteFreqBand == 3) // (MusicByt < 60) // 60 = 1500
        {
          if (NoteDivisor <= 1500) endSample = pgm_read_word_near(&MsfNoteBand3[Instrument - 1][5][NoteReadCycle]) - 127;
          else
          {
            endSample = pgm_read_word_near(&MsfNoteBand3[Instrument - 1][4][NoteReadCycle]) - 127; // read note F#3
            if (NoteDivisor < 2122) // (MusicByt > 54) // 54 = 2122
            {
              int8_t endSample2 = pgm_read_word_near(&MsfNoteBand3[Instrument - 1][5][NoteReadCycle]) - 127;
              endSample = ((endSample * (NoteDivisor - 1500)) + (endSample2 * (2122 - NoteDivisor))) / (622 - comp); // (30 - comp);  // if notes G3 to B3 - mix F#3 with C4
            }
          }
        }
        else if (NoteFreqBand == 4) // (MusicByt < 66) // 66 = 1494
        {
          if (NoteDivisor <= 1494) endSample = pgm_read_word_near(&MsfNoteBand4[Instrument - 1][5][NoteReadCycle]) - 127;
          else
          {
            endSample = pgm_read_word_near(&MsfNoteBand4[Instrument - 1][4][NoteReadCycle]) - 127;  // read note C4
            if (NoteDivisor < 2112) // (MusicByt > 60) // 60 = 2112
            {
              int8_t endSample2 = pgm_read_word_near(&MsfNoteBand4[Instrument - 1][5][NoteReadCycle]) - 127; // 2112 - 1494 = 618
              endSample = ((endSample * (NoteDivisor - 1494)) + (endSample2 * (2112 - NoteDivisor))) / (618 - comp); // 6; // if notes C#4 to F4 - mix C4 with F#4
            }
          }
        }
        else if (NoteFreqBand == 5) // (MusicByt < 72)
        {
          if (NoteDivisor <= 1486) endSample = pgm_read_word_near(&MsfNoteBand5[Instrument - 1][5][NoteReadCycle]) - 127;
          else
          {
            endSample = pgm_read_word_near(&MsfNoteBand5[Instrument - 1][4][NoteReadCycle]) - 127; // read note F#4
            if (NoteDivisor < 2102) //(MusicByt > 66) // 66 = 2102
            {
              int8_t endSample2 = pgm_read_word_near(&MsfNoteBand5[Instrument - 1][5][NoteReadCycle]) - 127; // 2102 - 1486 = 616
              endSample = ((endSample * (NoteDivisor - 1486)) + (endSample2 * (2102 - NoteDivisor))) / (616 - comp); // (30 - comp);  // if notes G4 to B4 - mix F#4 with C5
            }
          }
        }
        else if (NoteFreqBand == 6) // (MusicByt < 78) // 78 = 1494
        {
          if (NoteDivisor <= 1494) endSample = pgm_read_word_near(&MsfNoteBand6[Instrument - 1][5][NoteReadCycle]) - 127;
          else
          {
            endSample = pgm_read_word_near(&MsfNoteBand6[Instrument - 1][4][NoteReadCycle]) - 127;  // read note C5
            if (NoteDivisor < 2112) // (MusicByt > 72) // 72 = 2112
            {
              int8_t endSample2 = pgm_read_word_near(&MsfNoteBand6[Instrument - 1][5][NoteReadCycle]) - 127;
              endSample = ((endSample * (NoteDivisor - 1494)) + (endSample2 * (2112 - NoteDivisor))) / (618 - comp); // (30 - comp); // if notes C#5 to F5 - mix C5 with F#5
            }
          }
        }
        else if (NoteFreqBand == 7) // (MusicByt < 84)
        {
          if (NoteDivisor <= 1486) endSample = pgm_read_word_near(&MsfNoteBand7[Instrument - 1][5][NoteReadCycle]) - 127;
          else
          {
            endSample = pgm_read_word_near(&MsfNoteBand7[Instrument - 1][4][NoteReadCycle]) - 127; // read note F#5
            if (NoteDivisor < 2102) // (MusicByt > 78) // 78 = 2102
            {
              int8_t endSample2 = pgm_read_word_near(&MsfNoteBand7[Instrument - 1][5][NoteReadCycle]) - 127;
              endSample = ((endSample * (NoteDivisor - 1486)) + (endSample2 * (2102 - NoteDivisor))) / (616 - comp); // 6;  // if notes G5 to B5 - mix F#5 with C6
            }
          }
        }
        else if (NoteFreqBand == 8) endSample = pgm_read_word_near(&MsfNoteBand8[Instrument - 1][2][NoteReadCycle]) - 127; // if notes C6 to F6
        else if (NoteFreqBand == 9) endSample = pgm_read_word_near(&MsfNoteBand9[Instrument - 1][2][NoteReadCycle]) - 127; // if notes F#6 to C7
      }
// SAMPLES CREATED ABOVE ARE MIXED TOGETHER BELOW:
      if (NoteReadTotal < StartBegin + CycleLen) // read start cycle data:
      {
        tempSample = startSample * 16;
        NoteReadTotal++;
      }
      else if (NoteReadTotal < StartBegin + MidBegin) // progressively mix startCycle with midCycle:
      {
        int16_t startSampleMix;
        int16_t midSampleMix;
        startSampleMix = ((startSample * 16) * (StartBegin + MidBegin - NoteReadTotal)) / MidBegin;
        if (SkipMid) midSampleMix = (endSample * 16 * (NoteReadTotal - StartBegin)) / MidBegin; // if skipping mid cycle, mix from start cycle straight to end cycle
        else         midSampleMix = (midSample * 16 * (NoteReadTotal - StartBegin)) / MidBegin;
        tempSample = constrain(tempSample + (startSampleMix + midSampleMix), -2047, 2047);
        NoteReadTotal++;
      }
      else if (!SkipMid && NoteReadTotal < StartBegin + MidBegin + CycleLen) // read from mid cycle if not skipping mid cycle:
      {
        tempSample = midSample * 16; // read from mid cycle only
        NoteReadTotal++;
      }
      else if (!SkipMid && NoteReadTotal < StartBegin + MidBegin + EndBegin) // progressively mix midCycle with endCycle if not skipping mid cycle:
      {
        int16_t midSampleMix;
        int16_t endSampleMix;
        midSampleMix = (midSample * 16 * (StartBegin + MidBegin + EndBegin - NoteReadTotal)) / EndBegin;
        endSampleMix = (endSample * 16 * (NoteReadTotal - StartBegin - MidBegin)) / EndBegin;
        tempSample = midSampleMix + endSampleMix; // ) * Modulation) / WAVERESOL) + HALFRESOL;
        NoteReadTotal++;
      }
      else // read from end cycle only:
      {
        tempSample = endSample * 16;
        NoteReadTotal = 65535;
      }
      NoteReadCycle++;
    }
    if (Instrument == 4) // if trumpet, gradually add low pass filter with time & if between adjacent slurred notes
    {
      NoteFilter += (tempSample - NoteFilter) * 10000 / max(min(NoteReadTotal + 10000, 20000), SlurStrength); // low pass filter - increase strength with time (NoteReadTotal) & if between adjacent slurred notes
      tempSample = NoteFilter;
    }
    
    else if (Instrument == 1) // piano
    {
      NoteFilter += (tempSample - NoteFilter) * Velocity / (SlurStrength / 100); // low pass filter - increase strength with lower velocity
      tempSample = NoteFilter;
    }
    
    else // if (Instrument == 3, 5 or 6) // if marimba, sax or violin
    {
      if (SlurFilter)
      {
        NoteFilter += (tempSample - NoteFilter) * 10000 / SlurStrength; // apply low pass filter between adjacent slurred notes
        tempSample = NoteFilter;
      }
      if (Instrument == 5 && MusicByt < 76) // if sax  (highest note is 89), add filtered noise:
      {
        int16_t noise = trng_read_output_data(TRNG);
        TrngNum += (noise - TrngNum) * MusicByt / 84; // NoiseCol = 701 // * 10000 / min(MusicByt - 36 + 10000, 20000);
        tempSample = constrain(tempSample + (TrngNum * (76 - max(MusicByt, 42)) / 10000), -2047, 2047); // NoiseAmp = 26
      }
    }
    DACC->DACC_CDR = ((tempSample * Modulation) / WAVERESOL) + HALFRESOL;
  }
  else DACC->DACC_CDR = HALFRESOL - 1; // silence when no note
  if (FadeOut) Modulation = Modulation * 299 / 300; // 15 / 16;
}

//void TC3_Handler() // not used // write music voice 2 to analogue DAC pin - clocked at voice 2 pitch sample rate (TC_setup7)
//{
//  TC_GetStatus(TC1, 0);
// //  DACC->DACC_CDR = constrain(((((TrngNum / 16) * NoiseFil / 100) + ((TrngSlo / 16) * NoiseLFB / 70) + ((fastR / 16) * NoiseHFB / 1000)) * NoiseAmp / 100) + HALFRESOL, 0, 4095); // reduce from 16 bit to 12 bit and adjust balance and amplitude
//}

void TC4_Handler() // timing for tempo - clocked at tempo rate * timeSig (TC_setup6)
{
  TC_GetStatus(TC1, 1);
  if (TempoCount < 15000) TempoCount++;
}

void TC5_Handler() // timing for music note envelope, etc - clocked at 20kHz during decay delay, then 1kHz (by TC_setup8())
{
  TC_GetStatus(TC1, 2);
  // envelope:
  if (!FadeOut)
  {
    if (SlurFilter) // if start of slurred note (not end of previous slurred note)
    {
      if (TimingCount < 500)
      {
        if (SlurVioFade) SlurVioFade = 0;
        SlurStrength = constrain(SlurStrength - SlurIncr, 10000, SlurMax); // if start of slurred note decrease low pass filtering
      }
      else if (TimingCount == 500) { SlurFilter = 0; SlurStrength = 10000; } // 500 @ 20kHz = 25mSec
    }
    if (EnvelopeDivisor != 42000 && TimingCount >= DecayDelay) // end of decay delay time period
    {
      EnvelopeDivisor = 42000; // set to 1kHz when attack / decay delay is finished // EnvelopeDivisor = 2100 (20kHz) at start of note
      TC_setup8(); // set timer
    }
    if (Rest || TempoCount >= NotePlayLen) // if note finished playing:
    {
      if (EnvelopeDivisor != 42000) // if attack / decay delay not finished - important if staccato
      {
        EnvelopeDivisor = 42000; // set to 1kHz if note is finished   105000; // set to 400Hz if note is finished
        TC_setup8(); // set timer
      }
      if (Instrument >= 4 && ModulationCalc > PeakLevel / 5)
      {
        if (Instrument == 6) ModulationCalc = ModulationCalc * 219 / 220; // if violin, reduce amplitude to 20% quickly before using release rate
        else                  ModulationCalc = ModulationCalc * 99 / 100; // if trumpet or sax, reduce amplitude to 20% very quickly before using release rate
      }
      else ModulationCalc = max(0, ModulationCalc - max(1,((ModulationCalc * Envelope[Instrument][4]) / 5000))); // if note finished, reduce amplitude at release rate (semi-log)
    }
    else // if note not finished playing:
    {
      if (TimingCount < DecayDelay) ModulationCalc = min(PeakLevel, ModulationCalc + AttackRate[Instrument]); // if before decay time, increase amplitude at attack rate
      else // if (TimingCount >= DecayDelay) // if attack / decay delay finished
      {
        byte dkr8 = DecayRate;
        if (Instrument == 1 && Modulation > 10 * Velocity) // if piano increase DecayRate at start of note
        {
          if (MusicByt > 57) dkr8 += (min(30, MusicByt - 56) * 3); // increase DecayRate more at higher pitch up to note 86 (D6) ie: 30 + 56
          else dkr8 += 3;
        }
        uint16_t susLevel = PeakLevel * Envelope[Instrument][3] / 100;
        if      (ModulationCalc > susLevel && dkr8 > 0)  ModulationCalc -= max(1,((ModulationCalc * dkr8) / 15000)); // max(Envelope[Instrument][3], ModulationCalc - DecayRate); // if note decays (or de/crescendo in progress) & amplitude above sustain level, reduce amplitude at decay rate to no less than sustain level (semi-log)
        else if (ModulationCalc < susLevel && CresEndVel > 0) ModulationCalc = susLevel;
      }
    }
    if (!SlurVioFade) Modulation = ModulationCalc / 16;
    if (TimingCount < 30000) TimingCount++; // 
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

void TC_setup2c() // system timer clock set-up for voice 1 pitch
{
  pmc_set_writeprotect(false);     // disable write protection for pmc registers
  pmc_enable_periph_clk(ID_TC0);   // enable peripheral clock TC0
  // we want wavesel 01 with RC:
  TC_Configure(/* clock */TC0,/* channel */0, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1); // select 42 MHz clock
  TC_SetRC(TC0, 0, NoteDivisor); // select divisor according to the note pitch
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

void TC_setup6() // timer clock set-up for tempo
{
  pmc_set_writeprotect(false);     // disable write protection for pmc registers
  pmc_enable_periph_clk(ID_TC4);   // enable peripheral clock TC1, channel 1
  TC_Configure(/* clock */TC1,/* channel */1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1); // select 42 MHz clock
 /*/ if (TimeSig < 7)*/ TC_SetRC(TC1, 1, 2520000000 / (Tempo + 15) / 24); // divisor of 2520000000 (42MHz / 2520000000) = reset at 1 minute - tempo is a 0-255 byte representing 15-270
 //;;; else             TC_SetRC(TC1, 1, 2520000000 / (Tempo + 15) / 12); // divisor of 2520000000 (42MHz / 2520000000) = reset at 1 minute - tempo is a 0-255 byte representing 15-270
  TC_Start(TC1, 1);
  TC1->TC_CHANNEL[1].TC_IER = TC_IER_CPCS; // IER = interrupt enable register
  TC1->TC_CHANNEL[1].TC_IDR = ~TC_IER_CPCS; // IDR = interrupt disable register
  // Enable the interrupt in the nested vector interrupt controller
  // TC4_IRQn where 4 is the timer number * timer channels (3) + the channel number (=(1*3)+1) for timer 1 channel 1:
  NVIC_EnableIRQ(TC4_IRQn);
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

void TC_setup8() // timer clock set-up for envelope timing
{
  pmc_set_writeprotect(false);     // disable write protection for pmc registers
  pmc_enable_periph_clk(ID_TC5);   // enable peripheral clock TC1, channel 0
  TC_Configure(/* clock */TC1,/* channel */2, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1); // select 42 MHz clock
  TC_SetRC(TC1, 2, EnvelopeDivisor); // divisor of 2100 (42 MHz / 2100) - clocks at 20kHz // 42000 // set to 1kHz   105000); // divisor of 105000 (42 MHz / 105000) - clocks at 400Hz
  TC_Start(TC1, 2);
  TC1->TC_CHANNEL[2].TC_IER = TC_IER_CPCS; // IER = interrupt enable register
  TC1->TC_CHANNEL[2].TC_IDR = ~TC_IER_CPCS; // IDR = interrupt disable register
  // Enable the interrupt in the nested vector interrupt controller
  // TC5_IRQn where 5 is the timer number * timer channels (3) + the channel number (=(1*3)+2) for timer 1 channel 2:
  NVIC_EnableIRQ(TC5_IRQn);
}

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