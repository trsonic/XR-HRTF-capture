#pragma once
#include <JuceHeader.h>
#include "OscTransceiver.h"
#include "MeasurementTable.h"

class MeasurementLogic	: public Component
						, public OSCReceiver::Listener<OSCReceiver::RealtimeCallback>
						, public Timer
						, public ChangeBroadcaster
{
public:
	MeasurementLogic(OscTransceiver &m_oscTxRx);
	~MeasurementLogic() override;

	void paint(juce::Graphics& g) override;
	void resized() override;
	void oscMessageReceived(const OSCMessage& message) override;
	void oscBundleReceived(const OSCBundle& bundle) override;
	void timerCallback() override;

	void nextMeasurement();
	String getCurrentName();

	bool isOrientationLocked()
	{
		return orientationLocked;
	}

	bool isMeasurementOn()
	{
		return m_startStopButton.getToggleState();
	}

	bool isReferenceMeasurementOn()
	{
		return referenceMeasurementOn;
	}

private:
	OscTransceiver& m_oscTxRx;
	void processOscMessage(const OSCMessage& message);

	void analyzeOscMsgList();

	double m_activationTime = 0.0f;
	StringArray oscMessageList;

	bool orientationLocked = false;
	bool referenceMeasurementOn = false;

	int m_currentMeasurement;
	TextButton m_startStopButton{"Start"}, m_nextMeasurementButton{ "Next" }, m_referenceMeasurementButton{ "Reference" };

	MeasurementTable m_table;
	TextEditor m_lastMessage, m_logHeaderTE;
	String m_logHeader = "salte_time,osc_pattern,ml_time,azimuth,elevation,hdSpkAngDev,spkHdAngDev,distance,distDev\n";
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeasurementLogic)
};
