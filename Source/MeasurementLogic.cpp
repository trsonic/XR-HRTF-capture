#include "MeasurementLogic.h"

MeasurementLogic::MeasurementLogic(OscTransceiver& oscTxRx) : m_currentMeasurement(0)
															, m_oscTxRx(oscTxRx)
{
	m_oscTxRx.addListener(this);

	startTimerHz(60);

	addAndMakeVisible(m_startStopButton);
	m_startStopButton.setToggleState(false, dontSendNotification);
	m_startStopButton.setClickingTogglesState(true);
	m_startStopButton.onClick = [this]
	{
		if (m_startStopButton.getToggleState())
		{
			m_startStopButton.setButtonText("Stop");
			m_oscTxRx.sendOscMessage("/targetVis", 1);
			nextMeasurement();
			m_nextMeasurementButton.setEnabled(true);

		}
		else
		{
			m_startStopButton.setButtonText("Start");
			m_nextMeasurementButton.setEnabled(false);
			m_currentMeasurement = 0;
			m_table.selectMeasurementRow(0);
			m_oscTxRx.sendOscMessage("/targetVis", 0);
		}
	};

	addAndMakeVisible(m_nextMeasurementButton);
	m_nextMeasurementButton.setEnabled(false);
	m_nextMeasurementButton.onClick = [this]
	{
		nextMeasurement();
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
}

void MeasurementLogic::resized()
{
	m_startStopButton.setBounds(10, 90, 60, 25);
	m_nextMeasurementButton.setBounds(80, 90, 60, 25);
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
	if (message.getAddressPattern().toString() == "/currentAzEl")
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
		String messageText = String((int)time) + "," + "blabla" + ",";
		messageText += "no stimulus present," + message.getAddressPattern().toString() + arguments + "\n";


		oscMessageList.add(messageText);
	}

	//if (message.getAddressPattern().toString() == "/control")
	//{
	//	// control the test component
	//	if (message[0].isString() && message[0].getString() == "confirm")
	//	{
	//		//m_nextTrial.triggerClick(); //changeTrial(m_currentTrialIndex + 1);
	//	}
	//}
}

void MeasurementLogic::timerCallback()
{
	//m_messageCounter.setText(String(oscMessageList.size()), dontSendNotification);

	if (oscMessageList.size() > 0)
	{
		//m_saveLogButton.setEnabled(true);

		String lastMsg = oscMessageList[oscMessageList.size() - 1];
		lastMsg = lastMsg.replace(",", "\n");
		m_lastMessage.setText(lastMsg, dontSendNotification);
	}
	else
	{
		//m_saveLogButton.setEnabled(false);
		m_lastMessage.setText("", dontSendNotification);
	}
}

void MeasurementLogic::nextMeasurement()
{
	if (m_table.getNumRows() > m_currentMeasurement)
	{
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