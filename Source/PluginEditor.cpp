#include "PluginEditor.h"
#include <cstring>
#if JucePlugin_Build_Standalone
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

using namespace sd80;

static void styleKnob(juce::Slider& s, const juce::String& name)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 42, 14);
    s.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                          juce::MathConstants<float>::pi * 2.8f, true);
    s.setName(name);
}

void ModernEdirolSd80Editor::CategoryModel::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel)
{
    if (! juce::isPositiveAndBelow(row, names.size()))
        return;
    if (sel)
    {
        g.setColour(selBg);
        g.fillRect(0, 0, w, h);
    }
    g.setColour(text);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(names[row], 8, 0, w - 12, h, juce::Justification::centredLeft);
}

void ModernEdirolSd80Editor::CategoryModel::selectedRowsChanged(int row)
{
    if (row >= 0 && row != selected)
    {
        selected = row;
        if (onSel) onSel(row);
    }
}

void ModernEdirolSd80Editor::PatchModel::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel)
{
    if (! juce::isPositiveAndBelow(row, (int) items.size()))
        return;
    if (sel)
    {
        g.setColour(selBg);
        g.fillRect(0, 0, w, h);
    }
    auto* e = items[(size_t) row];
    g.setColour(sel ? selFg : text);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(e->name, 8, 0, w - 72, h, juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("PC " + juce::String(e->pc + 1), w - 66, 0, 58, h, juce::Justification::centredRight);
}

void ModernEdirolSd80Editor::PatchModel::selectedRowsChanged(int row)
{
    selected = row;
}

void ModernEdirolSd80Editor::PatchModel::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        if (onLock) onLock();
        return;
    }
    selected = row;
    if (onSel) onSel(row);
}

