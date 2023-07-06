# Pink Noise & TTL Generator

 - **Author:** Brian James, <brian@tofof.com>
 - **Date:** 2023-07-02
 - **Version:** 0.1.0
 - **Contact:** Ben Auerbach, Auerbach Lab, University of Illinois \<bda5@illinois.edu>


## Description
This is an arduino-based pink noise generator that sends a TTL signal while audio is playing. It was constructed for use with the RWD R820 photometry system, but is compatible with any devices that accept TTL signals. The specific sequence and timing of signals can be adjusted in software. 

![A photograph of the control box and the speaker, showing the test, start, and abort buttons and the blue and green LEDs](https://raw.githubusercontent.com/Auerbach-Lab/PinkNoiseDue/7478535de894f6554dea8b48933255e167ce98e0/docs/pink_noise_generator.jpg?token=GHSAT0AAAAAACAWGXVK76VXROSJHU5K4STOZFGNJEQ)


## Usage
 1. Connect the power supply to the jack on the rear face of the control box. 
 2. Connect the speaker using a headphone cable to the jacks on the speaker housing and the forward face of the control box.
 3. Connect a BNC cable (male-male) to the jack on the forward face of the control box and to an "Input" connection on the photometry system. 
 4.  In the left pane of the RWD Photometry software ([manual](https://www.rwdstco.com/wp-content/uploads/2021/10/R820-Tricolor-Multichannel-Fiber-Photometry-System-User-Manual_A-0831-02.pdf)), scroll down to find the "Event/Output Setting" section. **Make sure that "Hardware" is checked** to enable the incoming TTL signal to annotate the recording. 
 5. Start the recording in the photometry software and press the green START button on the control box to begin the pre-programmed 10 minute sequence. 
 
**LED Indicators**<br>
Green LED illuminates to indicate that the sequence is active. 
Blue LED illuminates when TTL is being transmitted, i.e. when a sound is playing.

**Button Details**<br>
To test the device, press the black TEST button. The blue LED will activate, indicating sound is playing and a TTL signal is being sent. 
- Verify the volume is acceptable using a meter or a smartphone (suggested app: DecibelX for [Android](https://play.google.com/store/apps/details?id=com.skypaw.decibel&hl=en_US&gl=US) or [iOS](https://apps.apple.com/us/app/decibel-x-db-sound-level-meter/id448155923)).
- Verify that the TTL signal is being received by starting a recording in the RWD Photometry software and watching the color highlight appear during the recording while TEST is depressed.

Pressing START while a sequence is already running has no effect.

Press the red ABORT button to cancel a running sequence and immediately silence any sounds.

**Volume**<br>
To adjust the volume in software, edit `#define VOLUME 200` near the top of main.ino. See the rough decibel correspondence in the chart immediately following this line.

To adjust the volume in hardware, use the trim potentiometer on the underside of the control box, using a flat eyeglasses screwdriver or similar implement. There is a very small usable range for adjustments; as little as 2 degrees of turn will produce several dB of change within this range. Most positions the trimmer can be set to leave the speaker at maximum volume. **Use this adjustment sparingly**; trim pots are generally only rated for a 200 cycle lifespan.

![A photograph of the underside of the control box, showing the cutout to access the volume trim potentiometer](https://raw.githubusercontent.com/Auerbach-Lab/PinkNoiseDue/7478535de894f6554dea8b48933255e167ce98e0/docs/control_box_underside.jpg?token=GHSAT0AAAAAACAWGXVKO5XCKNZ22WOZ7NEGZFGNIVQ)


## Sound Sequence
 - 120 seconds of silence
 - 355 seconds of 5-second bursts of pink noise that are cosine gated in and out across 500 ms, then 30 seconds of silence (11 bursts, 10 silent periods)
 - 120 seconds of silence
 Total duration: 9 minutes 55 seconds

## Sound Spectral Quality
Real world results show spectrally-accurate pink noise in the range of 6.5-25 kHz, and possibly beyond (smartphone app does not support measurements in frequencies above 25 kHz). Between 3-6.5 kHz intensity is mostly uniform, i.e. white noise. Intensity falls off below 3 kHz to levels undistinguishable from ambient at 750 Hz. 

![Screenshot from the Spectroid app showing a linear falloff of intensity vs frequency in the range of 6.5-25 kHz, with reference levels of -30 dB @ 6.5 kHz to -75 dB @ 25 kHz](https://raw.githubusercontent.com/Auerbach-Lab/PinkNoiseDue/7478535de894f6554dea8b48933255e167ce98e0/docs/pink_noise_spectral.png?token=GHSAT0AAAAAACAWGXVKXTQ7REA2ZXSONFE2ZFGNJRA)


## Programming
Repository: https://github.com/Auerbach-Lab/PinkNoiseDue

 1. Install [PlatformIO for VSCode](https://platformio.org/install/ide?install=vscode) or for the [editor of your choice](https://platformio.org/install/integration). ([instructions](https://docs.platformio.org/en/stable/integration/ide/vscode.html#quick-start))
 2. Clone the repository above ([instructions](https://code.visualstudio.com/docs/sourcecontrol/github))
 3. Edit `main.ino`, located in the `src` directory. Near the top of that file:
```
// EDIT THESE VALUES to adjust timings on sequence
#define RECORDING_DURATION  600000   // ms duration of entire recording sequence, must be at least BOOKEND_DURATION * 2 + SOUND_DURATION for a single sound
#define BOOKEND_DURATION    120000   // ms duration of silence at beginning and end, must be less than 1/2 RECORDING_DURATION
#define GAP_DURATION         30000   // ms between sounds
#define SOUND_DURATION        5000   // ms duration of sound to play
#define COSINE_PERIOD          500   // ms duration of cosine gate function, must be less than or equal to 1/2 SOUND_DURATION
```
4. Connect a short micro-usb cable to the programming port of the arduino, accessible through a cutout in the control box. 
5. Build and upload the sketch to the arduino using the buttons in the PlatformIO toolbar at the bottom of the editor. 


## Parts
**Speaker** - Fostex FT17H tweeter, with a rated frequency response of 5-40 kHz. Produces audio down to 500 Hz , but with diminished intensity outside its rated range. As a result, the peak intensity of the pink noise is at 3 kHz; frequencies between 0.5-3 kHz have diminished intensity from true pink noise. The speaker housing includes as 1/4"-20 threaded nut allowing it to be mounted to microphone/camera booms.

**Control box** - The control box consists of an Arduino Due, an TPA3116D2 amplifier circuit, and a relay. The arduino may be reprogrammed using either of the micro-usb ports. Although the arduino can be powered through these ports alone during testing, the amplifier will be unpowered so there will be no audio output. The control box also incorporates circuitry to protect the vulnerable and precious DAC output pin from static electricity or other inrush current, including a replaceable fuse.

**Power supply** - 12VDC 2A, 5.5x2.1mm (short) plug. Higher amperage power supplies are also suitable. The short (8-9mm) variant of this size plug is uncommon; the more common length (10-11mm) may be used but will not fully insert. This would not be a safety hazard; the exposed portion of plug is ground.

**Headphone cable** - male-male mono or stereo 3.5mm minijack cable.

**BNC cable** - A 50Ω male-male cable is provided, matching the impedance on the control box and the RWD R820 sockets. This is by far the more common type of BNC cable, but in practice a 75Ω cable would also work perfectly in this application (and presents no hazard to either device). 

**C-stand** - desk mount arm with a 1/4"-20 screw post to attach the speaker to, so it can be positioned directly above the center of an open field enclosure.


## Troubleshooting
If the blue LED illuminates but no sound can be heard from the speaker:
 - Make sure the audio cable is firmly connected to both the speaker housing and the control box. 
 - Turn the volume trim pot on the underside of the control box fully clockwise, to its maximum position.
 - Ensure that external DC power is plugged into the control box, and that the box is not being supplied from the micro-usb port which only supplies the arduino (not the amplifier). 
 - If the programming has been edited, re-upload the program to the arduino, after verifying that the `VOLUME` #define has not been set too low (200 is the original default). 
 - Finally, open the control box and inspect the fuse (50 mA / 0.05 A, 5x20 mm), replacing it if necessary. The fuse is located on the vertical breadboard attached to the lid of the enclosure.


## Notes for Future Maintainers
The relay is used to completely silence the amplifier output, to eliminate background hiss. There is some amount of hiss (white noise) during audio playback, but it is indistinguishable beneath the much louder pink noise and does not measurably impact the intensity/frequency relationship of the pink noise output. If the box is converted to playing tones, expect 25 dB of background white noise beneath the tone. 

Output from the DAC on the Due is DC-biased by half the operating voltage (i.e. by 2.5 V). To remedy this, the signal is passed through a coupling capacitor so that only the AC portion passes through. The signal is still slightly DC-biased, but only by about 0.2 V. It is unknown whether a differently sized capacitor would completely eliminate the bias.

The capacitor size also attenuates low-frequency signal, which could be a contributing factor to the peak intensity of the current system being at 3 kHz. It is possible that the 22nF capacitor in use should be replaced with a 1uF or even 10uF cap to permit 1000 Hz or 100 Hz signals ([guide](https://www.learningaboutelectronics.com/Articles/What-is-a-coupling-capacitor)). 

No lowpass filtering capacitor is used directly on the speaker, despite it being a tweeter. Experiments with adding a lowpass filtering capacitor resulted in diminished volume from the speaker. The FT17H is an 8Ω speaker, suggesting a 25-50uF capacitor would be appropriate ([guide](https://how-to-install-car-audio-systems.blogspot.com/2016/03/how-to-add-capacitor-to-car-tweeter.html)) if this is to be pursued in the future.

The software incorporates the [Due Arbitrary Waveform Generator](https://projecthub.arduino.cc/BruceEvans/4281674f-b6ae-4d5c-af6a-2fe70bb86825?f=1), a very powerful suite that can generate, as the name suggests, any type of wave or tone. In addition to the DueAWGController GUI, which can be downloaded from the project's github, it supports an interactive serial interface which can be accessed by:
```
C:\Users\USERNAME\.platformio\penv\Scripts\platformio.exe device monitor -b 115200 --filter send_on_enter --echo
```


## Sources
**Due Arbitrary Waveform Generator**<br>
Bruce Evans, 2017<br>
Create arbitrary waves, classic waves, noise or even music - and the Due will generate it.<br>
https://projecthub.arduino.cc/BruceEvans/4281674f-b6ae-4d5c-af6a-2fe70bb86825
