# Tone/noise generator with synchronized TTL

 - **Author:** Brian James, <james29@illinois.edu>
 - **Date:** 2023-09-05
 - **Version:** 1.1.0
 - **Contact:** Ben Auerbach, Auerbach Lab, University of Illinois \<bda5@illinois.edu>


## Description
This is an arduino-based sound generator that sends a TTL signal while audio is playing. It was constructed for use with the RWD R820 photometry system, but is compatible with any devices that accept TTL signals. The specific sequence and timing of signals can be adjusted in software. 

![A photograph of the control box and the speaker, showing the test, start, and abort buttons and the blue and green LEDs](https://github.com/Auerbach-Lab/PinkNoiseDue/blob/3dfdd038347436ca910bea0f4c5e393d52d1ded2/docs/pink_noise_generator.jpg?raw=true)


## Usage
 1. Connect the BLUE-labelled power supply to the jack on the rear face of the control box.
 2. Connect the RED-labelled power supply to the jack on the rear face of the amplifier.
 3. Set the volume of the amplifier to the maximum (farthest clockwise) using the large knob.
 4. Connect the headphone jack on the control box to the RCA-in ports on the amplifier, using a 3.5mm-RCA cable.
 5. Connect the output of the amplifier to the headphone jack on the speakers, using a 3.5mm-Banana cable.
 6. Connect a BNC cable (male-male) to the jack on the forward face of the control box and to an "Input" connection on the photometry system. 
 7.  In the left pane of the RWD Photometry software ([manual](https://www.rwdstco.com/wp-content/uploads/2021/10/R820-Tricolor-Multichannel-Fiber-Photometry-System-User-Manual_A-0831-02.pdf)), scroll down to find the "Event/Output Setting" section. **Make sure that "Hardware" is checked** to enable the incoming TTL signal to annotate the recording.
 8. Turn the frequency selection knob on the control box to the desired setting.
 9. Start the recording in the photometry software and press the green START button on the control box to begin the pre-programmed 10 minute sequence. 
 
**LED Indicators**<br>
Green LED illuminates to indicate that the sequence is active. 
Blue LED illuminates when TTL is being transmitted, i.e. when a sound is playing.

**Button Details**<br>
To test the device, press the black TEST button. The blue LED will activate, indicating sound is playing and a TTL signal is being sent. 
- Verify the volume is acceptable using a meter or a smartphone (suggested app: DecibelX for [Android](https://play.google.com/store/apps/details?id=com.skypaw.decibel&hl=en_US&gl=US) or [iOS](https://apps.apple.com/us/app/decibel-x-db-sound-level-meter/id448155923)). Volume should be 90 dB at approximately 0.5m distance.
- Verify that the TTL signal is being received by starting a recording in the RWD Photometry software and watching the color highlight appear during the recording while TEST is depressed.

Pressing START while a sequence is already running has no effect.

Changing the frequency selection knob while a sequence is running will immediately abort the current sequence; you will have to press START again to begin another sequence.

Press the red ABORT button to cancel a running sequence and immediately silence any sounds.

**Volume**<br>
To adjust the volume in software, edit values in the `VOLUME CALIBRATION` section near the top of main.ino. 

## Sound Sequence
 - 60 seconds of silence
 - 5-second sounds that are cosine gated in and out across 500 ms, then 25 seconds of silence (18 sounds and 17 silent periods, for 515 seconds)
 - 60 seconds of silence
 Total duration: 10 minutes 35 seconds

The sounds are played in the following order of intensities (dB):
> 40, 80, 20, 70, 60, 30, 90, 10, 50, 20, 60, 10, 30, 90, 70, 40, 50, 80

Note that each intensity is played twice, with intensities of 10-90 dB.

## Sound Spectral Quality
Real world results using the [included amplifier and speaker](https://github.com/Auerbach-Lab/PinkNoiseDue/blob/main/README.md#parts) show spectrally-accurate pink noise in the range of 6.5-25 kHz, and possibly beyond (smartphone app does not support measurements in frequencies above 25 kHz). Between 3-6.5 kHz intensity is mostly uniform, i.e. white noise. As frequency decreases, intensity falls off, beginning below 3 kHz and reaching levels undistinguishable from ambient below 750 Hz.

![Screenshot from the Spectroid app showing a linear falloff of intensity vs frequency in the range of 6.5-25 kHz, with reference levels of -30 dB @ 6.5 kHz to -75 dB @ 25 kHz](https://github.com/Auerbach-Lab/PinkNoiseDue/blob/7478535de894f6554dea8b48933255e167ce98e0/docs/pink_noise_spectral.png?raw=true)


## Programming
Repository: https://github.com/Auerbach-Lab/PinkNoiseDue

 1. Install [PlatformIO for VSCode](https://platformio.org/install/ide?install=vscode) or for the [editor of your choice](https://platformio.org/install/integration). ([instructions](https://docs.platformio.org/en/stable/integration/ide/vscode.html#quick-start))
 2. Clone the repository above ([instructions](https://code.visualstudio.com/docs/sourcecontrol/github))
 3. Edit `main.ino`, located in the `src` directory. Near the top of that file:
```
// EDIT THESE VALUES to adjust timings on sequence
BOOKEND_DURATION * 2 + SOUND_DURATION for a single sound
#define BOOKEND_DURATION     60000   // ms duration of silence at beginning and end, must be less than 1/2 RECORDING_DURATION
#define GAP_DURATION         25000   // ms between sounds
#define SOUND_DURATION        5000   // ms duration of sound to play
#define COSINE_PERIOD          500   // ms duration of cosine gate function, must be less than or equal to 1/2 SOUND_DURATION
#define SOUND_COUNT             18   // total number of samples to play

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
```
4. Connect a short micro-usb cable to the programming port of the arduino, accessible through a cutout in the control box. 
5. Build and upload the sketch to the arduino using the buttons in the PlatformIO toolbar at the bottom of the editor. 


## Parts
**Speaker** - Fostex FT17H tweeter, with a rated frequency response of 5-40 kHz. Produces audio down to 500 Hz , but with diminished intensity outside its rated range. As a result, the peak intensity of the pink noise is at 3 kHz; frequencies between 0.5-3 kHz have diminished intensity from true pink noise. The speaker housing includes as 1/4"-20 threaded nut allowing it to be mounted to microphone/camera booms.

**Control box** - The control box consists of an Arduino Due, a DS1881 digital audio potentiometer, and a relay. The arduino may be reprogrammed using either of the micro-usb ports. The control box also incorporates circuitry to protect the vulnerable and precious DAC output pin from static electricity or other inrush current, including a fuse.

**Amplifier** - Fosi Audio V3 Stereo Amplifier, based on a TPA3255 chipset. 

**Power supplies** - For the arduino: 12VDC 2A, 5.5x2.1mm (short) plug. Higher amperage power supplies are also suitable. The short (8-9mm) variant of this size plug is uncommon; the more common length (10-11mm) may be used but will not fully insert. Such use is not a significant safety hazard; the outer barrel is ground, and voltage and current are both low. For the amplifier: 32VDC 5A, also in 5.5x2.1 plug. **Do not confuse the two power supplies or you will destroy the arduino control box.** Colored labels near the tip of each wire are included and should not be removed.

**3.5mm-RCA cable** - male-male mono or stereo adapter cable, converting between 3.5mm minijack and RCA, used between the control box and the amplifier.

**3.5mm-Banana plug cable** - male-male mono or stereo adapter cable, converting between 3.5mm minijack and banana plugs (or bare terminals). Used between the speaker and the amplifier. If bare terminals are used, they can be screwed into place on the amplifier instead of plugged into the center socket of the outputs.

**BNC cable** - A 50Ω male-male cable is provided, matching the impedance on the control box and the RWD R820 sockets. This is by far the most common impedence for BNC cables, but in practice a 75Ω cable should also work perfectly in this application (and should present no hazard to either device). 

**C-stand** - desk mount arm with a 1/4"-20 screw post to attach the speaker to, so it can be positioned directly above the center of an open field enclosure.


## Troubleshooting
If the blue LED illuminates but no sound can be heard from the speaker:
 - Make sure the control box and amplifier are both plugged in, and that the amplifier's volume knob is set to maximum, and the blue power led on the amplifier is on.
 - Make sure the audio cables are firmly connected between the control box and amplifier, and between the amplifier and speaker. The RCA connection is the most likely to work itself loose.
 - Finally, open the control box to inspect the fuse (50 mA / 0.05 A, 5x20 mm), replacing it if necessary. **BE VERY CAREFUL OF STATIC DISCHARGE** while doing this - handling the fuse can send static electricity into the DAC and destroy it. It is best to first disconnect the DAC output pin completely as a precaution. The fuse is located on a small breakout PCB and is hardwired into place for reliability.


## Notes for Future Maintainers
The relay is present to completely disconnect the audio output, to eliminate background hiss. It is ancillary after the introduction of the DS1881 digital audio potentiometer, which advertises capability of a similar hard disconnect without risk of a transient (pop or click) in the audio output by using zero-crossing detection. In practice, transients were still present even after incorporation of the DS1881.

No lowpass filtering capacitor is used directly on the speaker, despite it being a tweeter. Experiments with adding a lowpass filtering capacitor resulted in diminished volume from the speaker. The FT17H is an 8Ω speaker, suggesting a 25-50uF capacitor would be ([appropriate](https://how-to-install-car-audio-systems.blogspot.com/2016/03/how-to-add-capacitor-to-car-tweeter.html)) if this is to be pursued in the future.

The software incorporates the [Due Arbitrary Waveform Generator](https://projecthub.arduino.cc/BruceEvans/4281674f-b6ae-4d5c-af6a-2fe70bb86825?f=1), a very powerful suite that can generate, as the name suggests, any type of wave or tone. In addition to the DueAWGController GUI, which can be downloaded from the project's github, it supports an interactive serial interface which can be accessed by:
```
C:\Users\USERNAME\.platformio\penv\Scripts\platformio.exe device monitor -b 115200 --filter send_on_enter --echo
```


## Sources
**Due Arbitrary Waveform Generator**<br>
Bruce Evans, 2017<br>
Create arbitrary waves, classic waves, noise or even music - and the Due will generate it.<br>
https://projecthub.arduino.cc/BruceEvans/4281674f-b6ae-4d5c-af6a-2fe70bb86825