ModernEdirolSd80Editor::ModernEdirolSd80Editor(ModernEdirolSd80Processor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&lnf);
    setSize(1440, 900);
    setResizable(true, true);
    setResizeLimits(1180, 760, 2400, 1600);

    auto lockFn = [this](const juce::String& id) { return proc.isLocked(id); };
    auto togFn  = [this](const juce::String& id) { toggleLock(id); };

    title.setText("MODERN EDIROL SD-80", juce::dontSendNotification);
    title.setFont(juce::FontOptions(22.0f).withStyle("Bold"));
    addAndMakeVisible(title);

    subtitle.setText("32-part USB MIDI controller  |  Native / GM2 / GS / XG Lite", juce::dontSendNotification);
    subtitle.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(subtitle);

    creditsLabel.setText("by Crimson Redstone", juce::dontSendNotification);
    creditsLabel.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(creditsLabel);

    queueLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(queueLabel);

    dropHint.setText("Drop a .mid on the mixer to auto-assign banks. Player tab is a separate cassette deck.",
                     juce::dontSendNotification);
    dropHint.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(dropHint);

    for (auto* b : { &syncButton, &partAButton, &partBButton, &tabMixer, &tabPlayer, &tabDemos, &tabOptions,
                     &savePreset, &loadPreset })
    {
        b->addListener(this);
        addAndMakeVisible(*b);
    }
    partAButton.setClickingTogglesState(true);
    partBButton.setClickingTogglesState(true);
    partAButton.setRadioGroupId(1);
    partBButton.setRadioGroupId(1);
    partAButton.setToggleState(true, juce::dontSendNotification);

    tabMixer.setClickingTogglesState(true);
    tabPlayer.setClickingTogglesState(true);
    tabDemos.setClickingTogglesState(true);
    tabOptions.setClickingTogglesState(true);
    tabMixer.setRadioGroupId(3);
    tabPlayer.setRadioGroupId(3);
    tabDemos.setRadioGroupId(3);
    tabOptions.setRadioGroupId(3);
    tabMixer.setToggleState(true, juce::dontSendNotification);

    modeBox.addItem("Native", 1);
    modeBox.addItem("GM2", 2);
    modeBox.addItem("GS", 3);
    modeBox.addItem("XG Lite", 4);
    modeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.apvts, "mode", modeBox);
    modeBox.addListener(this);
    wireLock(modeBox, "mode");
    modeBox.lockedFn = lockFn; modeBox.toggleFn = togFn;

    throttleBox.addItem("20 ms", 20);
    throttleBox.addItem("30 ms", 30);
    throttleBox.addItem("40 ms", 40);
    throttleBox.addItem("50 ms", 50);
    throttleBox.setSelectedId(juce::jlimit(20, 50, (int) proc.apvts.getRawParameterValue("throttle")->load()),
                              juce::dontSendNotification);
    throttleBox.addListener(this);
    wireLock(throttleBox, "throttle");

    outABox.addListener(this);
    outBBox.addListener(this);

    addAndMakeVisible(mixerPage);
    addAndMakeVisible(playerPage);
    addAndMakeVisible(demoPage);
    addAndMakeVisible(optionsPage);
    optionsPage.addAndMakeVisible(optionsView);
    optionsView.setViewedComponent(&optionsInner, false);

    for (int i = 0; i < 16; ++i)
    {
        auto& st = strips[(size_t) i];
        st.ch.setText(juce::String(i + 1), juce::dontSendNotification);
        st.ch.setJustificationType(juce::Justification::centred);
        st.ch.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        st.name.setJustificationType(juce::Justification::centred);
        st.name.setFont(juce::FontOptions(11.0f));
        st.name.setInterceptsMouseClicks(true, false);
        st.name.setTooltip("Right-click to lock this instrument");
        st.name.lockedFn = lockFn; st.name.toggleFn = togFn;
        st.select.setButtonText("SEL");
        st.select.setClickingTogglesState(true);
        st.select.addListener(this);
        st.mute.setButtonText("M");
        st.mute.setClickingTogglesState(true);
        st.solo.setButtonText("S");
        st.solo.setClickingTogglesState(true);
        st.vol.setSliderStyle(juce::Slider::LinearVertical);
        st.vol.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 36, 14);
        st.pan.setSliderStyle(juce::Slider::LinearHorizontal);
        st.pan.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        mixerPage.addAndMakeVisible(st.ch);
        mixerPage.addAndMakeVisible(st.name);
        mixerPage.addAndMakeVisible(st.select);
        mixerPage.addAndMakeVisible(st.mute);
        mixerPage.addAndMakeVisible(st.solo);
        mixerPage.addAndMakeVisible(st.vol);
        mixerPage.addAndMakeVisible(st.pan);
        st.mute.addListener(this);
        st.solo.addListener(this);
        st.mute.lockedFn = lockFn; st.mute.toggleFn = togFn;
        st.solo.lockedFn = lockFn; st.solo.toggleFn = togFn;
        st.vol.lockedFn = lockFn; st.vol.toggleFn = togFn;
        st.pan.lockedFn = lockFn; st.pan.toggleFn = togFn;
        st.select.lockedFn = lockFn; st.select.toggleFn = togFn;
    }

    muteAllBtn.addListener(this);
    unmuteAllBtn.addListener(this);
    unsoloAllBtn.addListener(this);
    mixerPage.addAndMakeVisible(muteAllBtn);
    mixerPage.addAndMakeVisible(unmuteAllBtn);
    mixerPage.addAndMakeVisible(unsoloAllBtn);

    inspectorTitle.setText("DEEP EDIT", juce::dontSendNotification);
    inspectorTitle.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    mixerPage.addAndMakeVisible(inspectorTitle);
    patchNameLabel.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    mixerPage.addAndMakeVisible(patchNameLabel);

    mapBox.addItem("Classical", 1);
    mapBox.addItem("Contemporary", 2);
    mapBox.addItem("Solo", 3);
    mapBox.addItem("Enhanced", 4);
    mapBox.addItem("Special 1", 5);
    mapBox.addItem("Special 2", 6);
    mapBox.addItem("User", 7);
    mapBox.addListener(this);
    mixerPage.addAndMakeVisible(mapBox);
    mapBox.lockedFn = lockFn; mapBox.toggleFn = togFn;

    mixerPage.addAndMakeVisible(drumToggle);
    drumToggle.addListener(this);
    drumToggle.lockedFn = lockFn; drumToggle.toggleFn = togFn;

    catTitle.setText("Category", juce::dontSendNotification);
    catTitle.setFont(juce::FontOptions(11.0f));
    patchTitle.setText("Patch", juce::dontSendNotification);
    patchTitle.setFont(juce::FontOptions(11.0f));
    mixerPage.addAndMakeVisible(catTitle);
    mixerPage.addAndMakeVisible(patchTitle);

    patchSearch.setTextToShowWhenEmpty("Search patches", lnf.muted);
    patchSearch.onTextChange = [this] { rebuildPatchBrowser(); };
    mixerPage.addAndMakeVisible(patchSearch);

    catModel.onSel = [this](int) { rebuildPatchBrowser(); };
    patchModel.onSel = [this](int row) { pickPatchAt(row); };
    patchModel.onLock = [this]
    {
        const int part = proc.getSelectedPart();
        const bool next = ! proc.isLocked(ModernEdirolSd80Processor::pid(part, "pc"));
        proc.setLocked(ModernEdirolSd80Processor::pid(part, "msb"), next);
        proc.setLocked(ModernEdirolSd80Processor::pid(part, "lsb"), next);
        proc.setLocked(ModernEdirolSd80Processor::pid(part, "pc"), next);
        repaint();
    };
    categoryList.setModel(&catModel);
    patchList.setModel(&patchModel);
    categoryList.setRowHeight(20);
    patchList.setRowHeight(20);
    mixerPage.addAndMakeVisible(categoryList);
    mixerPage.addAndMakeVisible(patchList);

    auto addK = [this, &lockFn, &togFn](sd80lock::Slider& s, juce::Label& l, const char* n)
    {
        styleKnob(s, n);
        l.setText(n, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::FontOptions(10.0f));
        mixerPage.addAndMakeVisible(s);
        mixerPage.addAndMakeVisible(l);
        s.lockedFn = lockFn;
        s.toggleFn = togFn;
    };
    addK(cut, cutL, "Cutoff");
    addK(res, resL, "Resonance");
    addK(atk, atkL, "Attack");
    addK(dec, decL, "Decay");
    addK(rel, relL, "Release");
    addK(vr, vrL, "Vib Rate");
    addK(vd, vdL, "Vib Depth");
    addK(vdl, vdlL, "Vib Delay");
    addK(expr, exprL, "Expression");
    addK(rev, revL, "Reverb");
    addK(cho, choL, "Chorus");
    addK(mfxSend, mfxL, "MFX Send");
    addK(portaT, portaL, "Porta T");
    mixerPage.addAndMakeVisible(porta);
    porta.addListener(this);
    porta.lockedFn = lockFn; porta.toggleFn = togFn;

    mfxSel.addItem("Insert MFX A", 1);
    mfxSel.addItem("Insert MFX B", 2);
    mfxSel.addItem("Insert MFX C", 3);
    mfxSel.addListener(this);
    mixerPage.addAndMakeVisible(mfxSel);
    mfxSel.lockedFn = lockFn; mfxSel.toggleFn = togFn;

    outAsg.addItem("Out: MFX", 1);
    outAsg.addItem("Out: MAIN", 2);
    outAsg.addListener(this);
    mixerPage.addAndMakeVisible(outAsg);
    outAsg.lockedFn = lockFn; outAsg.toggleFn = togFn;

    fxTitle.setText("SYSTEM FX  +  MULTI-FX  (categorized)", juce::dontSendNotification);
    fxTitle.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    mixerPage.addAndMakeVisible(fxTitle);
    for (auto* b : { &reverbTypeBtn, &chorusTypeBtn, &mfxABtn, &mfxBBtn, &mfxCBtn })
    {
        b->addListener(this);
        mixerPage.addAndMakeVisible(*b);
    }
    for (auto* b : { &mfxAOn, &mfxBOn, &mfxCOn })
    {
        b->setClickingTogglesState(true);
        mixerPage.addAndMakeVisible(*b);
        b->lockedFn = lockFn; b->toggleFn = togFn;
    }
    mfxAOnA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "mfxAOn", mfxAOn);
    mfxBOnA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "mfxBOn", mfxBOn);
    mfxCOnA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "mfxCOn", mfxCOn);
    const juce::String mfxTip = "Multi FX dumps a large SysEx block per tweak and can stall the USB queue for a long time. Default is OFF. Not recommended while MIDI is playing.";
    mfxABtn.setTooltip(mfxTip);
    mfxBBtn.setTooltip(mfxTip);
    mfxCBtn.setTooltip(mfxTip);
    mfxAOn.setTooltip(mfxTip);
    mfxBOn.setTooltip(mfxTip);
    mfxCOn.setTooltip(mfxTip);
    mfxWarn.setText("Multi FX: leave OFF unless you need it. Each change floods the USB queue.",
                    juce::dontSendNotification);
    mfxWarn.setJustificationType(juce::Justification::centredLeft);
    mfxWarn.setFont(juce::FontOptions(11.0f));
    mixerPage.addAndMakeVisible(mfxWarn);

    auto addFxK = [this, &lockFn, &togFn](sd80lock::Slider& s, juce::Label& l, const char* n)
    {
        styleKnob(s, n);
        l.setText(n, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::FontOptions(10.0f));
        mixerPage.addAndMakeVisible(s);
        mixerPage.addAndMakeVisible(l);
        s.lockedFn = lockFn; s.toggleFn = togFn;
    };
    addFxK(reverbTime, revTimeL, "Rev Time");
    addFxK(chorusRate, choRateL, "Cho Rate");
    addFxK(chorusDepth, choDepthL, "Cho Depth");
    addFxK(chorusFb, choFbL, "Cho Fb");
    revTimeA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, "reverbTime", reverbTime);
    choRateA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, "chorusRate", chorusRate);
    choDepthA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, "chorusDepth", chorusDepth);
    choFbA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, "chorusFeedback", chorusFb);
    wireLock(reverbTime, "reverbTime");
    wireLock(chorusRate, "chorusRate");
    wireLock(chorusDepth, "chorusDepth");
    wireLock(chorusFb, "chorusFeedback");
    wireLock(mfxAOn, "mfxAOn");
    wireLock(mfxBOn, "mfxBOn");
    wireLock(mfxCOn, "mfxCOn");

    for (int slot = 0; slot < 3; ++slot)
        for (int k = 0; k < 4; ++k)
        {
            auto& s = mfxKnobs[slot][k];
            auto& l = mfxKnobL[slot][k];
            styleKnob(s, "P");
            l.setText("P" + juce::String(k + 1), juce::dontSendNotification);
            l.setJustificationType(juce::Justification::centred);
            l.setFont(juce::FontOptions(10.0f));
            mixerPage.addAndMakeVisible(s);
            mixerPage.addAndMakeVisible(l);
            s.lockedFn = lockFn; s.toggleFn = togFn;
            const auto id = ModernEdirolSd80Processor::mfxParamId(slot, k);
            mfxKnobA[slot][k] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                proc.apvts, id, s);
            wireLock(s, id);
            s.setTooltip(mfxTip);
        }
    resetFxBtn.addListener(this);
    mixerPage.addAndMakeVisible(resetFxBtn);

    playerPage.addAndMakeVisible(deck);
    playerHelp.setText("The cassette plays SMF out Part A USB. Mute/Solo on the mixer silence channels. Send setup only if you want this file to rewrite patches.",
                       juce::dontSendNotification);
    playerHelp.setJustificationType(juce::Justification::centred);
    playerPage.addAndMakeVisible(playerHelp);
    playerWarn.setText("Changing many parameters while MIDI is playing can cause volume spikes on this 2002 USB module.",
                       juce::dontSendNotification);
    playerWarn.setJustificationType(juce::Justification::centred);
    playerWarn.setFont(juce::FontOptions(11.0f));
    playerPage.addAndMakeVisible(playerWarn);
    deck.onPlay = [this] { proc.player.play(); updatePlayerUi(); };
    deck.onPause = [this] { proc.player.pause(); updatePlayerUi(); };
    deck.onStop = [this] { proc.player.stop(); updatePlayerUi(); };
    deck.onLoad = [this] { loadPlayerFile(); };
    deck.onLoop = [this](bool v) { proc.player.setLooping(v); };
    deck.loop.setTooltip("Repeat the cassette when it ends");
    deck.onApplySetup = [this]
    {
        auto f = proc.player.getFile();
        if (f.existsAsFile())
        {
            proc.applyMidiFile(f);
            refreshStripLabels();
            bindSelectedPart();
        }
    };

    demoTitle.setText("INTERNAL DEMOS", juce::dontSendNotification);
    demoTitle.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    demoPage.addAndMakeVisible(demoTitle);
    demoHelp.setText("Buttons fire immediately on Part A/B USB (not queued): Native SysEx, Song Select, MIDI Start and MMC Play. Stop sends MIDI Stop + All Notes Off.\n\nThe Owner's Manual (p.13) starts demos from the front-panel DEMO key. If you hear nothing, USB is closed (pick Part A/B in OPTIONS) or this firmware only honours that key.\n\nStop a demo before you SYNC HARDWARE.",
                     juce::dontSendNotification);
    demoHelp.setJustificationType(juce::Justification::topLeft);
    demoPage.addAndMakeVisible(demoHelp);
    for (auto* b : { &demo1, &demo2, &demo3, &demoStop })
    {
        b->addListener(this);
        demoPage.addAndMakeVisible(*b);
    }

    auto& oi = optionsInner;
    optionsTitle.setText("OPTIONS", juce::dontSendNotification);
    optionsTitle.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    oi.addAndMakeVisible(optionsTitle);

    donateBtn.addListener(this);
    oi.addAndMakeVisible(donateBtn);
    creditsBody.setText("Modern Edirol SD-80  v1.4.1  |  JUCE 9.0.1\nFreeware MIDI controller by Crimson Redstone.\nUnofficial editor for the Edirol / Roland Studio Canvas SD-80.\nRight-click any fader, knob, toggle, menu or strip name to lock it.",
                        juce::dontSendNotification);
    creditsBody.setFont(juce::FontOptions(13.0f));
    oi.addAndMakeVisible(creditsBody);

    audioTitle.setText("AUDIO", juce::dontSendNotification);
    audioTitle.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    oi.addAndMakeVisible(audioTitle);
    hostAudioBanner.setText("Audio Settings Controlled by Host", juce::dontSendNotification);
    hostAudioBanner.setJustificationType(juce::Justification::centred);
    hostAudioBanner.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    oi.addAndMakeVisible(hostAudioBanner);
#if JucePlugin_Build_Standalone
    ensureStandaloneAudio();
