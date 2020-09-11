#pragma once
#include <JuceHeader.h>

class AudioAnalyzer : public Component
					, public Timer
{
public:
	AudioAnalyzer() : forwardFFT(fftOrder)
	{
		startTimerHz(40);
	};

	~AudioAnalyzer()
	{

	};

	void init(int samplesPerBlockExpected, double sampleRate)
	{
		currentSampleRate = sampleRate;
		currentBlockSize = samplesPerBlockExpected;
		currentFFTSize = fftSize;
	}

	void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill)
	{
		if (bufferToFill.buffer->getNumChannels() > 0)
		{
			auto* channelData = bufferToFill.buffer->getReadPointer(1, bufferToFill.startSample);
			for (auto i = 0; i < bufferToFill.numSamples; ++i)
				pushNextSampleIntoFifo(channelData[i]);
		}

		bufferToFill.clearActiveBufferRegion();
	}

	void paint(juce::Graphics& g) override
	{
		g.fillAll(Colours::black);

		int startX = 109;
		int startY = 55;
		int scaleX = 1;
		int scaleY = 1;
		int startBin = 0;

		for (int i = 0; i < fftSize; ++i)
		{
			g.setColour(Colours::orange);
			int j = startBin + i;
			g.drawLine(startX + i * scaleX, startY - fftDataCopy[j] * scaleY, startX + (i + 1) * scaleX, startY - fftDataCopy[j + 1] * scaleY, 1.0f);
		}

		g.setColour(Colours::white);
		g.drawText("Sample rate: " + String(currentSampleRate), 5, 5, 140, 15, Justification::centredLeft);
		g.drawText("Buffer size: " + String(currentBlockSize), 5, 20, 140, 15, Justification::centredLeft);
		g.drawText("FFT size: " + String(currentFFTSize), 5, 35, 140, 15, Justification::centredLeft);
	};

	void resized() override
	{

	};

	void timerCallback() override
	{
		if (nextFFTBlockReady)
		{
			forwardFFT.performFrequencyOnlyForwardTransform(fftData);
			FloatVectorOperations::copy(fftDataCopy, fftData, fftSize / 2);
			repaint();
			nextFFTBlockReady = false;
		}
	};

	enum
	{
		fftOrder = 9,
		fftSize = 1 << fftOrder
	};

private:

	void pushNextSampleIntoFifo(float sample)
	{
		// if the fifo contains enough data, set a flag to say
		// that the next line should now be rendered..
		if (fifoIndex == fftSize)    // [8]
		{
			if (!nextFFTBlockReady) // [9]
			{
				zeromem(fftData, sizeof(fftData));
				memcpy(fftData, fifo, sizeof(fifo));
				nextFFTBlockReady = true;
			}
			fifoIndex = 0;
		}
		fifo[fifoIndex++] = sample;  // [9]
	}

	juce::dsp::FFT forwardFFT;
	float fifo[fftSize];
	float fftData[2 * fftSize];
	float fftDataCopy[2 * fftSize];
	int fifoIndex = 0;
	bool nextFFTBlockReady = false;

	int currentSampleRate, currentBlockSize, currentFFTSize;
};
