// control.cpp

#include "control.h"

void Control::begin() {
    // Setup Row Pins (OUTPUT)
    for (int i = 0; i < NUM_ROWS; i++) {
        pinMode(rowPins[i], OUTPUT);
        digitalWrite(rowPins[i], HIGH); // Rows start HIGH
    }
    
    // Setup Column Pins (INPUT_PULLUP)
    for (int i = 0; i < NUM_COLS; i++) {
        pinMode(colPins[i], INPUT_PULLUP);
    }
    
    // Initialize debouncing state
    previousBitmap = 0;
    currentStableBitmap = 0;
    lastChangeTime = millis();
    
    Serial.println("Control: Keypad Initialized.");
}

uint16_t Control::getPressedKeysBitmap() {
    uint16_t currentBitmap = 0;
    
    // --- SCAN THE 4x4 MATRIX AND BUILD BITMAP (same as before) ---
    for (int row = 0; row < NUM_ROWS; row++) {
        
        // ACTIVATE the current Row: Drive it LOW
        digitalWrite(rowPins[row], LOW);
        delayMicroseconds(10); // Settle time for reading

        // READ all 4 Columns
        for (int col = 0; col < NUM_COLS; col++) {
            int keyIndex = (row * NUM_COLS) + col;
            
            // LOW = Pressed 
            if (digitalRead(colPins[col]) == LOW) {
                currentBitmap |= (1 << keyIndex);
            }
        }
        
        // DEACTIVATE the current Row: Drive it HIGH
        digitalWrite(rowPins[row], HIGH);
    }

    // --- DEBOUNCE LOGIC (NEW) ---
    // 1. If the reading changed from the previous state, reset the timer
    if (currentBitmap != previousBitmap) {
        lastChangeTime = millis();
    }
    
    // 2. If the time since the last change exceeds the delay, stabilize the output
    if ((millis() - lastChangeTime) > DEBOUNCE_DELAY) {
        currentStableBitmap = currentBitmap;
    }
    
    // 3. Update the previous state for the next loop
    previousBitmap = currentBitmap;
    
    return currentStableBitmap; // Return the stable, debounced bitmap
}