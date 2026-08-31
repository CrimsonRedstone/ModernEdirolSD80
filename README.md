# Modern Edirol SD-80

<p align="center">
  <a href="https://www.youtube.com/watch?v=LJgOhjn1Xi0" target="_blank">
    <img src="https://img.shields.io/badge/▶_Watch_Demo_on_YouTube-FF0000?style=for-the-badge&logo=youtube&logoColor=white" alt="Watch Video"/><br/>
    <img src="https://img.youtube.com/vi/LJgOhjn1Xi0/maxresdefault.jpg" alt="Modern Edirol SD80 Showcase Video" width="50%" height="50%"/>
  </a>
</p>

VST3 / AU / CLAP / Standalone **MIDI controller** for the Edirol / Roland Studio Canvas **SD-80** (32-part, USB).

**v1.5.4** by **[Crimson Redstone](https://crimsonredstone.bandcamp.com/)**. Freeware. If you'd like to support me, consider [purchasing my music](https://crimsonredstone.bandcamp.com/).

This is an unofficial editor. Roland / Edirol names and the SD-80 sound set are their trademarks.

> **The hardware is from 2002.** USB MIDI on the SD-80 drops messages if you dump too fast.  
> **When in doubt, press SYNC HARDWARE.** That is the first thing to try if patches, mute, solo or FX do not match what you hear.  
> **Tested only on Windows 11, standalone and VST.** AU, CLAP, macOS and Linux ship in the tree and have not been hardware-tested.

## Warnings

- **Volume spikes.** Changing a lot of parameters at once while MIDI is playing can jump the output on this module. The PLAYER tab repeats this in small type.
- **Multi FX USB queue.** Tweaking Multi FX A/B/C dumps a large Native SysEx block per change and can stall USB for a long time. Multi FX defaults **OFF**. System reverb/chorus are fine. Hover A/B/C for the same warning.

## What it is

A JUCE 9 plugin that talks to the module over the **two** USB MIDI ports the SD-80 exposes (Part A = MIDI OUT 1, Part B = MIDI OUT 2). You compile it. It is not a software synth — the sound still comes out of the hardware.

- 32 channel strips (Part A 1–16 / Part B 17–32)
- Native, GM2, GS, XG Lite
- Sound maps: Classical, Contemporary, Solo, Enhanced, Special 1, Special 2, User
- 1,050+ named patches from the Owner's Manual
- Categorized patch browser
- Mix + tone CCs (volume, pan, filter, envelope, vibrato, portamento, sends)
- System reverb + chorus + 3 × 90-type Multi FX (grouped submenus, 4 knobs each)
- Per-part reverb / chorus / MFX send in Deep Edit
- **SEL** (Shift-click to multi-select) routes live MIDI onto those parts by default. OPTIONS can instead send host MIDI Part A / Part B as-played for a 16-channel piano roll.
- Mute / Solo, MUTE ALL / UNMUTE ALL / UNSOLO ALL. Right-click a strip **name** to lock that instrument
- **PLAYER** cassette deck with a **LOOP** tape button and a **colour piano-roll** (one colour per instrument)
- **DEMOS** tab — trigger the three internal songs (or the panel DEMO key if SysEx is ignored)
- **OPTIONS**: full audio/MIDI device selector in standalone (greyed “Controlled by Host” in a DAW), USB ports, host MIDI route, module volume, Pull from SD-80, skins, shortcuts, emergency reset
- Nine persistable skins, including **As God Intended** (SD-80 silver chassis)
- Right-click lock on every adjuster
- `.mesd80preset` snapshots
- ASIO in the Windows standalone
- Cassette-reel app icon on the standalone, VST3 and taskbar

## When in doubt: SYNC HARDWARE

The SD-80 is a 2002 USB 1.1 module. It has two MIDI ports, a tiny buffer, and no patience. This plugin throttles SysEx and CC (20–50 ms) on purpose.

If the mixer and the speakers disagree:

1. Confirm **Part A USB** and **Part B USB** in OPTIONS actually point at the two EDIROL SD-80 ports. Mute, solo and the cassette only reach the module through those sockets. The plugin auto-selects them when it sees “EDIROL”, “SD-80” or “Studio Canvas”.
2. Press **SYNC HARDWARE** in the header. Wait for the USB queue to drain.
3. If it is still wrong, OPTIONS → **Emergency Hardware Reset** (Native On + GS Reset + All Notes/Sound Off + plugin factory defaults). This does **not** rewrite firmware and does **not** erase User cards in the module.

Leave the hardware in **USB mode**. Do not fight it with a second DAW MIDI output on the same ports unless you know you want that.

## Build

See [BUILD.md](BUILD.md). Short version (Windows):

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target ModernEdirolSD80_VST3 ModernEdirolSD80_Standalone
```

Or double-click `build.bat`. It writes `logs\build.log` even when CMake fails. JUCE 9.0.1 and (on Windows) the Steinberg ASIO SDK are fetched at configure. Delete the `build` folder once if you previously fetched JUCE 8.

Requires **CMake 3.22+**, a C++20 compiler, and Git. JUCE is **GPL v3** unless you have a commercial JUCE license — see [LICENSE](LICENSE).

## Manual sources

Patch names, bank MSB/LSB, SysEx addresses, checksum, MFX type list and CC numbers come from the **SD-80 Owner’s Manual** (Roland, 2002):

- Sound maps & bank select — pp. 55–62
- Mode SysEx — p. 53
- GM2 reverb/chorus SysEx — p. 63
- Native MFX SysEx — pp. 64–68
- MIDI implementation chart — p. 123 (Decay is **CC#75**)
- Instrument lists — pp. 95–104
- Drum lists — p. 105
- 90 MFX algorithms — pp. 80–94

Model ID `00H 48H` (shared with the SD-90). Roland checksum: `128 - (sum % 128)`.

More detail: `docs/MIDI-MAP.md` in the source tree.

## Shortcuts

| Action | What it does |
|---|---|
| Right-click any fader / knob / toggle / menu / strip name | Lock / unlock. Locked values survive patch, MIDI import and presets |
| Click SEL | Exclusive select that part for Deep Edit and live MIDI |
| Shift + click SEL | Add or remove that part from the live-play selection |
| Shift + click MUTE | Mute all, or unmute all if everything is already muted |
| Shift + click SOLO | Unsolo all |
| MUTE ALL / UNMUTE ALL / UNSOLO ALL | Mixer toolbar |
| LOOP | PLAYER cassette row — wrap the SMF when it hits the end |

## Hardware reminder

Read [HARDWARE.md](HARDWARE.md) before you panic. The module is old. Treat it like old gear.

## License

Plugin source: see [LICENSE](LICENSE). JUCE itself is GPL v3 (or a paid JUCE license). ASIO is a trademark of Steinberg Media Technologies GmbH.

## Disclaimer
I know enough to understand and mess with code but nothing advanced, most of the code here was written by AI.
Therefore if you have any complaints, bug reports, or suggestions make sure to be percise, detailed & with images where neccessary.

## Known Bugs:
Nothing in the Demo tab works and you should not mess with it.
