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
