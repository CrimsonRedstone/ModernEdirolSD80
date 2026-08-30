#pragma once

#include <cmath>
#include <cstdint>
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include "MidiThrottleQueue.h"

// One note-on/note-off pair from the loaded SMF (PLAYER piano-roll).
struct PlayerNote
{
    double startSec { 0.0 };
    double endSec { 0.25 };
    int note { 60 };     // 0-127
    int channel { 1 };   // 1-16  (Part A)
};

// SMF playback engine. Timestamps are converted to seconds.
// Separate from drag-and-drop hardware setup (applyMidiFile).
class MidiPlayerEngine
{
public:
    bool load(const juce::File& file)
    {
        juce::FileInputStream in(file);
        if (! in.openedOk())
            return false;
        juce::MidiFile midi;
        if (! midi.readFrom(in))
            return false;
        midi.convertTimestampTicksToSeconds();

        juce::ScopedLock sl(lock);
        sequence.clear();
        for (int t = 0; t < midi.getNumTracks(); ++t)
            if (auto* tr = midi.getTrack(t))
                sequence.addSequence(*tr, 0.0);
        sequence.sort();
        sequence.updateMatchedPairs();
        lengthSec = sequence.getEndTime();
        positionSec = 0.0;
        loaded = true;
        playing = false;
        panic = false;
        name = file.getFileName();
        path = file;
        nextIndex = 0;
        rebuildScoreUnlocked();
        return true;
    }

    void play()  { juce::ScopedLock sl(lock); if (loaded) playing = true; }
    void pause()
    {
        juce::ScopedLock sl(lock);
        playing = false;
        panic = true;
    }
    void stop()
    {
        juce::ScopedLock sl(lock);
        playing = false;
        positionSec = 0.0;
        nextIndex = 0;
        panic = true;
    }

    void setLooping(bool v) { juce::ScopedLock sl(lock); looping = v; }
    bool isLooping() const { juce::ScopedLock sl(lock); return looping; }

    bool isPlaying() const { juce::ScopedLock sl(lock); return playing; }
    bool isLoaded()  const { juce::ScopedLock sl(lock); return loaded; }
    double getPosition() const { juce::ScopedLock sl(lock); return positionSec; }
    double getLength()   const { juce::ScopedLock sl(lock); return lengthSec; }
    juce::String getName() const { juce::ScopedLock sl(lock); return name; }
    juce::File getFile() const { juce::ScopedLock sl(lock); return path; }

    // Copies the score when the loaded file has changed. Cheap no-op otherwise.
    void copyScore(int& seenGen, std::vector<PlayerNote>& dest,
                   double& lengthOut, int& lo, int& hi, std::uint32_t& mask) const
    {
        juce::ScopedLock sl(lock);
        if (seenGen == loadGeneration)
            return;
        dest = notes;
        lengthOut = lengthSec;
        lo = loNote;
        hi = hiNote;
        mask = channelMask;
        seenGen = loadGeneration;
    }

    // Channel 1-16 -> Part A USB. All-notes-off is emitted on pause/stop.
    void render(double sampleRate, int numSamples,
                const std::function<void(const juce::MidiMessage&, int /*sample*/, MidiPort)>& emit)
    {
        juce::ScopedLock sl(lock);
        if (sampleRate <= 0.0)
            return;

        if (panic)
        {
            for (int ch = 1; ch <= 16; ++ch)
            {
                emit(juce::MidiMessage::allNotesOff(ch), 0, MidiPort::A);
                emit(juce::MidiMessage::controllerEvent(ch, 123, 0), 0, MidiPort::A);
            }
            panic = false;
        }

        if (! playing || ! loaded)
            return;

        const double blockSec = (double) numSamples / sampleRate;
        const double start = positionSec;
        const double end = positionSec + blockSec;
        const int n = sequence.getNumEvents();

        while (nextIndex < n)
        {
            auto* ev = sequence.getEventPointer(nextIndex);
            const double t = ev->message.getTimeStamp();
            if (t >= end)
                break;
            if (t >= start - 1.0e-7)
            {
                int sample = (int) std::llround((t - start) * sampleRate);
                sample = juce::jlimit(0, numSamples - 1, sample);
                auto msg = ev->message;
                if (! msg.isMetaEvent())
                    emit(msg, sample, MidiPort::A);
            }
            ++nextIndex;
        }

        positionSec = end;
        if (positionSec >= lengthSec && lengthSec > 0.0)
        {
            if (looping)
            {
                positionSec = 0.0;
                nextIndex = 0;
            }
            else
            {
                playing = false;
                positionSec = lengthSec;
                panic = true;
            }
        }
    }

private:
    void rebuildScoreUnlocked()
    {
        notes.clear();
        channelMask = 0;
        loNote = 127;
        hiNote = 0;
        const int n = sequence.getNumEvents();
        notes.reserve((size_t) juce::jmax(0, n / 2));
        for (int i = 0; i < n; ++i)
        {
            auto* ev = sequence.getEventPointer(i);
            if (ev == nullptr)
                continue;
            const auto& m = ev->message;
            if (! m.isNoteOn() || m.getVelocity() <= 0)
                continue;
            const int ch = m.getChannel();
            if (ch < 1 || ch > 16)
                continue;
            const int nn = juce::jlimit(0, 127, m.getNoteNumber());
            const double start = m.getTimeStamp();
            double end = start + 0.35;
            if (ev->noteOffObject != nullptr)
                end = juce::jmax(start + 0.04, ev->noteOffObject->message.getTimeStamp());
            notes.push_back({ start, end, nn, ch });
            channelMask |= (1u << (ch - 1));
            loNote = juce::jmin(loNote, nn);
            hiNote = juce::jmax(hiNote, nn);
        }
        if (hiNote < loNote)
        {
            loNote = 48;
            hiNote = 72;
        }
        ++loadGeneration;
    }

    juce::MidiMessageSequence sequence;
    std::vector<PlayerNote> notes;
    juce::File path;
    juce::String name;
    double positionSec { 0.0 };
    double lengthSec { 0.0 };
    int nextIndex { 0 };
    int loNote { 48 };
    int hiNote { 72 };
    std::uint32_t channelMask { 0 };
    int loadGeneration { 0 };
    bool playing { false };
    bool loaded { false };
    bool panic { false };
    bool looping { false };
    mutable juce::CriticalSection lock;
};
