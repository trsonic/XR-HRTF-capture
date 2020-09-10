#include "MeasurementLogic.h"

MeasurementLogic::MeasurementLogic()
{
	startTimerHz(60);

	addAndMakeVisible(m_nextMeasurement);
	m_nextMeasurement.onClick = [this]
	{
		m_currentMeasurement++;
		repaint();
	};

	addAndMakeVisible(m_table);
	if (m_table.getNumRows() > 0)
	{
		m_currentMeasurement = 1;
	}

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

	g.drawText("Current ID: " + String(m_currentMeasurement), 10, 30, 200, 25, Justification::centredLeft);
	g.drawText("Target azimuth: " + m_table.getFromXML(m_currentMeasurement,"targetAz"), 10, 60, 200, 25, Justification::centredLeft);
	g.drawText("Target elevation: " + m_table.getFromXML(m_currentMeasurement, "targetEl"), 10, 90, 200, 25, Justification::centredLeft);
}

void MeasurementLogic::resized()
{
	m_nextMeasurement.setBounds(5, 5, 60, 25);
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
	if (message.getAddressPattern().toString() == "/etptr")
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

	if (message.getAddressPattern().toString() == "/control")
	{
		// control the test component
		if (message[0].isString() && message[0].getString() == "confirm")
		{
			//m_nextTrial.triggerClick(); //changeTrial(m_currentTrialIndex + 1);
		}
	}
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

void MeasurementLogic::readTableData()
{
	//m_table.getText(2, 1);
}