#else
    hostAudioBanner.setVisible(true);
    hostDriverL.setText("Driver", juce::dontSendNotification);
    hostDeviceL.setText("Output device", juce::dontSendNotification);
    hostSrL.setText("Sample rate", juce::dontSendNotification);
    hostBufL.setText("Audio buffer", juce::dontSendNotification);
    hostDriverBox.addItem("Controlled by host", 1);
    hostDeviceBox.addItem("Controlled by host", 1);
    hostSrBox.addItem("Controlled by host", 1);
    hostBufBox.addItem("Controlled by host", 1);
    hostDriverBox.setSelectedId(1, juce::dontSendNotification);
    hostDeviceBox.setSelectedId(1, juce::dontSendNotification);
    hostSrBox.setSelectedId(1, juce::dontSendNotification);
    hostBufBox.setSelectedId(1, juce::dontSendNotification);
    for (auto* c : { &hostDriverBox, &hostDeviceBox, &hostSrBox, &hostBufBox })
        c->setEnabled(false);
    hostAudioDummy.addAndMakeVisible(hostDriverL);
    hostAudioDummy.addAndMakeVisible(hostDeviceL);
    hostAudioDummy.addAndMakeVisible(hostSrL);
    hostAudioDummy.addAndMakeVisible(hostBufL);
    hostAudioDummy.addAndMakeVisible(hostDriverBox);
    hostAudioDummy.addAndMakeVisible(hostDeviceBox);
    hostAudioDummy.addAndMakeVisible(hostSrBox);
    hostAudioDummy.addAndMakeVisible(hostBufBox);
    hostAudioDummy.setEnabled(false);
    oi.addAndMakeVisible(hostAudioDummy);
#endif

    portsTitle.setText("SD-80 USB PORTS  /  MODE  /  THROTTLE", juce::dontSendNotification);
    portsTitle.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    oi.addAndMakeVisible(portsTitle);
    portAL.setFont(juce::FontOptions(11.0f));
    portBL.setFont(juce::FontOptions(11.0f));
    oi.addAndMakeVisible(portAL);
    oi.addAndMakeVisible(portBL);
    oi.addAndMakeVisible(outABox);
    oi.addAndMakeVisible(outBBox);
    oi.addAndMakeVisible(modeBox);
    oi.addAndMakeVisible(throttleBox);
    oi.addAndMakeVisible(hostMirrorA);
    oi.addAndMakeVisible(hostMirrorB);
    midiNote.setText("Pick Part A USB and Part B USB so they match the two EDIROL SD-80 ports (Windows often lists them as EDIROL SD-80 and MIDIIN2 / MIDIOUT2). Live play, mute/solo and the cassette all go through these two sockets. The plugin auto-selects them when it sees the module.",
                     juce::dontSendNotification);
    oi.addAndMakeVisible(midiNote);

    hostRouteL.setText("Live MIDI destination (keyboard follows SEL by default)", juce::dontSendNotification);
    oi.addAndMakeVisible(hostRouteL);
    hostRouteBox.addItem("Follow SEL", 1);
    hostRouteBox.addItem("Part A (as played)", 2);
    hostRouteBox.addItem("Part B (as played)", 3);
    hostRouteAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "hostRoute", hostRouteBox);
    oi.addAndMakeVisible(hostRouteBox);

    masterVolL.setText("Module volume (SysEx master — does not lock the physical knob)", juce::dontSendNotification);
    oi.addAndMakeVisible(masterVolL);
    masterVol.setSliderStyle(juce::Slider::LinearHorizontal);
    masterVol.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 18);
    masterVolAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "masterVol", masterVol);
    wireLock(masterVol, "masterVol");
    oi.addAndMakeVisible(masterVol);

    audioMidiNote.setText("Standalone: enable your MIDI keyboard under MIDI Input above. SD-80 USB outputs stay as Part A / Part B — do not also enable those as host MIDI outs or you will double-drive the module.",
                          juce::dontSendNotification);
    oi.addAndMakeVisible(audioMidiNote);

    pullHardwareBtn.addListener(this);
    oi.addAndMakeVisible(pullHardwareBtn);

    skinTitle.setText("SKINS  (remembered)", juce::dontSendNotification);
    oi.addAndMakeVisible(skinTitle);
    for (int i = 0; i < kNumSkins; ++i)
    {
        skinButtons[(size_t) i].setButtonText(kSkins[i].name);
        skinButtons[(size_t) i].setClickingTogglesState(true);
        skinButtons[(size_t) i].setRadioGroupId(4);
        skinButtons[(size_t) i].addListener(this);
        oi.addAndMakeVisible(skinButtons[(size_t) i]);
    }

    shortcutsTitle.setText("SHORTCUTS", juce::dontSendNotification);
    shortcutsTitle.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    oi.addAndMakeVisible(shortcutsTitle);
    shortcutsBody.setText(
        "Right-click any fader, knob, toggle, menu or strip name  -  lock / unlock (survives patch, MIDI import, presets)\n"
        "Shift + click SEL  -  add or remove that part from the live-play selection (multi-select)\n"
        "Click SEL  -  exclusive select that part for Deep Edit and live MIDI\n"
        "Shift + click MUTE  -  mute all parts, or unmute all if everything is already muted\n"
        "Shift + click SOLO  -  unsolo all parts\n"
        "MUTE ALL / UNMUTE ALL / UNSOLO ALL  -  mixer toolbar\n"
        "When in doubt, press SYNC HARDWARE. The SD-80 is a 2002 USB module and drops messages if you dump too fast.",
        juce::dontSendNotification);
    oi.addAndMakeVisible(shortcutsBody);

    emergencyBtn.addListener(this);
    oi.addAndMakeVisible(emergencyBtn);

    hostAAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "hostMirrorA", hostMirrorA);
    hostBAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "hostMirrorB", hostMirrorB);
    hostMirrorA.lockedFn = lockFn; hostMirrorA.toggleFn = togFn;
    hostMirrorB.lockedFn = lockFn; hostMirrorB.toggleFn = togFn;
    wireLock(hostMirrorA, "hostMirrorA");
    wireLock(hostMirrorB, "hostMirrorB");

    rebuildDeviceLists();
    strips[0].select.setToggleState(true, juce::dontSendNotification);
    applySkin();
    bindSelectedPart();
    refreshStripLabels();
    setTab(Tab::Mixer);
    startTimerHz(12);

    proc.onImportFinished = [this]
    {
        refreshStripLabels();
        bindSelectedPart();
        dropHint.setText(proc.lastImportSummary, juce::dontSendNotification);
    };
    proc.onSkinChanged = [this] { applySkin(); };
}

ModernEdirolSd80Editor::~ModernEdirolSd80Editor()
{
    proc.onImportFinished = nullptr;
    proc.onSkinChanged = nullptr;
    categoryList.setModel(nullptr);
    patchList.setModel(nullptr);
    setLookAndFeel(nullptr);
}

void ModernEdirolSd80Editor::wireLock(sd80lock::Slider& s, const juce::String& id)
{
    s.paramId = id;
    s.lockColour = lnf.amber;
}
void ModernEdirolSd80Editor::wireLock(sd80lock::TextButton& b, const juce::String& id)
{
    b.paramId = id;
    b.lockColour = lnf.amber;
}
void ModernEdirolSd80Editor::wireLock(sd80lock::ComboBox& b, const juce::String& id)
{
    b.paramId = id;
    b.lockColour = lnf.amber;
}
void ModernEdirolSd80Editor::wireLock(sd80lock::ToggleButton& b, const juce::String& id)
{
    b.paramId = id;
    b.lockColour = lnf.amber;
}
void ModernEdirolSd80Editor::wireLock(sd80lock::Label& b, const juce::String& id)
{
    b.paramId = id;
    b.lockColour = lnf.amber;
}

void ModernEdirolSd80Editor::ensureStandaloneAudio()
{
#if JucePlugin_Build_Standalone
    if (audioSelector != nullptr)
        return;
    auto* holder = juce::StandalonePluginHolder::getInstance();
    if (holder == nullptr)
        return;
    audioSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        holder->deviceManager, 0, 2, 0, 2, true, false, true, false);
    optionsInner.addAndMakeVisible(*audioSelector);
    hostAudioBanner.setVisible(false);
    hostAudioDummy.setVisible(false);
    resized();
#endif
}

void ModernEdirolSd80Editor::toggleLock(const juce::String& id)
{
    if (id.isEmpty())
        return;
    if (id.contains("_pc") || id.contains("_msb") || id.contains("_lsb"))
    {
        const auto prefix = id.upToLastOccurrenceOf("_", false, false);
        const bool next = ! proc.isLocked(prefix + "_pc");
        proc.setLocked(prefix + "_msb", next);
        proc.setLocked(prefix + "_lsb", next);
        proc.setLocked(prefix + "_pc", next);
        repaint();
        return;
    }
    proc.setLocked(id, ! proc.isLocked(id));
    repaint();
}

