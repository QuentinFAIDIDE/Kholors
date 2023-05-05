#include "MainComponent.h"

#include <cstdlib>
#include <memory>

#include "EmptyTab.h"
#include "RobotoFont.h"

//==============================================================================
MainComponent::MainComponent()
    : mixingBus(notificationArea, activityManager), arrangementArea(mixingBus, notificationArea, activityManager),
      actionTabs(juce::TabbedButtonBar::Orientation::TabsAtTop)
{
    configureLookAndFeel();

    activityManager.registerTaskListener(&notificationArea);
    activityManager.registerTaskListener(&mixingBus);
    activityManager.registerTaskListener(&arrangementArea);

    // initialize audio app with two outputs
    setAudioChannels(0, 2);

    printAudioDeviceSettings();

    // set size of the component
    setSize(1400, 800);

    actionTabs.addTab("Audio Library", juce::Colour(25, 24, 24), &audioLibraryTab, false);
    actionTabs.addTab("Sample Processing", juce::Colour(25, 24, 24), &sampleProcessingTab, false);
    actionTabs.addTab("Mastering", juce::Colour(25, 24, 24), &masteringTab, false);

    // tells the sample player where to report file import for count
    mixingBus.setFileImportedCallback(audioLibraryTab.fileWasImported);

    // make subwidgets visible
    addAndMakeVisible(arrangementArea);
    addAndMakeVisible(notificationArea);
    addAndMakeVisible(actionTabs);

    // set the mixingBus callback to repaint arrangement area
    mixingBus.setTrackRepaintCallback([this] {
        const juce::MessageManagerLock mmLock;
        arrangementArea.repaint();
    });
}

void MainComponent::printAudioDeviceSettings()
{
    // print audio device settings
    std::cerr << "Input Device: " << deviceManager.getAudioDeviceSetup().inputDeviceName << std::endl;
    std::cerr << "Output Device: " << deviceManager.getAudioDeviceSetup().outputDeviceName << std::endl;
    std::cerr << "Buffer size: " << deviceManager.getAudioDeviceSetup().bufferSize << std::endl;
    std::cerr << "Sample Rate: " << deviceManager.getAudioDeviceSetup().sampleRate << std::endl;
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
    // shutdown the audio
    shutdownAudio();
}

void MainComponent::configureLookAndFeel()
{
    // create Roboto font
    juce::Typeface::Ptr tface =
        juce::Typeface::createSystemTypefaceFor(RobotoFont::RobotoRegular_ttf, RobotoFont::RobotoRegular_ttfSize);
    // set roboto as default font
    appLookAndFeel.setDefaultSansSerifTypeface(tface);
    setLookAndFeel(&appLookAndFeel);
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g)
{
    // draw background over bounds
    // paint the background of the area
    g.setColour(COLOR_APP_BACKGROUND);
    g.fillAll();
}

void MainComponent::resized()
{
    // get window coordinations
    juce::Rectangle<int> localBounds = getLocalBounds();
    // set notification area
    localBounds.setX(0);
    localBounds.setY(0);
    localBounds.setHeight(NOTIF_HEIGHT + NOTIF_OUTTER_MARGINS + NOTIF_OUTTER_MARGINS);
    notificationArea.setBounds(localBounds);
    // set arrangement area to a FREQTIME_VIEW_HEIGHT band at
    // middle of the screen
    localBounds.setX(0);
    localBounds.setY(NOTIF_HEIGHT + NOTIF_OUTTER_MARGINS + NOTIF_OUTTER_MARGINS);
    localBounds.setHeight(FREQTIME_VIEW_HEIGHT);
    arrangementArea.setBounds(localBounds);

    int y = NOTIF_HEIGHT + NOTIF_OUTTER_MARGINS + NOTIF_OUTTER_MARGINS + FREQTIME_VIEW_HEIGHT + 6;
    localBounds = getLocalBounds();
    localBounds.setY(y);
    localBounds.setHeight(localBounds.getHeight() - y - 6);
    actionTabs.setBounds(localBounds);
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    // pass that callback down to the sample mananger
    mixingBus.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::releaseResources()
{
    // pass that callback down to the sample mananger
    mixingBus.releaseResources();
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill)
{
    // pass that callback down to the sample mananger
    mixingBus.getNextAudioBlock(bufferToFill);
}

void MainComponent::configureApp(Config &conf)
{
    audioLibraryTab.initAudioLibrary(conf);

    if (conf.getBufferSize() != 0)
    {
        auto oldSetup = deviceManager.getAudioDeviceSetup();
        oldSetup.bufferSize = conf.getBufferSize();

        deviceManager.setAudioDeviceSetup(oldSetup, true);

        std::cerr << "Audio Device Settings updated" << std::endl;
        printAudioDeviceSettings();
    }
}