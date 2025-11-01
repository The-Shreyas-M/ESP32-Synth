// control.h

#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>

// --- MATRIX CONFIGURATION CONSTANTS (Shared) ---
const int NUM_ROWS = 4;
const int NUM_COLS = 4;
const int TOTAL_KEYS = NUM_ROWS * NUM_COLS; 
// ----------------------------------------------

class Control {
private:
    // GPIO Pins (Drivers and Sensors)
    const int rowPins[NUM_ROWS] = {15, 2, 4, 5}; 
    const int colPins[NUM_COLS] = {18, 19, 21, 22};
    
    // Debouncing variables (NEW)
    uint16_t previousBitmap = 0;
    uint16_t currentStableBitmap = 0;
    unsigned long lastChangeTime = 0;
    // Set to 10ms—a reliable value for mechanical switches
    const unsigned long DEBOUNCE_DELAY = 10; 

public:
    void begin();
    uint16_t getPressedKeysBitmap();
};

#endif