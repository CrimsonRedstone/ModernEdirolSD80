#pragma once

// Roland / Edirol SD-80 SysEx helpers.
// Source: SD-80 Owner's Manual pp.53–68 (model ID 00H 48H, same as SD-90).
// Checksum: 128 - (sum(addr+data) % 128); if 128 then 0.  (Roland DT1/RQ1)

#include <cstdint>
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include "SD80PatchData.h"

namespace sd80
{
inline constexpr std::uint8_t kRolandId     = 0x41;
inline constexpr std::uint8_t kDefaultDevId = 0x10; // 17 decimal
inline constexpr std::uint8_t kModelId0     = 0x00;
inline constexpr std::uint8_t kModelId1     = 0x48; // SD-80 / SD-90
inline constexpr std::uint8_t kCmdDT1       = 0x12;
inline constexpr std::uint8_t kCmdRQ1       = 0x11;

// MIDI CC numbers — SD-80 MIDI Implementation Chart p.123 (GM2/Native).
// Decay is CC#75 on the hardware, not CC#80 (CC#80 = GP5 / Tone 1 Level).
namespace cc
{
inline constexpr int bankMSB     = 0;
inline constexpr int bankLSB     = 32;
inline constexpr int modulation  = 1;
inline constexpr int portaTime   = 5;
inline constexpr int volume      = 7;
inline constexpr int pan         = 10;
inline constexpr int expression  = 11;
inline constexpr int hold        = 64;
inline constexpr int portaSw     = 65;
inline constexpr int resonance   = 71;
inline constexpr int release     = 72;
inline constexpr int attack      = 73;
inline constexpr int cutoff      = 74;
inline constexpr int decay       = 75; // official chart; brief listed 80 in error
inline constexpr int vibRate     = 76;
inline constexpr int vibDepth    = 77;
inline constexpr int vibDelay    = 78;
inline constexpr int gp5Tone1    = 80; // General Purpose 5 / Tone 1 Level
inline constexpr int reverb      = 91;
inline constexpr int chorus      = 93;
inline constexpr int delay       = 94; // GS delay send; Native MFX send via SysEx
}

inline std::uint8_t rolandChecksum(const std::uint8_t* bytes, int n)
{
    unsigned sum = 0;
    for (int i = 0; i < n; ++i)
        sum += bytes[i];
    auto r = static_cast<std::uint8_t>(128u - (sum % 128u));
    return r == 128 ? 0 : r;
}

inline juce::MidiMessage makeSysex(const std::vector<std::uint8_t>& payload)
{
    return juce::MidiMessage::createSysExMessage(payload.data(), (int) payload.size());
}

// DT1: F0 41 dev 00 48 12 aa aa aa aa [data...] cs F7
inline juce::MidiMessage dt1(std::uint8_t a0, std::uint8_t a1, std::uint8_t a2, std::uint8_t a3,
                             const std::vector<std::uint8_t>& data,
                             std::uint8_t dev = kDefaultDevId)
{
    std::vector<std::uint8_t> body;
    body.reserve(8 + data.size());
    body.push_back(a0); body.push_back(a1); body.push_back(a2); body.push_back(a3);
    body.insert(body.end(), data.begin(), data.end());
    const auto cs = rolandChecksum(body.data(), (int) body.size());

    std::vector<std::uint8_t> p;
    p.push_back(kRolandId);
    p.push_back(dev);
    p.push_back(kModelId0);
    p.push_back(kModelId1);
    p.push_back(kCmdDT1);
    p.insert(p.end(), body.begin(), body.end());
    p.push_back(cs);
    return makeSysex(p);
}

// RQ1 data request: F0 41 dev 00 48 11 aa aa aa aa ss ss ss ss cs F7
inline juce::MidiMessage rq1(std::uint8_t a0, std::uint8_t a1, std::uint8_t a2, std::uint8_t a3,
                             std::uint8_t s0, std::uint8_t s1, std::uint8_t s2, std::uint8_t s3,
                             std::uint8_t dev = kDefaultDevId)
{
    std::uint8_t body[8] = { a0, a1, a2, a3, s0, s1, s2, s3 };
    const auto cs = rolandChecksum(body, 8);
    std::vector<std::uint8_t> p { kRolandId, dev, kModelId0, kModelId1, kCmdRQ1,
                                  a0, a1, a2, a3, s0, s1, s2, s3, cs };
    return makeSysex(p);
}

inline std::uint8_t partAddress(int partIndex0) // 0..31 → 20H..3FH
{
    return static_cast<std::uint8_t>(0x20 + juce::jlimit(0, 31, partIndex0));
}

// ---- Mode switches (manual p.53). These initialise the sound generator. ----
inline juce::MidiMessage gm2SystemOn()
{
    const std::uint8_t d[] = { 0x7E, 0x7F, 0x09, 0x03 };
    return juce::MidiMessage::createSysExMessage(d, 4);
}

inline juce::MidiMessage nativeOn(std::uint8_t dev = kDefaultDevId)
{
    // F0 41 10 00 48 12 00 00 00 00 00 00 F7
    return dt1(0x00, 0x00, 0x00, 0x00, { 0x00 }, dev);
}

inline juce::MidiMessage gsReset()
{
    // F0 41 10 42 12 40 00 7F 00 41 F7  (GS model ID 42H)
    const std::uint8_t addrData[] = { 0x40, 0x00, 0x7F, 0x00 };
    const auto cs = rolandChecksum(addrData, 4);
    std::vector<std::uint8_t> p { kRolandId, kDefaultDevId, 0x42, kCmdDT1,
                                  0x40, 0x00, 0x7F, 0x00, cs };
    return makeSysex(p);
}

inline juce::MidiMessage xgSystemOn()
{
    const std::uint8_t d[] = { 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00 };
    return juce::MidiMessage::createSysExMessage(d, 7);
}

inline juce::MidiMessage modeMessage(GeneratorMode m)
{
    switch (m)
    {
        case GeneratorMode::GM2:    return gm2SystemOn();
        case GeneratorMode::Native: return nativeOn();
        case GeneratorMode::GS:     return gsReset();
        case GeneratorMode::XGLite: return xgSystemOn();
    }
    return nativeOn();
}

// GM2 sound-set select (manual p.55):
// F0 41 10 00 48 12 10 00 pp 3F nn ss F7
// pp: 20H=Part1 … 3FH=Part32; nn: 00 Classical … 03 Enhanced
inline juce::MidiMessage gm2InstrumentSetSelect(int partIndex0, std::uint8_t set0to3)
{
    return dt1(0x10, 0x00, partAddress(partIndex0), 0x3F, { set0to3 });
}

// Native MFX type, COMMON (manual p.66):
// mm: 06=A, 08=B, 0A=C; tt: 00–5A (0=THROUGH … 90)
inline juce::MidiMessage mfxTypeCommon(int mfxIndex0, std::uint8_t type)
{
    const std::uint8_t mm = static_cast<std::uint8_t>(0x06 + mfxIndex0 * 2);
    return dt1(0x10, 0x00, mm, 0x00, { type });
}

// MFX source COMMON vs PART (manual p.65): address 10 00 00 mm, data 01=COMMON
inline juce::MidiMessage mfxSourceCommon(int mfxIndex0, bool common)
{
    const std::uint8_t mm = static_cast<std::uint8_t>(0x30 + mfxIndex0);
    return dt1(0x10, 0x00, 0x00, mm, { static_cast<std::uint8_t>(common ? 0x01 : 0x00) });
}

// Part output assign: 00=MFX (manual p.64)  address 10 00 pp 1F
inline juce::MidiMessage partOutputAssign(int partIndex0, std::uint8_t assign /*0=MFX*/)
{
    return dt1(0x10, 0x00, partAddress(partIndex0), 0x1F, { assign });
}

// Part output MFX select: 00=A 01=B 02=C  address 10 00 pp 20
inline juce::MidiMessage partOutputMfxSelect(int partIndex0, std::uint8_t mfx0to2)
{
    return dt1(0x10, 0x00, partAddress(partIndex0), 0x20, { mfx0to2 });
}

// Universal Real-Time GM2 reverb (manual p.63)
// F0 7F 10 04 05 01 01 01 01 01 pp vv F7
inline juce::MidiMessage gm2ReverbParam(std::uint8_t pp, std::uint8_t vv)
{
    const std::uint8_t d[] = { 0x7F, 0x10, 0x04, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, pp, vv };
    return juce::MidiMessage::createSysExMessage(d, 11);
}

// F0 7F 10 04 05 01 01 01 01 02 pp vv F7  (chorus) — note manual example uses 01 02 01 00 03
inline juce::MidiMessage gm2ChorusParam(std::uint8_t pp, std::uint8_t vv)
{
    const std::uint8_t d[] = { 0x7F, 0x10, 0x04, 0x05, 0x01, 0x01, 0x01, 0x01, 0x02, pp, vv };
    return juce::MidiMessage::createSysExMessage(d, 11);
}

// 16-bit signed MFX parameter packed as 4 nibbles around 8000H = 0 (manual p.67)
inline std::vector<std::uint8_t> encodeMfxValue(int value /* typically -20000..20000, 0=unity */)
{
    const int encoded = 0x8000 + value;
    const unsigned u = static_cast<unsigned>(encoded) & 0xFFFFu;
    return {
        static_cast<std::uint8_t>((u >> 12) & 0x0F),
        static_cast<std::uint8_t>((u >> 8) & 0x0F),
        static_cast<std::uint8_t>((u >> 4) & 0x0F),
        static_cast<std::uint8_t>(u & 0x0F)
    };
}

inline juce::MidiMessage mfxParamCommon(int mfxIndex0, int paramIndex1to32, int value)
{
    // param 1 → 06 11, then +4 per param. After 28 params the high byte ticks.
    const int idx = juce::jlimit(1, 32, paramIndex1to32) - 1;
    int addr = 0x0611 + idx * 4;
    // MFX B starts 08 11, C 0A 11
    addr += mfxIndex0 * 0x0200;
    const auto a0 = static_cast<std::uint8_t>((addr >> 8) & 0x7F);
    const auto a1 = static_cast<std::uint8_t>(addr & 0x7F);
    return dt1(0x10, 0x00, a0, a1, encodeMfxValue(value));
}

inline juce::MidiMessage gmMasterVolume(int value0to127)
{
    // Universal Real-Time Master Volume (device 7F = all). Physical knob still works.
    const std::uint8_t d[] = { 0x7F, 0x7F, 0x04, 0x01, 0x00,
                               static_cast<std::uint8_t>(juce::jlimit(0, 127, value0to127)) };
    return juce::MidiMessage::createSysExMessage(d, 6);
}

inline juce::MidiMessage mmcPlay()
{
    const std::uint8_t d[] = { 0x7F, 0x7F, 0x06, 0x02 };
    return juce::MidiMessage::createSysExMessage(d, 4);
}

inline juce::MidiMessage mmcStop()
{
    const std::uint8_t d[] = { 0x7F, 0x7F, 0x06, 0x01 };
    return juce::MidiMessage::createSysExMessage(d, 4);
}

// MIDI Song Select (F3 nn). JUCE 9 has no MidiMessage::songSelect.
inline juce::MidiMessage songSelect(int song0to127)
{
    const std::uint8_t d[] = { 0xF3, static_cast<std::uint8_t>(juce::jlimit(0, 127, song0to127)) };
    return juce::MidiMessage(d, 2);
}

// Internal demo. Owner's Manual p.13 is the panel DEMO key (the documented path).
// We fire several triggers immediately: Native DT1 00 00 01 00, DT1 00 00 00 7F,
// Song Select + MIDI Start, MMC Play. Firmware that ignores SysEx still needs the panel.
inline juce::MidiMessage demoPlay(int song0stop1to3, std::uint8_t dev = kDefaultDevId)
{
    return dt1(0x00, 0x00, 0x01, 0x00,
               { static_cast<std::uint8_t>(juce::jlimit(0, 3, song0stop1to3)) }, dev);
}

inline juce::MidiMessage demoPlayAlt(int song0stop1to3, std::uint8_t dev = kDefaultDevId)
{
    return dt1(0x00, 0x00, 0x00, 0x7F,
               { static_cast<std::uint8_t>(juce::jlimit(0, 3, song0stop1to3)) }, dev);
}

inline juce::MidiMessage makeCc(int channel1to16, int ccNum, int value)
{
    return juce::MidiMessage::controllerEvent(channel1to16, ccNum, juce::jlimit(0, 127, value));
}

inline juce::MidiMessage programChange(int channel1to16, int pc0to127)
{
    return juce::MidiMessage::programChange(channel1to16, juce::jlimit(0, 127, pc0to127));
}

// Bank + PC in the order the manual requires (p.59): CC0, CC32, PC.
inline void appendPatchSelect(std::vector<juce::MidiMessage>& out,
                              int channel1to16, std::uint8_t msb, std::uint8_t lsb, std::uint8_t pc)
{
    out.push_back(makeCc(channel1to16, cc::bankMSB, msb));
    out.push_back(makeCc(channel1to16, cc::bankLSB, lsb));
    out.push_back(programChange(channel1to16, pc));
}

struct MfxTypeInfo
{
    int id; // 0 = THROUGH
    const char* name;
};

// 90 algorithms + THROUGH. Names from SD-80 Owner's Manual MFX parameter list pp.80–94.
inline constexpr MfxTypeInfo kMfxTypes[] = {
    {  0, "THROUGH" },
    {  1, "Stereo EQ" },
    {  2, "Overdrive" },
    {  3, "Distortion" },
    {  4, "Phaser" },
    {  5, "Spectrum" },
    {  6, "Enhancer" },
    {  7, "Auto Wah" },
    {  8, "Rotary" },
    {  9, "Compressor" },
    { 10, "Limiter" },
    { 11, "Hexa-Chorus" },
    { 12, "Tremolo Chorus" },
    { 13, "Space-D" },
    { 14, "Stereo Chorus" },
    { 15, "Stereo Flanger" },
    { 16, "Step Flanger" },
    { 17, "Stereo Delay" },
    { 18, "Modulation Delay" },
    { 19, "Triple Tap Delay" },
    { 20, "Quadruple Tap Delay" },
    { 21, "Time Control Delay" },
    { 22, "2Voice Pitch Shifter" },
    { 23, "Fbk Pitch Shifter" },
    { 24, "Reverb" },
    { 25, "Gated Reverb" },
    { 26, "Overdrive → Chorus" },
    { 27, "Overdrive → Flanger" },
    { 28, "Overdrive → Delay" },
    { 29, "Distortion → Chorus" },
    { 30, "Distortion → Flanger" },
    { 31, "Distortion → Delay" },
    { 32, "Enhancer → Chorus" },
    { 33, "Enhancer → Flanger" },
    { 34, "Enhancer → Delay" },
    { 35, "Chorus → Delay" },
    { 36, "Flanger → Delay" },
    { 37, "Chorus → Flanger" },
    { 38, "Chorus/Delay" },
    { 39, "Flanger/Delay" },
    { 40, "Chorus/Flanger" },
    { 41, "Stereo Phaser" },
    { 42, "Keysync Flanger" },
    { 43, "Formant Filter" },
    { 44, "Ring Modulator" },
    { 45, "Multi Tap Delay" },
    { 46, "Reverse Delay" },
    { 47, "Shuffle Delay" },
    { 48, "3D Delay" },
    { 49, "3Voice Pitch Shifter" },
    { 50, "LoFi Compress" },
    { 51, "LoFi Noise" },
    { 52, "Speaker Simulator" },
    { 53, "Overdrive 2" },
    { 54, "Distortion 2" },
    { 55, "Stereo Compressor" },
    { 56, "Stereo Limiter" },
    { 57, "Gate" },
    { 58, "Slicer" },
    { 59, "Isolator" },
    { 60, "3D Chorus" },
    { 61, "3D Flanger" },
    { 62, "Tremolo" },
    { 63, "Auto Pan" },
    { 64, "Stereo Phaser 2" },
    { 65, "Stereo Auto Wah" },
    { 66, "Stereo Formant Filter" },
    { 67, "Multi Tap Delay 2" },
    { 68, "Reverse Delay 2" },
    { 69, "Shuffle Delay 2" },
    { 70, "3D Delay 2" },
    { 71, "Rotary 2" },
    { 72, "Rotary Multi" },
    { 73, "Keyboard Multi" },
    { 74, "Rhodes Multi" },
    { 75, "JD Multi" },
    { 76, "Stereo LoFi Compress" },
    { 77, "Stereo LoFi Noise" },
    { 78, "Guitar Amp Simulator" },
    { 79, "Stereo Overdrive" },
    { 80, "Stereo Distortion" },
    { 81, "Guitar Multi A" },
    { 82, "Guitar Multi B" },
    { 83, "Guitar Multi C" },
    { 84, "Clean Guitar Multi A" },
    { 85, "Clean Guitar Multi B" },
    { 86, "Bass Multi" },
    { 87, "Isolator 2" },
    { 88, "Stereo Spectrum" },
    { 89, "3D Auto Spin" },
    { 90, "3D Manual" },
};

inline constexpr int kNumMfxTypes = 91;

inline const char* gm2ReverbTypeName(int t)
{
    switch (t)
    {
        case 0: return "Small Room";
        case 1: return "Medium Room";
        case 2: return "Large Room";
        case 3: return "Medium Hall";
        case 4: return "Large Hall";
        case 8: return "Plate";
        default: return "Reverb";
    }
}

inline const char* gm2ChorusTypeName(int t)
{
    switch (t)
    {
        case 0: return "Chorus 1";
        case 1: return "Chorus 2";
        case 2: return "Chorus 3";
        case 3: return "Chorus 4";
        case 4: return "FB Chorus";
        case 5: return "Flanger";
        default: return "Chorus";
    }
}

inline const char* mfxGroupName(int id)
{
    if (id == 0) return "Utility";
    if (id == 1 || id == 5 || id == 6 || id == 43 || id == 59 || id == 66 || id == 87 || id == 88)
        return "EQ / Tone";
    if (id == 2 || id == 3 || id == 53 || id == 54 || id == 79 || id == 80)
        return "Drive";
    if (id == 9 || id == 10 || id == 50 || id == 51 || id == 55 || id == 56 || id == 57 || id == 58
        || id == 76 || id == 77)
        return "Dynamics";
    if (id == 4 || id == 7 || id == 8 || (id >= 11 && id <= 16) || id == 41 || id == 42 || id == 44
        || (id >= 60 && id <= 65) || id == 71)
        return "Modulation";
    if ((id >= 17 && id <= 21) || (id >= 45 && id <= 47) || (id >= 67 && id <= 70))
        return "Delay";
    if (id == 22 || id == 23 || id == 49)
        return "Pitch";
    if (id == 24 || id == 25)
        return "Reverb";
    if (id >= 26 && id <= 40)
        return "Chains";
    if (id >= 72 && id <= 86)
        return "Amp / Multi";
    if (id == 48 || id == 60 || id == 61 || id == 70 || id == 89 || id == 90)
        return "3D";
    return "Other";
}

inline constexpr const char* kMfxGroupOrder[] = {
    "Utility", "EQ / Tone", "Drive", "Dynamics", "Modulation",
    "Delay", "Pitch", "Reverb", "Chains", "Amp / Multi", "3D", "Other"
};
inline constexpr int kNumMfxGroups = 12;

} // namespace sd80