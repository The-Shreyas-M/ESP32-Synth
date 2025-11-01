// synth.cpp

#include "synth.h" 

// -------------------------------------------------------------------
// --- GLOBAL DEFINITIONS ---
// -------------------------------------------------------------------

// I2S Configuration
const i2s_config_t i2s_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
  .sample_rate = I2S_SAMPLE_RATE,
  .bits_per_sample = (i2s_bits_per_sample_t)16,
  .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
  .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_MSB,
  .intr_alloc_flags = 0,
  .dma_buf_count = 8,
  .dma_buf_len = DMA_BUF_LEN, 
  .use_apll = false 
};

// Note & Wave Names
const char* NOTE_NAMES[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
const char* WAVE_NAMES[] = {"Sine", "Square", "Sawtooth", "Triangle"};

// Scale Step Intervals
const int SCALE_MAJOR[] = {2, 2, 1, 2, 2, 2, 1}; 
const int SCALE_MINOR[] = {2, 1, 2, 2, 1, 2, 2}; 
const int SCALE_PENT_MAJOR[] = {2, 2, 3, 2, 3}; 
const int SCALE_PENT_MINOR[] = {3, 2, 2, 3, 2}; 

// Global Synth Objects
int16_t SINE_TABLE[SINE_TABLE_SIZE]; 
Synth synth; 


// -------------------------------------------------------------------
// --- HELPER FUNCTION: MIDI TO FREQUENCY ---
// -------------------------------------------------------------------
double midiToFrequency(int midiNote) {
    return 440.0 * pow(2.0, (midiNote - 69.0) / 12.0);
}


// -------------------------------------------------------------------
// --- OSCILLATOR CLASS IMPLEMENTATION ---
// -------------------------------------------------------------------

int16_t Oscillator::generateSine(double phase) { return SINE_TABLE[(int)phase]; }
int16_t Oscillator::generateSquare(double phase) { return (phase < SINE_TABLE_SIZE / 2) ? 32767 : -32767; }
int16_t Oscillator::generateSaw(double phase) { return (int16_t)((phase / SINE_TABLE_SIZE) * 2.0 * 32767.0 - 32767.0); }
int16_t Oscillator::generateTriangle(double phase) {
    if (phase < SINE_TABLE_SIZE / 2) {
        return (int16_t)((phase / (SINE_TABLE_SIZE / 2.0)) * 2.0 * 32767.0 - 32767.0);
    } else {
        double downPhase = phase - SINE_TABLE_SIZE / 2.0;
        return (int16_t)(32767.0 - (downPhase / (SINE_TABLE_SIZE / 2.0)) * 2.0 * 32767.0);
    }
}

void Oscillator::setFrequency(double freq) { 
    frequency = freq;
    phaseIncrement = frequency * SINE_TABLE_SIZE / I2S_SAMPLE_RATE;
}

int16_t Oscillator::getNextSample() {
    if (frequency <= 0.0) return 0; 

    WaveformFunc generator; 
    switch (wave) {
        case SQUARE: generator = &Oscillator::generateSquare; break;
        case SAW: generator = &Oscillator::generateSaw; break;
        case TRIANGLE: generator = &Oscillator::generateTriangle; break;
        case SINE: 
        default: generator = &Oscillator::generateSine; break;
    }
    
    int16_t sample = generator(phaseAccumulator);

    phaseAccumulator += phaseIncrement;
    if (phaseAccumulator >= SINE_TABLE_SIZE) {
        phaseAccumulator -= SINE_TABLE_SIZE;
    }

    return sample; 
}


// -------------------------------------------------------------------
// --- ENVELOPE CLASS IMPLEMENTATION (FIXED RELEASE) ---
// -------------------------------------------------------------------

void Envelope::setup(double attackTime, double decayTime, double sustainLvl, double releaseTime) {
    attackTime = max(0.001, attackTime); 
    decayTime = max(0.001, decayTime);
    releaseTime = max(0.001, releaseTime);
    sustainLvl = constrain(sustainLvl, 0.0, 1.0);
    
    attackRate = 1.0 / (attackTime * I2S_SAMPLE_RATE);
    decayRate = (1.0 - sustainLvl) / (decayTime * I2S_SAMPLE_RATE);
    releaseRateFixed = 1.0 / (releaseTime * I2S_SAMPLE_RATE); 
    sustainLevel = sustainLvl;
    
    currentGain = 0.0;
    state = IDLE;
}

void Envelope::noteOn() {
    state = ATTACK;
}

void Envelope::noteOff() {
    // Only trigger release if we are currently active (Attack, Decay, or Sustain)
    if (state == ATTACK || state == DECAY || state == SUSTAIN) {
        releaseStartGain = currentGain; // Capture current gain to calculate dynamic rate
        state = RELEASE;
    }
}

double Envelope::getNextGain() {
    switch (state) {
        case IDLE:
            currentGain = 0.0;
            break;
        case ATTACK:
            currentGain += attackRate;
            if (currentGain >= 1.0) {
                currentGain = 1.0;
                state = DECAY;
            }
            break;
        case DECAY:
            currentGain -= decayRate;
            if (currentGain <= sustainLevel) {
                currentGain = sustainLevel;
                state = SUSTAIN;
            }
            break;
        case SUSTAIN:
            currentGain = sustainLevel;
            break;
        case RELEASE:
            // Dynamic release rate: Calculated to ensure the decay reaches 0.0 from releaseStartGain 
            // in the total 'releaseTime'.
            // Simple linear decay: (Gain at noteOff / Release Time) / Sample Rate
            double dynamicReleaseRate = releaseStartGain * releaseRateFixed;
            
            currentGain -= dynamicReleaseRate;
            
            if (currentGain <= 0.0) {
                currentGain = 0.0;
                state = IDLE;
            }
            break;
    }
    // Clamp to ensure stability
    return constrain(currentGain, 0.0, 1.0);
}


// -------------------------------------------------------------------
// --- VOICE CLASS IMPLEMENTATION ---
// -------------------------------------------------------------------

void Voice::noteOn(double freq, WaveType wave1, WaveType wave2) {
    osc1.setWaveform(wave1);
    osc1.setFrequency(freq);
    
    osc2.setWaveform(wave2);
    osc2.setFrequency(freq); 
    
    envelope.noteOn(); 
}

void Voice::noteOff() {
    envelope.noteOff(); 
}

int16_t Voice::getSample() {
    double envGain = envelope.getNextGain(); 

    if (envGain <= 0.0 && envelope.getState() == Envelope::IDLE) {
        // Stop oscillator activity once the voice is fully silent (in IDLE state)
        if (osc1.getFrequency() > 0.0) {
            osc1.setFrequency(0.0);
            osc2.setFrequency(0.0);
        }
        return 0; 
    } 

    int16_t sample1 = (int16_t)(osc1.getNextSample() * synth.osc1Gain);
    
    int16_t sample2 = 0;
    if (synth.osc2Enabled && synth.osc2Gain > 0.0) {
        sample2 = (int16_t)(osc2.getNextSample() * synth.osc2Gain);
    }
    
    int16_t finalSample = (int16_t)(((double)sample1 + (double)sample2) * envGain / 2.0);
    
    return finalSample;
}


// -------------------------------------------------------------------
// --- SYNTH CLASS IMPLEMENTATION ---
// -------------------------------------------------------------------

void Synth::setADSR(double a, double d, double s, double r) {
    attackTime = a;
    decayTime = d;
    sustainLevel = s;
    releaseTime = r;
    
    // Update the envelope setup for ALL voices
    for (int i = 0; i < TOTAL_KEYS; i++) {
        voices[i].envelope.setup(attackTime, decayTime, sustainLevel, releaseTime);
    }
    Serial.printf("Synth: ADSR set to A:%.3fs, D:%.3fs, S:%.3f, R:%.3fs\n", a, d, s, r);
}


void Synth::begin() {
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        SINE_TABLE[i] = (int16_t)(sin(i * 2.0 * PI / SINE_TABLE_SIZE) * 32767);
    }
    
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, NULL);
    dac_output_enable(DAC_CHANNEL_1); 
    dac_output_enable(DAC_CHANNEL_2);
    
    setScale(MIDI_C4, 0); 
    
    setADSR(attackTime, decayTime, sustainLevel, releaseTime);

    Serial.println("Synth Engine: I2S, Controllable ADSR, & 16 Polyphonic Voices ready.");
}

