# Modern Edirol SD-80

<p align="center">
  <a href="https://www.youtube.com/watch?v=LJgOhjn1Xi0" target="_blank">
    <img src="https://img.shields.io/badge/▶_Watch_Demo_on_YouTube-FF0000?style=for-the-badge&logo=youtube&logoColor=white" alt="Watch Video"/><br/>
    <img src="https://img.youtube.com/vi/LJgOhjn1Xi0/maxresdefault.jpg" alt="Modern Edirol SD80 Showcase Video" width="50%" height="50%"/>
  </a>
</p>

VST3 / AU / CLAP / Standalone **MIDI controller** for the Edirol / Roland Studio Canvas **SD-80** (32-part, USB). You compile this JUCE 9.0.1 project; the plugin talks to the hardware over the two USB MIDI ports.

**v1.6.5** by **Crimson Redstone**. Freeware. If you'd like to support the author, consider [purchasing the music](https://crimsonredstone.bandcamp.com/).

> **When in doubt, press SYNC HARDWARE.** The SD-80 is 2002 USB hardware and drops messages if you dump too fast. Mute, solo and the cassette only reach the module through **Part A USB / Part B USB** in OPTIONS.
>
> **Tested only on Windows 11, standalone and VST.** AU, CLAP and other operating systems ship in CMake and have not been hardware-tested.
>
> Changing many parameters while MIDI is playing can cause **volume spikes**. Tweaking **Multi FX** can flood the USB queue — Multi FX defaults OFF.

Patch names, bank MSB/LSB, SysEx addresses, checksum, MFX type list and CC numbers are taken from the **SD-80 Owner’s Manual** (Roland, 2002):

- Sound maps & bank select — pp. 55–62
- Mode SysEx — p. 53
- GM2 reverb/chorus SysEx — p. 63
- Native MFX SysEx — pp. 64–68
- MIDI implementation chart — p. 123  
  Decay time is **CC#75** (the chart). CC#80 is GP5 / Tone 1 Level.
- Instrument lists — pp. 95–104
- Drum lists — p. 105
- 90 MFX algorithms — pp. 80–94

Model ID `00H 48H` (shared with the SD-90). Roland checksum: `128 - (sum % 128)`.

A GitHub-ready copy of this documentation lives in [`github/`](github/README.md) (README, CHANGELOG, BUILD, HARDWARE, LICENSE). Paste those files at the root of the GitHub repo when you push.

## What it does

- 32 channel strips (Part A 1-16 / Part B 17-32)
- Native, GM2, GS, XG Lite mode switches
- Sound maps: Classical, Contemporary, Solo, Enhanced, Special 1, Special 2, User
- Categorized patch browser (category list + patch list, not one giant menu)
- Mix CCs: volume, pan, expression, reverb/chorus/delay send
- Tone CCs: cutoff 74, resonance 71, attack 73, decay 75, release 72, vibrato 76–78, portamento 65/5
- System reverb (6 GM2 types) + chorus (6 types) + 3 × 90-type MFX in grouped submenus, each with 4 knobs
- Parts default to **output assign MFX** so insertion FX actually hit the sound
- **SEL** routes a live MIDI keyboard onto that part (Follow SEL). An FL / 16-channel piano roll should use OPTIONS **Part A as-played** so channel 1 hits A1, same as the cassette. Player piano-roll colours are display-only.
- Mute / Solo, plus **MUTE ALL / UNMUTE ALL / UNSOLO ALL** (Shift+mute / Shift+solo). Right-click a strip name to lock the instrument
- **Sync Hardware** pushes the full 32-part state through a 20–50 ms USB throttle queue
- Drag-and-drop `.mid` / `.midi` on the **mixer** auto-assigns bank/PC/mix and sets Live MIDI to Part A as-played
- Separate **PLAYER** tab: cassette deck with **LOOP** and a falling colour piano-roll (notes come toward the keyboard; Guitar Hero glow on hit). **POP OUT** / **FULL**; minimize or close docks it back. A playlist mirrors here (Part A or B) until you load a tape
- **PLAYLIST** tab: two deck cards (Part A / Part B) with live progress. Queue `.mid` files; PLAY starts A, then they ping-pong. LOOP on PLAYER while armed: whole playlist, this song, or off. Send setup is locked while armed
- **OPTIONS**: audio I/O (real selector in standalone; greyed “Controlled by Host” in a DAW), USB ports, host MIDI route, module volume, Pull from SD-80, 9 skins, shortcuts, emergency reset
- Right-click any fader, knob, toggle or menu to **lock** it. Locks survive patch, MIDI import and presets
- Session total recall via `getStateInformation` / `setStateInformation`
- `.mesd80preset` XML snapshots
- Standalone **ASIO** (Windows) plus WASAPI. No JUCE Options/Settings chrome — audio lives in OPTIONS. Cassette-reel app icon on the exe / VST3 / taskbar. Closing the standalone (X or Alt+F4) silences the module first so a note cannot stick.