void ModernEdirolSd80Editor::applySkin()
{
    lnf.applySkin(proc.getSkinIndex());
    title.setColour(juce::Label::textColourId, lnf.amber);
    subtitle.setColour(juce::Label::textColourId, lnf.muted);
    creditsLabel.setColour(juce::Label::textColourId, lnf.teal);
    queueLabel.setColour(juce::Label::textColourId, lnf.teal);
    dropHint.setColour(juce::Label::textColourId, lnf.muted);
    inspectorTitle.setColour(juce::Label::textColourId, lnf.teal);
    fxTitle.setColour(juce::Label::textColourId, lnf.teal);
    optionsTitle.setColour(juce::Label::textColourId, lnf.amber);
    skinTitle.setColour(juce::Label::textColourId, lnf.muted);
    playerHelp.setColour(juce::Label::textColourId, lnf.muted);
    playerWarn.setColour(juce::Label::textColourId, lnf.muteRed);
    demoTitle.setColour(juce::Label::textColourId, lnf.amber);
    demoHelp.setColour(juce::Label::textColourId, lnf.muted);
    mfxWarn.setColour(juce::Label::textColourId, lnf.muteRed);
    masterVolL.setColour(juce::Label::textColourId, lnf.muted);
    hostRouteL.setColour(juce::Label::textColourId, lnf.muted);
    audioMidiNote.setColour(juce::Label::textColourId, lnf.muted);
    creditsBody.setColour(juce::Label::textColourId, lnf.text);
    midiNote.setColour(juce::Label::textColourId, lnf.muted);
    audioTitle.setColour(juce::Label::textColourId, lnf.teal);
    portsTitle.setColour(juce::Label::textColourId, lnf.teal);
    portAL.setColour(juce::Label::textColourId, lnf.muted);
    portBL.setColour(juce::Label::textColourId, lnf.muted);
    shortcutsTitle.setColour(juce::Label::textColourId, lnf.teal);
    shortcutsBody.setColour(juce::Label::textColourId, lnf.text);
    hostAudioBanner.setColour(juce::Label::textColourId, lnf.amber);
    hostDriverL.setColour(juce::Label::textColourId, lnf.muted);
    hostDeviceL.setColour(juce::Label::textColourId, lnf.muted);
    hostSrL.setColour(juce::Label::textColourId, lnf.muted);
    hostBufL.setColour(juce::Label::textColourId, lnf.muted);
    donateBtn.setColour(juce::TextButton::buttonColourId, lnf.amber);
    donateBtn.setColour(juce::TextButton::textColourOffId, lnf.bg);
    donateBtn.setColour(juce::TextButton::textColourOnId, lnf.bg);
    patchSearch.setTextToShowWhenEmpty("Search patches", lnf.muted);

    for (auto& st : strips)
    {
        st.mute.setColour(juce::TextButton::buttonOnColourId, lnf.muteRed);
        st.mute.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        st.solo.setColour(juce::TextButton::buttonOnColourId, lnf.soloGreen);
        st.solo.setColour(juce::TextButton::textColourOnId, lnf.bg);
        st.select.setColour(juce::TextButton::buttonOnColourId, lnf.amber);
        st.select.setColour(juce::TextButton::textColourOnId, lnf.bg);
        st.ch.setColour(juce::Label::textColourId, lnf.muted);
        st.name.setColour(juce::Label::textColourId, lnf.text);
        st.vol.lockColour = lnf.amber;
        st.pan.lockColour = lnf.amber;
        st.mute.lockColour = lnf.amber;
        st.solo.lockColour = lnf.amber;
        st.name.lockColour = lnf.amber;
    }
    partAButton.setColour(juce::TextButton::buttonOnColourId, lnf.teal);
    partBButton.setColour(juce::TextButton::buttonOnColourId, lnf.teal);
    tabMixer.setColour(juce::TextButton::buttonOnColourId, lnf.amber);
    tabPlayer.setColour(juce::TextButton::buttonOnColourId, lnf.amber);
    tabDemos.setColour(juce::TextButton::buttonOnColourId, lnf.amber);
    tabOptions.setColour(juce::TextButton::buttonOnColourId, lnf.amber);
    deck.loop.setColour(juce::TextButton::buttonOnColourId, lnf.teal);
    demo1.setColour(juce::TextButton::buttonColourId, lnf.surface2);
    demo2.setColour(juce::TextButton::buttonColourId, lnf.surface2);
    demo3.setColour(juce::TextButton::buttonColourId, lnf.surface2);
    demoStop.setColour(juce::TextButton::buttonColourId, lnf.muteRed);
    demoStop.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    mfxAOn.setColour(juce::TextButton::buttonOnColourId, lnf.teal);
    mfxBOn.setColour(juce::TextButton::buttonOnColourId, lnf.teal);
    mfxCOn.setColour(juce::TextButton::buttonOnColourId, lnf.teal);

    catModel.text = lnf.text;
    catModel.muted = lnf.muted;
    catModel.selBg = lnf.teal.darker(0.35f);
    patchModel.text = lnf.text;
    patchModel.muted = lnf.muted;
    patchModel.selBg = lnf.teal.darker(0.35f);
    patchModel.selFg = lnf.text;

    for (int i = 0; i < kNumSkins; ++i)
    {
        const auto& s = kSkins[i];
        auto& b = skinButtons[(size_t) i];
        b.setColour(juce::TextButton::buttonColourId, juce::Colour(s.surface));
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(s.accent));
        b.setColour(juce::TextButton::textColourOffId, juce::Colour(s.text));
        b.setColour(juce::TextButton::textColourOnId, juce::Colour(s.bg));
        b.setToggleState(i == proc.getSkinIndex(), juce::dontSendNotification);
    }
    deck.setPalette(kSkins[proc.getSkinIndex()]);
    sendLookAndFeelChange();
    repaint();
}

void ModernEdirolSd80Editor::setTab(Tab t)
{
    tab = t;
    tabMixer.setToggleState(t == Tab::Mixer, juce::dontSendNotification);
    tabPlayer.setToggleState(t == Tab::Player, juce::dontSendNotification);
    tabDemos.setToggleState(t == Tab::Demos, juce::dontSendNotification);
    tabOptions.setToggleState(t == Tab::Options, juce::dontSendNotification);
    mixerPage.setVisible(t == Tab::Mixer);
    playerPage.setVisible(t == Tab::Player);
    demoPage.setVisible(t == Tab::Demos);
    optionsPage.setVisible(t == Tab::Options);
    if (t == Tab::Mixer)
        dropHint.setText("Drop a .mid on the mixer to auto-assign banks, programs and mix.", juce::dontSendNotification);
    else if (t == Tab::Player)
        dropHint.setText("Drop a .mid on the cassette to load a tape. Play does not rewrite mixer patches.", juce::dontSendNotification);
    else if (t == Tab::Demos)
        dropHint.setText("Internal SD-80 sequencer demos. Stop a demo before you SYNC HARDWARE.", juce::dontSendNotification);
    else
        dropHint.setText("Skins and locks are stored in Crimson Redstone app settings.", juce::dontSendNotification);
}

void ModernEdirolSd80Editor::paint(juce::Graphics& g)
{
    g.fillAll(lnf.bg);
    auto top = headerArea();
    g.setColour(lnf.surface);
    g.fillRect(top);
    g.setColour(lnf.amber);
    g.fillRect(0, top.getBottom(), getWidth(), 2);

    if (tab == Tab::Mixer)
    {
        auto r = mixerPage.getBounds();
        const int side = 380;
        auto inspect = juce::Rectangle<int>(r.getRight() - side, r.getY(), side, r.getHeight()).reduced(8);
        auto fx = juce::Rectangle<int>(r.getX() + 8, r.getBottom() - 252, r.getWidth() - side - 16, 242);
        g.setColour(lnf.surface);
        g.fillRoundedRectangle(inspect.toFloat(), 8.0f);
        g.fillRoundedRectangle(fx.toFloat(), 8.0f);
        g.setColour(lnf.border);
        g.drawRoundedRectangle(inspect.toFloat(), 8.0f, 1.0f);
        g.drawRoundedRectangle(fx.toFloat(), 8.0f, 1.0f);
    }

    if (dragging)
    {
        g.setColour(lnf.teal.withAlpha(0.18f));
        g.fillRect(getLocalBounds());
        g.setColour(lnf.teal);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(12.0f), 12.0f, 2.0f);
        g.setFont(juce::FontOptions(26.0f).withStyle("Bold"));
        const auto msg = (tab == Tab::Player)
                             ? "Drop MIDI file onto the cassette"
                             : "Drop MIDI file to configure the SD-80";
        g.drawText(msg, getLocalBounds(), juce::Justification::centred);
    }
}

