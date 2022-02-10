#include "MainComponent.h"

MainComponent::MainComponent() : m_audioSetup(audioDeviceManager)
{
    addAndMakeVisible(setupButton);
    setupButton.onClick = [this]
	{
		addAndMakeVisible(m_audioSetup);
		m_audioSetup.m_shouldBeVisible = true;
	};

	addAndMakeVisible(m_oscTxRx);
	addAndMakeVisible(m_recordingThumbnail);
	addAndMakeVisible(m_analyzer);
	addAndMakeVisible(m_logic);
	m_logic.addChangeListener(this);

    audioDeviceManager.initialise(2, 2, nullptr, true, {}, nullptr);
    audioDeviceManager.addAudioCallback(&m_recorder);

    setSize(1000, 700);
    loadSettings();
	m_recorder.addChangeListener(this);

	// log window
	logWindow.setMultiLine(true);
	logWindow.setReadOnly(true);
	logWindow.setCaretVisible(false);
	logWindow.setScrollbarsShown(true);
	addAndMakeVisible(logWindow);
}

MainComponent::~MainComponent()
{
    audioDeviceManager.removeAudioCallback(&m_recorder);
	saveSettings();
	m_oscTxRx.disconnectTxRx();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

	g.setColour(getLookAndFeel().findColour(Label::textColourId));

}

void MainComponent::resized()
{
	setupButton.setBounds(10, 10, 140, 60);
	m_oscTxRx.setBounds(160, 10, 360, 60);

	m_analyzer.setBounds(10, 75, 510, 130);
	m_recordingThumbnail.setBounds(530, 75, 460, 130);

	m_logic.setBounds(10, 215, 980, 260);
	logWindow.setBounds(10, 485, 980, 205);
}

void MainComponent::buttonClicked(Button* buttonThatWasClicked)
{
	//repaint();
}

void MainComponent::changeListenerCallback(ChangeBroadcaster* source)
{
	// check for debug messages
	if (source == &m_recorder)
	{
		if (m_recorder.m_currentLogMessage != "")
		{
			logWindowMessage += "Recorder: " + m_recorder.m_currentLogMessage;
			m_recorder.m_currentLogMessage.clear();
		}
	}
	else if (source == &m_oscTxRx)
	{
		if (m_oscTxRx.m_currentLogMessage != "")
		{
			logWindowMessage += "OSCTxRX: " + m_oscTxRx.m_currentLogMessage;
			m_oscTxRx.m_currentLogMessage.clear();
		}
	}
	else if (source == &m_logic)
	{
		if (m_logic.m_currentLogMessage != "")
		{
			logWindowMessage += "Logic: " + m_logic.m_currentLogMessage;
			m_logic.m_currentLogMessage.clear();
		}
	}

	logWindow.setText(logWindowMessage);
	logWindow.moveCaretToEnd();
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
		m_logic.loadSubjectFolder(File(appSettings.getUserSettings()->getValue("subjectFolder")));

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
	appSettings.getUserSettings()->setValue("subjectFolder", m_logic.getSubjectFolder().getFullPathName());

	// osc
	appSettings.getUserSettings()->setValue("clientTxIp", m_oscTxRx.clientTxIpLabel.getText());
	appSettings.getUserSettings()->setValue("clientTxPort", m_oscTxRx.clientTxPortLabel.getText());
	appSettings.getUserSettings()->setValue("clientRxPort", m_oscTxRx.clientRxPortLabel.getText());

	appSettings.getUserSettings()->setValue("loadSettingsFile", true);
}