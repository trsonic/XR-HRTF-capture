#pragma once
#include <JuceHeader.h>
#include "RecordingThumbnail.h"
#include "AudioRecorder.h"
#include "AudioSetup.h"
#include "AudioAnalyzer.h"
#include "OscTransceiver.h"
#include "MeasurementLogic.h"

class MainComponent     : public Component
                        , public Button::Listener
                        , private ChangeListener

{
public:
    
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* buttonThatWasClicked) override;
    void changeListenerCallback(ChangeBroadcaster* source) override;

private:
    void loadSweep(File file);
    void startRecording();
    void stopRecording();

    AudioDeviceManager audioDeviceManager;
    RecordingThumbnail recordingThumbnail;
    AudioAnalyzer m_analyzer;
    AudioRecorder recorder{ recordingThumbnail.getAudioThumbnail(), m_analyzer };
    AudioSetup m_audioSetup;
    OscTransceiver m_oscTxRx;
    MeasurementLogic m_logic{m_oscTxRx};

    TextButton loadSweepButton{"Load Sweep"}, measureButton{ "Measure" }, stopButton{ "Stop" }, setupButton{ "Audio Setup" };
    TextButton connectOscButton;
    Label clientTxIpLabel, clientTxPortLabel, clientRxPortLabel;
    File sweepFile, lastRecording;

    // app settings
    void loadSettings();
    void saveSettings();
    ApplicationProperties appSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
