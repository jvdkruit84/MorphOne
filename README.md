# Morph::One

A polyphonic VST3 synthesizer plugin built with [JUCE](https://juce.com/).

## Features

- 8-voice polyphonic sine wave synthesizer
- Gain control
- VST3 & Standalone formats
- Preset state save/load

## Download

Go to the [Releases](https://github.com/jvdkruit84/MorphOne/releases) page to download the latest compiled plugin.

### Installation

**Linux:** Copy `MorphOne.vst3` to `~/.vst3/`

**macOS:** Copy `MorphOne.vst3` to `~/Library/Audio/Plug-Ins/VST3/`

**Windows:** Copy `MorphOne.vst3` to `C:\Program Files\Common Files\VST3\`

## Build from source

### Requirements

- CMake 3.22+
- C++17 compiler
- JUCE dependencies (Linux):

```bash
sudo apt install cmake g++ libfreetype6-dev libx11-dev libxinerama-dev \
  libxrandr-dev libxcursor-dev libxi-dev libgl-dev libasound2-dev
```

### Build

```bash
git clone --recurse-submodules https://github.com/jvdkruit84/MorphOne.git
cd MorphOne
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)
```

The plugin will be in `build/MorphOne_artefacts/Release/VST3/`.

## Roadmap

- [ ] Multiple oscillator types (saw, square, triangle)
- [ ] ADSR envelope
- [ ] Low-pass filter with cutoff & resonance
- [ ] LFO modulation
- [ ] Effects (reverb, delay)
