#pragma once
#include <JuceHeader.h>
#include "RecordingThumbnail.h"
#include "AudioRecorder.h"
#include "AudioSetup.h"

class MainComponent  : public juce::Component
{
public:
    
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void startRecording();
    void stopRecording();

    AudioDeviceManager audioDeviceManager;
    RecordingThumbnail recordingThumbnail;
    AudioRecorder recorder{ recordingThumbnail.getAudioThumbnail() };
    AudioSetup m_audioSetup;

    TextButton measureButton{ "Measure" }, stopButton{ "Stop" }, setupButton{"Audio Setup"};
    File lastRecording;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
