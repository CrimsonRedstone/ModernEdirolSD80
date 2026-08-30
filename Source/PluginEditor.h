#pragma once

#include <array>
#include <functional>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "SD80LookAndFeel.h"
#include "ParamLock.h"
#include "CassetteDeck.h"
#include "MidiRoll.h"

class ModernEdirolSd80Editor : public juce::AudioProcessorEditor,
                               public juce::FileDragAndDropTarget,
                               private juce::Timer,
                               private juce::Button::Listener,
                               private juce::ComboBox::Listener,
                               private juce::Slider::Listener
{
public:
    explicit ModernEdirolSd80Editor(ModernEdirolSd80Processor&);
    ~ModernEdirolSd80Editor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray&, int, int) override;
    void fileDragExit(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray& files, int, int) override;

    enum class Tab { Mixer, Player, Demos, Options };

private:
    void timerCallback() override;
    void buttonClicked(juce::Button*) override;
    void comboBoxChanged(juce::ComboBox*) override;
    void sliderValueChanged(juce::Slider*) override;

    void applySkin();
    void rebuildDeviceLists();
    void refreshStripLabels();
    void bindSelectedPart();
    void rebuildPatchBrowser();
    void pickPatchAt(int row);
    void setTab(Tab t);
    void toggleLock(const juce::String& id);
    void wireLock(sd80lock::Slider& s, const juce::String& id);
    void wireLock(sd80lock::TextButton& b, const juce::String& id);
    void wireLock(sd80lock::ComboBox& b, const juce::String& id);
    void wireLock(sd80lock::ToggleButton& b, const juce::String& id);
    void wireLock(sd80lock::Label& b, const juce::String& id);
    void ensureStandaloneAudio();
    void showMfxMenu(int which);
    void showReverbMenu();
    void showChorusMenu();
    void loadPlayerFile();
    void updatePlayerUi();
    void updateFxLabels();
    void confirmDanger(const juce::String& title, const juce::String& body,
                       bool requireSure, bool showDontShow,
                       std::function<void(bool dontShow)> onProceed);

    juce::Rectangle<int> headerArea() const;
    juce::Rectangle<int> contentArea() const;

    ModernEdirolSd80Processor& proc;
    SD80LookAndFeel lnf;
    Tab tab { Tab::Mixer };

    juce::Label title, subtitle, creditsLabel, queueLabel, dropHint;
    juce::TextButton syncButton { "SYNC HARDWARE" };
    juce::TextButton partAButton { "PART A  1-16" };
    juce::TextButton partBButton { "PART B  17-32" };
    juce::TextButton tabMixer { "MIXER" };
    juce::TextButton tabPlayer { "PLAYER" };
    juce::TextButton tabDemos { "DEMOS" };
    juce::TextButton tabOptions { "OPTIONS" };
    juce::TextButton savePreset { "Save preset" };
    juce::TextButton loadPreset { "Load preset" };

    juce::Component mixerPage, playerPage, demoPage, optionsPage;
    juce::Viewport optionsView;
    juce::Component optionsInner;

    struct Strip
    {
        sd80lock::TextButton select, mute, solo;
        sd80lock::Label name;
        juce::Label ch;
        sd80lock::Slider vol, pan;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volAtt, panAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAtt, soloAtt;
    };
    std::array<Strip, 16> strips;
    juce::TextButton muteAllBtn { "MUTE ALL" };
    juce::TextButton unmuteAllBtn { "UNMUTE ALL" };
    juce::TextButton unsoloAllBtn { "UNSOLO ALL" };

    juce::Label inspectorTitle, patchNameLabel, catTitle, patchTitle;
    sd80lock::ComboBox mapBox;
    sd80lock::ToggleButton drumToggle { "Drum part" };
    juce::TextEditor patchSearch;
    juce::ListBox categoryList, patchList;

    struct CategoryModel : public juce::ListBoxModel
    {
        juce::StringArray names;
        int selected { 0 };
        std::function<void(int)> onSel;
        juce::Colour text, muted, selBg;
        int getNumRows() override { return names.size(); }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel) override;
        void selectedRowsChanged(int row) override;
    } catModel;

    struct PatchModel : public juce::ListBoxModel
    {
        std::vector<const sd80::PatchEntry*> items;
        int selected { -1 };
        std::function<void(int)> onSel;
        std::function<void()> onLock;
        juce::Colour text, muted, selBg, selFg;
        int getNumRows() override { return (int) items.size(); }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel) override;
        void selectedRowsChanged(int row) override;
        void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    } patchModel;

    sd80lock::Slider cut, res, atk, dec, rel, vr, vd, vdl, expr, rev, cho, mfxSend, portaT;
    sd80lock::ToggleButton porta { "Portamento" };
    sd80lock::ComboBox mfxSel, outAsg;
    juce::Label cutL, resL, atkL, decL, relL, vrL, vdL, vdlL, exprL, revL, choL, mfxL, portaL;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        cutA, resA, atkA, decA, relA, vrA, vdA, vdlA, exprA, revA, choA, mfxA, portaTA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> portaA, drumA;

    juce::Label fxTitle;
    juce::TextButton reverbTypeBtn { "Reverb" }, chorusTypeBtn { "Chorus" };
    juce::TextButton mfxABtn { "Multi FX A" }, mfxBBtn { "Multi FX B" }, mfxCBtn { "Multi FX C" };
    sd80lock::TextButton mfxAOn { "A" }, mfxBOn { "B" }, mfxCOn { "C" };
    sd80lock::Slider reverbTime, chorusRate, chorusDepth, chorusFb;
    juce::Label revTimeL, choRateL, choDepthL, choFbL;
    sd80lock::Slider mfxKnobs[3][4];
    juce::Label mfxKnobL[3][4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mfxKnobA[3][4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> revTimeA, choRateA, choDepthA, choFbA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> mfxAOnA, mfxBOnA, mfxCOnA;
    juce::TextButton resetFxBtn { "Reset effects" };
    juce::Label mfxWarn;

    CassetteDeck deck;
    MidiRoll roll;
    juce::Label playerHelp, playerWarn;

    juce::Label demoTitle, demoHelp;
    juce::TextButton demo1 { "Demo 1" }, demo2 { "Demo 2" }, demo3 { "Demo 3" }, demoStop { "Stop demo" };

    juce::Label optionsTitle, skinTitle, creditsBody, midiNote, shortcutsTitle, shortcutsBody;
    juce::Label audioTitle, hostAudioBanner, portsTitle;
    juce::Label portAL { {}, "Part A USB" }, portBL { {}, "Part B USB" };
    juce::TextButton donateBtn {
        "This is freeware. If you'd like to support me, consider purchasing my music."
    };
    std::array<juce::TextButton, kNumSkins> skinButtons;
    sd80lock::ComboBox modeBox, outABox, outBBox, throttleBox, hostRouteBox;
    sd80lock::ToggleButton hostMirrorA { "Host MIDI mirrors Part A" };
    sd80lock::ToggleButton hostMirrorB { "Host MIDI mirrors Part B" };
    sd80lock::Slider masterVol;
    juce::Label masterVolL, hostRouteL, audioMidiNote;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> hostAAtt, hostBAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt, hostRouteAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterVolAtt;
    juce::TextButton pullHardwareBtn { "Pull from SD-80" };
    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioSelector;
    juce::Component hostAudioDummy;
    juce::Label hostDriverL, hostDeviceL, hostSrL, hostBufL;
    juce::ComboBox hostDriverBox, hostDeviceBox, hostSrBox, hostBufBox;
    juce::TextButton emergencyBtn { "Emergency Hardware Reset" };

    bool dragging { false };
    int lastBoundPart { -1 };
    juce::TooltipWindow tooltipWindow { this, 700 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernEdirolSd80Editor)
};