## Standalone ASIO

On Windows the standalone lists **ASIO** in OPTIONS → Audio (Steinberg ASIO SDK is fetched at CMake configure). Pick your interface so the SD-80 can clock out over SPDIF or whatever you use.

If the SDK zip cannot be downloaded, pass `-DASIO_SDK_DIR=C:/path/to/asiosdk` pointing at a folder that contains `common/iasiodrv.h`. Disable with `-DMESD80_ASIO=OFF`.

The standalone window is a native title bar only. There is no extra JUCE Options/Settings menu. The cassette-reel icon in `Assets/icon.png` is baked into the Windows exe / VST3 and the macOS bundle — rebuild once so the taskbar picks it up. Closing the window (X or Alt+F4) sends all-notes-off on both USB ports first so a hanging note cannot stick on the module.

## Build (you compile)

Requires **CMake 3.22+**, a C++20 compiler, and Git (JUCE **9.0.1** is fetched on first configure). If you already built with JUCE 8, delete the `build` folder once. JUCE is GPL v3 unless you have a commercial JUCE license. See `github/BUILD.md` and `github/HARDWARE.md`.

### Windows (Visual Studio 2022)

Double-click **`build.bat`**. It writes **`logs\build.log`** — paste that file if it breaks. Builds **VST3 + Standalone**, then CLAP.

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target ModernEdirolSD80_VST3 ModernEdirolSD80_Standalone ModernEdirolSD80_CLAP
```

**AU cannot be built on Windows** — Audio Units are macOS only.

CLAP uses [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions) (JUCE 9.0.1 has no native CLAP client). Skip it with `-DMESD80_CLAP=OFF`.

### macOS (Xcode / AU + VST3 + CLAP)

```bash
cmake -B build -G Xcode
cmake --build build --config Release --target ModernEdirolSD80_VST3 ModernEdirolSD80_AU ModernEdirolSD80_CLAP
```

AU copies into `~/Library/Audio/Plug-Ins/Components`. CLAP copies next to the VST3 (or set your DAW's CLAP folder).

### Local JUCE instead of FetchContent

```bash
cmake -B build -DJUCE_DIR=/path/to/JUCE
```

## Project layout

```
Source/PluginProcessor.*    MIDI engine, APVTS, throttle, total recall, player, playlist, locks
Source/PluginEditor.*       Mixer, cassette player, playlist, options, skins
Source/StandaloneApp.cpp    Custom standalone (no JUCE Options/Settings chrome)
Source/SD80PatchData.h      Bank/PC lookup (generated from the manual)
Source/SD80Sysex.h          DT1 / RQ1 / mode / MFX helpers (makeCc, not cc)
Source/MidiThrottleQueue.h  20–50 ms FIFO
Source/MidiFileImporter.h   SMF parser for mixer auto-setup
Source/MidiPlayer.h         Cassette SMF playback
Source/CassetteDeck.h       Empty / loaded / spinning cassette UI
Source/Skin.h               9 palettes including As God Intended
Source/ParamLock.h          Right-click lock wrappers
github/                     GitHub homepage kit (README, CHANGELOG, BUILD, HARDWARE, LICENSE)
```

## Hardware reminder

Read [HARDWARE.md](HARDWARE.md) before you panic. The module is old. Treat it like old gear.

## License

Plugin source: see [LICENSE](LICENSE). JUCE itself is GPL v3 (or a paid JUCE license). ASIO is a trademark of Steinberg Media Technologies GmbH.

## Disclaimer
I know enough to understand and mess with code but nothing advanced, most of the code here was written by AI.
Therefore if you have any complaints, bug reports, or suggestions make sure to be percise, detailed & with images where neccessary.

## Known Bugs:
Midi through DAW has a sound discrepency compared to midi loaded via the player.
