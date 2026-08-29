# SD-80 MIDI map (from the Owner’s Manual)

## Mode SysEx (p.53)

| Mode | Message |
|---|---|
| GM2 | `F0 7E 7F 09 03 F7` |
| Native | `F0 41 10 00 48 12 00 00 00 00 00 00 F7` |
| GS Reset | `F0 41 10 42 12 40 00 7F 00 41 F7` |
| XG System On | `F0 43 10 4C 00 00 7E 00 F7` |

Device ID `10H`, model ID `00H 48H`. DT1 checksum = `128 - (sum(addr+data) % 128)`.

## Native bank MSB (p.59)

| Map | Inst MSB | Drum MSB |
|---|---|---|
| Special 1 | 80 (50H) | — |
| Special 2 | 81 (51H) | — |
| User | 87 (57H) | 86 (56H) |
| Classical | 96 (60H) | 104 (68H) |
| Contemporary | 97 (61H) | 105 (69H) |
| Solo | 98 (62H) | 106 (6AH) |
| Enhanced | 99 (63H) | 107 (6BH) |

GM2 part-mode MSBs: Inst **121 (79H)**, Drum **120 (78H)**. Then LSB + PC select the variation.

Order: **CC0 → CC32 → Program Change** (p.59).

## Channel CCs (implementation chart p.123)

| Param | CC | Range |
|---|---|---|
| Bank MSB / LSB | 0 / 32 | 0–127 |
| Volume / Pan / Expression | 7 / 10 / 11 | 0–127 |
| Portamento SW / Time | 65 / 5 | 0–127 |
| Resonance | 71 | 0–127 (64 = 0) |
| Release / Attack / Cutoff / Decay | 72 / 73 / 74 / **75** | 0–127 (64 = 0) |
| Vibrato Rate / Depth / Delay | 76 / 77 / 78 | 0–127 (64 = 0) |
| Reverb / Chorus / Delay send | 91 / 93 / 94 | 0–127 |
| GP5 Tone 1 Level | 80 | 0–127 |

## GM2 reverb / chorus (p.63)

Reverb: `F0 7F 10 04 05 01 01 01 01 01 pp vv F7`  
pp=0 type (00 Small Room … 04 Large Hall, 08 Plate), pp=1 time.

Chorus: `F0 7F 10 04 05 01 01 01 01 02 pp vv F7`  
pp=0 type (00–05 Chorus1–4, FB, Flanger).

## Native MFX (pp.64–66)

Common type: `F0 41 10 00 48 12 10 00 mm 00 tt cs F7` with mm = 06/08/0A for A/B/C.

Part address `pp` = `20H` + partIndex (Part 1 = 20H, Part 32 = 3FH).
