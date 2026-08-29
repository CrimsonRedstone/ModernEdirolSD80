#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_data_structures/juce_data_structures.h>
#include "SD80PatchData.h"
#include "SD80Sysex.h"
#include "MidiThrottleQueue.h"
#include "MidiFileImporter.h"
#include "MidiPlayer.h"

class ModernEdirolSd80Processor : public juce::AudioProcessor,
                                   public juce::Timer,
                                   private juce::AudioProcessorValueTreeState::Listener,
                                   private juce::MidiInputCallback
{
public:
    ModernEdirolSd80Processor();
    ~ModernEdirolSd80Processor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    MidiPlayerEngine player;

    juce::StringArray midiOutputNames() const;
    juce::StringArray midiInputNames() const;
    void setMidiOutputA(int deviceIndex);
    void setMidiOutputB(int deviceIndex);
    void setMidiInput(int deviceIndex);
    int getMidiOutputAIndex() const { return outAIndex; }
    int getMidiOutputBIndex() const { return outBIndex; }
    juce::String getMidiOutputAName() const { return outAName; }
    juce::String getMidiOutputBName() const { return outBName; }

    void enqueuePartPatch(int part);
    void enqueuePartMix(int part);
    void enqueuePartDeep(int part);
    void enqueueMfxBlock();
    void syncHardwarePush();
    void requestHardwareDump();
    void applyMidiFile(const juce::File&);
    void setGeneratorMode(sd80::GeneratorMode m);
    sd80::GeneratorMode getGeneratorMode() const;

    bool saveMesd80Preset(const juce::File&);
    bool loadMesd80Preset(const juce::File&);

    int getSelectedPart() const { return selectedPart.load(); }
    void setSelectedPart(int p);
    bool isPartSelected(int p) const;
    void togglePartSelected(int p, bool exclusive);
    std::uint32_t getSelectedMask() const { return selectedMask.load(); }

    int getVisibleGroup() const { return visibleGroup.load(); }
    void setVisibleGroup(int g) { visibleGroup.store(g ? 1 : 0); }

    juce::String getPartPatchName(int part) const;
    bool isPartDrum(int part) const;
    bool isPartSilenced(int part) const;
    int queueDepth() const { return throttle.size(); }

    bool isLocked(const juce::String& paramId) const;
    void setLocked(const juce::String& paramId, bool shouldLock);
    juce::StringArray getLockedIds() const;

    int getSkinIndex() const { return skinIndex; }
    void setSkinIndex(int i);

    void muteAll(bool shouldMute);
    void unsoloAll();
    void autoDetectUsbPorts();
    void factoryResetHardware();
    void resetEffectsToDefault();
    void pullFromHardware();
    void playInternalDemo(int song0stop1to3);
    void sendMasterVolume();
    bool consumeDumpDirty() { return dumpDirty.exchange(false); }
    bool skipFxResetWarning() const { return skipFxWarn; }
    void setSkipFxResetWarning(bool v);

    static juce::String mfxParamId(int slot0to2, int param0to3);

    std::function<void()> onImportFinished;
    std::function<void()> onSkinChanged;
    juce::String lastImportSummary;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String pid(int part, const char* key);

    void setParamInt(const juce::String& id, int v);

private:
    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void sendQueued(const QueuedMidi&, juce::MidiBuffer* hostOut, int sampleOffset);
    void openNamedOutput(std::unique_ptr<juce::MidiOutput>& slot, juce::String& stored, int& index, const juce::String& name);
    int paramInt(const juce::String& id, int fallback = 0) const;
    bool paramBool(const juce::String& id) const;
    MidiPort portForPart(int part) const { return part < 16 ? MidiPort::A : MidiPort::B; }
    int channelForPart(int part) const { return (part % 16) + 1; }
    void persistAppSettings();
    void restoreAppSettings();
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage&) override;
    void applyDumpMessage(const juce::MidiMessage&);
    void openNamedInput(std::unique_ptr<juce::MidiInput>& slot, juce::String& stored,
                        int& index, const juce::String& name);

    MidiThrottleQueue throttle;
    std::unique_ptr<juce::MidiOutput> midiOutA, midiOutB;
    std::unique_ptr<juce::MidiInput> midiInA, midiInB;
    juce::String outAName, outBName, inAName, inBName;
    int outAIndex { -1 }, outBIndex { -1 }, inAIndex { -1 }, inBIndex { -1 };
    double currentSampleRate { 44100.0 };
    std::atomic<int> selectedPart { 0 };
    std::atomic<int> visibleGroup { 0 };
    std::atomic<std::uint32_t> selectedMask { 1u };
    juce::CriticalSection deviceLock;
    std::atomic<bool> suppressOutgoing { false };

    juce::Array<juce::MidiMessage> dumpInbox;
    juce::CriticalSection dumpLock;
    int persistTicker { 0 };
    std::atomic<bool> dumpDirty { false };

    juce::StringArray lockedIds;
    int skinIndex { 0 };
    bool skipFxWarn { false };
    juce::ApplicationProperties appProps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernEdirolSd80Processor)
};
