#pragma once
#include <JuceHeader.h>
#include "MeasurementTable.h"

class MeasurementLogic	: public Component
						, public OSCReceiver::Listener<OSCReceiver::RealtimeCallback>
						, public Timer
{
public:
	MeasurementLogic();
	~MeasurementLogic() override;

	void paint(juce::Graphics& g) override;
	void resized() override;
	void oscMessageReceived(const OSCMessage& message) override;
	void oscBundleReceived(const OSCBundle& bundle) override;
	void timerCallback() override;

private:
	void processOscMessage(const OSCMessage& message);
	void readTableData();
	double m_activationTime = 0.0f;
	StringArray oscMessageList;

	int m_currentMeasurement = 0;
	TextButton m_nextMeasurement{"Next"};

	MeasurementTable m_table;
	TextEditor m_lastMessage, m_logHeaderTE;
	String m_logHeader = "salte_time,trial_index,stimulus,osc_pattern,ml_time,et_az,et_el,et_rot,et_dist,et_conf,cal_status,leye_conf,reye_conf,ptr_az,ptr_el,target_az,target_el\n";
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeasurementLogic)
};
