# XR-based HRTF Measurement Control App
This app is designed to be used as a part of the [XR-based HRTF Measurement System](https://trsonic.github.io/XR-HRTFs/).

## Supported Hardware
* Tested on Windows with RME Fireface UC using ASIO Drivers.
* Other platforms/interfaces should work, altough haven't been tested.

## Getting Started
* Installation
    * Compile from source:
        * Get JUCE.
        * ASIO SDK header files can be obtained from [Steinberg](https://www.steinberg.net/developers/).
    * Or download the latest executable from the [Releases](../../releases/latest) page.
* Usage
    * Make sure that the Quest is on the same local network as the PC and its running the [XR Interface App](https://github.com/trsonic/XR-HRTF-Q2/).
    * Input the IP address displayed by the XR app and click `Connect OSC`.
    * Load a Subject Folder. A template Subject Folder named `YYYYMMDD-XR-SUBJ000` can be found in [Resources](/Resources/).
    * Adjust Output Level:
        * Start with a low value eg. -40 dB and gradually increase to a considerable level.
        * Test sweeps can be triggered by clicking `Reference` and `HP EQ` buttons.
    * The app is ready to run measurements.