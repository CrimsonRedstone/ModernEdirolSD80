# Changelog

All notable changes to **Modern Edirol SD-80** live here. Copy this file to the GitHub release notes when you push.

## 1.4.1 — 2026-08-29

MSVC: `juce::MidiMessage::songSelect` is not in JUCE 9.0.1. DEMOS send Song Select as raw `F3 nn`.

## 1.4.0 — 2026-08-29

JUCE **9.0.1**. Keyboard Follow SEL. Demos send immediately. Multi FX warning in the FX bar.

- Live MIDI **Follow SEL** is the default (and the meaning of saved value 0). Part A / Part B as-played remain in OPTIONS for 16-channel host MIDI.
- DEMOS send immediately on both USB ports (Native SysEx, Song Select, MIDI Start, MMC). Not queued. Footer reports if Part A/B USB are closed. Panel DEMO key is still the documented path.
- Multi FX USB-queue warning is in the Multi FX columns, not the MUTE ALL row.
- CMake `GIT_TAG 9.0.1`. Delete `build/` once if you configured JUCE 8. CLAP remains clap-juce-extensions (JUCE 9.0.1 has no `juce_audio_plugin_client_CLAP`).

## 1.3.1 — 2026-08-29

MSVC: `AudioDeviceManager::setMidiOutputDeviceEnabled` is not in JUCE 8.0.8 (C2039). Standalone now clears the default MIDI out with `setDefaultMidiOutputDevice({})`. MIDI keyboards still use `setMidiInputDeviceEnabled`. The processor keeps Part A/B USB.

## 1.3.0 — 2026-08-29

Standalone audio/MIDI restored, mixer follows hardware, host MIDI routing, DEMOS tab.

### Bugs

- **OPTIONS audio settings were removed instead of migrated.** Standalone now hosts the full JUCE `AudioDeviceSelectorComponent` in OPTIONS (driver including ASIO, output device, sample rate, buffer, MIDI keyboard inputs). VST stays greyed with **Audio Settings Controlled by Host**. The redundant “Standalone: choose Driver: ASIO…” footnote under OPTIONS is gone.
- **Selector appeared then vanished / overlapped other OPTIONS.** The holder can be null when the editor is first constructed. The selector is created on a timer once `StandalonePluginHolder::getInstance()` exists, reserved ~460 px, and the OPTIONS page scrolls so USB/mode/skin cannot sit under it.
- **Standalone MIDI keyboard dead.** MIDI Input is shown (`showMidiInputOptions = true`). The custom standalone app auto-enables every MIDI input that is **not** the SD-80, and disables host MIDI **outputs** so Part A/B USB are the only sockets talking to the module.
- **PLAYER Loop** is a **LOOP** toggle in the cassette button row with PLAY / PAUSE / STOP / LOAD / Send setup — not a stray checkbox under the deck.
- **Reload flattened the mixer while the hardware kept its sounds.** Two causes: the standalone window destroyed `ApplicationProperties` before the plugin holder (state save wrote into a dead PropertySet), and launch used to **push** defaults. `appProps` now outlives the holder; launch restores the last session, then RQ1-dumps documented Native addresses (MFX types, part map, output assign). OPTIONS → **Pull from SD-80** repeats that dump. Bank/program are MIDI CC + PC — the Owner’s Manual does not publish an RQ1 for the sounding patch, so those come from the last session. Launch does **not** overwrite the module.
- **Right-click a loaded mixer instrument** (strip name or patch list row) locks bank MSB/LSB + program together.
- **Host MIDI in VST went to Part B.** OPTIONS → **Host MIDI destination**: Part A as-played (default), Part B as-played, or Follow SEL.

### Features

- **DEMOS** tab between PLAYER and OPTIONS. Demo 1 / 2 / 3 / Stop send Native SysEx address `00 00 01 00` (`00` = stop, `01`–`03` = songs). Owner’s Manual p.13 is the front-panel DEMO flow; some firmware only honours that key — the tab says so. Stop a demo before SYNC HARDWARE.
- OPTIONS **Module volume** (Universal Real-Time Master Volume `F0 7F 7F 04 01 00 vv F7`). The physical knob still works and cannot be locked out.
- PLAYER footer + GitHub: **massive multi-parameter changes while MIDI is playing can cause volume spikes** on this 2002 USB module.
- **Multi FX defaults OFF.** Hover the Multi FX A/B/C controls for the USB-queue warning. System reverb/chorus are fine. Tweaking Multi FX dumps a large SysEx block per change and can stall the queue for a long time.
- GitHub disclaimer: **tested only on Windows 11, only standalone and VST.** AU, CLAP, macOS and Linux are shipped in the CMake tree and have not been hardware-tested.

## 1.2.1 — 2026-08-29


MSVC compile fix from the v1.2 Windows log, plus CLAP.