void ModernEdirolSd80Editor::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop(72);
    auto t1 = top.removeFromTop(40).reduced(16, 6);
    title.setBounds(t1.removeFromLeft(340).removeFromTop(24));
    subtitle.setBounds(title.getX(), title.getBottom() - 2, 420, 16);
    creditsLabel.setBounds(title.getRight() + 8, title.getY() + 4, 180, 18);

    auto tools = t1;
    queueLabel.setBounds(tools.removeFromRight(200).removeFromTop(18));
    syncButton.setBounds(tools.removeFromRight(140).reduced(2));
    loadPreset.setBounds(tools.removeFromRight(110).reduced(2));
    savePreset.setBounds(tools.removeFromRight(110).reduced(2));

    auto t2 = top.reduced(16, 0);
    partAButton.setBounds(t2.removeFromLeft(118).reduced(2));
    partBButton.setBounds(t2.removeFromLeft(128).reduced(2));
    t2.removeFromLeft(8);
    tabMixer.setBounds(t2.removeFromLeft(90).reduced(2));
    tabPlayer.setBounds(t2.removeFromLeft(90).reduced(2));
    tabDemos.setBounds(t2.removeFromLeft(90).reduced(2));
    tabOptions.setBounds(t2.removeFromLeft(100).reduced(2));

    dropHint.setBounds(getLocalBounds().removeFromBottom(24).reduced(16, 2));

    auto content = contentArea();
    mixerPage.setBounds(content);
    playerPage.setBounds(content);
    demoPage.setBounds(content);
    optionsPage.setBounds(content);
    optionsView.setBounds(optionsPage.getLocalBounds());

    {
        auto mp = mixerPage.getLocalBounds();
        const int side = 380;
        auto inspect = mp.removeFromRight(side).reduced(16, 12);
        auto fx = mp.removeFromBottom(252).reduced(10, 8);
        auto mixBar = mp.removeFromTop(28).reduced(8, 2);
        muteAllBtn.setBounds(mixBar.removeFromLeft(100).reduced(2));
        unmuteAllBtn.setBounds(mixBar.removeFromLeft(110).reduced(2));
        unsoloAllBtn.setBounds(mixBar.removeFromLeft(110).reduced(2));
        auto mix = mp.reduced(8, 4);

        const int stripW = juce::jmax(1, mix.getWidth() / 16);
        for (int i = 0; i < 16; ++i)
        {
            auto& st = strips[(size_t) i];
            auto s = juce::Rectangle<int>(mix.getX() + i * stripW, mix.getY(), stripW - 4, mix.getHeight());
            st.ch.setBounds(s.removeFromTop(16));
            st.name.setBounds(s.removeFromTop(34));
            st.select.setBounds(s.removeFromTop(22).reduced(6, 1));
            auto ms = s.removeFromTop(22);
            st.mute.setBounds(ms.removeFromLeft(ms.getWidth() / 2).reduced(3, 1));
            st.solo.setBounds(ms.reduced(3, 1));
            st.pan.setBounds(s.removeFromBottom(18).reduced(4, 0));
            st.vol.setBounds(s.reduced(8, 4));
        }

        inspectorTitle.setBounds(inspect.removeFromTop(18));
        patchNameLabel.setBounds(inspect.removeFromTop(24));
        auto pick = inspect.removeFromTop(26);
        mapBox.setBounds(pick.removeFromLeft(150).reduced(2));
        drumToggle.setBounds(pick.reduced(2));
        patchSearch.setBounds(inspect.removeFromTop(24).reduced(0, 2));
        auto lists = inspect.removeFromTop(160);
        auto cats = lists.removeFromLeft(lists.getWidth() / 2);
        catTitle.setBounds(cats.removeFromTop(16));
        categoryList.setBounds(cats.reduced(1));
        patchTitle.setBounds(lists.removeFromTop(16));
        patchList.setBounds(lists.reduced(1));

        auto route = inspect.removeFromTop(26);
        mfxSel.setBounds(route.removeFromLeft(route.getWidth() / 2).reduced(2));
        outAsg.setBounds(route.reduced(2));
        porta.setBounds(inspect.removeFromTop(22));

        sd80lock::Slider* knobs[] = { &cut, &res, &atk, &dec, &rel, &vr, &vd, &vdl, &expr, &rev, &cho, &mfxSend, &portaT };
        juce::Label* labs[] = { &cutL, &resL, &atkL, &decL, &relL, &vrL, &vdL, &vdlL, &exprL, &revL, &choL, &mfxL, &portaL };
        const int cols = 5;
        const int cellW = juce::jmax(1, inspect.getWidth() / cols);
        const int cellH = juce::jmin(86, juce::jmax(70, inspect.getHeight() / 3));
        for (int i = 0; i < 13; ++i)
        {
            const int col = i % cols;
            const int rowI = i / cols;
            auto cell = juce::Rectangle<int>(inspect.getX() + col * cellW,
                                             inspect.getY() + rowI * cellH, cellW, cellH);
            labs[i]->setBounds(cell.removeFromTop(14));
            knobs[i]->setBounds(cell.reduced(4, 0));
        }

        auto fxHead = fx.removeFromTop(22);
        fxTitle.setBounds(fxHead.removeFromLeft(fxHead.getWidth() - 130));
        resetFxBtn.setBounds(fxHead.reduced(2));

        auto warnRow = fx.removeFromTop(16);
        mfxWarn.setBounds(warnRow.withTrimmedLeft(warnRow.getWidth() * 2 / 5).reduced(6, 0));

        const int colW = fx.getWidth() / 5;
        auto revCol = fx.removeFromLeft(colW).reduced(4, 2);
        auto choCol = fx.removeFromLeft(colW).reduced(4, 2);
        juce::Rectangle<int> mfxCols[3] = { fx.removeFromLeft(colW).reduced(4, 2),
                                            fx.removeFromLeft(colW).reduced(4, 2),
                                            fx.reduced(4, 2) };

        reverbTypeBtn.setBounds(revCol.removeFromTop(26).reduced(1));
        revTimeL.setBounds(revCol.removeFromTop(14));
        reverbTime.setBounds(revCol.reduced(8, 0));

        chorusTypeBtn.setBounds(choCol.removeFromTop(26).reduced(1));
        auto choK = choCol;
        const int ck = choK.getWidth() / 3;
        auto placeCho = [&](sd80lock::Slider& s, juce::Label& l)
        {
            auto c = choK.removeFromLeft(ck);
            l.setBounds(c.removeFromTop(14));
            s.setBounds(c.reduced(4, 0));
        };
        placeCho(chorusRate, choRateL);
        placeCho(chorusDepth, choDepthL);
        placeCho(chorusFb, choFbL);

        juce::TextButton* mfxBtns[3] = { &mfxABtn, &mfxBBtn, &mfxCBtn };
        sd80lock::TextButton* mfxOns[3] = { &mfxAOn, &mfxBOn, &mfxCOn };
        for (int slot = 0; slot < 3; ++slot)
        {
            auto col = mfxCols[slot];
            auto row = col.removeFromTop(26);
            mfxOns[slot]->setBounds(row.removeFromRight(28).reduced(1));
            mfxBtns[slot]->setBounds(row.reduced(1));
            const int pk = col.getWidth() / 4;
            for (int k = 0; k < 4; ++k)
            {
                auto c = col.removeFromLeft(pk);
                mfxKnobL[slot][k].setBounds(c.removeFromTop(14));
                mfxKnobs[slot][k].setBounds(c.reduced(2, 0));
            }
        }
    }

    {
        auto pp = playerPage.getLocalBounds().reduced(40);
        playerWarn.setBounds(pp.removeFromBottom(22));
        playerHelp.setBounds(pp.removeFromBottom(36));
        deck.setBounds(pp.removeFromTop(juce::jmin(360, pp.getHeight())));
    }

    {
        auto dp = demoPage.getLocalBounds().reduced(48, 36);
        demoTitle.setBounds(dp.removeFromTop(28));
        dp.removeFromTop(12);
        auto row = dp.removeFromTop(52);
        const int bw = juce::jmax(1, row.getWidth() / 4);
        demo1.setBounds(row.removeFromLeft(bw).reduced(6, 6));
        demo2.setBounds(row.removeFromLeft(bw).reduced(6, 6));
        demo3.setBounds(row.removeFromLeft(bw).reduced(6, 6));
        demoStop.setBounds(row.reduced(6, 6));
        dp.removeFromTop(18);
        demoHelp.setBounds(dp.removeFromTop(180));
    }

    {
        const int innerW = juce::jmax(700, optionsView.getWidth() - 24);
        optionsInner.setSize(innerW, 1680);
        auto op = optionsInner.getLocalBounds().reduced(24, 16);
        optionsTitle.setBounds(op.removeFromTop(28));
        donateBtn.setBounds(op.removeFromTop(40).reduced(0, 4));
        creditsBody.setBounds(op.removeFromTop(80));
        audioTitle.setBounds(op.removeFromTop(22));
        if (audioSelector != nullptr)
        {
            hostAudioBanner.setBounds({});
            hostAudioDummy.setBounds({});
            audioSelector->setBounds(op.removeFromTop(460));
        }
        else
        {
            hostAudioBanner.setBounds(op.removeFromTop(28));
            auto dummy = op.removeFromTop(140);
            hostAudioDummy.setBounds(dummy);
            auto d = hostAudioDummy.getLocalBounds().reduced(4);
            auto row1 = d.removeFromTop(60);
            auto r1a = row1.removeFromLeft(row1.getWidth() / 2).reduced(4);
            auto r1b = row1.reduced(4);
            hostDriverL.setBounds(r1a.removeFromTop(16));
            hostDriverBox.setBounds(r1a.removeFromTop(26));
            hostDeviceL.setBounds(r1b.removeFromTop(16));
            hostDeviceBox.setBounds(r1b.removeFromTop(26));
            auto row2 = d.removeFromTop(60);
            auto r2a = row2.removeFromLeft(row2.getWidth() / 2).reduced(4);
            auto r2b = row2.reduced(4);
            hostSrL.setBounds(r2a.removeFromTop(16));
            hostSrBox.setBounds(r2a.removeFromTop(26));
            hostBufL.setBounds(r2b.removeFromTop(16));
            hostBufBox.setBounds(r2b.removeFromTop(26));
        }
        portsTitle.setBounds(op.removeFromTop(22));
        auto portLabs = op.removeFromTop(16);
        portAL.setBounds(portLabs.removeFromLeft(portLabs.getWidth() / 2).reduced(2, 0));
        portBL.setBounds(portLabs.reduced(2, 0));
        auto ports = op.removeFromTop(32);
        outABox.setBounds(ports.removeFromLeft(ports.getWidth() / 2).reduced(2));
        outBBox.setBounds(ports.reduced(2));
        auto modeRow = op.removeFromTop(32);
        modeBox.setBounds(modeRow.removeFromLeft(160).reduced(2));
        throttleBox.setBounds(modeRow.removeFromLeft(100).reduced(2));
        auto mir = op.removeFromTop(28);
        hostMirrorA.setBounds(mir.removeFromLeft(mir.getWidth() / 2));
        hostMirrorB.setBounds(mir);
        midiNote.setBounds(op.removeFromTop(56));
        hostRouteL.setBounds(op.removeFromTop(18));
        hostRouteBox.setBounds(op.removeFromTop(28).removeFromLeft(280).reduced(0, 2));
        masterVolL.setBounds(op.removeFromTop(18));
        masterVol.setBounds(op.removeFromTop(28));
        audioMidiNote.setBounds(op.removeFromTop(44));
        pullHardwareBtn.setBounds(op.removeFromTop(36).removeFromLeft(220));
        op.removeFromTop(8);
        skinTitle.setBounds(op.removeFromTop(22));
        auto skinRow1 = op.removeFromTop(40);
        auto skinRow2 = op.removeFromTop(40);
        const int n1 = 5;
        const int sw1 = juce::jmax(1, skinRow1.getWidth() / n1);
        const int n2 = kNumSkins - n1;
        const int sw2 = juce::jmax(1, skinRow2.getWidth() / juce::jmax(1, n2));
        for (int i = 0; i < kNumSkins; ++i)
        {
            auto& row = (i < n1) ? skinRow1 : skinRow2;
            const int sw = (i < n1) ? sw1 : sw2;
            skinButtons[(size_t) i].setBounds(row.removeFromLeft(sw).reduced(3, 2));
        }
        shortcutsTitle.setBounds(op.removeFromTop(22));
        shortcutsBody.setBounds(op.removeFromTop(140));
        emergencyBtn.setBounds(op.removeFromTop(36).removeFromLeft(280));
    }
}

