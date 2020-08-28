#include "MainComponent.h"

MainComponent::MainComponent() : m_audioSetup(audioDeviceManager)
{

    addAndMakeVisible(recordingThumbnail);

    addAndMakeVisible(measureButton);
    measureButton.onClick = [this]
    {
        startRecording();
    };

    addAndMakeVisible(stopButton);
    stopButton.onClick = [this]
    {
        stopRecording();
    };

    addAndMakeVisible(setupButton);
    setupButton.onClick = [this]
    {
        addAndMakeVisible(m_audioSetup);
        m_audioSetup.m_shouldBeVisible = true;
    };

    audioDeviceManager.initialise(2, 2, nullptr, true, {}, nullptr);
    audioDeviceManager.addAudioCallback(&recorder);

    File sweep = juce::File("C:/TR_FILES/SWEEPS/ZZ_sweep.wav");
    recorder.loadSweep(sweep);
    setSize(800, 600);
}

MainComponent::~MainComponent()
{
    audioDeviceManager.removeAudioCallback(&recorder);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    recordingThumbnail.setBounds(area.removeFromTop(80).reduced(8));
    measureButton.setBounds(area.removeFromTop(36).removeFromLeft(140).reduced(8));
    stopButton.setBounds(area.removeFromTop(36).removeFromLeft(140).reduced(8));
    setupButton.setBounds(area.removeFromTop(36).removeFromLeft(140).reduced(8));
}

void MainComponent::startRecording()
{
    lastRecording = File("C:/TR_FILES/SWEEPS/LR_temp_sweep.wav");
    recorder.startRecording(lastRecording);
}

void MainComponent::stopRecording()
{
    recorder.stop();
    lastRecording = juce::File();
}
