#pragma once

// Parses SMF Type 0/1 and extracts per-channel Bank MSB/LSB, Program Change,
// Volume, Pan, Reverb, Chorus. MIDI Port meta (FF 21) maps port 1+ onto Part B.

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include "SD80Sysex.h"

struct ImportedPart
{
    bool used = false;
    std::uint8_t msb = 96, lsb = 0, pc = 0;
    std::uint8_t volume = 100, pan = 64, reverb = 40, chorus = 0;
    bool hasBank = false, hasPc = false;
};

struct ImportResult
{
    std::array<ImportedPart, 32> parts {};
    int partsTouched = 0;
    juce::String summary;
};

inline ImportResult parseMidiFileForSd80(const juce::File& file)
{
    ImportResult result;
    juce::FileInputStream in(file);
    if (! in.openedOk())
    {
        result.summary = "Could not open file.";
        return result;
    }

    juce::MidiFile midi;
    if (! midi.readFrom(in))
    {
        result.summary = "Not a valid Standard MIDI File.";
        return result;
    }

    const int nTracks = midi.getNumTracks();
    for (int t = 0; t < nTracks; ++t)
    {
        const auto* seq = midi.getTrack(t);
        if (seq == nullptr)
            continue;

        int port = 0; // 0 → Part A
        for (int i = 0; i < seq->getNumEvents(); ++i)
        {
            const auto msg = seq->getEventPointer(i)->message;

            if (msg.isMetaEvent() && msg.getMetaEventType() == 0x21
                && msg.getMetaEventLength() >= 1)
            {
                port = msg.getRawData()[msg.getMetaEventLength() > 0 ? 3 : 2];
                // Meta layout: FF 21 01 <port>. juce stores sysex-like; use getSysExData if needed.
                const auto* raw = msg.getRawData();
                const int sz = msg.getRawDataSize();
                if (sz >= 1)
                    port = raw[sz - 1];
            }

            if (! msg.isForChannel(msg.getChannel()) && ! msg.isControllerOfType(0)
                && ! msg.isProgramChange() && ! msg.isController())
            {
                // fall through — juce channel messages have channel 1-16
            }

            if (! (msg.isController() || msg.isProgramChange()))
                continue;

            const int ch = msg.getChannel(); // 1-16
            if (ch < 1 || ch > 16)
                continue;

            const int part = (port >= 1 ? 16 : 0) + (ch - 1);
            if (part < 0 || part > 31)
                continue;

            auto& p = result.parts[(size_t) part];
            p.used = true;

            if (msg.isController())
            {
                const int cc = msg.getControllerNumber();
                const int v  = msg.getControllerValue();
                if (cc == sd80::cc::bankMSB) { p.msb = (std::uint8_t) v; p.hasBank = true; }
                else if (cc == sd80::cc::bankLSB) { p.lsb = (std::uint8_t) v; p.hasBank = true; }
                else if (cc == sd80::cc::volume) p.volume = (std::uint8_t) v;
                else if (cc == sd80::cc::pan) p.pan = (std::uint8_t) v;
                else if (cc == sd80::cc::reverb) p.reverb = (std::uint8_t) v;
                else if (cc == sd80::cc::chorus) p.chorus = (std::uint8_t) v;
            }
            else if (msg.isProgramChange())
            {
                p.pc = (std::uint8_t) msg.getProgramChangeNumber();
                p.hasPc = true;
            }
        }
    }

    int n = 0;
    for (auto& p : result.parts)
        if (p.used && (p.hasPc || p.hasBank))
            ++n;
    result.partsTouched = n;
    result.summary = "Mapped " + juce::String(n) + " parts from " + file.getFileName();
    return result;
}