juce::Rectangle<int> ModernEdirolSd80Editor::headerArea() const
{
    return { 0, 0, getWidth(), 72 };
}

juce::Rectangle<int> ModernEdirolSd80Editor::contentArea() const
{
    return { 0, 74, getWidth(), getHeight() - 74 - 24 };
}

void ModernEdirolSd80Editor::rebuildDeviceLists()
{
    auto fill = [](juce::ComboBox& b, const juce::StringArray& names, const juce::String& current,
                   const juce::String& noneLabel)
    {
        b.clear(juce::dontSendNotification);
        int sel = 1;
        for (int i = 0; i < names.size(); ++i)
        {
            b.addItem(i == 0 ? noneLabel : names[i], i + 1);
            if (i > 0 && names[i] == current)
                sel = i + 1;
        }
        b.setSelectedId(sel, juce::dontSendNotification);
    };
    auto outs = proc.midiOutputNames();
    fill(outABox, outs, proc.getMidiOutputAName(), "Part A USB: (none)");
    fill(outBBox, outs, proc.getMidiOutputBName(), "Part B USB: (none)");
}

void ModernEdirolSd80Editor::refreshStripLabels()
{
    const int group = proc.getVisibleGroup();
    for (int i = 0; i < 16; ++i)
    {
        const int part = group * 16 + i;
        auto& st = strips[(size_t) i];
        st.ch.setText((group == 0 ? "A" : "B") + juce::String(i + 1), juce::dontSendNotification);
        st.name.setText(proc.getPartPatchName(part), juce::dontSendNotification);
        st.volAtt.reset();
        st.panAtt.reset();
        st.muteAtt.reset();
        st.soloAtt.reset();
        const auto volId = ModernEdirolSd80Processor::pid(part, "vol");
        const auto panId = ModernEdirolSd80Processor::pid(part, "pan");
        const auto muteId = ModernEdirolSd80Processor::pid(part, "mute");
        const auto soloId = ModernEdirolSd80Processor::pid(part, "solo");
        st.volAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, volId, st.vol);
        st.panAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, panId, st.pan);
        st.muteAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, muteId, st.mute);
        st.soloAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, soloId, st.solo);
        wireLock(st.vol, volId);
        wireLock(st.pan, panId);
        wireLock(st.mute, muteId);
        wireLock(st.solo, soloId);
        wireLock(st.name, ModernEdirolSd80Processor::pid(part, "pc"));
        st.select.setToggleState(proc.isPartSelected(part), juce::dontSendNotification);
    }
}

void ModernEdirolSd80Editor::bindSelectedPart()
{
    const int part = proc.getSelectedPart();
    lastBoundPart = part;
    patchNameLabel.setText(proc.getPartPatchName(part), juce::dontSendNotification);

    cutA.reset(); resA.reset(); atkA.reset(); decA.reset(); relA.reset();
    vrA.reset(); vdA.reset(); vdlA.reset(); exprA.reset(); revA.reset(); choA.reset();
    mfxA.reset(); portaTA.reset(); portaA.reset(); drumA.reset();

    auto bindS = [this, part](std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att,
                              sd80lock::Slider& s, const char* key)
    {
        const auto id = ModernEdirolSd80Processor::pid(part, key);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, s);
        wireLock(s, id);
    };
    bindS(cutA, cut, "cut");
    bindS(resA, res, "res");
    bindS(atkA, atk, "atk");
    bindS(decA, dec, "dec");
    bindS(relA, rel, "rel");
    bindS(vrA, vr, "vr");
    bindS(vdA, vd, "vd");
    bindS(vdlA, vdl, "vdl");
    bindS(exprA, expr, "expr");
    bindS(revA, rev, "rev");
    bindS(choA, cho, "cho");
    bindS(mfxA, mfxSend, "mfx");
    bindS(portaTA, portaT, "portaT");
    portaA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, ModernEdirolSd80Processor::pid(part, "porta"), porta);
    drumA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, ModernEdirolSd80Processor::pid(part, "drum"), drumToggle);
    wireLock(porta, ModernEdirolSd80Processor::pid(part, "porta"));
    wireLock(drumToggle, ModernEdirolSd80Processor::pid(part, "drum"));
    wireLock(mapBox, ModernEdirolSd80Processor::pid(part, "map"));
    wireLock(mfxSel, ModernEdirolSd80Processor::pid(part, "mfxSel"));
    wireLock(outAsg, ModernEdirolSd80Processor::pid(part, "outAsg"));

    const int map = (int) proc.apvts.getRawParameterValue(ModernEdirolSd80Processor::pid(part, "map"))->load();
    mapBox.setSelectedId(map + 1, juce::dontSendNotification);
    mfxSel.setSelectedId((int) proc.apvts.getRawParameterValue(ModernEdirolSd80Processor::pid(part, "mfxSel"))->load() + 1,
                         juce::dontSendNotification);
    outAsg.setSelectedId((int) proc.apvts.getRawParameterValue(ModernEdirolSd80Processor::pid(part, "outAsg"))->load() + 1,
                         juce::dontSendNotification);

    const int rt = (int) proc.apvts.getRawParameterValue("reverbType")->load();
    reverbTypeBtn.setButtonText(juce::String("Reverb: ") + gm2ReverbTypeName(rt));
    chorusTypeBtn.setButtonText(juce::String("Chorus: ") + gm2ChorusTypeName((int) proc.apvts.getRawParameterValue("chorusType")->load()));
    updateFxLabels();

    rebuildPatchBrowser();
}

void ModernEdirolSd80Editor::rebuildPatchBrowser()
{
    const int part = proc.getSelectedPart();
    const auto wantMap = static_cast<SoundMap>(juce::jlimit(0, 8, mapBox.getSelectedId() - 1));
    const bool drums = drumToggle.getToggleState();
    const auto q = patchSearch.getText().trim().toLowerCase();

    const juce::String previousCat = juce::isPositiveAndBelow(catModel.selected, catModel.names.size())
                                         ? catModel.names[catModel.selected]
                                         : "All";

    catModel.names.clear();
    catModel.names.add("All");
    bool present[(int) Category::NumCategories] {};
    for (int i = 0; i < kNumPatches; ++i)
    {
        const auto& e = kPatches[i];
        if (e.map != wantMap || e.drum != drums)
            continue;
        present[(int) e.category] = true;
    }
    for (int c = 0; c < (int) Category::NumCategories; ++c)
        if (present[c])
            catModel.names.add(categoryName((Category) c));

    int newCat = 0;
    for (int i = 0; i < catModel.names.size(); ++i)
        if (catModel.names[i] == previousCat)
            newCat = i;
    catModel.selected = newCat;

    const juce::String wantCat = catModel.names[catModel.selected];
    patchModel.items.clear();
    const auto curMsb = (std::uint8_t) proc.apvts.getRawParameterValue(ModernEdirolSd80Processor::pid(part, "msb"))->load();
    const auto curLsb = (std::uint8_t) proc.apvts.getRawParameterValue(ModernEdirolSd80Processor::pid(part, "lsb"))->load();
    const auto curPc  = (std::uint8_t) proc.apvts.getRawParameterValue(ModernEdirolSd80Processor::pid(part, "pc"))->load();
    int selectRow = -1;
    for (int i = 0; i < kNumPatches; ++i)
    {
        const auto& e = kPatches[i];
        if (e.map != wantMap || e.drum != drums)
            continue;
        if (wantCat != "All" && juce::String(categoryName(e.category)) != wantCat)
            continue;
        if (q.isNotEmpty() && ! juce::String(e.name).toLowerCase().contains(q))
            continue;
        if (e.msb == curMsb && e.lsb == curLsb && e.pc == curPc)
            selectRow = (int) patchModel.items.size();
        patchModel.items.push_back(&e);
    }

    categoryList.updateContent();
    categoryList.selectRow(catModel.selected, false);
    patchList.updateContent();
    if (selectRow >= 0)
        patchList.selectRow(selectRow, false);
}

