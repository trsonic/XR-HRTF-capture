#pragma once
#include <JuceHeader.h>

class AudioRecorder : public juce::AudioIODeviceCallback
{
public:
    AudioRecorder(juce::AudioThumbnail& thumbnailToUpdate)
        : thumbnail(thumbnailToUpdate)
    {
        backgroundThread.startThread();
    }

    ~AudioRecorder() override
    {
        stop();
    }

    void loadSweep(const juce::File& file)
    {
        juce::AudioFormatManager afm;
        afm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(afm.createReaderFor(file));
        sweepBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&sweepBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
    }

    void startRecording(const juce::File& file)
    {
        stop();

        if (sampleRate > 0)
        {
            file.deleteFile();

            if (auto fileStream = std::unique_ptr<juce::FileOutputStream>(file.createOutputStream()))
            {
                juce::WavAudioFormat wavFormat;
                if (auto writer = wavFormat.createWriterFor(fileStream.get(), sampleRate, 2, 16, {}, 0))
                {
                    fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)

                    // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
                    // write the data to disk on our background thread.
                    threadedWriter.reset(new juce::AudioFormatWriter::ThreadedWriter(writer, backgroundThread, 32768));

                    // Reset our recording thumbnail
                    thumbnail.reset(writer->getNumChannels(), writer->getSampleRate());
                    nextSampleNum = 0;

                    // And now, swap over our active writer pointer so that the audio callback will start using it..
                    const juce::ScopedLock sl(writerLock);
                    activeWriter = threadedWriter.get();
                }
            }
        }
    }

    void stop()
    {
        const juce::ScopedLock sl(writerLock);
        activeWriter = nullptr;
        threadedWriter.reset();
    }

    bool isRecording() const
    {
        return activeWriter.load() != nullptr;
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        sampleRate = device->getCurrentSampleRate();
    }

    void audioDeviceStopped() override
    {
        sampleRate = 0;
    }

    void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels,
        float** outputChannelData, int numOutputChannels, int numSamples) override
    {
        const juce::ScopedLock sl(writerLock);

        if (activeWriter.load() != nullptr && numInputChannels >= thumbnail.getNumChannels())
        {
            activeWriter.load()->write(inputChannelData, numSamples);

            // Create an AudioBuffer to wrap our incoming data, note that this does no allocations or copies, it simply references our input data
            juce::AudioBuffer<float> buffer(const_cast<float**> (inputChannelData), thumbnail.getNumChannels(), numSamples);
            thumbnail.addBlock(nextSampleNum, buffer, 0, numSamples);
            nextSampleNum += numSamples;

            int sweepLength = sweepBuffer.getNumSamples();
            if (sweepPosition + numSamples <= sweepLength)
            {
                int numberOfSweepSamples = jmin(numSamples, sweepLength - sweepPosition);
                for (int i = 0; i < numOutputChannels; ++i)
                {
                    FloatVectorOperations::copy(outputChannelData[i], sweepBuffer.getReadPointer(0, sweepPosition), numberOfSweepSamples);
                }
                sweepPosition += numberOfSweepSamples;
            }
            else
            {
                sweepPosition = 0;
                stop();
            }


        }
        else
        {
            // We need to clear the output buffers, in case they're full of junk..
            for (int i = 0; i < numOutputChannels; ++i)
            {
                if (outputChannelData[i] != nullptr) FloatVectorOperations::clear(outputChannelData[i], numSamples);
            }
        }


    }

private:
    juce::AudioThumbnail& thumbnail;
    juce::TimeSliceThread backgroundThread{ "Audio Recorder Thread" }; // the thread that will write our audio data to disk
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter; // the FIFO used to buffer the incoming data
    double sampleRate = 0.0;
    juce::int64 nextSampleNum = 0;

    juce::AudioSampleBuffer sweepBuffer;
    int sweepPosition = 0;

    juce::CriticalSection writerLock;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter{ nullptr };
};