- **MSVC C3318** in `PluginEditor.cpp` resized(): `auto mfxCols[3]` is illegal on Visual C++ (an array cannot have element type `auto`). Typed as `juce::Rectangle<int> mfxCols[3]`.
- **CLAP** target `ModernEdirolSD80_CLAP` via unofficial [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions). JUCE 8.0.8 cannot emit CLAP natively (that is JUCE 9). Disable with `-DMESD80_CLAP=OFF`.
- **AU** is still in CMake `FORMATS` but **Windows cannot build it**. Use Xcode on macOS for `ModernEdirolSD80_AU`.
- Windows compile script is **`01 Compile.bat`**: logs to `build_log.txt`, builds VST3 + Standalone + CLAP when the CLAP vcxproj exists. `run.bat` just calls it.

## 1.2.0 — 2026-08-29

Hardware-test bugs from v1.1 plus the requested mixer / options / player features.

### Bugs

- **Mute / solo while the cassette plays.** Notes from the PLAYER and from live MIDI are now skipped for silenced parts (`isPartSilenced`). Volume CC mute still runs, but it only reaches the module if Part A/B USB are actually open — which is why mute used to look like a no-op when the combos sat on “(none)”.
- **USB combos defaulted to a bare “(none)”.** Labels are now **Part A USB: (none)** / **Part B USB: (none)**. On launch the plugin auto-selects devices whose names contain EDIROL, SD-80 or Studio Canvas.
- **FX bar layout.** System FX is five columns: Reverb | Chorus | Multi FX A | B | C. Knobs sit under the control they belong to. A/B/C on-toggles are a fixed 28 px, they no longer stretch. Each Multi FX has four knobs (P1–P4) sent as Native MFX COMMON parameters.
- **THROUGH labelled as THROUGH.** Default type 0 now shows as **Multi FX A / B / C**. The algorithm name is appended only after you pick one.
- **Global vs per-part FX.** GM2 reverb/chorus are system-wide. Native MFX A/B/C are insert (COMMON source, part output assign MFX). Deep Edit still has per-part reverb send, chorus send, MFX send, MFX select and output assign.
- **Mixer MIDI import kept previous MFX mishmash.** Unlocked global FX and unused parts now reset to SD-80 defaults, then `SYNC HARDWARE`. Locked parameters stay.
- **Cassette reels.** Both reels spin the same direction at 60 Hz.
- **Standalone chrome.** Custom app window — native title bar only. The JUCE top-left **Options** and top-right **Settings** are gone. Audio I/O lives in the plugin **OPTIONS** tab. The extra-host-MIDI-out footnote is gone.
- **Header clutter.** USB ports, generator mode and throttle moved to OPTIONS.

### Features

- MUTE ALL / UNMUTE ALL / UNSOLO ALL on the mixer. Shift+mute = mute or unmute all. Shift+solo = unsolo all.
- OPTIONS → SHORTCUTS lists every click modifier.
- Shift+SEL multi-select; live MIDI fans out to every selected part.
- PLAYER **Loop tape** checkbox.
- Skin **As God Intended** — SD-80 silver chassis.
- `github/` folder (README, CHANGELOG, BUILD, HARDWARE, LICENSE) for the GitHub repo.
- VST OPTIONS shows a greyed “Audio Settings Controlled by Host” mock of the standalone device panel. Standalone hosts a real `AudioDeviceSelectorComponent`.
- **Emergency Hardware Reset** with a “Yes, I am sure” dialog (Native On, GS Reset, All Notes/Sound Off, plugin factory defaults). Does not touch firmware or User cards.
- **Reset effects** on the FX bar, with sure-checkbox + optional “don’t show again”, then syncs hardware.

## 1.1.1 — MSVC compile

- Lockable buttons inherit JUCE constructors (MSVC could not construct `ToggleButton`/`TextButton` from a label).
- SEL routing no longer calls `MidiMessage::withChannel` (not in JUCE 8.0.8).
- Patch category compare uses `juce::String` so MSVC is not ambiguous.
- GS “808 Tom” bank was 808 (uint8 overflow) — corrected to PC 118 / LSB 1.

## 1.1.0 — Crimson Redstone

Bugfixes from the v1 hardware test:

- Part A / Part B labels use ASCII `1-16` / `17-32` so they no longer render as tofu.
- Deep Edit is a right-hand sidebar; knobs stay inside the panel.
- Patch picking is a category list + patch list (searchable), not one overloaded combo.
- MFX types open as grouped submenus (Drive, Delay, Amp / Multi, …).
- **SEL** rewrites live MIDI onto the selected part’s channel and USB port.
- Mute / Solo are real toggles with red / green on-colours; mute zeros volume, any solo silences the rest.
- MFX actually hits the sound: parts default to output assign MFX, COMMON source is set, Native is required.
- Standalone enables **ASIO** (SDK fetched at configure) alongside WASAPI.

Extra features:

- Donation link to https://crimsonredstone.bandcamp.com/
- Credits: Crimson Redstone
- Eight persistable skins (Studio, Scarlett/Flandre, Baguette/Teto, Leek/Miku, Hakurei, Lunatic, Sakura, Matcha)
- PLAYER tab cassette deck
- Right-click lock on every adjuster

## 1.0.0

Initial JUCE 8 VST3 / AU / Standalone MIDI controller for the SD-80.