void Synth::calculateScale(int rootMIDI, int type) {
    const int* scaleIntervals;
    int numSteps;

    switch (type) {
        case 0: scaleIntervals = SCALE_MAJOR; numSteps = 7; break;
        case 1: scaleIntervals = SCALE_MINOR; numSteps = 7; break;
        case 2: scaleIntervals = SCALE_PENT_MAJOR; numSteps = 5; break;
        case 3: scaleIntervals = SCALE_PENT_MINOR; numSteps = 5; break;
        case 4: 
        default: return; 
    }
    
    for (int i = 0; i < TOTAL_KEYS; i++) {
        int midiNote = rootMIDI;
        
        for (int j = 0; j < i; j++) {
            midiNote += scaleIntervals[j % numSteps];
        }
        
        currentScale[i] = midiNote;
    }
}

void Synth::setScale(int rootMIDI, int type) {
    rootNoteMIDI = rootMIDI;
    scaleType = type;
    
    if (type != 4) {
        calculateScale(rootMIDI, type);
    }
    
    Serial.printf("Synth: Scale set to Type %d (Root MIDI %d). Mapped %d keys.\n", type, rootMIDI, TOTAL_KEYS);
}

void Synth::setCustomNote(int keyIndex, int midiNote) {
    if (keyIndex >= 0 && keyIndex < TOTAL_KEYS) {
        currentScale[keyIndex] = midiNote;
        scaleType = 4;
        Serial.printf("Synth: Custom Key K%d mapped to MIDI %d.\n", keyIndex + 1, midiNote);
    }
}

