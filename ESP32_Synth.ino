// main.ino

#include "control.h"
#include "synth.h"
#include "UI.h" 

// Global instances
Control synthControl;
// The Synth instance is globally defined in synth.cpp

void setup() {
    Serial.begin(115200); 

    // 1. Initialize Control (Keypad Pins)
    synthControl.begin();
    
    // 2. Initialize Synth Engine (I2S, Sine Table, voices)
    synth.begin();
    
    // 3. Start the Web UI Server
    uiSetup();

    // 4. Create the audio task and pin it to Core 1
    xTaskCreatePinnedToCore(
        Synth::audioTask,      
        "AudioTask",    
        10000,          
        NULL,           
        2,              // Priority 2
        NULL,           
        1               // Core 1
    );
    
    Serial.println("System Boot Complete. Polyphonic Synth Ready.");
}

void loop() {
    // 1. Read Keypad state
    uint16_t keyStateBitmap = synthControl.getPressedKeysBitmap(); 

    // 2. Feed the state to the Synth Engine (triggers Note ON/OFF events)
    synth.setKeyBitmap(keyStateBitmap);

    // 3. Handle Web Client Requests (Runs on Core 0)
    uiLoop();
    
    // Minimal delay to throttle the loop speed
    delay(1); 
}