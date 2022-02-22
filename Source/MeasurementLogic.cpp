#include "MeasurementLogic.h"

MeasurementLogic::MeasurementLogic(OscTransceiver& m_oscTxRx, AudioRecorder& m_recorder, RecordingThumbnail& m_thumbnail) : 
	currentMeasurementIndex(0), currentMeasurementType(none), oscTxRx(m_oscTxRx), recorder(m_recorder), thumbnail(m_thumbnail)
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
	m_referenceMeasurementButton.setToggleState(false, dontSendNotification);
	m_referenceMeasurementButton.setClickingTogglesState(true);
	m_referenceMeasurementButton.onClick = [this]
	{
		if (m_referenceMeasurementButton.getToggleState())
			switchMeasurementType(reference);
		else
			switchMeasurementType(none);
	};

	addAndMakeVisible(m_refCountResetButton);
	m_refCountResetButton.onClick = [this]
	{
		referenceMeasurementCount = 0;
		repaint();
	};

	addAndMakeVisible(m_hpeqMeasurementButton);
	m_hpeqMeasurementButton.setToggleState(false, dontSendNotification);
	m_hpeqMeasurementButton.setClickingTogglesState(true);
	m_hpeqMeasurementButton.onClick = [this]
	{
		if (m_hpeqMeasurementButton.getToggleState())
			switchMeasurementType(hpeq);
		else
			switchMeasurementType(none);
	};

	addAndMakeVisible(m_hpeqCountResetButton);
	m_hpeqCountResetButton.onClick = [this]
	{
		hpMeasurementCount = 0;
		repaint();
	};

	addAndMakeVisible(m_startStopButton);
	m_startStopButton.setToggleState(false, dontSendNotification);
	m_startStopButton.setClickingTogglesState(true);
	m_startStopButton.onClick = [this]
	{
		if (m_startStopButton.getToggleState())
			switchMeasurementType(hrir);
		else
			switchMeasurementType(none);
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

	switchMeasurementType(none);
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

	g.drawText("N: " + String(referenceMeasurementCount), 170, 90, 90, 25, Justification::centredLeft);
	g.drawText("N: " + String(hpMeasurementCount), 170, 120, 90, 25, Justification::centredLeft);

	int y = 180;
	g.drawText("Current speaker ID: " + String(currentMeasurementIndex), 10, 10 + y, 200, 15, Justification::centredLeft);
	g.drawText("Speaker azimuth: " + m_table.getFromXML(currentMeasurementIndex,"spkAz"), 10, 25 + y, 200, 15, Justification::centredLeft);
	g.drawText("Speaker elevation: " + m_table.getFromXML(currentMeasurementIndex, "spkEl"), 10, 40 + y, 200, 15, Justification::centredLeft);
	g.drawText("Speaker distance: " + m_table.getFromXML(currentMeasurementIndex, "spkDist"), 10, 55 + y, 200, 15, Justification::centredLeft);

	//g.drawText("Number of OSC messages: " + String(oscMessageList.size()), 220, 10, 200, 15, Justification::centredLeft);
}

void MeasurementLogic::resized()
{
	m_loadSubjectFolderButton.setBounds(10, 10, 155, 25);

	m_referenceMeasurementButton.setBounds(10, 90, 90, 25);
	m_hpeqMeasurementButton.setBounds(10, 120, 90, 25);
	m_refCountResetButton.setBounds(105, 90, 60, 25);
	m_hpeqCountResetButton.setBounds(105, 120, 60, 25);

	m_startStopButton.setBounds(10, 150, 90, 25);
	m_nextMeasurementButton.setBounds(105, 150, 60, 25);

	m_table.setBounds(200, 5, 485, 250);
	m_logHeaderTE.setBounds(680+10, 10, 85, 240);
	m_lastMessage.setBounds(680+95, 10, 195, 240);
}