void Synth::setKeyBitmap(uint16_t bitmap) {
    currentKeyBitmap = bitmap;
    
    for (int i = 0; i < TOTAL_KEYS; i++) {
        bool isPressed = (bitmap & (1 << i));
        
        if (isPressed) {
            // Key is pressed: Trigger noteOn only if voice is silent or has already released
            if (voices[i].envelope.getState() == Envelope::IDLE || voices[i].envelope.getState() == Envelope::RELEASE) { 
                double freq = midiToFrequency(currentScale[i]);
                voices[i].noteOn(freq, osc1Wave, osc2Wave); 
                voices[i].keyIndex = i;
                lastPlayingKeyIndex = i; 
            }
        } else {
            // Key is released: Trigger noteOff only if the voice is currently in a key-down phase
            if (voices[i].envelope.getState() != Envelope::IDLE && voices[i].envelope.getState() != Envelope::RELEASE) { 
                voices[i].noteOff();
            }
        }
        
        // This check is a safety for UI status reporting
        if (voices[i].envelope.getState() == Envelope::IDLE) {
            voices[i].keyIndex = -1;
        }
    }
}

void Synth::audioGeneratorLoop() {
    while (true) {
        int samplesToGenerate = i2s_config.dma_buf_len;
        int totalVoicesActive = 0;

        for (int i = 0; i < samplesToGenerate; i++) {
            int32_t mixedSample32Bit = 0;
            totalVoicesActive = 0;

            for (int v = 0; v < TOTAL_KEYS; v++) {
                if (voices[v].envelope.getState() != Envelope::IDLE) {
                    mixedSample32Bit += voices[v].getSample();
                    totalVoicesActive++;
                }
            }
            
            // Mixing factor divided by 4 for headroom
            int16_t finalMixedSample = (int16_t)(mixedSample32Bit / 4);

            // DAC output needs 8-bit samples * 256 for 16-bit space
            // This is the correct way to map a signed 16-bit sample (centered at 0) 
            // to an unsigned 8-bit sample (centered at 128) and then shift it.
            uint8_t sample8Bit = (uint8_t)((finalMixedSample >> 8) + 128);
            int16_t final_sample = sample8Bit << 8;

            audioBuffer[i * 2] = final_sample;
            audioBuffer[i * 2 + 1] = final_sample;
        }

        size_t bytes_written;
        i2s_write(I2S_PORT, audioBuffer, sizeof(audioBuffer), &bytes_written, portMAX_DELAY);
        
        if (totalVoicesActive == 0) {
            vTaskDelay(1); 
        }
    }
}

void Synth::audioTask(void *parameter) {
    synth.audioGeneratorLoop(); 
}