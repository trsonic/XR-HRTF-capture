#include "OscTransceiver.h"

OscTransceiver::OscTransceiver()
{
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
	connectOscButton.onClick = [this]
	{
		if (!isConnected())
		{
			// OSC sender and receiver connect
			String clientIp = clientTxIpLabel.getText();
			int clientSendToPort = clientTxPortLabel.getText().getIntValue();
			int clientReceiveAtPort = clientRxPortLabel.getText().getIntValue();
			connectTxRx(clientIp, clientSendToPort, clientReceiveAtPort);

			auto addresses = IPAddress::getAllAddresses(false);
			String localIpAddress = "127.0.0.1";

			for (auto& a : addresses)
			{
				if (a.toString().contains("192.168"))
				{
					localIpAddress = a.toString();
				}
			}
			sendMsgToLogWindow("Local IP: " + localIpAddress);
			sendOscMessage("/rendererIp", (String)localIpAddress);
		}
		else
		{
			disconnectTxRx();
		}

		if (isConnected())
		{
			connectOscButton.setColour(TextButton::buttonColourId, Colours::green);
			connectOscButton.setButtonText("OSC connected");
		}
		else
		{
			connectOscButton.setColour(TextButton::buttonColourId, Component::findColour(TextButton::buttonColourId));
			connectOscButton.setButtonText("Connect OSC");
		}
	};

	addAndMakeVisible(&connectOscButton);
	//connectOscButton.triggerClick(); // connect OSC on startup
}

OscTransceiver::~OscTransceiver()
{
	disconnectTxRx();
}

void OscTransceiver::paint(juce::Graphics& g)
{
	g.setColour(Colours::black);
	g.drawRect(getLocalBounds(), 1);

	g.setColour(getLookAndFeel().findColour(Label::textColourId));
	g.setFont(14.0f);
	g.drawText("HMD IP", 0, 50, 25, Justification::centredLeft, true);
	g.drawText("Send to", 135, 0, 60, 25, Justification::centredLeft, true);
	g.drawText("Receive at", 190, 0, 60, 25, Justification::centredLeft, true);
}

void OscTransceiver::resized()
{
	connectOscButton.setBounds(260, 10, 80, 40);
	clientTxIpLabel.setBounds(10, 25, 120, 25);
	clientTxPortLabel.setBounds(135, 25, 55, 25);
	clientRxPortLabel.setBounds(195, 25, 55, 25);
}

void OscTransceiver::connectTxRx(String ipToSendTo, int portToSendTo, int portToReceiveAt)
{
	if (!sender.connect(ipToSendTo, portToSendTo))
		showConnectionErrorMessage("Error: could not connect sender to UDP port: " + String(portToSendTo));
	else
		isSenderConnected = true;

	if (!connect(portToReceiveAt))
		showConnectionErrorMessage("Error: could not connect receiver to UDP port " + String(portToReceiveAt));
	else
		isReceiverConnected = true;
}

void OscTransceiver::disconnectTxRx()
{
	if (!sender.disconnect())
		showConnectionErrorMessage("Error: could not disconnect sender from the currently used UDP port");
	else
		isSenderConnected = false;

	if (!disconnect())
		showConnectionErrorMessage("Error: could not disconnect receiver from the currently used UDP port.");
	else
		isReceiverConnected = false;
}

bool OscTransceiver::isConnected()
{
	if (isSenderConnected && isReceiverConnected)
		return true;
	else
		return false;
}

void OscTransceiver::showConnectionErrorMessage(const String& messageText)
{
	AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
		"Connection error",
		messageText,
		"OK");
}

void OscTransceiver::sendMsgToLogWindow(String message)
{
	m_currentLogMessage += message + "\n";
	sendChangeMessage();  // broadcast change message to inform and update the editor
}
