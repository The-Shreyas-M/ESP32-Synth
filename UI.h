// UI.h

#ifndef UI_H
#define UI_H

#include <WiFi.h>
#include <WebServer.h>
#include "synth.h"
#include "HTML_content.h"

// WiFi credentials
const char* ssid = "APSIT_SYNTH";
const char* password = "12345678";

// Instantiate the web server
WebServer server(80);

// --- HANDLERS ---

void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

void handleSetOsc() {
    int oscNum = server.arg("osc").toInt(); 
    int waveType = server.arg("wave").toInt(); 

    if (waveType >= SINE && waveType <= TRIANGLE) {
        if (oscNum == 1) {
            synth.osc1Wave = (WaveType)waveType;
        } else if (oscNum == 2) {
            synth.osc2Wave = (WaveType)waveType;
        }
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Invalid Wave Type");
    }
}

void handleSetGain() {
    int oscNum = server.arg("osc").toInt();
    
    // Handle Gain (0 to 100)
    if (server.hasArg("gain")) {
        double gain = server.arg("gain").toInt() / 100.0;
        
        if (oscNum == 1) {
            synth.osc1Gain = gain;
        } else if (oscNum == 2) {
            synth.osc2Gain = gain;
        }
        server.send(200, "text/plain", "OK");
        return;
    }
    
    // Handle OSC 2 Enable/Disable
    if (server.hasArg("enabled")) {
        bool enabled = server.arg("enabled").toInt() == 1;
        synth.osc2Enabled = enabled;
        server.send(200, "text/plain", "OK");
        return;
    }
    
    server.send(400, "text/plain", "Invalid Parameter");
}

void handleSetADSR() {
    double attack = server.arg("a").toFloat();
    double decay = server.arg("d").toFloat();
    double sustain = server.arg("s").toFloat();
    double release = server.arg("r").toFloat();
    
    synth.setADSR(attack, decay, sustain, release);
    
    server.send(200, "text/plain", "OK");
}


void handleSetScale() {
    int rootMIDI = server.arg("root").toInt();
    int type = server.arg("type").toInt();
    
    synth.setScale(rootMIDI, type);
    
    server.send(200, "text/plain", "OK");
}

// FIX: Corrected the string parsing logic to avoid infinite loop
void handleSetCustomNote() {
    if (server.method() == HTTP_POST && server.hasArg("key_data")) {
        String data = server.arg("key_data");
        
        int startIndex = 0;
        // Loop while there is remaining data to process
        while (startIndex < data.length()) {
            int commaIndex = data.indexOf(',', startIndex);
            String pair;
            
            if (commaIndex == -1) {
                // Last segment, no comma found
                pair = data.substring(startIndex);
                startIndex = data.length(); // Force termination of the loop next cycle
            } else {
                // Segment found before comma
                pair = data.substring(startIndex, commaIndex);
                startIndex = commaIndex + 1;
            }
            
            int colonIndex = pair.indexOf(':');
            if (colonIndex != -1) {
                // The pair format is "keyIndex:midiNote"
                int keyIndex = pair.substring(0, colonIndex).toInt();
                int midiNote = pair.substring(colonIndex + 1).toInt();

                // Set the note in the synth engine if index is valid
                if (keyIndex >= 0 && keyIndex < TOTAL_KEYS) {
                    synth.setCustomNote(keyIndex, midiNote);
                }
            }
        }
        
        // Ensure the synth switches to custom scale mode
        synth.scaleType = 4;
        server.send(200, "text/plain", "Custom Scale Set OK");
    } else {
        server.send(400, "text/plain", "Invalid Custom Note Request");
    }
}

void handleStatus() {
    String json = "{\"note\": \"";
    if (synth.lastPlayingKeyIndex != -1) {
        int midi = synth.currentScale[synth.lastPlayingKeyIndex];
        const char* noteName = NOTE_NAMES[midi % 12];
        int octave = (midi / 12) - 1;
        json += "K" + String(synth.lastPlayingKeyIndex + 1) + " (" + String(noteName) + String(octave) + ")";
    } else {
        json += "None";
    }
    json += "\"}";
    server.send(200, "application/json", json);
}

// --- SETUP & LOOP ---

void uiSetup() {
    WiFi.softAP(ssid, password);
    Serial.println("\n--- Starting UI Server ---");
    Serial.printf("Hotspot: %s\n", ssid);
    Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());

    server.on("/", handleRoot);
    server.on("/setosc", HTTP_GET, handleSetOsc);
    server.on("/setgain", HTTP_GET, handleSetGain);
    server.on("/setscale", HTTP_GET, handleSetScale);
    server.on("/setadsr", HTTP_GET, handleSetADSR); 
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/setcustom", HTTP_POST, handleSetCustomNote); 

    server.begin();
    Serial.println("Web Server running.");
}

void uiLoop() {
    server.handleClient();
}

#endif