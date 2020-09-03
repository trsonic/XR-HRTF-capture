#include "MainComponent.h"

MainComponent::MainComponent() : m_audioSetup(audioDeviceManager)
{
	addAndMakeVisible(loadSweepButton);
	loadSweepButton.onClick = [this]
	{
		FileChooser fc("Select the sweep wav file...", sweepFile.getParentDirectory());

		if (fc.browseForFileToOpen())
		{
			sweepFile = fc.getResult();
			loadSweep(sweepFile);
		}
	};

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
    setupButton.onClick = [this]
    {
        addAndMakeVisible(m_audioSetup);
        m_audioSetup.m_shouldBeVisible = true;
    };

	addAndMakeVisible(recordingThumbnail);

    audioDeviceManager.initialise(2, 2, nullptr, true, {}, nullptr);
    audioDeviceManager.addAudioCallback(&recorder);

    setSize(800, 600);
    loadSettings();
	loadSweep(sweepFile);

	recorder.addChangeListener(this);
}

MainComponent::~MainComponent()
{
    audioDeviceManager.removeAudioCallback(&recorder);
	saveSettings();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
	g.drawText(sweepFile.getFullPathName(), 160, 10, 630, 25, Justification::centredLeft);
}

void MainComponent::resized()
{
	loadSweepButton.setBounds(10, 10, 140, 25);
    measureButton.setBounds(10, 40, 140, 25);
    stopButton.setBounds(10, 70, 140, 25);
    setupButton.setBounds(10, 100, 140, 25);
	recordingThumbnail.setBounds(160, 40, 630, 85);
}

void MainComponent::loadSweep(File file)
{
	recorder.loadSweep(file);
	recordingThumbnail.setThumbnailLength(recorder.getSweepLength());
	repaint();
}

void MainComponent::startRecording()
{
    lastRecording = File("C:/TR_FILES/local_repositories/XR-HRTF-processing/sweeps/LR_temp_sweep.wav");
    recorder.startRecording(lastRecording);
}

void MainComponent::stopRecording()
{
    recorder.stop();
    lastRecording = juce::File();
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
		//clientTxIpLabel.setText(appSettings.getUserSettings()->getValue("clientTxIp"), dontSendNotification);
		//clientTxPortLabel.setText(appSettings.getUserSettings()->getValue("clientTxPort"), dontSendNotification);
		//clientRxPortLabel.setText(appSettings.getUserSettings()->getValue("clientRxPort"), dontSendNotification);
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
	//appSettings.getUserSettings()->setValue("clientTxIp", clientTxIpLabel.getText());
	//appSettings.getUserSettings()->setValue("clientTxPort", clientTxPortLabel.getText());
	//appSettings.getUserSettings()->setValue("clientRxPort", clientRxPortLabel.getText());

	appSettings.getUserSettings()->setValue("loadSettingsFile", true);
}