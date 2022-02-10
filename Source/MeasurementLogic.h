#pragma once
#include <JuceHeader.h>
#include "OscTransceiver.h"
#include "MeasurementTable.h"
#include "AudioRecorder.h"
#include "RecordingThumbnail.h"

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

	bool isMeasurementOn()
	{
		return m_startStopButton.getToggleState();
	}

	String m_currentLogMessage;

	void loadSubjectFolder(File folder);
	File getSubjectFolder();
private:
	OscTransceiver& oscTxRx;
	AudioRecorder& recorder;
	RecordingThumbnail& thumbnail;

	void startRecording();
	void stopRecording();

	void processOscMessage(const OSCMessage& message);

	void analyzeOscMsgList();

	void sendMsgToLogWindow(String message);
	File subjectFolder;
	File sweepFile, lastRecording;

	double m_activationTime = 0.0f;
	StringArray oscMessageList;
	float meanAz, meanEl, meanDist;

	bool orientationLocked = false;
	bool referenceMeasurementOn = false;

	int m_currentMeasurement;
	TextButton	m_loadSubjectFolderButton{ "Load Subject Folder" }
			, m_referenceMeasurementButton{ "Reference" }
			, m_hpeqMeasurementButton{ "HP EQ" }
			, m_startStopButton{ "Start" }
			, m_nextMeasurementButton{ "Next" };


	MeasurementTable m_table;
	TextEditor m_lastMessage, m_logHeaderTE;
	String m_logHeader = "salte_time,osc_pattern,ml_time,azimuth,elevation,distance,hdSpkAngDev,spkHdAngDev,distDev\n";
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeasurementLogic)
};
