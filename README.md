# Cyberverb X1

Open Source VST3 / AU audio effect

<img width="2244" height="1256" alt="Screenshot_CyberverbX1" src="https://github.com/user-attachments/assets/e33a6b54-459f-438f-89ee-d0f2d847ad6e" />
<br/><br/>
**Cyberverb X1** is a dual-engine hybrid reverb and delay workstation built with JUCE / C++. Designed around two independent, morphable stereo processing spaces (**Space A** and **Space B**), it combines flexible signal routing, tempo-synced delays, modulated reverb tanks, internal dynamics, and real-time modulation in a unified hardware-inspired interface.

---

## Key Features

### Dual Hybrid Space Engines (Space A & Space B)
* **Independent Delay + Reverb Chains:** Each space features a dedicated stereo delay stage feeding into a modulated stereo reverb tank, with continuous `Delay > Reverb` routing balance.
* **Delay Modes & Sync:** Supports **Stereo**, **Ping-Pong**, **Tape Echo** (with warm saturation), and **Dual Tap** modes, switchable between free millisecond timing (`1 ms – 2000 ms`) and host BPM tempo sync (`1/16` to `1/1`).
* **Tailored Reverb Characters:**
  * **Space A:** *Plate*, *Room*, *Hall*, *Chamber*, and *Vintage Spring*
  * **Space B:** *Shimmer Cloud*, *Cyber Hall*, *Granular Mist*, *Dark Void*, and *Reverse Bloom*
* **Scene Morphing & Cross-Feedback:** Blend seamlessly between both spaces using the dedicated `SPACE A`, `MORPH`, and `SPACE B` faders, or feed both engines into each other via adjustable `Cross Feedback`.

### Flexible Routing & Internal Dynamics
* **5 Signal Routing Modes:**
  * **Parallel:** Both engines process the input signal simultaneously.
  * **Serial A > B / Serial B > A:** Cascade one space directly into the other for dense, evolving textures.
  * **Mid / Side:** Process the center (Mid) and stereo sides (Side) independently.
  * **Band Split:** Linkwitz-Riley crossover (`60 Hz – 8 kHz`) to keep low frequencies tight in Space A while expanding highs in Space B.
* **Input Drive & 4-Band Wet EQ:** Pre-stage saturation (`Input Drive`) and a post-engine 4-band equalizer (`Low Shelf`, `Mid 1`, `Mid 2`, `High Shelf`) to sculpt the wet signal without affecting the dry path.
* **Self-Ducking & Gated Reverb:** Built-in input-triggered sidechain ducking (`Ducking Amount` & `Release`) keeps dry transients clear under massive tails, complemented by a threshold-controlled noise gate for classic 80s gated reverb effects.

### Modulation & Performance Workflow
* **Modulation Engine & 4-Slot Matrix:** Two multi-waveform LFOs (*Sine*, *Triangle*, *Saw Up*, *Square*, *Sample & Hold*) and an **Input Envelope Follower** can be routed to morphing, stereo width, delay times, feedback, decay, and room size.
* **Live Performance Controls:**
  * **MIX LOCK:** Locks `DRY` and `WET` levels when browsing presets (ideal for Aux/Send bus setups).
  * **TAILS:** True bypass spillover lets active reverb and delay tails ring out naturally when bypassed.
  * **LINK A/B:** Links corresponding parameters across Space A and Space B for simultaneous editing.
  * **MUTE IN & FREEZE:** Cut the engine input send on the fly while letting tails ring out, or freeze reverb/delay buffers infinitely.
  * **FLUSH:** Instantly clears all active delay and reverb buffers.
* **Direct Numeric Keypad & MIDI Learn:** Enter exact parameter values via the on-screen 10-key pad or hardware keyboard, and assign MIDI CCs (with **Soft-Takeover / Pickup** and **Relative Encoder** support) to the 8 main engine performance knobs.
* **Dual Stereo Metering & Preset Manager:** Real-time `IN` and `OUT` L/R peak meters with peak-hold indicators, plus an XML-based preset browser supporting Factory and User categories.

---

## Building from Source

Cyberverb X1 uses **CMake** (3.22+) and automatically fetches **JUCE 8.0.4** via `FetchContent` during the initial configuration step.

### Prerequisites (All Platforms)
* **Git**
* **CMake** `3.22` or newer
* A C++20-compatible compiler (Apple Clang, GCC/Clang, or MSVC 2022)

---

### macOS (Apple Silicon & Intel)

