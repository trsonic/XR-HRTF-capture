#pragma once
#include <JuceHeader.h>
#include "RecordingThumbnail.h"
#include "AudioRecorder.h"
#include "AudioSetup.h"

class MainComponent     : public Component
                        , private ChangeListener
{
public:
    
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(ChangeBroadcaster* source) override;

private:
    void loadSweep(File file);
    void startRecording();
    void stopRecording();

    AudioDeviceManager audioDeviceManager;
    RecordingThumbnail recordingThumbnail;
    AudioRecorder recorder{ recordingThumbnail.getAudioThumbnail() };
    AudioSetup m_audioSetup;

    TextButton loadSweepButton{"Load Sweep"}, measureButton{ "Measure" }, stopButton{ "Stop" }, setupButton{ "Audio Setup" };
    File sweepFile, lastRecording;

    // app settings
    void loadSettings();
    void saveSettings();
    ApplicationProperties appSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