void MeasurementLogic::loadSubjectFolder(File folder)
{
	// initialize
	currentMeasurementIndex = 0;
	currentMeasurementType = none;
	referenceMeasurementCount = 0;
	hpMeasurementCount = 0;
	repaint();


	sendMsgToLogWindow("Loading subject folder: " + folder.getFullPathName());

	// check if the subject folder exists
	if (!folder.exists())
	{
		sendMsgToLogWindow("The subject folder does not exist.");
		return;
	}

	// check if the LS sweep file exists
	lsSweepFile = folder.getChildFile("sweeps/LS_sweep.wav");
	if (!lsSweepFile.existsAsFile())
	{
		sendMsgToLogWindow("The sweep wav file (sweeps/LS_sweep.wav) does not exist.");
		return;
	}

	// check if the HP sweep file exists
	hpSweepFile = folder.getChildFile("sweeps/HP_sweep.wav");
	if (!lsSweepFile.existsAsFile())
	{
		sendMsgToLogWindow("The sweep wav file (sweeps/HP_sweep.wav) does not exist.");
		return;
	}

	// check if folder has speaker angles xml (not implemented yet)
	
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
		if (recorder.recordingFinished && currentMeasurementType != none)
		{
			if (currentMeasurementType == reference) referenceMeasurementCount++;
			else if (currentMeasurementType == hpeq) hpMeasurementCount++;
			File measuredSweep = lsSweepFile.getParentDirectory().getChildFile(getCurrentName());
			lastRecording.copyFileTo(measuredSweep);
			recorder.recordingFinished = false;
			sendMsgToLogWindow("Last capture saved to: " + measuredSweep.getFullPathName());
			if (currentMeasurementType == hrir) nextMeasurement();
			else switchMeasurementType(none);
			repaint();
		}
	}
}

void MeasurementLogic::switchMeasurementType(mTypes newType)
{
	currentMeasurementType = newType;

	if (currentMeasurementType == reference)
	{
		recorder.loadSweep(lsSweepFile);
		thumbnail.setThumbnailLength(recorder.getSweepLength());

		startRecording();
		m_referenceMeasurementButton.setButtonText("Stop");
		m_hpeqMeasurementButton.setEnabled(false);
		m_startStopButton.setEnabled(false);
		m_refCountResetButton.setEnabled(false);
		m_hpeqCountResetButton.setEnabled(false);
	}
	else if (currentMeasurementType == hpeq)
	{
		recorder.loadSweep(hpSweepFile);
		thumbnail.setThumbnailLength(recorder.getSweepLength());

		startRecording();
		m_hpeqMeasurementButton.setButtonText("Stop");
		m_referenceMeasurementButton.setEnabled(false);
		m_startStopButton.setEnabled(false);
		m_refCountResetButton.setEnabled(false);
		m_hpeqCountResetButton.setEnabled(false);
	}
	else if (currentMeasurementType == hrir)
	{
		recorder.loadSweep(lsSweepFile);
		thumbnail.setThumbnailLength(recorder.getSweepLength());

		m_referenceMeasurementButton.setEnabled(false);
		m_hpeqMeasurementButton.setEnabled(false);
		nextMeasurement();
		oscTxRx.sendOscMessage("/targetVis", 1);

		m_startStopButton.setButtonText("Stop");
		m_nextMeasurementButton.setEnabled(true);
		m_refCountResetButton.setEnabled(false);
		m_hpeqCountResetButton.setEnabled(false);
	}
	else if (currentMeasurementType == none)
	{
		stopRecording();
		currentMeasurementIndex = 0;
		m_table.selectMeasurementRow(0);
		oscTxRx.sendOscMessage("/targetVis", 0);

		m_referenceMeasurementButton.setButtonText("Reference");
		m_referenceMeasurementButton.setToggleState(false, dontSendNotification);
		m_referenceMeasurementButton.setEnabled(true);

		m_hpeqMeasurementButton.setButtonText("HP EQ");
		m_hpeqMeasurementButton.setToggleState(false, dontSendNotification);
		m_hpeqMeasurementButton.setEnabled(true);

		m_startStopButton.setButtonText("HRIRs");
		m_startStopButton.setToggleState(false, dontSendNotification);
		m_startStopButton.setEnabled(true);
		m_nextMeasurementButton.setEnabled(false);
		m_refCountResetButton.setEnabled(true);
		m_hpeqCountResetButton.setEnabled(true);
	}
}

