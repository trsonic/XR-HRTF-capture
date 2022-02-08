#include "MainComponent.h"

MainComponent::MainComponent() : m_audioSetup(audioDeviceManager)
{
	addAndMakeVisible(loadSweepButton);
	loadSweepButton.addListener(this);

    addAndMakeVisible(measureButton);
    measureButton.onClick = [this]
    {
        startRecording();
    };

    addAndMakeVisible(stopButton);
	stopButton.setEnabled(false);
    stopButton.onClick = [this]
    {
        stopRecording();
    };

    addAndMakeVisible(setupButton);
    setupButton.addListener(this);
	addAndMakeVisible(m_oscTxRx);
	addAndMakeVisible(recordingThumbnail);
	addAndMakeVisible(m_analyzer);
	addAndMakeVisible(m_logic);
	m_logic.addChangeListener(this);

    audioDeviceManager.initialise(2, 2, nullptr, true, {}, nullptr);
    audioDeviceManager.addAudioCallback(&recorder);

    setSize(1000, 700);
    loadSettings();
	loadSweep(sweepFile);
	recorder.addChangeListener(this);
}

MainComponent::~MainComponent()
{
    audioDeviceManager.removeAudioCallback(&recorder);
	saveSettings();
	m_oscTxRx.disconnectTxRx();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

	g.setColour(getLookAndFeel().findColour(Label::textColourId));
	g.drawText(sweepFile.getFullPathName(), 160, 10, 630, 25, Justification::centredLeft);

}

void MainComponent::resized()
{
	loadSweepButton.setBounds(10, 10, 140, 25);
	setupButton.setBounds(10, 40, 140, 25);
	m_oscTxRx.setBounds(160, 40, 360, 60);
	measureButton.setBounds(530, 70, 140, 25);
	stopButton.setBounds(675, 70, 140, 25);
	m_analyzer.setBounds(10, 105, 510, 100);
	recordingThumbnail.setBounds(530, 105, 460, 100);

	m_logic.setBounds(10, 215, 980, 260);
}

void MainComponent::buttonClicked(Button* buttonThatWasClicked)
{
	if (buttonThatWasClicked == &loadSweepButton)
	{
		FileChooser fc("Select the sweep wav file...", sweepFile.getParentDirectory());

		if (fc.browseForFileToOpen())
		{
			sweepFile = fc.getResult();
			loadSweep(sweepFile);
		}
	}
	else if (buttonThatWasClicked == &setupButton)
	{
		addAndMakeVisible(m_audioSetup);
		m_audioSetup.m_shouldBeVisible = true;
	}

	repaint();
}

void MainComponent::changeListenerCallback(ChangeBroadcaster* source)
{
	if (source == &recorder)
	{
		if (recorder.isRecording())
		{
			measureButton.setEnabled(false);
			stopButton.setEnabled(true);
		}
		else
		{
			measureButton.setEnabled(true);
			stopButton.setEnabled(false);

			if (recorder.isRecordingFinished() && (m_logic.isMeasurementOn() || m_logic.isReferenceMeasurementOn()))
			{
				File measuredSweep = sweepFile.getParentDirectory().getChildFile(m_logic.getCurrentName());;
				lastRecording.copyFileTo(measuredSweep);
				if(m_logic.isMeasurementOn()) m_logic.nextMeasurement();
			}
		}
	}

	if (source == &m_logic)
	{
		if (m_logic.isMeasurementOn())
		{
			if (m_logic.isOrientationLocked())
				startRecording();
			else
				stopRecording();
		}
		else if (m_logic.isReferenceMeasurementOn())
		{
			stopRecording();
			startRecording();
		}
	}
}

void MainComponent::loadSweep(File file)
{
	if (file.existsAsFile())
	{
		recorder.loadSweep(file);
		recordingThumbnail.setThumbnailLength(recorder.getSweepLength());
		repaint();
	}
	else
	{
		DBG("Can't find the sweep wav file");
	}

}

void MainComponent::startRecording()
{
	if (!recorder.isRecording())
	{
		lastRecording = sweepFile.getParentDirectory().getChildFile("LR_temp_sweep.wav");
		recorder.startRecording(lastRecording);
	}
}

void MainComponent::stopRecording()
{
	if (recorder.isRecording())
	{
		recorder.stop();
		lastRecording = juce::File();
	}
}

void MainComponent::loadSettings()
{
	// audio
	String filePath = File::getSpecialLocation(File::SpecialLocationType::currentApplicationFile).getParentDirectory().getFullPathName();
	File audioSettingsFile = File(filePath + "/" + "IRCapAudioSettings.conf");
	if (audioSettingsFile.existsAsFile())
	{
		XmlDocument asxmldoc(audioSettingsFile);
		std::unique_ptr<XmlElement> audioDeviceSettings(asxmldoc.getDocumentElement());
		audioDeviceManager.initialise(0, 64, audioDeviceSettings.get(), true);
	}

	// other
	PropertiesFile::Options options;
	options.applicationName = "IRCapSettings";
	options.filenameSuffix = ".conf";
	options.osxLibrarySubFolder = "Application Support";
	options.folderName = File::getSpecialLocation(File::SpecialLocationType::currentApplicationFile).getParentDirectory().getFullPathName();
	options.storageFormat = PropertiesFile::storeAsXML;
	appSettings.setStorageParameters(options);

	if (appSettings.getUserSettings()->getBoolValue("loadSettingsFile"))
	{
		sweepFile = File(appSettings.getUserSettings()->getValue("sweepFile"));

		// osc
		m_oscTxRx.clientTxIpLabel.setText(appSettings.getUserSettings()->getValue("clientTxIp"), dontSendNotification);
		m_oscTxRx.clientTxPortLabel.setText(appSettings.getUserSettings()->getValue("clientTxPort"), dontSendNotification);
		m_oscTxRx.clientRxPortLabel.setText(appSettings.getUserSettings()->getValue("clientRxPort"), dontSendNotification);
	}
}

void MainComponent::saveSettings()
{
	// audio
	std::unique_ptr<XmlElement> audioDeviceSettings(audioDeviceManager.createStateXml());
	if (audioDeviceSettings.get())
	{
		String filePath = File::getSpecialLocation(File::SpecialLocationType::currentApplicationFile).getParentDirectory().getFullPathName();
		File audioSettingsFile = File(filePath + "/" + "IRCapAudioSettings.conf");
		audioDeviceSettings->writeTo(audioSettingsFile);
	}

	// other
	appSettings.getUserSettings()->setValue("sweepFile", sweepFile.getFullPathName());

	// osc
	appSettings.getUserSettings()->setValue("clientTxIp", m_oscTxRx.clientTxIpLabel.getText());
	appSettings.getUserSettings()->setValue("clientTxPort", m_oscTxRx.clientTxPortLabel.getText());
	appSettings.getUserSettings()->setValue("clientRxPort", m_oscTxRx.clientRxPortLabel.getText());

	appSettings.getUserSettings()->setValue("loadSettingsFile", true);
}