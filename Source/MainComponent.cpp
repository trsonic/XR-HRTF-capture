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

	addAndMakeVisible(recordingThumbnail);

	// OSC labels
	clientTxIpLabel.setEditable(false, true, false);
	clientTxPortLabel.setEditable(false, true, false);
	clientRxPortLabel.setEditable(false, true, false);
	clientTxIpLabel.setText("127.0.0.1", dontSendNotification);
	clientTxPortLabel.setText("6000", dontSendNotification);
	clientRxPortLabel.setText("9000", dontSendNotification);
	clientTxIpLabel.setColour(Label::outlineColourId, Colours::black);
	clientTxPortLabel.setColour(Label::outlineColourId, Colours::black);
	clientRxPortLabel.setColour(Label::outlineColourId, Colours::black);
	clientTxIpLabel.setJustificationType(Justification::centred);
	clientTxPortLabel.setJustificationType(Justification::centred);
	clientRxPortLabel.setJustificationType(Justification::centred);
	addAndMakeVisible(clientTxIpLabel);
	addAndMakeVisible(clientTxPortLabel);
	addAndMakeVisible(clientRxPortLabel);

	connectOscButton.setButtonText("Connect OSC");
	connectOscButton.addListener(this);
	addAndMakeVisible(&connectOscButton);

	addAndMakeVisible(m_logic);

    audioDeviceManager.initialise(2, 2, nullptr, true, {}, nullptr);
    audioDeviceManager.addAudioCallback(&recorder);

    setSize(800, 600);
    loadSettings();
	loadSweep(sweepFile);
	connectOscButton.triggerClick(); // connect OSC on startup
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
	g.drawText(sweepFile.getFullPathName(), 160, 10, 630, 25, Justification::centredLeft);

	// OSC WINDOW
	juce::Rectangle<int> oscRect(250 + 140, 210 - 75, 400, 60);        // osc status / vr interface status
	g.setColour(Colours::black);
	g.drawRect(oscRect, 1);
	g.setColour(getLookAndFeel().findColour(Label::textColourId));
	g.setFont(14.0f);
	g.drawText("IP", 310 + 140, 210 - 75, 50, 25, Justification::centredLeft, true);
	g.drawText("Send to", 435 + 140, 210 - 75, 60, 25, Justification::centredLeft, true);
	g.drawText("Receive at", 490 + 140, 210 - 75, 60, 25, Justification::centredLeft, true);
	g.drawText("Client", 260 + 140, 235 - 75, 50, 25, Justification::centredLeft, true);
}

void MainComponent::resized()
{
	loadSweepButton.setBounds(10, 10, 140, 25);
    measureButton.setBounds(10, 40, 140, 25);
    stopButton.setBounds(10, 70, 140, 25);
    setupButton.setBounds(10, 100, 140, 25);
	recordingThumbnail.setBounds(160, 40, 630, 85);

	connectOscButton.setBounds(560 + 140, 220 - 75, 80, 40);
	clientTxIpLabel.setBounds(310 + 140, 235 - 75, 120, 25);
	clientTxPortLabel.setBounds(435 + 140, 235 - 75, 55, 25);
	clientRxPortLabel.setBounds(495 + 140, 235 - 75, 55, 25);

	m_logic.setBounds(10, 205, 780, 380);
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
	else if (buttonThatWasClicked == &connectOscButton)
	{
		if (!m_oscTxRx.isConnected())
		{
			// OSC sender and receiver connect
			String clientIp = clientTxIpLabel.getText();
			int clientSendToPort = clientTxPortLabel.getText().getIntValue();
			int clientReceiveAtPort = clientRxPortLabel.getText().getIntValue();
			m_oscTxRx.connectTxRx(clientIp, clientSendToPort, clientReceiveAtPort);
		}
		else
		{
			m_oscTxRx.disconnectTxRx();
		}

		if (m_oscTxRx.isConnected())
		{
			connectOscButton.setColour(TextButton::buttonColourId, Colours::green);
			connectOscButton.setButtonText("OSC connected");
		}
		else
		{
			connectOscButton.setColour(TextButton::buttonColourId, Component::findColour(TextButton::buttonColourId));
			connectOscButton.setButtonText("Connect OSC");
		}
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
		}
	}
}

void MainComponent::loadSweep(File file)
{
	recorder.loadSweep(file);
	recordingThumbnail.setThumbnailLength(recorder.getSweepLength());
	repaint();
}

void MainComponent::startRecording()
{
	lastRecording = sweepFile.getParentDirectory().getChildFile("LR_temp_sweep.wav");
    recorder.startRecording(lastRecording);
}

void MainComponent::stopRecording()
{
    recorder.stop();
    lastRecording = juce::File();
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
		clientTxIpLabel.setText(appSettings.getUserSettings()->getValue("clientTxIp"), dontSendNotification);
		clientTxPortLabel.setText(appSettings.getUserSettings()->getValue("clientTxPort"), dontSendNotification);
		clientRxPortLabel.setText(appSettings.getUserSettings()->getValue("clientRxPort"), dontSendNotification);
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
	appSettings.getUserSettings()->setValue("clientTxIp", clientTxIpLabel.getText());
	appSettings.getUserSettings()->setValue("clientTxPort", clientTxPortLabel.getText());
	appSettings.getUserSettings()->setValue("clientRxPort", clientRxPortLabel.getText());

	appSettings.getUserSettings()->setValue("loadSettingsFile", true);
}