void MeasurementLogic::startRecording()
{
	if (!recorder.isRecording())
	{
		lastRecording = lsSweepFile.getParentDirectory().getChildFile("LR_temp_sweep.wav");
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
	if (m_table.getNumRows() > currentMeasurementIndex)
	{
		orientationLocked = false;
		oscTxRx.sendOscMessage("/orientationLocked", 0);
		oscMessageList.clear();
		currentMeasurementIndex++;
		m_table.selectMeasurementRow(currentMeasurementIndex);
		float speakerAz = m_table.getFromXML(currentMeasurementIndex, "spkAz").getFloatValue();
		float speakerEl = m_table.getFromXML(currentMeasurementIndex, "spkEl").getFloatValue();
		float speakerDist = m_table.getFromXML(currentMeasurementIndex, "spkDist").getFloatValue();
		oscTxRx.sendOscMessage("/speaker", speakerAz, speakerEl, speakerDist);
	}
	else
	{
		switchMeasurementType(none);
	}

	repaint();
}

void MeasurementLogic::analyzeOscMsgList()
{
	int msgHistorySize = 60; // number of messages that is required to start calculating avg angle for orientation lock
	int listSize = oscMessageList.size();
	float angErrLimit = m_table.getFromXML(currentMeasurementIndex, "angErrLim").getFloatValue(); // angular error limit
	float distErrLimit = m_table.getFromXML(currentMeasurementIndex, "distErrLim").getFloatValue(); // distance error limit

	if (listSize >= msgHistorySize && orientationLocked == false)
	{
		StatisticsAccumulator<float> angAcc, distAcc;
		for (int i = 0; i < msgHistorySize; ++i)
		{
			StringArray msg;
			msg.addTokens(oscMessageList[listSize - msgHistorySize + i], ",", "\"");
			angAcc.addValue(msg[6].getFloatValue());
			distAcc.addValue(msg[8].getFloatValue());
		}
		float maxAngle = angAcc.getMaxValue();
		float maxDist = distAcc.getMaxValue();

		if (maxAngle <= angErrLimit && maxDist <= distErrLimit)
		{
			orientationLocked = true;
			oscTxRx.sendOscMessage("/orientationLocked", 1);
			oscMessageList.clear();
			sendMsgToLogWindow("Accuracy OK to start. Max angular error: " + String(maxAngle) + " deg, max distance error: " + String(maxDist) + " m");
			if (currentMeasurementType == hrir) startRecording();
		}
	}
	else if (orientationLocked == true)
	{
		StatisticsAccumulator<float> angAcc, distAcc;
		for (int i = 0; i < listSize; ++i)
		{
			StringArray msg;
			msg.addTokens(oscMessageList[i], ",", "\"");
			angAcc.addValue(msg[6].getFloatValue());
			distAcc.addValue(msg[8].getFloatValue());
		}
		float maxAngle = angAcc.getMaxValue();
		float maxDist = distAcc.getMaxValue();

		if (maxAngle > angErrLimit)
		{
			oscMessageList.clear();
			orientationLocked = false;
			oscTxRx.sendOscMessage("/orientationLocked", 0);
			sendMsgToLogWindow("Angular error to high to continue: " + String(maxAngle) + " deg");
			if (currentMeasurementType == hrir) stopRecording();
		}
		else if (maxDist > distErrLimit)
		{
			oscMessageList.clear();
			orientationLocked = false;
			oscTxRx.sendOscMessage("/orientationLocked", 0);
			sendMsgToLogWindow("Distance error to high to continue: " + String(maxDist) + " m");
			if (currentMeasurementType == hrir) stopRecording();
		}
		else
		{
			// accuracy and precision ok
			// calculate mean azimuth, elevation and distance
			SphericalOperations sphops;
			for (int i = 0; i < listSize; ++i)
			{
				StringArray msg;
				msg.addTokens(oscMessageList[i], ",", "\"");
				TransformFrame currentFrame;
				currentFrame.setSphericalData(msg[2].getIntValue(), msg[3].getFloatValue(), msg[4].getFloatValue(), msg[5].getFloatValue());
				sphops.addTransformFrame(currentFrame);
			}

			TransformFrame meanFrame = sphops.getMeanTransformFrame();
			//float maxAngle = sphops.getAngularAngle(targetFrame, meanFrame);
			meanAz = meanFrame.getAz();
			meanEl = meanFrame.getEl();
			meanDist = meanFrame.getDist();
		}
	}
}

String MeasurementLogic::getCurrentName()
{
	String filename;

	if (currentMeasurementType == reference)
	{
		filename += "00_reference_" + String(referenceMeasurementCount).paddedLeft('0', 2) + ".wav";
	}
	else if (currentMeasurementType == hpeq)
	{
		filename += "00_hptf_" + String(hpMeasurementCount).paddedLeft('0', 2) + ".wav";
	}
	else if(currentMeasurementType == hrir)
	{
		filename += String(m_table.getFromXML(currentMeasurementIndex, "ID").getTrailingIntValue()).paddedLeft('0', 2);
		filename += "_az_" + m_table.getFromXML(currentMeasurementIndex, "spkAz");
		filename += "_el_" + m_table.getFromXML(currentMeasurementIndex, "spkEl");
		filename += "_dist_" + m_table.getFromXML(currentMeasurementIndex, "spkDist");
		filename += "_maz_" + String(meanAz, 2);
		filename += "_mel_" + String(meanEl, 2);
		filename += "_mdist_" + String(meanDist, 2);
		filename += ".wav";
	}

	return filename;
}

void MeasurementLogic::sendMsgToLogWindow(String message)
{
	m_currentLogMessage += message + "\n";
	sendChangeMessage();  // broadcast change message to inform and update the editor
}