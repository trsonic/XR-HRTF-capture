#include "MeasurementLogic.h"

MeasurementLogic::MeasurementLogic()
{
	startTimerHz(60);

	loadConfig();

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
}

void MeasurementLogic::resized()
{
	m_table.setBounds(5, 5, 480, 370);
	m_logHeaderTE.setBounds(340+150, 10, 85, 285);
	m_lastMessage.setBounds(425+150, 10, 195, 285);
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

void MeasurementLogic::loadConfig()
{
	//auto dir = juce::File::getCurrentWorkingDirectory();

	//int numTries = 0;

	//while (!dir.getChildFile("Resources").exists() && numTries++ < 15)
	//	dir = dir.getParentDirectory();

	//auto tableFile = dir.getChildFile("Resources").getChildFile("target_angles.csv");

	//if (tableFile.exists())
	//{
	//	StringArray loadedData;
	//	loadedData.clear();
	//	loadedData.addLines(tableFile.loadFileAsString());
	//	if (loadedData[0].startsWith("measId,targetAz,targetEl,angDev,targetDist,distDev"))
	//	{
	//		m_hrtfList.clear(true);
	//		for (int i = 1; i < loadedData.size(); i++)
	//		{
	//			StringArray tokens;
	//			m_hrtfList.add(new HrtfMeasurement);
	//			tokens.addTokens(loadedData[i], ",", "\"");

	//			m_hrtfList.getLast()->setTarget(
	//				tokens[0].getIntValue()
	//				, tokens[1].getFloatValue()
	//				, tokens[2].getFloatValue()
	//				, tokens[3].getFloatValue()
	//				, tokens[4].getFloatValue()
	//				, tokens[5].getFloatValue()
	//			);
	//		}

	//	}
	//	else
	//	{
	//		AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, "Invalid config file:\n", tableFile.getFullPathName());
	//	}
	//}
}

void MeasurementLogic::updateTable()
{

}