void ModernEdirolSd80Editor::pickPatchAt(int row)
{
    if (! juce::isPositiveAndBelow(row, (int) patchModel.items.size()))
        return;
    const int part = proc.getSelectedPart();
    if (proc.isLocked(ModernEdirolSd80Processor::pid(part, "msb"))
        || proc.isLocked(ModernEdirolSd80Processor::pid(part, "pc")))
        return;
    auto* e = patchModel.items[(size_t) row];
    proc.setParamInt(ModernEdirolSd80Processor::pid(part, "msb"), e->msb);
    proc.setParamInt(ModernEdirolSd80Processor::pid(part, "lsb"), e->lsb);
    proc.setParamInt(ModernEdirolSd80Processor::pid(part, "pc"), e->pc);
    proc.enqueuePartPatch(part);
    refreshStripLabels();
    patchNameLabel.setText(e->name, juce::dontSendNotification);
}

void ModernEdirolSd80Editor::updateFxLabels()
{
    auto mfxLabel = [](int type, const char* tag)
    {
        type = juce::jlimit(0, kNumMfxTypes - 1, type);
        if (type == 0)
            return juce::String(tag);
        return juce::String(tag) + ": " + kMfxTypes[type].name;
    };
    mfxABtn.setButtonText(mfxLabel((int) proc.apvts.getRawParameterValue("mfxAType")->load(), "Multi FX A"));
    mfxBBtn.setButtonText(mfxLabel((int) proc.apvts.getRawParameterValue("mfxBType")->load(), "Multi FX B"));
    mfxCBtn.setButtonText(mfxLabel((int) proc.apvts.getRawParameterValue("mfxCType")->load(), "Multi FX C"));
    const bool thruA = (int) proc.apvts.getRawParameterValue("mfxAType")->load() == 0;
    const bool thruB = (int) proc.apvts.getRawParameterValue("mfxBType")->load() == 0;
    const bool thruC = (int) proc.apvts.getRawParameterValue("mfxCType")->load() == 0;
    for (int k = 0; k < 4; ++k)
    {
        mfxKnobs[0][k].setEnabled(! thruA);
        mfxKnobs[1][k].setEnabled(! thruB);
        mfxKnobs[2][k].setEnabled(! thruC);
    }
}

void ModernEdirolSd80Editor::showMfxMenu(int which)
{
    juce::PopupMenu root;
    for (int g = 0; g < kNumMfxGroups; ++g)
    {
        juce::PopupMenu sub;
        const char* group = kMfxGroupOrder[g];
        for (int i = 0; i < kNumMfxTypes; ++i)
        {
            if (std::strcmp(mfxGroupName(kMfxTypes[i].id), group) == 0)
                sub.addItem(i + 1,
                            juce::String(kMfxTypes[i].id).paddedLeft('0', 2) + "  " + kMfxTypes[i].name);
        }
        if (sub.getNumItems() > 0)
            root.addSubMenu(group, sub);
    }
    juce::Component* target = which == 0 ? &mfxABtn : which == 1 ? &mfxBBtn : &mfxCBtn;
    const char* ids[] = { "mfxAType", "mfxBType", "mfxCType" };
    root.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(target),
                       [this, which, ids](int result)
                       {
                           if (result <= 0)
                               return;
                           if (proc.isLocked(ids[which]))
                               return;
                           if (proc.getGeneratorMode() != GeneratorMode::Native)
                               proc.setGeneratorMode(GeneratorMode::Native);
                           proc.setParamInt(ids[which], result - 1);
                           proc.setParamInt(ModernEdirolSd80Processor::pid(proc.getSelectedPart(), "outAsg"), 0);
                           proc.enqueueMfxBlock();
                           updateFxLabels();
                           bindSelectedPart();
                       });
}

void ModernEdirolSd80Editor::showReverbMenu()
{
    juce::PopupMenu m;
    m.addItem(1, "Small Room");
    m.addItem(2, "Medium Room");
    m.addItem(3, "Large Room");
    m.addItem(4, "Medium Hall");
    m.addItem(5, "Large Hall");
    m.addItem(9, "Plate");
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&reverbTypeBtn),
                    [this](int result)
                    {
                        if (result <= 0 || proc.isLocked("reverbType"))
                            return;
                        proc.setParamInt("reverbType", result - 1);
                        reverbTypeBtn.setButtonText(juce::String("Reverb: ") + gm2ReverbTypeName(result - 1));
                    });
}

void ModernEdirolSd80Editor::showChorusMenu()
{
    juce::PopupMenu m;
    for (int i = 0; i < 6; ++i)
        m.addItem(i + 1, gm2ChorusTypeName(i));
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&chorusTypeBtn),
                    [this](int result)
                    {
                        if (result <= 0 || proc.isLocked("chorusType"))
                            return;
                        proc.setParamInt("chorusType", result - 1);
                        chorusTypeBtn.setButtonText(juce::String("Chorus: ") + gm2ChorusTypeName(result - 1));
                    });
}

void ModernEdirolSd80Editor::loadPlayerFile()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Load cassette MIDI",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.mid;*.midi");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                         [this, chooser](const juce::FileChooser& fc)
                         {
                             auto f = fc.getResult();
                             if (f != juce::File())
                             {
                                 proc.player.load(f);
                                 updatePlayerUi();
                             }
                         });
}

void ModernEdirolSd80Editor::updatePlayerUi()
{
    deck.setState(proc.player.isLoaded(), proc.player.isPlaying(), proc.player.getName(),
                  proc.player.getPosition(), proc.player.getLength());
    deck.setLooping(proc.player.isLooping());
}

void ModernEdirolSd80Editor::confirmDanger(const juce::String& titleText, const juce::String& bodyText,
                                           bool requireSure, bool showDontShow,
                                           std::function<void(bool dontShow)> onProceed)
{
    struct Panel : public juce::Component, private juce::Button::Listener
    {
        juce::Label title;
        juce::TextEditor body;
        juce::ToggleButton sure { "Yes, I am sure" };
        juce::ToggleButton dontShow { "Don't show this again" };
        juce::TextButton proceed { "Proceed" }, cancel { "Cancel" };
        std::function<void(bool)> onOk;
        bool require = true;
        bool showDont = false;

        Panel(const juce::String& t, const juce::String& b, bool req, bool ds,
              std::function<void(bool)> cb)
            : onOk(std::move(cb)), require(req), showDont(ds)
        {
            title.setText(t, juce::dontSendNotification);
            title.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
            title.setColour(juce::Label::textColourId, juce::Colours::whitesmoke);
            body.setMultiLine(true);
            body.setReadOnly(true);
            body.setScrollbarsShown(false);
            body.setCaretVisible(false);
            body.setPopupMenuEnabled(false);
            body.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
            body.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
            body.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
            body.setColour(juce::TextEditor::textColourId, juce::Colours::whitesmoke);
            body.setFont(juce::FontOptions(13.0f));
            body.setText(b, false);
            addAndMakeVisible(title);
            addAndMakeVisible(body);
            addAndMakeVisible(sure);
            addAndMakeVisible(dontShow);
            addAndMakeVisible(proceed);
            addAndMakeVisible(cancel);
            dontShow.setVisible(showDont);
            sure.setVisible(require);
            proceed.setEnabled(! require);
            sure.onClick = [this] { proceed.setEnabled(! require || sure.getToggleState()); };
            proceed.addListener(this);
            cancel.addListener(this);
            setSize(480, showDont ? 300 : 270);
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced(16);
            title.setBounds(r.removeFromTop(24));
            body.setBounds(r.removeFromTop(120));
            if (require) sure.setBounds(r.removeFromTop(24));
            if (showDont) dontShow.setBounds(r.removeFromTop(24));
            auto row = r.removeFromBottom(32);
            cancel.setBounds(row.removeFromRight(110).reduced(4));
            proceed.setBounds(row.removeFromRight(110).reduced(4));
        }

        void buttonClicked(juce::Button* b) override
        {
            if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            {
                if (b == &proceed && onOk)
                    onOk(dontShow.getToggleState());
                dw->exitModalState(b == &proceed ? 1 : 0);
            }
        }
    };

    auto* panel = new Panel(titleText, bodyText, requireSure, showDontShow, std::move(onProceed));
    juce::DialogWindow::LaunchOptions opt;
    opt.dialogTitle = titleText;
    opt.dialogBackgroundColour = juce::Colour(0xff1a1d27);
    opt.content.setOwned(panel);
    opt.escapeKeyTriggersCloseButton = true;
    opt.useNativeTitleBar = true;
    opt.resizable = false;
    opt.launchAsync();
}

