// HTML_content.h

#ifndef HTML_CONTENT_H
#define HTML_CONTENT_H

const char INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>APSIT_SYNTH Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin: 0; background-color: #222; color: #eee; }
        .container { max-width: 500px; margin: auto; padding: 15px; background-color: #333; box-shadow: 0 4px 8px rgba(0,0,0,0.5); }
        h1 { color: #88F; border-bottom: 2px solid #555; padding-bottom: 10px; margin-bottom: 20px; }
        h3 { color: #fff; margin-top: 25px; margin-bottom: 10px; }
        .control-group { margin-bottom: 20px; padding: 10px; border: 1px solid #444; border-radius: 8px; background-color: #444; text-align: left; }
        label { display: block; margin-bottom: 5px; font-weight: bold; color: #ccc; }
        select, input[type="range"] { width: 100%; padding: 8px; margin-bottom: 10px; border: 1px solid #666; border-radius: 4px; box-sizing: border-box; background-color: #555; color: #eee; }
        select { height: 35px; }
        .switch { position: relative; display: inline-block; width: 60px; height: 34px; margin-left: 20px; vertical-align: middle; }
        .switch input { opacity: 0; width: 0; height: 0; }
        .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; }
        .slider:before { position: absolute; content: ""; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }
        input:checked + .slider { background-color: #2196F3; }
        input:checked + .slider:before { transform: translateX(26px); }
        .slider.round { border-radius: 34px; }
        .slider.round:before { border-radius: 50%; }
        #note_status { font-size: 1.2em; font-weight: bold; color: #0F0; margin-bottom: 15px; }
        
        /* New Styles for Keypad */
        #custom_mapping_group { display: none; }
        .key-grid {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 5px;
            margin-top: 10px;
        }
        .key-map-item {
            background-color: #555;
            padding: 5px;
            border-radius: 5px;
            text-align: center;
        }
        .key-map-item label { margin-bottom: 2px; font-size: 0.8em; }
        .key-map-item select { font-size: 0.7em; padding: 2px; height: 25px; margin-bottom: 0; }
        #save_custom_map { margin-top: 15px; padding: 10px 20px; background-color: #88F; border: none; border-radius: 5px; color: white; cursor: pointer; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 Polyphonic Synthesizer</h1>
        <div id="note_status">Current Note: None</div>

        <div class="control-group">
            <h3>ADSR Envelope (A/D/R in seconds, S is 0-1)</h3>
            <label>Attack Time (A): <span id="adsr_a_value">0.050s</span></label>
            <input type="range" id="adsr_a" min="0" max="300" value="50" oninput="updateADSRValue('a', this.value)" onmouseup="sendADSR()">
            
            <label>Decay Time (D): <span id="adsr_d_value">0.100s</span></label>
            <input type="range" id="adsr_d" min="0" max="300" value="100" oninput="updateADSRValue('d', this.value)" onmouseup="sendADSR()">
            
            <label>Sustain Level (S): <span id="adsr_s_value">0.50</span></label>
            <input type="range" id="adsr_s" min="0" max="100" value="50" oninput="updateADSRValue('s', this.value)" onmouseup="sendADSR()">
            
            <label>Release Time (R): <span id="adsr_r_value">0.500s</span></label>
            <input type="range" id="adsr_r" min="0" max="500" value="500" oninput="updateADSRValue('r', this.value)" onmouseup="sendADSR()">
        </div>
        
        <div class="control-group">
            <h3>Oscillator 1</h3>
            <label for="osc1_wave">Waveform:</label>
            <select id="osc1_wave" onchange="sendWaveform(1, this.value)">
                <option value="0" selected>Sine</option>
                <option value="1">Square</option>
                <option value="2">Sawtooth</option>
                <option value="3">Triangle</option>
            </select>
            <label>Gain: <span id="gain_value_1">1.00</span></label>
            <input type="range" id="osc1_gain" min="0" max="100" value="100" oninput="updateGainValue(1, this.value)" onmouseup="sendGain(1, this.value)">
        </div>

        <div class="control-group">
            <h3>Oscillator 2</h3>
            <label for="osc2_wave">Waveform:</label>
            <select id="osc2_wave" onchange="sendWaveform(2, this.value)">
                <option value="0" selected>Sine</option>
                <option value="1">Square</option>
                <option value="2">Sawtooth</option>
                <option value="3">Triangle</option>
            </select>
            <label>Gain: <span id="gain_value_2">0.00</span></label>
            <input type="range" id="osc2_gain" min="0" max="100" value="0" oninput="updateGainValue(2, this.value)" onmouseup="sendGain(2, this.value)">
            
            <label>OSC 2 Enable: 
                <label class="switch">
                    <input type="checkbox" id="osc2_enable" onchange="sendOsc2Toggle(this.checked)">
                    <span class="slider"></span>
                </label>
            </label>
        </div>
        
        <div class="control-group">
            <h3>Scale Mapping</h3>
            
            <label for="root_note">Root Note (C4 by default):</label>
            <select id="root_note" onchange="sendScaleUpdate()">
                </select>
            
            <label for="scale_type">Scale Type:</label>
            <select id="scale_type" onchange="handleScaleTypeChange()">
                <option value="0" selected>Major (Default)</option>
                <option value="1">Minor</option>
                <option value="2">Pentatonic Major</option>
                <option value="3">Pentatonic Minor</option>
                <option value="4">Custom/Free Map</option>
            </select>
        </div>

        <div class="control-group" id="custom_mapping_group">
            <h3>Custom Key Assignments (4x4 Matrix)</h3>
            <div class="key-grid" id="key_map_grid">
                </div>
            <button id="save_custom_map" onclick="sendCustomMap()">Save Custom Map</button>
        </div>
    </div>

    <script>
        const rootNoteSelect = document.getElementById('root_note');
        const scaleTypeSelect = document.getElementById('scale_type');
        const customMappingGroup = document.getElementById('custom_mapping_group');
        const keyMapGrid = document.getElementById('key_map_grid');
        
        // --- Note Data ---
        const MIDI_NOTE_NAMES = [
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
        ];
        
        function getNoteName(midi) {
            const name = MIDI_NOTE_NAMES[midi % 12];
            const octave = Math.floor(midi / 12) - 1;
            return name + octave;
        }

        // --- UI Initialization Functions ---
        
        function populateRootNotes() {
            // Populate root notes from MIDI 48 (C3) to 84 (C6)
            for (let midi = 48; midi <= 84; midi++) {
                const option = document.createElement('option');
                option.value = midi;
                option.textContent = getNoteName(midi);
                if (midi === 60) { // C4 is MIDI 60, make it the default selected
                    option.selected = true;
                }
                rootNoteSelect.appendChild(option);
            }
        }
        
        function populateKeyMapGrid() {
            let html = '';
            // Generate 16 key map items
            for (let i = 0; i < 16; i++) {
                html += '<div class="key-map-item">';
                html += '<label>K' + (i + 1) + '</label>';
                html += '<select id="key_' + i + '_note" data-key-index="' + i + '">';
                
                // Populate dropdown with notes from MIDI 36 (C2) to 96 (C7)
                for (let midi = 36; midi <= 96; midi++) {
                    const noteName = getNoteName(midi);
                    html += '<option value="' + midi + '"';
                    
                    // Set default notes (e.g., Chromatic C4 scale)
                    // K1=C4(60), K2=C#4(61), ...
                    if (midi === (60 + i)) {
                        html += ' selected';
                    }
                    
                    html += '>' + noteName + '</option>';
                }
                html += '</select>';
                html += '</div>';
            }
            keyMapGrid.innerHTML = html;
        }

        // --- Event Handlers & Sending Data ---

        // Handler for ADSR
        const attackSlider = document.getElementById('adsr_a');
        const decaySlider = document.getElementById('adsr_d');
        const sustainSlider = document.getElementById('adsr_s');
        const releaseSlider = document.getElementById('adsr_r');
        const sustainValue = document.getElementById('adsr_s_value');
        
        function updateADSRValue(param, value) {
            let val = parseInt(value);
            let displayElement;
            let actualValue;

            if (param === 's') {
                actualValue = val / 100.0;
                displayElement = sustainValue;
                displayElement.textContent = actualValue.toFixed(2);
            } else {
                actualValue = val / 1000.0;
                displayElement = document.getElementById('adsr_' + param + '_value');
                displayElement.textContent = actualValue.toFixed(3) + 's';
            }
        }
        
        function sendADSR() {
            const a = parseInt(attackSlider.value) / 1000.0;
            const d = parseInt(decaySlider.value) / 1000.0;
            const s = parseInt(sustainSlider.value) / 100.0;
            const r = parseInt(releaseSlider.value) / 1000.0;

            const xhr = new XMLHttpRequest();
            const url = '/setadsr?a=' + a.toFixed(3) + '&d=' + d.toFixed(3) + '&s=' + s.toFixed(2) + '&r=' + r.toFixed(3);
            xhr.open('GET', url, true);
            xhr.send();
        }

        // Handler for Scale Type change (show/hide custom map)
        function handleScaleTypeChange() {
            if (scaleTypeSelect.value === '4') {
                customMappingGroup.style.display = 'block';
            } else {
                customMappingGroup.style.display = 'none';
                sendScaleUpdate(); // Send update when switching away from custom mode
            }
        }

        // Handler for Root Note / Scale Type update
        function sendScaleUpdate() {
            if (scaleTypeSelect.value !== '4') {
                const root = rootNoteSelect.value;
                const type = scaleTypeSelect.value;
                
                const xhr = new XMLHttpRequest();
                xhr.open("GET", "/setscale?root=" + root + "&type=" + type, true);
                xhr.send();
            }
        }
        
        // Handler for Custom Map save (POST request with all key data)
        function sendCustomMap() {
            const selects = keyMapGrid.querySelectorAll('select');
            let keyDataArray = [];
            
            selects.forEach(select => {
                const keyIndex = select.getAttribute('data-key-index');
                const midiNote = select.value;
                keyDataArray.push(keyIndex + ':' + midiNote);
            });
            
            const keyDataString = keyDataArray.join(',');
            
            const xhr = new XMLHttpRequest();
            xhr.open("POST", "/setcustom", true);
            // Set the content type header for POST data
            xhr.setRequestHeader("Content-Type", "application/x-www-form-urlencoded"); 
            // Send the data string
            xhr.send("key_data=" + keyDataString); 
            
            alert("Custom Key Map saved! (Requires 'Custom/Free Map' to be selected)");
        }
        

        // Existing functions...

        function sendWaveform(oscNum, waveType) {
            const xhr = new XMLHttpRequest();
            xhr.open("GET", "/setosc?osc=" + oscNum + "&wave=" + waveType, true);
            xhr.send();
        }

        function updateGainValue(oscNum, value) {
            let gain = parseInt(value) / 100.0;
            document.getElementById("gain_value_" + oscNum).textContent = gain.toFixed(2);
        }

        function sendGain(oscNum, value) {
            const xhr = new XMLHttpRequest();
            xhr.open("GET", "/setgain?osc=" + oscNum + "&gain=" + value, true);
            xhr.send();
        }

        function sendOsc2Toggle(checked) {
            const xhr = new XMLHttpRequest();
            const value = checked ? 1 : 0;
            xhr.open("GET", "/setgain?osc=2&enabled=" + value, true);
            xhr.send();
        }
        
        // Status update loop
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('note_status').textContent = "Current Note: " + data.note;
                })
                .catch(error => {
                    console.error('Error fetching status:', error);
                    document.getElementById('note_status').textContent = "Current Note: Error";
                });
        }

        setInterval(updateStatus, 100); // Poll status every 100ms
        
        // Initialize gain display and ADSR display on load
        document.addEventListener('DOMContentLoaded', () => {
            populateRootNotes(); // NEW
            populateKeyMapGrid(); // NEW
            
            updateGainValue(1, 100); 
            updateGainValue(2, 0);   
            // Initialize ADSR display
            const initialA = parseInt(attackSlider.value);
            const initialD = parseInt(decaySlider.value);
            const initialS = parseInt(sustainSlider.value);
            const initialR = parseInt(releaseSlider.value);
            updateADSRValue('a', initialA);
            updateADSRValue('d', initialD);
            updateADSRValue('s', initialS);
            updateADSRValue('r', initialR);
        });

    </script>
</body>
</html>
)rawliteral";

#endif