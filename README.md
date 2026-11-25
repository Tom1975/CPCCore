# CPCCore

[![Build Windows](https://github.com/Tom1975/CPCCore/actions/workflows/build_windows.yml/badge.svg)](https://github.com/Tom1975/CPCCore/actions/workflows/build_windows.yml)
[![Build Ubuntu](https://github.com/Tom1975/CPCCore/actions/workflows/build_ubuntu.yml/badge.svg)](https://github.com/Tom1975/CPCCore/actions/workflows/build_ubuntu.yml)
[![Build MacOS](https://github.com/Tom1975/CPCCore/actions/workflows/build_macos.yml/badge.svg)](https://github.com/Tom1975/CPCCore/actions/workflows/build_macos.yml)
[![Test Windows](https://github.com/Tom1975/CPCCore/actions/workflows/test_windows.yml/badge.svg)](https://github.com/Tom1975/CPCCore/actions/workflows/test_windows.yml)
[![Test Ubuntu](https://github.com/Tom1975/CPCCore/actions/workflows/test_ubuntu.yml/badge.svg)](https://github.com/Tom1975/CPCCore/actions/workflows/test_ubuntu.yml)

Cross-platform Amstrad CPC emulation core (C++17). Emulates the CPC 464 / 664 / 6128 and CPC+ (ASIC): Z80 CPU, CRTC, Gate Array, PPI 8255, PSG AY-3-8912, FDC µPD765, tape, DMA, memory banking.

Used by:
- [SugarboxV2](https://github.com/Tom1975/SugarboxV2) — Qt desktop emulator
- SugarConvDsk, SugarConvTape — disk/tape conversion tools
- SugarLibRetro — libretro core

## Table of contents

- [Build](#build)
- [Unit tests](#unit-tests)
- [CMake options](#cmake-options)
- [References](#references)
- [Acknowledgements](#acknowledgements)

---

## Build

**Requirements:** CMake ≥ 3.16, a C++17 compiler.

```bash
git clone https://github.com/Tom1975/CPCCore.git
cd CPCCore
git submodule update --init --recursive
```

### Linux

```bash
sudo apt-get install build-essential libgl1-mesa-dev libx11-dev \
    libxrandr-dev libfreetype6-dev libglew-dev libjpeg-dev libudev-dev \
    freeglut3-dev xvfb

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make all
```

### Windows

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build --config Release
cmake --install build
```

### macOS

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make -j$(sysctl -n hw.logicalcpu)
```

---

## Unit tests

The test suite covers FDC emulation, copy-protection schemes (Speedlock, Alkatraz, CodeMasters…), tape loading, and CPU correctness.

Build with tests enabled:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DGENERATE_UNITTESTS=TRUE
make all
```

Run (Linux — requires a display or xvfb):

```bash
xvfb-run --auto-servernum ./UnitTests/Release/unitTests
```

Run (Windows):

```powershell
.\build\UnitTests\Release\unitTests.exe
```

---

## CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `GENERATE_UNITTESTS` | `OFF` | Build the unit test binary |
| `GENERATE_BENCHMARK` | `OFF` | Build the benchmark binary |
| `CMAKE_POLICY_VERSION_MINIMUM` | — | Set to `3.5` when building with CMake 4.x |

---

## References

- **The Amstrad CPC CRTC Compendium** by Longshot (Serge Querné) / [Logon System](http://www.logonsystem.eu), version 1.11 (08/2026), licensed under [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).
  It is the reference for the CRTC emulation (the five CRTC types: HD6845S/UM6845, UM6845R, MC6845, AMS40489 and the CPC+ ASIC), and for the related timings of the Gate Array, the Z80 I/O and the interrupts.
  The code comments cite its chapters (for example "13.2.1"), and the unit tests follow it chapter by chapter: [`UnitTests/COMPENDIUM_TESTS.md`](UnitTests/COMPENDIUM_TESTS.md) lists each paragraph, its status and the tests that cover it.
  Attribution, as the licence requires: technical information on the CRTC from « Amstrad CPC CRTC Compendium » by Longshot (CC BY-NC-ND).
- **SHAKER**, the CRTC benchmark by Logon System, and its results portal: [shaker.logonsystem.eu](https://shaker.logonsystem.eu).

---

## Acknowledgements

- **Longshot (Serge Querné) and Logon System**, for the Amstrad CPC CRTC Compendium and SHAKER: years of measurements on real machines, without which an accurate CRTC emulation would not be possible.
- Everyone who ran SHAKER on their own CPC and shared the results.
