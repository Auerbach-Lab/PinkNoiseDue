#include <Arduino.h>
#include <debounce.h>
#include <DueFlashStorage.h>
#include <efc.h>
#include <flash_efc.h>

void Setup_DAWG();
void Setup2();
void Settings(byte, int, boolean);
void SaveToFlash(int);
void SaveMusicToFlash(int);
void CreateWaveFull(byte);
void CreateWaveTable();
void CreateNewWave();
void Create1stHalfNewWave(byte, float);
void Create2ndHalfNewWave(int, float);
void Loop_DAWG();
void MusicEnterExit(byte);
void SetFreqPeriod();
void SetDutyPulse();
void SaveSliderDefaults();
void SendSettings(int);
void SendArbitraryWave();
void ReceiveNotes(int8_t);
void PlayTune(byte, char);
void PlayNotes(uint16_t);
float FindNoteWaveFreq(byte);
int FindNoteDivisor(byte, byte);
void EnterTimerMode();
void timerRun();
void ExitTimerMode();
void ChangeWaveShape(bool);
void ToggleExactFreqMode();
void ToggleSquareWaveSync(bool);
void EnterSweepMode();
void ExitSweepMode();
void SweepFreq();
void PrintSyncedWaveFreq();
void PrintSyncedWavePeriod();
void PrintUnsyncedSqWaveFreq();
void PrintUnsyncedSqWavePeriod();
float tcToFreq(int);
int freqToTc(float);
void WavePolarity();
void SetWaveFreq(bool);
void CalculateWaveDuty(bool);
void SetFreqAndDuty(bool, bool);
void SetPWM(byte, uint32_t, uint32_t);
void TC1_Handler();
void DACC_Handler();
void TC0_Handler();
void RunInterrupt3();
void TC4_Handler();
void TC5_Handler();
void TC2_Handler();
void TC_setup();
void TC_setup1();
void TC_setup2();
void TC_setup2a();
void TC_setup2b();
void TC_setup2c();
void TC_setup3();
void TC_setup4();
void TC_setup5();
void TC_setup6();
void TC_setup8();
void NoiseFilterSetup();
void dac_setup();
void dac_setup2();

extern uint16_t NoiseAmp;