void ModernEdirolSd80Editor::timerCallback()
{
    ensureStandaloneAudio();
    queueLabel.setText("USB queue  " + juce::String(proc.queueDepth()) + "  |  throttle "
                           + juce::String((int) proc.apvts.getRawParameterValue("throttle")->load()) + " ms",
                       juce::dontSendNotification);
    if (proc.consumeDumpDirty())
    {
        refreshStripLabels();
        bindSelectedPart();
    }
    if (lastBoundPart != proc.getSelectedPart())
        bindSelectedPart();
    if (tab == Tab::Player)
        updatePlayerUi();
}

void ModernEdirolSd80Editor::buttonClicked(juce::Button* b)
{
    if (b == &syncButton) { proc.syncHardwarePush(); return; }
    if (b == &partAButton)
    {
        proc.setVisibleGroup(0);
        proc.setSelectedPart(proc.getSelectedPart() % 16);
        refreshStripLabels();
        bindSelectedPart();
        return;
    }
    if (b == &partBButton)
    {
        proc.setVisibleGroup(1);
        proc.setSelectedPart(16 + proc.getSelectedPart() % 16);
        refreshStripLabels();
        bindSelectedPart();
        return;
    }
    if (b == &tabMixer) { setTab(Tab::Mixer); return; }
    if (b == &tabPlayer) { setTab(Tab::Player); updatePlayerUi(); return; }
    if (b == &tabDemos) { setTab(Tab::Demos); return; }
    if (b == &tabOptions) { setTab(Tab::Options); return; }
    if (b == &demo1) { proc.playInternalDemo(1); return; }
    if (b == &demo2) { proc.playInternalDemo(2); return; }
    if (b == &demo3) { proc.playInternalDemo(3); return; }
    if (b == &demoStop) { proc.playInternalDemo(0); return; }
    if (b == &pullHardwareBtn) { proc.pullFromHardware(); return; }
    if (b == &reverbTypeBtn) { showReverbMenu(); return; }
    if (b == &chorusTypeBtn) { showChorusMenu(); return; }
    if (b == &mfxABtn) { showMfxMenu(0); return; }
    if (b == &mfxBBtn) { showMfxMenu(1); return; }
    if (b == &mfxCBtn) { showMfxMenu(2); return; }
    if (b == &donateBtn)
    {
        juce::URL("https://crimsonredstone.bandcamp.com/").launchInDefaultBrowser();
        return;
    }
    if (b == &muteAllBtn) { proc.muteAll(true); refreshStripLabels(); return; }
    if (b == &unmuteAllBtn) { proc.muteAll(false); refreshStripLabels(); return; }
    if (b == &unsoloAllBtn) { proc.unsoloAll(); refreshStripLabels(); return; }
    if (b == &resetFxBtn)
    {
        auto run = [this] { proc.resetEffectsToDefault(); bindSelectedPart(); refreshStripLabels(); };
        if (proc.skipFxResetWarning())
        {
            run();
            return;
        }
        confirmDanger("Reset all effects?",
                      "This sets system reverb/chorus, Multi FX A/B/C and every part's reverb/chorus/MFX sends back to SD-80 defaults, then syncs the hardware.\n\n"
                      "Locked parameters stay locked.\n\n"
                      "The SD-80 is 2002 USB hardware - a burst of SysEx can hiccup the module. Cancel if you are in the middle of a take.",
                      true, true,
                      [this, run](bool dont)
                      {
                          if (dont) proc.setSkipFxResetWarning(true);
                          run();
                      });
        return;
    }
    if (b == &emergencyBtn)
    {
        confirmDanger("Emergency Hardware Reset",
                      "This initialises the SD-80 sound generator (Native On + GS Reset), sends All Notes/Sound Off on every channel of Part A and Part B, and restores THIS plugin session to factory defaults (patches, mix, FX, locks, skin).\n\n"
                      "It does not rewrite firmware and does not erase User cards stored inside the module.\n\n"
                      "Because the hardware is a 2002 USB device, a full dump can stall MIDI for a few seconds. Check Yes I am sure, then Proceed. Cancel to abort.",
                      true, false,
                      [this](bool)
                      {
                          proc.factoryResetHardware();
                          applySkin();
                          refreshStripLabels();
                          bindSelectedPart();
                          rebuildDeviceLists();
                      });
        return;
    }
    for (int i = 0; i < kNumSkins; ++i)
    {
        if (b == &skinButtons[(size_t) i])
        {
            proc.setSkinIndex(i);
            applySkin();
            return;
        }
    }
    if (b == &savePreset)
    {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Save Modern Edirol preset",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.mesd80preset");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                             [this, chooser](const juce::FileChooser& fc)
                             {
                                 auto f = fc.getResult();
                                 if (f == juce::File()) return;
                                 if (! f.hasFileExtension(".mesd80preset"))
                                     f = f.withFileExtension(".mesd80preset");
                                 proc.saveMesd80Preset(f);
                             });
        return;
    }
    if (b == &loadPreset)
    {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Load Modern Edirol preset",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.mesd80preset");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this, chooser](const juce::FileChooser& fc)
                             {
                                 auto f = fc.getResult();
                                 if (f != juce::File())
                                 {
                                     proc.loadMesd80Preset(f);
                                     applySkin();
                                     refreshStripLabels();
                                     bindSelectedPart();
                                 }
                             });
        return;
    }
    if (b == &drumToggle)
    {
        bindSelectedPart();
        proc.enqueuePartPatch(proc.getSelectedPart());
        return;
    }
    if (b == &porta)
    {
        proc.enqueuePartDeep(proc.getSelectedPart());
        return;
    }
    for (int i = 0; i < 16; ++i)
    {
        if (b == &strips[(size_t) i].select)
        {
            const int part = proc.getVisibleGroup() * 16 + i;
            const bool exclusive = ! juce::ModifierKeys::getCurrentModifiers().isShiftDown();
            proc.togglePartSelected(part, exclusive);
            refreshStripLabels();
            bindSelectedPart();
            return;
        }
        if (b == &strips[(size_t) i].mute)
        {
            if (juce::ModifierKeys::getCurrentModifiers().isShiftDown())
            {
                proc.muteAll(strips[(size_t) i].mute.getToggleState());
                refreshStripLabels();
                return;
            }
            for (int p = 0; p < 32; ++p)
                proc.enqueuePartMix(p);
            return;
        }
        if (b == &strips[(size_t) i].solo)
        {
            if (juce::ModifierKeys::getCurrentModifiers().isShiftDown())
            {
                proc.unsoloAll();
                refreshStripLabels();
                return;
            }
            for (int p = 0; p < 32; ++p)
                proc.enqueuePartMix(p);
            return;
        }
    }
}

void ModernEdirolSd80Editor::comboBoxChanged(juce::ComboBox* box)
{
    if (box == &outABox)
    {
        proc.setMidiOutputA(outABox.getSelectedId() - 2);
        return;
    }
    if (box == &outBBox)
    {
        proc.setMidiOutputB(outBBox.getSelectedId() - 2);
        return;
    }
    if (box == &throttleBox)
    {
        proc.setParamInt("throttle", box->getSelectedId());
        return;
    }
    if (box == &modeBox)
    {
        proc.setGeneratorMode(static_cast<GeneratorMode>(box->getSelectedId() - 1));
        return;
    }
    if (box == &mapBox)
    {
        const int part = proc.getSelectedPart();
        proc.setParamInt(ModernEdirolSd80Processor::pid(part, "map"), mapBox.getSelectedId() - 1);
        const auto sm = static_cast<SoundMap>(mapBox.getSelectedId() - 1);
        const bool drum = drumToggle.getToggleState();
        proc.setParamInt(ModernEdirolSd80Processor::pid(part, "msb"),
                         drum ? drumMsbForMap(sm) : instMsbForMap(sm));
        rebuildPatchBrowser();
        return;
    }
    if (box == &mfxSel)
    {
        const int part = proc.getSelectedPart();
        proc.setParamInt(ModernEdirolSd80Processor::pid(part, "mfxSel"), mfxSel.getSelectedId() - 1);
        proc.enqueuePartDeep(part);
        return;
    }
    if (box == &outAsg)
    {
        const int part = proc.getSelectedPart();
        proc.setParamInt(ModernEdirolSd80Processor::pid(part, "outAsg"), outAsg.getSelectedId() - 1);
        proc.enqueuePartDeep(part);
        return;
    }
}

void ModernEdirolSd80Editor::sliderValueChanged(juce::Slider*)
{
}

bool ModernEdirolSd80Editor::isInterestedInFileDrag(const juce::StringArray& files)
{
    return files.size() > 0 && (files[0].endsWithIgnoreCase(".mid") || files[0].endsWithIgnoreCase(".midi"));
}

void ModernEdirolSd80Editor::fileDragEnter(const juce::StringArray&, int, int)
{
    dragging = true;
    repaint();
}

void ModernEdirolSd80Editor::fileDragExit(const juce::StringArray&)
{
    dragging = false;
    repaint();
}

void ModernEdirolSd80Editor::filesDropped(const juce::StringArray& files, int, int)
{
    dragging = false;
    if (files.isEmpty()) return;
    const juce::File f(files[0]);
    if (tab == Tab::Player)
    {
        proc.player.load(f);
        updatePlayerUi();
    }
    else
    {
        proc.applyMidiFile(f);
        refreshStripLabels();
        bindSelectedPart();
    }
    repaint();
}
