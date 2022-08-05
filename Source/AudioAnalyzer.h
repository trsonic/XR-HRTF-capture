#pragma once
#include <JuceHeader.h>

class AudioAnalyzer : public Component
					, public Timer
					, public MouseListener
{
public:
	AudioAnalyzer() : forwardFFT(fftOrder)
					, window(fftSize, juce::dsp::WindowingFunction<float>::hamming)
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

		int startX = 85;
		int startY = 5;
		float scaleX = 1.6f;
		float scaleY = 0.75f;
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

		// peak level meters
		float alpha = 0.95f;
		smoothedPeakValue[0] = smoothedPeakValue[0] * alpha + peakValue[0] * (1 - alpha);
		smoothedPeakValue[1] = smoothedPeakValue[1] * alpha + peakValue[1] * (1 - alpha);

		float leftPeakdB = Decibels::gainToDecibels(smoothedPeakValue[0]);
		float rightPeakdB = Decibels::gainToDecibels(smoothedPeakValue[1]);
		float leftPeakMaxdB = Decibels::gainToDecibels(peakValueMax[0]);
		float rightPeakMaxdB = Decibels::gainToDecibels(peakValueMax[1]);

		int scale = 4;
		float range = 110.f;
		int h = 10;
		g.setColour(Colours::green);
		g.fillRect(5, getHeight() - 35, (int)(range + leftPeakdB) * scale, h);
		g.fillRect(4 + (int)(range + leftPeakMaxdB) * scale, getHeight() - 35, 2, h);
		g.setColour(Colours::red);
		g.fillRect(5, getHeight() - 20, (int)(range + rightPeakdB) * scale, h);
		g.fillRect(4 + (int)(range + rightPeakMaxdB) * scale, getHeight() - 20, 2, h);
		g.setColour(Colours::white);
		g.drawText("L max: " + Decibels::toString(leftPeakMaxdB), getWidth() - 105, getHeight() - 35, 100, h, Justification::centredLeft);
		g.drawText("R max: " + Decibels::toString(rightPeakMaxdB), getWidth() - 105, getHeight() - 20, 100, h, Justification::centredLeft);
		
	};

	void resized() override
	{

	};

	void timerCallback() override
	{
		if (nextFFTBlockReady)
		{
			for (int i = 0; i < 2; ++i)
			{
				window.multiplyWithWindowingTable(fftData[i], fftSize);
				forwardFFT.performFrequencyOnlyForwardTransform(fftData[i]);
				FloatVectorOperations::copy(fftDataCopy[i], fftData[i], fftSize / 2);
			}
			repaint();
			nextFFTBlockReady = false;
		}
	};

	enum
	{
		fftOrder = 9,
		fftSize = 1 << fftOrder
	};

	void mouseDown(const MouseEvent& event) override
	{
		peakValueMax[0] = 0.f;
		peakValueMax[1] = 0.f;
	}

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

				for (int i = 0; i < 2; ++i)
				{
					memcpy(fftData[i], fifo[i], sizeof(fifo[i]));
					FloatVectorOperations::abs(fifo[i], fifo[i], sizeof(fifo[i]));
					peakValue[i] = FloatVectorOperations::findMaximum(fifo[i], 1);
					if (peakValue[i] > peakValueMax[i]) peakValueMax[i] = peakValue[i];
				}
				
				nextFFTBlockReady = true;
			}
			fifoIndex = 0;
		}
		fifo[channel][fifoIndex] = sample;
		fifoIndex++;
	}

	juce::dsp::FFT forwardFFT;
	juce::dsp::WindowingFunction<float> window;

	float fifo[2][fftSize];
	float peakValue[2], smoothedPeakValue[2], peakValueMax[2];
	float fftData[2][2 * fftSize];
	float fftDataCopy[2][2 * fftSize];
	int fifoIndex = 0;
	bool nextFFTBlockReady = false;

	double currentSampleRate = 0.0;
	int currentBlockSize = 0, currentFFTSize = 0;
};
