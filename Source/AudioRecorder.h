#pragma once
#include <JuceHeader.h>
#include "RecordingThumbnail.h"
#include "AudioAnalyzer.h"

class AudioRecorder : public juce::AudioIODeviceCallback
                    , public ChangeBroadcaster
{
public:
    AudioRecorder(juce::AudioThumbnail& thumbnailToUpdate, AudioAnalyzer& anal)
        : thumbnail(thumbnailToUpdate)
        , analyzer(anal)
    {
        backgroundThread.startThread();
    }

    ~AudioRecorder() override
    {
        stop();
    }

    bool loadSweep(const juce::File& file)
    {
        if (file.existsAsFile())
        {
            juce::AudioFormatManager afm;
            afm.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader(afm.createReaderFor(file));
            sweepBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
            reader->read(&sweepBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
            sweepLength = (float)((int)reader->lengthInSamples / (int)reader->sampleRate);
        }
        else
        {
            sweepLength = 0.0;
            return false;
        }

    }

    float getSweepLength()
    {
        return sweepLength;
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

                    sendChangeMessage();
                }
            }
        }
    }

    void stop()
    {
        const juce::ScopedLock sl(writerLock);
        sweepPosition = 0;
        activeWriter = nullptr;
        threadedWriter.reset();
        sendChangeMessage();
    }

    bool isRecording() const
    {
        return activeWriter.load() != nullptr;
    }

    bool isRecordingFinished() const
    {
        return recordingFinished;
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        sampleRate = device->getCurrentSampleRate();
        analyzer.init(device->getCurrentBufferSizeSamples(), sampleRate);
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
            // output
            int sweepLength = sweepBuffer.getNumSamples();
            if (sweepPosition + numSamples <= sweepLength)
            {
                recordingFinished = false;
                int numberOfSweepSamples = jmin(numSamples, sweepLength - sweepPosition);
                for (int i = 0; i < numOutputChannels; ++i)
                {
                    FloatVectorOperations::copy(outputChannelData[i], sweepBuffer.getReadPointer(0, sweepPosition), numberOfSweepSamples);
                }
                sweepPosition += numberOfSweepSamples;
            }
            else
            {
                recordingFinished = true;
                stop();
                return;
            }

            // input
            activeWriter.load()->write(inputChannelData, numSamples);
            juce::AudioBuffer<float> buffer(const_cast<float**> (inputChannelData), thumbnail.getNumChannels(), numSamples);
            thumbnail.addBlock(nextSampleNum, buffer, 0, numSamples);
            nextSampleNum += numSamples;
        }
        else
        {
            // We need to clear the output buffers, in case they're full of junk..
            for (int i = 0; i < numOutputChannels; ++i)
            {
                if (outputChannelData[i] != nullptr) FloatVectorOperations::clear(outputChannelData[i], numSamples);
            }
        }

        //analyzer
        if (numInputChannels >= 2)
        {
            juce::AudioBuffer<float> buffer(const_cast<float**> (inputChannelData), 2, numSamples);
            analyzer.getNextAudioBlock(AudioSourceChannelInfo(&buffer, 0, numSamples));
        }
    }

private:
    juce::AudioThumbnail& thumbnail;
    AudioAnalyzer& analyzer;
    juce::TimeSliceThread backgroundThread{ "Audio Recorder Thread" }; // the thread that will write our audio data to disk
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter; // the FIFO used to buffer the incoming data
    double sampleRate = 0.0;
    juce::int64 nextSampleNum = 0;

    juce::AudioSampleBuffer sweepBuffer;
    float sweepLength = 0.0;
    int sweepPosition = 0;
    bool recordingFinished = true;

    juce::CriticalSection writerLock;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter{ nullptr };
};
