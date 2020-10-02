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
	g.drawText("Speaker azimuth: " + m_table.getFromXML(m_currentMeasurement,"speakerAz"), 10, 25, 200, 15, Justification::centredLeft);
	g.drawText("Speaker elevation: " + m_table.getFromXML(m_currentMeasurement, "speakerEl"), 10, 40, 200, 15, Justification::centredLeft);
	g.drawText("Speaker distance: " + m_table.getFromXML(m_currentMeasurement, "speakerDist"), 10, 55, 200, 15, Justification::centredLeft);

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
		sendChangeMessage();
		oscMessageList.clear();
		m_currentMeasurement++;
		m_table.selectMeasurementRow(m_currentMeasurement);
		float speakerAz = m_table.getFromXML(m_currentMeasurement, "speakerAz").getFloatValue();
		float speakerEl = m_table.getFromXML(m_currentMeasurement, "speakerEl").getFloatValue();
		float speakerDist = m_table.getFromXML(m_currentMeasurement, "speakerDist").getFloatValue();
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
	int oscBufferLength = 60;
	int listSize = oscMessageList.size();
	if (listSize >= oscBufferLength)
	{
		Array<float> ang;
		for (int i = 0; i < oscBufferLength; ++i)
		{
			StringArray msg;
			msg.addTokens(oscMessageList[listSize - oscBufferLength + i], ",", "\"");
			ang.set(i, msg[5].getFloatValue());
		}

		float angAvg = 0.0f;
		for (int i = 0; i < ang.size(); ++i)
		{
			angAvg += ang[i];
		}
		angAvg /= ang.size();

		if (angAvg <= 2.0f && orientationLocked == false)
		{
			orientationLocked = true;
			sendChangeMessage();
			DBG("OK! - " + String(angAvg));
		}
		else if (angAvg > 2.0f && orientationLocked == true)
		{
			oscMessageList.clear();
			orientationLocked = false;
			sendChangeMessage();
			DBG("NO! - " + String(angAvg));
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
		filename += m_table.getFromXML(m_currentMeasurement, "ID");
		filename += "_azi_" + m_table.getFromXML(m_currentMeasurement, "speakerAz");
		filename += "_ele_" + m_table.getFromXML(m_currentMeasurement, "speakerEl");
		filename += ".wav";
	}

	return filename;
}