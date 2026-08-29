#pragma once

// Non-blocking FIFO of MIDI messages with a configurable inter-message delay.
// SD-80 USB overflows if CC/SysEx is dumped too fast (manual / field reports).
// Default 30 ms, range 20–50 ms as specified.

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <deque>

enum class MidiPort : std::uint8_t { A = 0, B = 1, Both = 2 };

struct QueuedMidi
{
    juce::MidiMessage message;
    MidiPort port { MidiPort::A };
};

class MidiThrottleQueue
{
public:
    void setDelayMs(int ms) noexcept
    {
        delayMs.store(juce::jlimit(20, 50, ms), std::memory_order_relaxed);
    }

    int getDelayMs() const noexcept
    {
        return delayMs.load(std::memory_order_relaxed);
    }

    void push(const juce::MidiMessage& m, MidiPort port)
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        queue.push_back({ m, port });
    }

    void pushMany(const std::vector<juce::MidiMessage>& msgs, MidiPort port)
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        for (auto& m : msgs)
            queue.push_back({ m, port });
    }

    void clear()
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        queue.clear();
        pendingSamples = 0;
    }

    int size() const
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        return (int) queue.size();
    }

    // Call from processBlock. Pops at most one message every delayMs.
    bool popDue(double sampleRate, int numSamples, QueuedMidi& out)
    {
        const int gap = (int) std::llround(sampleRate * (getDelayMs() / 1000.0));
        const juce::SpinLock::ScopedLockType sl(lock);
        pendingSamples += numSamples;
        if (queue.empty())
        {
            pendingSamples = juce::jmin(pendingSamples, gap);
            return false;
        }
        if (pendingSamples < gap && !firstImmediate)
            return false;
        firstImmediate = false;
        pendingSamples = 0;
        out = std::move(queue.front());
        queue.pop_front();
        return true;
    }

    // Editor-thread / timer path when audio is idle.
    bool popNow(QueuedMidi& out)
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        if (queue.empty())
            return false;
        out = std::move(queue.front());
        queue.pop_front();
        firstImmediate = false;
        return true;
    }

    void resetTiming()
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        pendingSamples = 0;
        firstImmediate = true;
    }

private:
    mutable juce::SpinLock lock;
    std::deque<QueuedMidi> queue;
    std::atomic<int> delayMs { 30 };
    int pendingSamples { 0 };
    bool firstImmediate { true };
};
