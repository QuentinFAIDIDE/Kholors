#include "MainComponent.h"

#include <cstdlib>
#include <memory>

#include "RobotoFont.h"

//==============================================================================
MainComponent::MainComponent()
    : notificationArea(activityManager), mixingBus(activityManager), arrangementArea(mixingBus, activityManager),
      actionTabs(juce::TabbedButtonBar::Orientation::TabsAtTop)
{

    arrangementAreaHeight = FREQTIME_VIEW_HEIGHT;

    configureLookAndFeel();

    activityManager.registerTaskListener(&mixingBus);
    activityManager.registerTaskListener(&arrangementArea);
    activityManager.registerTaskListener(&notificationArea);
    activityManager.registerTaskListener(&audioLibraryTab);

    // initialize audio app with two outputs
    setAudioChannels(0, 2);

    printAudioDeviceSettings();

    // set size of the component
    setSize(1400, 800);

    actionTabs.addTab("Audio Library", COLOR_BACKGROUND, &audioLibraryTab, false);
    actionTabs.addTab("Generators", COLOR_BACKGROUND, &generatorsTab, false);
    actionTabs.addTab("Sample Processing", juce::Colour(25, 24, 24), &sampleProcessingTab, false);
    actionTabs.addTab("Mastering", juce::Colour(25, 24, 24), &masteringTab, false);

    // make subwidgets visible
    addAndMakeVisible(arrangementArea);
    addAndMakeVisible(notificationArea);
    addAndMakeVisible(actionTabs);

    // set the mixingBus callback to repaint arrangement area
    mixingBus.setTrackRepaintCallback([this] {
        const juce::MessageManagerLock mmLock;
        arrangementArea.repaint();
    });

    // spread the mixing bus data source for the visualizer
    notificationArea.setDataSource(mixingBus.getMixbusDataSource());
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
    activityManager.stopTaskBroadcast();
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
    // set arrangement area to a arrangementAreaHeight band at
    // middle of the screen
    localBounds.setX(0);
    localBounds.setY(NOTIF_HEIGHT + NOTIF_OUTTER_MARGINS + NOTIF_OUTTER_MARGINS);
    localBounds.setHeight(arrangementAreaHeight);
    arrangementArea.setBounds(localBounds);

    // area where a click = a resizing
    resizeHandleArea.setX(0);
    resizeHandleArea.setY(localBounds.getBottom());
    resizeHandleArea.setWidth(localBounds.getWidth());
    resizeHandleArea.setHeight(MAINVIEW_RESIZE_HANDLE_HEIGHT);

    int y = NOTIF_HEIGHT + NOTIF_OUTTER_MARGINS + NOTIF_OUTTER_MARGINS + arrangementAreaHeight +
            MAINVIEW_RESIZE_HANDLE_HEIGHT;
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

void MainComponent::mouseDrag(const juce::MouseEvent &me)
{
    arrangementAreaHeight = me.getMouseDownY();
}

void MainComponent::mouseMove(const juce::MouseEvent &event)
{
    if (resizeHandleArea.contains(event.getMouseDownPosition()))
    {
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}