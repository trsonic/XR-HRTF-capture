#include "MeasurementLogic.h"

MeasurementLogic::MeasurementLogic(OscTransceiver& oscTxRx) : m_currentMeasurement(0)
															, m_oscTxRx(oscTxRx)
{
	m_oscTxRx.addListener(this);

	startTimerHz(60);
	m_activationTime = Time::getMillisecondCounterHiRes();

	addAndMakeVisible(m_startStopButton);
	m_startStopButton.setToggleState(false, dontSendNotification);
	m_startStopButton.setClickingTogglesState(true);
	m_startStopButton.onClick = [this]
	{
		if (m_startStopButton.getToggleState())
		{
			referenceMeasurementOn = false;
			m_startStopButton.setButtonText("Stop");
			m_oscTxRx.sendOscMessage("/targetVis", 1);
			nextMeasurement();
			m_nextMeasurementButton.setEnabled(true);
			m_referenceMeasurementButton.setEnabled(false);
		}
		else
		{
			m_startStopButton.setButtonText("Start");
			m_nextMeasurementButton.setEnabled(false);
			m_currentMeasurement = 0;
			m_table.selectMeasurementRow(0);
			m_oscTxRx.sendOscMessage("/targetVis", 0);
			m_referenceMeasurementButton.setEnabled(true);
		}
	};

	addAndMakeVisible(m_nextMeasurementButton);
	m_nextMeasurementButton.setEnabled(false);
	m_nextMeasurementButton.onClick = [this]
	{
		nextMeasurement();
	};

	addAndMakeVisible(m_referenceMeasurementButton);
	m_referenceMeasurementButton.setEnabled(true);
	m_referenceMeasurementButton.onClick = [this]
	{
		referenceMeasurementOn = true;
		sendChangeMessage();
	};

	addAndMakeVisible(m_table);

	m_lastMessage.setColour(Label::outlineColourId, Colours::black);
	m_lastMessage.setMultiLine(true, false);
	m_lastMessage.setReadOnly(true);
	m_lastMessage.setCaretVisible(false);
	m_lastMessage.setScrollbarsShown(false);
	addAndMakeVisible(m_lastMessage);

	m_logHeaderTE.setColour(Label::outlineColourId, Colours::black);
	m_logHeaderTE.setMultiLine(true, false);
	m_logHeaderTE.setReadOnly(true);
	m_logHeaderTE.setCaretVisible(false);
	m_logHeaderTE.setScrollbarsShown(false);
	m_logHeaderTE.setText(m_logHeader.replace(",", "\n"), dontSendNotification);
	addAndMakeVisible(m_logHeaderTE);
}

MeasurementLogic::~MeasurementLogic()
{

}

void MeasurementLogic::paint(juce::Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

	g.setColour(Colours::black);
	g.drawRect(getLocalBounds(), 1);

	g.drawText("Current ID: " + String(m_currentMeasurement), 10, 10, 200, 15, Justification::centredLeft);
	g.drawText("Speaker azimuth: " + m_table.getFromXML(m_currentMeasurement,"spkAz"), 10, 25, 200, 15, Justification::centredLeft);
	g.drawText("Speaker elevation: " + m_table.getFromXML(m_currentMeasurement, "spkEl"), 10, 40, 200, 15, Justification::centredLeft);
	g.drawText("Speaker distance: " + m_table.getFromXML(m_currentMeasurement, "spkDist"), 10, 55, 200, 15, Justification::centredLeft);

	//g.drawText("Number of OSC messages: " + String(oscMessageList.size()), 220, 10, 200, 15, Justification::centredLeft);
}

void MeasurementLogic::resized()
{
	m_startStopButton.setBounds(10, 90, 60, 25);
	m_nextMeasurementButton.setBounds(80, 90, 60, 25);
	m_referenceMeasurementButton.setBounds(150, 90, 90, 25);
	m_table.setBounds(5, 120, 480, 250);
	m_logHeaderTE.setBounds(340+150, 120, 85, 250);
	m_lastMessage.setBounds(425+150, 120, 195, 250);
}

void MeasurementLogic::oscMessageReceived(const OSCMessage& message)
{
	processOscMessage(message);
}

void MeasurementLogic::oscBundleReceived(const OSCBundle& bundle)
{
	OSCBundle::Element elem = bundle[0];
	processOscMessage(elem.getMessage());
}

