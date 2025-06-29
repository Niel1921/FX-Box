# FX-Box
**Accessible Effects Processor for Bela Board with Touch Input**

FX-Box is an open-source, entry-level effects processor designed for music therapy and accessible music production. Built for the Bela board, FX-Box provides real-time audio processing with intuitive touch controls, visual feedback, and straightforward operation—making creative audio effects approachable for everyone.

---

## Requirements

To use this software, you will need:

- **Bela Board** (with BeagleBone Black and Bela Cape)
- **Audio input/output cables** (3.5mm jack)
- **Trill Touch Sensors** (Trill Bar for single parameter, Trill Square for X/Y parameter)
- **OLED Display** (64x128 pixels, 0.96" monochrome)
- **3 Buttons** (for menu navigation and selection)

Optional:
- **3D-printed housing** 

---

## Features

- **Real-Time Effects Processing**  
  Built with C++ on Bela for ultra-low latency audio FX and instant parameter changes.

- **Accessible Touch Control**  
  Touch strips (single and X/Y) make parameter control intuitive for users with a wide range of abilities, ideal for music therapy contexts.

- **Visual Feedback**  
  The OLED display provides clear, simple menus and live visualizers for each effect, helping users understand and engage with sound-shaping.

- **Multiple Effects Included**
  - **Delay** (X/Y control: delay time & feedback)
  - **Reverb** (X/Y control: reverb time & density)
  - **High/Low Pass Filter** (X/Y control: cutoff frequency & resonance)
  - **Tremolo** (X/Y control: rate & depth)
  - **Oscillator Mode** (for learning FX without external input)

- **Recording Functionality**  
  Record up to 15 seconds of processed audio with a single button press.

- **Easy Navigation**  
  Three buttons allow simple menu movement, mode selection, and operation.

- **Designed for Music Therapy and Learning**  
  Multi-sensory feedback (audio, visual, tactile) helps engage users, especially those with ASD or beginners in music tech.

---

## Getting Started

1. **Assemble hardware:**  
   Connect the Bela board, touch sensors, OLED display, and buttons according to the provided schematic and enclosure files.

2. **Install software:**  
   Clone this repository to your Bela board and build according to the [Bela documentation](https://github.com/BelaPlatform/Bela).

3. **Run FX-Box:**  
   Power on the Bela board; FX-Box should start automatically. Use the buttons to navigate menus and the touch strips to control effect parameters.

4. **Audio I/O:**  
   Plug a microphone or instrument into the 3.5mm input jack. Connect headphones or speakers to the output.

---

## System Overview

- **Main Modes:**
  - *Voice Mode*: Apply effects live to microphone/instrument input.
  - *Oscillator Mode*: Learn effects using internal sound sources (various waveforms).
  - *Recording Mode*: Capture short processed clips.

- **User Interface:**
  - Large, readable menus and icons.
  - Visualizers for each effect, showing real-time changes.

- **Touch Sensors:**
  - Single-parameter: simple FX control (e.g., filter sweep).
  - X/Y-parameter: simultaneous control of two effect parameters.

---

## Hardware Breakdown

To use this software you require:

Bela board
Audio input and output cables
Touch strips (X/Y and Single parameter)
OLED Display 64*128
3 buttons

---


## License

FX-Box is open-source and available under the MIT License.
