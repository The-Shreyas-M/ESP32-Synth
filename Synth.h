// synth.h

#ifndef SYNTH_H
#define SYNTH_H

#include <Arduino.h>
#include "control.h" 
#include "driver/i2s.h"
#include "driver/dac.h"
#include <math.h>

// Waveform Function Pointers
typedef int16_t (*WaveformFunc)(double phase);

// --- I2S Configuration & Audio Constants ---
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE 44100
#define SINE_TABLE_SIZE 512
#define DMA_BUF_LEN 64
#define AUDIO_BUFFER_SIZE (DMA_BUF_LEN * 2) 

// Global I2S Configuration (DECLARED HERE, DEFINED IN synth.cpp)
extern const i2s_config_t i2s_config;

// --- Note & Scale Constants (DECLARED HERE, DEFINED IN synth.cpp) ---
const int MIDI_C4 = 60;
extern const char* NOTE_NAMES[];

// Scale Step Intervals (in semitones) (DECLARED HERE, DEFINED IN synth.cpp)
extern const int SCALE_MAJOR[]; 
extern const int SCALE_MINOR[]; 
extern const int SCALE_PENT_MAJOR[]; 
extern const int SCALE_PENT_MINOR[]; 

// Waveform Enumeration
enum WaveType { SINE, SQUARE, SAW, TRIANGLE };
extern const char* WAVE_NAMES[];

// Global Array to hold the pre-calculated Sine Table
extern int16_t SINE_TABLE[SINE_TABLE_SIZE];

// Forward declaration of the global Synth instance
class Synth;
extern Synth synth; 

// --- Core Oscillator Class ---
class Oscillator {
private:
    double phaseAccumulator = 0.0;
    double frequency = 0.0; 
    double phaseIncrement = 0.0;
    WaveType wave = SINE;
    
    static int16_t generateSine(double phase);
    static int16_t generateSquare(double phase);
    static int16_t generateSaw(double phase);
    static int16_t generateTriangle(double phase);
    
public:
    void setWaveform(WaveType type) { wave = type; }
    void setFrequency(double freq);
    double getFrequency() const { return frequency; }
    int16_t getNextSample();
};

// --- Envelope Class (MODIFIED) ---
class Envelope {
public:
    enum State { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };

private:
    State state = IDLE;
    double currentGain = 0.0;
    
    // Calculated increments/coefficients
    double attackRate;
    double decayRate;
    double releaseRateFixed; // Fixed rate of 1.0 / (R_Time * SR)
    double sustainLevel;
    
    double releaseStartGain; // NEW: Capture the gain when noteOff is triggered
    
public:
    void setup(double attackTime, double decayTime, double sustainLvl, double releaseTime); 
    void noteOn();
    void noteOff();
    double getNextGain();
    State getState() const { return state; }
};


// --- Voice Class ---
class Voice {
public:
    Oscillator osc1;
    Oscillator osc2;
    Envelope envelope; 
    int keyIndex = -1; 
    
    void noteOn(double freq, WaveType wave1, WaveType wave2);
    void noteOff();
    int16_t getSample();
};


// --- Main Synth Class ---
class Synth {
private: 
    int16_t audioBuffer[AUDIO_BUFFER_SIZE]; 
    uint16_t currentKeyBitmap = 0; 
    
    void calculateScale(int rootMIDI, int type);

public:
    // Global parameters controlled by Web UI
    WaveType osc1Wave = SINE;
    WaveType osc2Wave = SINE;
    double osc1Gain = 1.0;
    double osc2Gain = 0.0; 
    bool osc2Enabled = false;

    // ADSR Envelope Parameters
    double attackTime = 0.05; // seconds
    double decayTime = 0.1;   // seconds
    double sustainLevel = 0.5; // 0.0 to 1.0
    double releaseTime = 0.5; // seconds

    // Polyphony: An array of 16 independent voices
    Voice voices[TOTAL_KEYS]; 

    // Scale mapping and UI state
    int currentScale[TOTAL_KEYS]; 
    int rootNoteMIDI = MIDI_C4; 
    int scaleType = 0; 
    
    // UI state for key reporting
    int lastPlayingKeyIndex = -1; 
    
    void begin();
    void setKeyBitmap(uint16_t bitmap);
    void setScale(int rootMIDI, int type);
    void setCustomNote(int keyIndex, int midiNote);
    
    void setADSR(double a, double d, double s, double r);
    
    void audioGeneratorLoop();

    static void audioTask(void *parameter);
};

// --- Helper function for MIDI to Frequency Conversion ---
double midiToFrequency(int midiNote);

#endif