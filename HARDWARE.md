# The SD-80 is old. Treat it that way.

The Edirol / Roland Studio Canvas SD-80 is a **2002 USB 1.1** sound module. It has two MIDI ports, a small buffer, and it will drop SysEx if you dump a full 32-part scene in one gulp. This plugin exists because a DAW piano roll cannot honestly drive 32 parts + Native MFX on that box without help.

**Tested only on Windows 11, and only as standalone + VST.** AU, CLAP, macOS and Linux are in the CMake tree. They have not been hardware-tested.

## Warnings you should actually read

- **Volume spikes.** Changing many parameters at once while MIDI is playing (live, host, or cassette) can jump the output. The PLAYER tab repeats this in small type. Don’t dump a 32-part scene mid-take.
- **Multi FX USB queue.** Each Multi FX A/B/C tweak sends a large Native COMMON SysEx block. That can stall USB for a long time. Multi FX defaults **OFF**. System reverb and chorus are fine. Hover A/B/C for the same warning. Leave them off unless you need them.
- **Physical volume knob.** OPTIONS → Module volume sends Universal SysEx Master Volume. It does **not** lock or bypass the hardware knob — both still work.

## First response to “it isn’t doing what the mixer says”


1. **SYNC HARDWARE.** Always. The header button pushes mode, FX, every part’s bank/PC/mix/tone and MFX routing through the 20–50 ms throttle. Watch **USB queue** go to 0.
2. **OPTIONS → Part A USB / Part B USB.** Mute, solo, SEL, Deep Edit and the cassette all go out these two sockets. If they say **Part A USB: (none)** the plugin is moving faders in memory and the module never hears it. The plugin tries to auto-pick names containing EDIROL, SD-80 or Studio Canvas. Windows often lists them as `EDIROL SD-80` and `MIDIOUT2 (EDIROL SD-80)`.
3. Leave the hardware in **USB mode**. The rear-panel switch is not decorative.
4. Do not also route the same ports from the DAW’s own MIDI output unless *Host MIDI mirrors Part A/B* is what you want. Double-driving the module is how you get stuck notes and bank fights.
5. If a take is already rolling, do **not** hit Emergency Hardware Reset. That dump can stall MIDI for a few seconds.

## Mixer vs hardware on launch

Reload used to show a flat mixer while the module still had the previous sounds. Two things now happen instead:

1. The last session is restored (standalone `ApplicationProperties` + host plugin state).
2. The plugin **RQ1-dumps** documented Native addresses (MFX types, part map, output assign) and overlays unlocked parameters. Launch does **not** push mixer defaults onto the module.

Bank/program are MIDI CC + program change. The Owner’s Manual does not publish an RQ1 for the sounding patch, so those come from the last session. OPTIONS → **Pull from SD-80** repeats the dump. **SYNC HARDWARE** is the other direction: mixer → module.

## What Emergency Hardware Reset actually does

OPTIONS → **Emergency Hardware Reset** (sure-checkbox required):

- All Notes Off + All Sound Off + CC 121 on every channel of Part A **and** Part B
- Native On, then GS Reset, then Native On again
- Plugin session restored to factory defaults (patches, mix, FX, locks, skin)

It does **not**:

- Rewrite firmware
- Erase User patches stored on the module / memory card
- Change your audio interface

## FX: global vs per-part

From the Owner’s Manual:

| | What | Where |
|---|---|---|
| System reverb | GM2 reverb type + time | Mixer FX bar (Reverb column) |
| System chorus | GM2 chorus type + rate/depth/feedback | Mixer FX bar (Chorus column) |
| Multi FX A/B/C | Native insert, 90 algorithms + THROUGH | Mixer FX bar, COMMON source |
| Per-part reverb send | CC 91 | Deep Edit **Reverb** |
| Per-part chorus send | CC 93 | Deep Edit **Chorus** |
| Per-part MFX send | CC 94 + SysEx | Deep Edit **MFX Send** |
| Which MFX a part hits | output assign MFX + MFX select A/B/C | Deep Edit **Insert MFX** / **Out: MFX** |

Parts default to **Out: MFX** so insertion FX actually bite. Multi FX A/B/C default **OFF** (THROUGH) because enabling them floods the USB queue. THROUGH on Multi FX A/B/C means that slot is a bypass — pick an algorithm (Overdrive, Delay, …) **and** turn the slot on for P1–P4 to matter.

## Throttle

OPTIONS → 20 / 30 / 40 / 50 ms. Default 30. Lower is snappier and more likely to drop. Higher is safer on a flaky USB hub. Use a powered hub if you can; 2002 USB devices hate bus power plus a 32-part dump.

## Cassette PLAYER vs mixer drop

- Drop a `.mid` on the **mixer** → bank/PC/mix auto-assign, unlocked FX reset, then sync.
- Drop a `.mid` on the **PLAYER** cassette → it plays. It does **not** rewrite patches unless you press **Send setup to SD-80**.
- Mute/solo on the mixer silence cassette channels. That only reaches the module through Part A USB.