void MeasurementLogic::processOscMessage(const OSCMessage& message)
{
	if (message.getAddressPattern().toString() == "/headOrientation")
	{
		// add message to the message list
		String arguments;
		for (int i = 0; i < message.size(); ++i)
		{
			if (message[i].isString()) arguments += "," + message[i].getString();
			else if (message[i].isFloat32()) arguments += "," + String(message[i].getFloat32());
			else if (message[i].isInt32()) arguments += "," + String(message[i].getInt32());
		}

		double time = Time::getMillisecondCounterHiRes() - m_activationTime;
		String messageText = String((int)time) + ",";
		messageText += message.getAddressPattern().toString() + arguments + "\n";

		oscMessageList.add(messageText);
	}
}

void MeasurementLogic::timerCallback()
{
	if (oscMessageList.size() > 0)
	{
		String lastMsg = oscMessageList[oscMessageList.size() - 1];
		lastMsg = lastMsg.replace(",", "\n");
		m_lastMessage.setText(lastMsg, dontSendNotification);

		analyzeOscMsgList();
	}
	else
	{
		m_lastMessage.setText("", dontSendNotification);
	}

}

void MeasurementLogic::nextMeasurement()
{
	if (m_table.getNumRows() > m_currentMeasurement)
	{
		orientationLocked = false;
		m_oscTxRx.sendOscMessage("/orientationLocked", 0);
		sendChangeMessage();
		oscMessageList.clear();
		m_currentMeasurement++;
		m_table.selectMeasurementRow(m_currentMeasurement);
		float speakerAz = m_table.getFromXML(m_currentMeasurement, "spkAz").getFloatValue();
		float speakerEl = m_table.getFromXML(m_currentMeasurement, "spkEl").getFloatValue();
		float speakerDist = m_table.getFromXML(m_currentMeasurement, "spkDist").getFloatValue();
		m_oscTxRx.sendOscMessage("/speaker", speakerAz, speakerEl, speakerDist);
	}
	else
	{
		m_startStopButton.setToggleState(false, sendNotification);
	}

	repaint();
}

void MeasurementLogic::analyzeOscMsgList()
{
	int msgHistorySize = 60; // number of messages that is required to start calculating avg angle for orientation lock
	int listSize = oscMessageList.size();
	float angAccLimit = m_table.getFromXML(m_currentMeasurement, "angAcc").getFloatValue(); // accuracy condition to start the measurement
	float angPrecLimit = m_table.getFromXML(m_currentMeasurement, "angPrec").getFloatValue(); // precision condition to complete the measurement

	if (listSize >= msgHistorySize && orientationLocked == false)
	{
		StatisticsAccumulator<float> angAcc;
		for (int i = 0; i < msgHistorySize; ++i)
		{
			StringArray msg;
			msg.addTokens(oscMessageList[listSize - msgHistorySize + i], ",", "\"");
			angAcc.addValue(msg[6].getFloatValue());
		}
		float angAvg = angAcc.getAverage();

		if (angAvg <= angAccLimit)
		{
			orientationLocked = true;
			m_oscTxRx.sendOscMessage("/orientationLocked", 1);
			oscMessageList.clear();
			sendChangeMessage();
			DBG("Accuracy OK to start: " + String(angAvg));
		}
	}
	else if (orientationLocked == true)
	{
		StatisticsAccumulator<float> angAcc;
		for (int i = 0; i < listSize; ++i)
		{
			StringArray msg;
			msg.addTokens(oscMessageList[i], ",", "\"");
			angAcc.addValue(msg[6].getFloatValue());
		}
		float angStd = angAcc.getStandardDeviation();

		if (angStd > angPrecLimit)
		{
			oscMessageList.clear();
			orientationLocked = false;
			m_oscTxRx.sendOscMessage("/orientationLocked", 0);
			sendChangeMessage();
			DBG("Not enough precision to continue:" + String(angStd));
		}
		else
		{
			// accuracy and precision ok
			// calculate mean azimuth, elevation and distance
			StatisticsAccumulator<float> azi, ele, dist;
			for (int i = 0; i < listSize; ++i)
			{
				StringArray msg;
				msg.addTokens(oscMessageList[i], ",", "\"");
				//azi.addValue(msg[3].getFloatValue()); this should be averaged in cartesian or using circstats
				//ele.addValue(msg[4].getFloatValue());
				dist.addValue(msg[5].getFloatValue());

			}
			meanDist = angAcc.getAverage();
		}
	}
}

String MeasurementLogic::getCurrentName()
{
	String filename;

	if (referenceMeasurementOn)
	{
		filename += "00_reference.wav";
	}
	else
	{
		filename += String(m_table.getFromXML(m_currentMeasurement, "ID").getTrailingIntValue()).paddedLeft('0', 2);
		filename += "_azi_" + m_table.getFromXML(m_currentMeasurement, "spkAz");
		filename += "_ele_" + m_table.getFromXML(m_currentMeasurement, "spkEl");
		filename += "_dist_" + String(meanDist,2);
		filename += ".wav";
	}

	return filename;
}