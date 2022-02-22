#pragma once
#include <JuceHeader.h>
#include "OscTransceiver.h"
#include "MeasurementTable.h"
#include "AudioRecorder.h"
#include "RecordingThumbnail.h"

class TransformFrame
{
public:
	void setSphericalData(int time, float azimuth, float elevation, float distance)
	{
		frameTime = time;
		frameAz = azimuth;
		frameEl = elevation;
		frameDist = distance;
		updateCartesian();
	}

	void setCartesianData(int time, float x, float y, float z)
	{
		frameTime = time;
		frameX = x;
		frameY = y;
		frameZ = z;
		updateSpherical();
	}

	int getTime() { return frameTime; }
	float getAz() { return frameAz; }
	float getEl() { return frameEl; }
	float getDist() { return frameDist; }
	float getX() { return frameX; }
	float getY() { return frameY; }
	float getZ() { return frameZ; }
	
private:
	void updateCartesian()
	{
		frameZ = frameDist * cos(degreesToRadians(frameEl)) * cos(degreesToRadians(frameAz));
		frameX = frameDist * cos(degreesToRadians(frameEl)) * sin(degreesToRadians(frameAz));
		frameY = frameDist * sin(degreesToRadians(frameEl));
	}

	void updateSpherical()
	{
		frameDist = sqrt(frameX * frameX + frameY * frameY + frameZ * frameZ);
		frameAz = radiansToDegrees(atan2(frameX, frameZ));
		frameEl = (radiansToDegrees(acos(frameY / frameDist)) - 90.f) * -1.f;
	}

	int frameTime;
	float frameAz, frameEl, frameDist;
	float frameX, frameY, frameZ;
};

class SphericalOperations
{
public:
	void addTransformFrame(TransformFrame newFrame)
	{
		frames.add(new TransformFrame(newFrame));
	}

	void reset()
	{
		frames.clear();
	}

	bool hasEnoughFrames(int requiredTime)
	{
		return frames.getLast()->getTime() - frames.getFirst()->getTime() >= requiredTime;
	}

	TransformFrame getMeanTransformFrame()
	{
		StatisticsAccumulator<float> x, y, z;
		for (int i = 0; i < frames.size(); ++i)
		{
			x.addValue(frames[i]->getX());
			y.addValue(frames[i]->getY());
			z.addValue(frames[i]->getZ());
		}

		TransformFrame meanFrame;
		meanFrame.setCartesianData(0, x.getAverage(), y.getAverage(), z.getAverage());
		return meanFrame;
	}

	float getAngularAngle(TransformFrame targetFrame, TransformFrame meanFrame)
	{
		float lon1 = degreesToRadians(targetFrame.getAz());
		float lat1 = degreesToRadians(targetFrame.getEl());
		float lon2 = degreesToRadians(meanFrame.getAz());
		float lat2 = degreesToRadians(meanFrame.getEl());
		float dlon = lon2 - lon1;
		float dlat = lat2 - lat1;
		float a = powf(sin(dlat / 2.f), 2.f) + cos(lat1) * cos(lat2) * powf((sin(dlon / 2.f)), 2.f);
		return radiansToDegrees(2 * atan2(sqrt(a), sqrt(1.f - a)));
	}

private:
	OwnedArray<TransformFrame> frames;
};

class MeasurementLogic	: public Component
						, public OSCReceiver::Listener<OSCReceiver::RealtimeCallback>
						, public Timer
						, public ChangeBroadcaster
						, public ChangeListener
{
public:
	MeasurementLogic(OscTransceiver &m_oscTxRx, AudioRecorder &m_recorder, RecordingThumbnail &m_thumbnail);
	~MeasurementLogic() override;

	void paint(juce::Graphics& g) override;
	void resized() override;
	void oscMessageReceived(const OSCMessage& message) override;
	void oscBundleReceived(const OSCBundle& bundle) override;
	void timerCallback() override;
	void changeListenerCallback(ChangeBroadcaster* source) override;
	void nextMeasurement();
	String getCurrentName();

	String m_currentLogMessage;

	void loadSubjectFolder(File folder);
	File getSubjectFolder();
private:
	OscTransceiver& oscTxRx;
	AudioRecorder& recorder;
	RecordingThumbnail& thumbnail;

	enum mTypes {none, reference, hpeq, hrir};

	mTypes currentMeasurementType;
	int referenceMeasurementCount = 0;
	int hpMeasurementCount = 0;

	void switchMeasurementType(mTypes newtype);

	void startRecording();
	void stopRecording();

	void processOscMessage(const OSCMessage& message);

	void analyzeOscMsgList();

	void sendMsgToLogWindow(String message);
	File subjectFolder;
	File lsSweepFile, hpSweepFile, lastRecording;

	double m_activationTime = 0.0f;
	StringArray oscMessageList;
	
	float meanAz, meanEl, meanDist;

	bool orientationLocked = false;
	bool referenceMeasurementOn = false;

	int currentMeasurementIndex;
	TextButton	m_loadSubjectFolderButton{ "Load Subject Folder" }
			, m_referenceMeasurementButton
			, m_refCountResetButton{ "Reset" }
			, m_hpeqMeasurementButton
		    , m_hpeqCountResetButton{ "Reset" }
			, m_startStopButton
			, m_nextMeasurementButton{ "Next" };


	MeasurementTable m_table;
	TextEditor m_lastMessage, m_logHeaderTE;
	String m_logHeader = "salte_time,osc_pattern,ml_time,azimuth,elevation,distance,hdSpkAngDev,spkHdAngDev,distDev\n";
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeasurementLogic)
};
