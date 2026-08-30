# Build Modern Edirol SD-80

You compile this. There is no pre-built VST in the repo.

Requires:

- CMake **3.22+**
- A C++20 compiler (Visual Studio 2022 on Windows, Xcode on macOS)
- Git (JUCE **9.0.1** is fetched on first configure)

JUCE is **GPL v3** unless you hold a commercial JUCE license.

## Windows (Visual Studio 2022)

Double-click **`build.bat`**. The window stays open. The only log is **`logs\build.log`** — paste that if it fails.

```bat
build.bat
:: or:
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target ModernEdirolSD80_VST3 ModernEdirolSD80_Standalone ModernEdirolSD80_CLAP
```

VST3 lands under `build/ModernEdirolSD80_artefacts/Release/VST3/`. Copy that bundle into your VST3 folder (usually `C:\Program Files\Common Files\VST3`). CLAP lands under `.../Release/CLAP/` (or the clap-juce-extensions copy path).

**AU cannot be built on Windows.** Audio Units are an Apple format.

If you previously configured JUCE 8, delete the `build` folder once so CMake re-fetches **9.0.1**.

### CLAP

JUCE 9.0.1 does not ship a native CLAP client. This project fetches [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions) (JUCE 9 compatible). If that fetch fails:

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64 -DMESD80_CLAP=OFF
```

then build VST3 + Standalone only.

### ASIO

On Windows the standalone lists **ASIO** in OPTIONS → Audio (Steinberg ASIO SDK 2.3.4 is fetched at CMake configure, GPL-3 since 2025). Pick your interface so the SD-80 can clock out over SPDIF.

If the SDK zip cannot be downloaded:

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64 -DASIO_SDK_DIR=C:/path/to/asiosdk
```

The folder must contain `common/iasiodrv.h`. Disable ASIO with `-DMESD80_ASIO=OFF`.

## macOS (Xcode / AU + VST3 + CLAP)

```bash
cmake -B build -G Xcode
cmake --build build --config Release --target ModernEdirolSD80_VST3 ModernEdirolSD80_AU ModernEdirolSD80_CLAP
```

AU needs to be copied into `~/Library/Audio/Plug-Ins/Components`.

## Local JUCE instead of FetchContent

```bash
cmake -B build -DJUCE_DIR=/path/to/JUCE
```

Use JUCE **9.0.1**. If you already configured JUCE 8, delete the `build` folder so FetchContent downloads 9.0.1.

## After it builds

1. Plug in the SD-80, leave it in **USB mode**.
2. Launch standalone or load the VST3 in a DAW.
3. OPTIONS → Part A USB / Part B USB. They should auto-fill to the two EDIROL ports. If not, pick them.
4. Press **SYNC HARDWARE**. Wait for the USB queue in the header to drain.
5. Play. If it is silent, read [HARDWARE.md](HARDWARE.md) before you assume the plugin is broken.

## Project layout

```
Source/PluginProcessor.*    MIDI engine, APVTS, throttle, total recall, player, locks
Source/PluginEditor.*       Mixer, cassette player, options, skins
Source/StandaloneApp.cpp    Custom standalone (no JUCE Options/Settings chrome)
Source/SD80PatchData.h      Bank/PC lookup (from the manual)
Source/SD80Sysex.h          DT1 / RQ1 / mode / MFX helpers (makeCc, not cc)
Source/MidiThrottleQueue.h  20–50 ms FIFO
Source/MidiFileImporter.h   SMF parser for mixer auto-setup
Source/MidiPlayer.h         Cassette SMF playback (with loop)
Source/MidiRoll.h           Colour piano-roll under the cassette
Source/CassetteDeck.h       Empty / loaded / spinning cassette UI
Source/Skin.h               9 palettes including As God Intended
Source/ParamLock.h          Right-click lock wrappers
Assets/icon.png             Cassette-reel icon (CMake ICON_BIG, 1024)
Assets/icon-small.png       Same icon at 256 (CMake ICON_SMALL)
Assets/icon.ico             Multi-size Windows icon
github/                     Files for the GitHub repo homepage
docs/MIDI-MAP.md            Address / CC reference
build.bat                   One-click Windows compile (writes logs\build.log)
```
