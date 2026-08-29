#include <JuceHeader.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

// Custom standalone: native title bar only. No JUCE "Options" / "Settings" chrome.
// Audio + MIDI-input live in the plugin OPTIONS tab (AudioDeviceSelectorComponent).
class Mesd80Window : public juce::DocumentWindow
{
public:
    Mesd80Window()
        : DocumentWindow(JucePlugin_Name,
                         juce::Colours::black,
                         DocumentWindow::allButtons)
    {
        juce::PropertiesFile::Options opt;
        opt.applicationName = "ModernEdirolSD80";
        opt.filenameSuffix = "settings";
        opt.osxLibrarySubFolder = "Application Support";
        opt.folderName = "CrimsonRedstone";
        appProps.setStorageParameters(opt);

        // appProps MUST outlive holder (holder saves filterState into it).
        // autoOpenMidiDevices = false so SD-80 USB outs stay on Part A/B selectors.
        // MIDI keyboards are enabled below (every input that is not the SD-80).
        holder = std::make_unique<juce::StandalonePluginHolder>(
            appProps.getUserSettings(), false, juce::String(), nullptr,
            juce::Array<juce::StandalonePluginHolder::PluginInOuts>{}, false);

        enableKeyboardMidiInputs();

        setUsingNativeTitleBar(true);
        setResizable(true, false);
        auto* editor = holder->processor->createEditorIfNeeded();
        setContentOwned(editor, true);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    ~Mesd80Window() override
    {
        if (holder != nullptr)
            holder->savePluginState();
        if (auto* s = appProps.getUserSettings())
            s->saveIfNeeded();

        clearContentComponent();
        if (holder != nullptr)
            holder->stopPlaying();
        holder.reset();
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplicationBase::quit();
    }

    // ApplicationProperties first so it is destroyed last.
    juce::ApplicationProperties appProps;
    std::unique_ptr<juce::StandalonePluginHolder> holder;

private:
    void enableKeyboardMidiInputs()
    {
        if (holder == nullptr)
            return;
        auto& dm = holder->deviceManager;
        for (auto& d : juce::MidiInput::getAvailableDevices())
        {
            auto n = d.name.toLowerCase();
            const bool sd80 = n.contains("sd-80") || n.contains("sd80")
                           || n.contains("edirol") || n.contains("studio canvas");
            if (! sd80)
                dm.setMidiInputDeviceEnabled(d.identifier, true);
        }
        // JUCE 8 has one default MIDI out, not per-device enable. Empty id = none.
        // Part A/B USB are opened by the processor, not the host device manager.
        dm.setDefaultMidiOutputDevice({});
    }
};

class Mesd80App : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return JucePlugin_Name; }
    const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<Mesd80Window>();
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    std::unique_ptr<Mesd80Window> mainWindow;
};

START_JUCE_APPLICATION(Mesd80App)
