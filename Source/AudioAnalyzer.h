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
		if (bufferToFill.buffer->getNumChannels() == 2)
		{
			for (int ch = 0; ch < 2; ++ch)
			{
				auto* channelData = bufferToFill.buffer->getReadPointer(ch, bufferToFill.startSample);
				for (auto i = 0; i < bufferToFill.numSamples; ++i)
					pushNextSampleIntoFifo(ch, channelData[i]);
			}

		}

		bufferToFill.clearActiveBufferRegion();
	}

	void paint(juce::Graphics& g) override
	{
		g.fillAll(Colours::black);

		int startX = 100;
		int startY = 0;
		int scaleX = 1;
		int scaleY = 1;
		int startBin = 0;

		for (int i = 0; i < fftSize; ++i)
		{
			for (int ch = 0; ch < 2; ++ch)
			{
				if (ch == 0) g.setColour(Colours::green);
				else g.setColour(Colours::red);
				int j = startBin + i;
				float v1 = juce::Decibels::gainToDecibels(fftDataCopy[ch][j]);
				float v2 = juce::Decibels::gainToDecibels(fftDataCopy[ch][j + 1]);
				g.drawLine(startX + i * scaleX, startY - v1 * scaleY, startX + (i + 1) * scaleX, startY - v2 * scaleY, 1.0f);
			}
		}

		g.setFont(12.0f);
		g.setColour(Colours::white);
		g.drawText("Sample rate: " + String(currentSampleRate), 5, 3, 140, 14, Justification::centredLeft);
		g.drawText("Buffer size: " + String(currentBlockSize), 5, 17, 140, 14, Justification::centredLeft);
		g.drawText("FFT size: " + String(currentFFTSize), 5, 31, 140, 14, Justification::centredLeft);
		g.setColour(Colours::green);
		g.drawText("L: " + Decibels::toString(Decibels::gainToDecibels(peakValue[0])), 5, 45, 140, 14, Justification::centredLeft);
		g.setColour(Colours::red);
		g.drawText("R: " + Decibels::toString(Decibels::gainToDecibels(peakValue[1])), 5, 59, 140, 14, Justification::centredLeft);

	};

	void resized() override
	{

	};

	void timerCallback() override
	{
		if (nextFFTBlockReady)
		{
			forwardFFT.performFrequencyOnlyForwardTransform(fftData[0]);
			forwardFFT.performFrequencyOnlyForwardTransform(fftData[1]);
			FloatVectorOperations::copy(fftDataCopy[0], fftData[0], fftSize / 2);
			FloatVectorOperations::copy(fftDataCopy[1], fftData[1], fftSize / 2);
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

	void pushNextSampleIntoFifo(int channel, float sample)
	{
		// if the fifo contains enough data, set a flag to say
		// that the next line should now be rendered..
		if (fifoIndex == fftSize)
		{
			if (!nextFFTBlockReady && channel == 1)
			{
				zeromem(fftData, sizeof(fftData));
				memcpy(fftData[0], fifo[0], sizeof(fifo[0]));
				memcpy(fftData[1], fifo[1], sizeof(fifo[1]));
				FloatVectorOperations::abs(fifo[0], fifo[0], sizeof(fifo[0]));
				FloatVectorOperations::abs(fifo[1], fifo[1], sizeof(fifo[1]));
				peakValue[0] = FloatVectorOperations::findMaximum(fifo[0], 1);
				peakValue[1] = FloatVectorOperations::findMaximum(fifo[1], 1);
				nextFFTBlockReady = true;
			}
			fifoIndex = 0;
		}
		fifo[channel][fifoIndex] = sample;
		fifoIndex++;
	}

	juce::dsp::FFT forwardFFT;
	float fifo[2][fftSize];
	float peakValue[2];
	float fftData[2][2 * fftSize];
	float fftDataCopy[2][2 * fftSize];
	int fifoIndex = 0;
	bool nextFFTBlockReady = false;

	int currentSampleRate = 0, currentBlockSize = 0, currentFFTSize = 0;
};