#### 1. Install Build Tools
Install Xcode Command Line Tools and CMake (via [Homebrew](https://brew.sh/)):
```bash
xcode-select --install
brew install cmake
```

#### 2. Configure & Build (Universal Binary)
```bash
git clone https://github.com/NetAndif/cyberverb-x1.git
cd cyberverb-x1

cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release -j
```

#### 3. Install Plugins & Standalone App
The compiled artifacts are located in `build/CyberverbX1_artefacts/Release/`:
```bash
cp -R "build/CyberverbX1_artefacts/Release/VST3/CyberverbX1.vst3" ~/Library/Audio/Plug-Ins/VST3/
cp -R "build/CyberverbX1_artefacts/Release/AU/CyberverbX1.component" ~/Library/Audio/Plug-Ins/Components/
cp -R "build/CyberverbX1_artefacts/Release/Standalone/CyberverbX1.app" /Applications/
```

#### 4. macOS Gatekeeper & Unsigned App Notice
Pre-built GitHub releases (or bundles copied from another Mac) are **ad-hoc signed** and not notarized with an Apple Developer ID certificate. When launching the Standalone app or scanning the VST3/AU plugins in your DAW for the first time, macOS Gatekeeper may show a warning (*"Cyberverb X1 cannot be opened because the developer cannot be verified"* or *"is damaged and can't be opened"*).

To allow macOS to run the unsigned binaries, remove the quarantine attribute in Terminal:
```bash
xattr -cr "/Applications/CyberverbX1.app"
xattr -cr ~/Library/Audio/Plug-Ins/VST3/"CyberverbX1.vst3"
xattr -cr ~/Library/Audio/Plug-Ins/Components/"CyberverbX1.component"
```
Alternatively, open **System Settings → Privacy & Security**, scroll down to the **Security** section, and click **Open Anyway** / **Allow Anyway** next to the blocked Cyberverb X1 entry.

*(Note: After installing a new Audio Unit `.component` on macOS, you may need to log out and back in—or run `killall -9 AudioComponentRegistrar`—for Logic Pro / GarageBand to recognize the new AU plugin.)*

---

### Ubuntu / Debian Linux

#### 1. Install Dependencies
Install the required compiler toolchain and JUCE Linux development libraries (X11, ALSA, FreeType, WebKit/GTK, cURL):
```bash
sudo apt update
sudo apt install -y build-essential cmake git pkg-config \
    libasound2-dev libjack-jackd2-dev \
    libfreetype6-dev libfontconfig1-dev \
    libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
    libxinerama-dev libxrandr-dev libxrender-dev \
    libgl1-mesa-dev libglu1-mesa-dev \
    libgtk-3-dev libwebkit2gtk-4.1-dev libcurl4-openssl-dev
```
*(Note: On older Ubuntu releases such as 22.04 LTS, replace `libwebkit2gtk-4.1-dev` with `libwebkit2gtk-4.0-dev`.)*

#### 2. Configure & Build
```bash
git clone https://github.com/NetAndif/cyberverb-x1.git
cd cyberverb-x1

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)
```

#### 3. Install VST3 & Standalone Binary
```bash
mkdir -p ~/.vst3 ~/.local/bin
cp -r "build/CyberverbX1_artefacts/Release/VST3/CyberverbX1.vst3" ~/.vst3/
cp "build/CyberverbX1_artefacts/Release/Standalone/CyberverbX1" ~/.local/bin/
```

---

### Windows (10 / 11)

#### 1. Install Build Tools
* Install **Visual Studio 2022** (Community Edition is sufficient) with the **"Desktop development with C++"** workload selected.
* Install **CMake** (or use the CMake version bundled with Visual Studio) and **Git for Windows**.

#### 2. Configure & Build (PowerShell or x64 Native Tools Command Prompt)
```powershell
git clone https://github.com/NetAndif/cyberverb-x1.git
cd cyberverb-x1

cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

#### 3. Install VST3 & Standalone Executable
Copy the build outputs from `build\CyberverbX1_artefacts\Release\`:
* **VST3 Plugin:** Copy `CyberverbX1.vst3` to `C:\Program Files\Common Files\VST3\`
* **Standalone App:** Copy `CyberverbX1.exe` to any folder of your choice (e.g., `C:\Program Files\Cyberverb<img width="1024" height="1024" alt="icon" src="https://github.com/user-attachments/assets/f04e9184-7119-4496-8ac9-b1d1e413dd11" />
X1\`)

*(Note: If Windows Defender SmartScreen displays a warning on unsigned standalone builds, click **More info → Run anyway**.)*
