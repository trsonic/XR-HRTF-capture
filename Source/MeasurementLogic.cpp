#include "MeasurementLogic.h"

MeasurementLogic::MeasurementLogic(OscTransceiver& m_oscTxRx, AudioRecorder& m_recorder, RecordingThumbnail& m_thumbnail) : 
						m_currentMeasurement(0), oscTxRx(m_oscTxRx), recorder(m_recorder), thumbnail(m_thumbnail)
{
	oscTxRx.addListener(this);
	recorder.addChangeListener(this);

	startTimerHz(60);
	m_activationTime = Time::getMillisecondCounterHiRes();

	addAndMakeVisible(m_loadSubjectFolderButton);
	m_loadSubjectFolderButton.setEnabled(true);
	m_loadSubjectFolderButton.onClick = [this]
	{
		FileChooser fc("Select the subject data folder...", subjectFolder.getParentDirectory());

		if (fc.browseForDirectory())
		{
			loadSubjectFolder(fc.getResult());
		}
	};

	addAndMakeVisible(m_referenceMeasurementButton);
	m_referenceMeasurementButton.setEnabled(true);
	m_referenceMeasurementButton.onClick = [this]
	{
		referenceMeasurementOn = true;
		stopRecording();
		startRecording();
	};

	addAndMakeVisible(m_hpeqMeasurementButton);
	m_hpeqMeasurementButton.setEnabled(true);
	m_hpeqMeasurementButton.onClick = [this]
	{

	};

	addAndMakeVisible(m_startStopButton);
	m_startStopButton.setToggleState(false, dontSendNotification);
	m_startStopButton.setClickingTogglesState(true);
	m_startStopButton.onClick = [this]
	{
		if (m_startStopButton.getToggleState())
		{
			referenceMeasurementOn = false;
			m_startStopButton.setButtonText("Stop");
			oscTxRx.sendOscMessage("/targetVis", 1);
			nextMeasurement();
			m_nextMeasurementButton.setEnabled(true);
			m_referenceMeasurementButton.setEnabled(false);
			m_hpeqMeasurementButton.setEnabled(false);
		}
		else
		{
			m_startStopButton.setButtonText("Start");
			m_nextMeasurementButton.setEnabled(false);
			m_currentMeasurement = 0;
			m_table.selectMeasurementRow(0);
			oscTxRx.sendOscMessage("/targetVis", 0);
			m_referenceMeasurementButton.setEnabled(true);
			m_hpeqMeasurementButton.setEnabled(true);
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

	g.setColour(getLookAndFeel().findColour(Label::textColourId));
	int y = 180;
	g.drawText("Current speaker ID: " + String(m_currentMeasurement), 10, 10 + y, 200, 15, Justification::centredLeft);
	g.drawText("Speaker azimuth: " + m_table.getFromXML(m_currentMeasurement,"spkAz"), 10, 25 + y, 200, 15, Justification::centredLeft);
	g.drawText("Speaker elevation: " + m_table.getFromXML(m_currentMeasurement, "spkEl"), 10, 40 + y, 200, 15, Justification::centredLeft);
	g.drawText("Speaker distance: " + m_table.getFromXML(m_currentMeasurement, "spkDist"), 10, 55 + y, 200, 15, Justification::centredLeft);

	//g.drawText("Number of OSC messages: " + String(oscMessageList.size()), 220, 10, 200, 15, Justification::centredLeft);
}

void MeasurementLogic::resized()
{
	m_loadSubjectFolderButton.setBounds(10, 10, 140, 25);

	m_referenceMeasurementButton.setBounds(10, 90, 90, 25);
	m_hpeqMeasurementButton.setBounds(10, 120, 90, 25);

	m_startStopButton.setBounds(10, 150, 65, 25);
	m_nextMeasurementButton.setBounds(80, 150, 65, 25);

	m_table.setBounds(200, 5, 485, 250);
	m_logHeaderTE.setBounds(680+10, 10, 85, 240);
	m_lastMessage.setBounds(680+95, 10, 195, 240);
}

void MeasurementLogic::loadSubjectFolder(File folder)
{
	sendMsgToLogWindow("Loading subject folder: " + folder.getFullPathName());

	// check if the sweep file exists
	sweepFile = folder.getChildFile("sweeps/ZZ_sweep.wav");
	if (sweepFile.existsAsFile())
	{
		recorder.loadSweep(sweepFile);
		thumbnail.setThumbnailLength(recorder.getSweepLength());
	}
	else
	{
		sendMsgToLogWindow("The sweep wav file (sweeps/ZZ_sweep.wav) does not exist.");
		//initialize
		return;
	}

	// if folder has speaker angles xml

	subjectFolder = folder;


}

File MeasurementLogic::getSubjectFolder()
{
	return subjectFolder;
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
void MeasurementLogic::changeListenerCallback(ChangeBroadcaster* source)
{
	if (source == &recorder)
	{
		if (recorder.isRecordingFinished() && (isMeasurementOn() || referenceMeasurementOn))
		{
			File measuredSweep = sweepFile.getParentDirectory().getChildFile(getCurrentName());
			lastRecording.copyFileTo(measuredSweep);
			sendMsgToLogWindow("Saved last capture to: " + measuredSweep.getFullPathName());
			if (isMeasurementOn()) nextMeasurement();
		}
	}
}

void MeasurementLogic::startRecording()
{
	if (!recorder.isRecording())
	{
		lastRecording = sweepFile.getParentDirectory().getChildFile("LR_temp_sweep.wav");
		recorder.startRecording(lastRecording);
	}
}

void MeasurementLogic::stopRecording()
{
	if (recorder.isRecording())
	{
		recorder.stop();
		lastRecording = juce::File();
	}
}

void MeasurementLogic::nextMeasurement()
{
	if (m_table.getNumRows() > m_currentMeasurement)
	{
		orientationLocked = false;
		oscTxRx.sendOscMessage("/orientationLocked", 0);
		sendChangeMessage();
		oscMessageList.clear();
		m_currentMeasurement++;
		m_table.selectMeasurementRow(m_currentMeasurement);
		float speakerAz = m_table.getFromXML(m_currentMeasurement, "spkAz").getFloatValue();
		float speakerEl = m_table.getFromXML(m_currentMeasurement, "spkEl").getFloatValue();
		float speakerDist = m_table.getFromXML(m_currentMeasurement, "spkDist").getFloatValue();
		oscTxRx.sendOscMessage("/speaker", speakerAz, speakerEl, speakerDist);
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
			oscTxRx.sendOscMessage("/orientationLocked", 1);
			oscMessageList.clear();
			sendMsgToLogWindow("Accuracy OK to start: " + String(angAvg));
			if (isMeasurementOn()) startRecording();
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
			oscTxRx.sendOscMessage("/orientationLocked", 0);
			sendChangeMessage();
			sendMsgToLogWindow("Not enough precision to continue: " + String(angStd));
			if (isMeasurementOn()) stopRecording();
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
			meanDist = dist.getAverage();
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

void MeasurementLogic::sendMsgToLogWindow(String message)
{
	m_currentLogMessage += message + "\n";
	sendChangeMessage();  // broadcast change message to inform and update the editor
}