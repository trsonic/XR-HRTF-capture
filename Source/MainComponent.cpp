#include "MainComponent.h"

MainComponent::MainComponent()
{

    juce::File sweep = juce::File("C:/TR_FILES/SWEEPS/ZZ_sweep.wav");
    juce::AudioFormatManager afm;
    afm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (afm.createReaderFor(sweep));
    sweepBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
    reader->read(&sweepBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
    
    
    setSize(800, 600);
    setAudioChannels(2, 2);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}


void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{

}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto numInputChannels = sweepBuffer.getNumChannels();
    auto numOutputChannels = bufferToFill.buffer->getNumChannels();

    auto outputSamplesRemaining = bufferToFill.numSamples;
    auto outputSamplesOffset = bufferToFill.startSample;

    while (outputSamplesRemaining > 0)
    {
        auto bufferSamplesRemaining = sweepBuffer.getNumSamples() - position;
        auto samplesThisTime = juce::jmin(outputSamplesRemaining, bufferSamplesRemaining);

        for (auto channel = 0; channel < numOutputChannels; ++channel)
        {
            bufferToFill.buffer->copyFrom(channel,
                outputSamplesOffset,
                sweepBuffer,
                channel % numInputChannels,
                position,
                samplesThisTime);

            //bufferToFill.buffer->applyGainRamp(channel, outputSamplesOffset, samplesThisTime, startLevel, level);
        }

        outputSamplesRemaining -= samplesThisTime;
        outputSamplesOffset += samplesThisTime;
        position += samplesThisTime;

        if (position == sweepBuffer.getNumSamples())
            position = 0;
    }
}

void MainComponent::releaseResources()
{

}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

}

void MainComponent::resized()
{

}
