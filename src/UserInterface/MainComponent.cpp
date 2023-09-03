#include "MainComponent.h"

#include <cstdlib>
#include <memory>

#include "RobotoFont.h"

//==============================================================================
MainComponent::MainComponent()
    : mixingBus(activityManager), arrangementArea(mixingBus, activityManager), topbarArea(activityManager),
      actionTabs(juce::TabbedButtonBar::Orientation::TabsAtTop)
{

    arrangementAreaHeight = FREQTIME_VIEW_HEIGHT;

    activityManager.registerTaskListener(this);

    configureLookAndFeel();

    activityManager.registerTaskListener(&mixingBus);
    activityManager.registerTaskListener(&topbarArea);
    activityManager.registerTaskListener(&audioLibraryTab);

    // initialize audio app with two outputs
    setAudioChannels(0, 2);

    printAudioDeviceSettings();

    // set size of the component
    setSize(1400, 800);

    actionTabs.addTab("Audio Library", COLOR_BACKGROUND, &audioLibraryTab, false);
    actionTabs.addTab("Generators", COLOR_BACKGROUND, &generatorsTab, false);
    actionTabs.addTab("Effects", juce::Colour(25, 24, 24), &sampleProcessingTab, false);
    actionTabs.addTab("Mastering", juce::Colour(25, 24, 24), &masteringTab, false);

    // make subwidgets visible
    addAndMakeVisible(topbarArea);
    addAndMakeVisible(arrangementArea);
    addAndMakeVisible(actionTabs);

    // set the mixingBus callback to repaint arrangement area
    mixingBus.setTrackRepaintCallback([this] {
        const juce::MessageManagerLock mmLock;
        arrangementArea.repaint();
    });

    // spread the mixing bus data source for the visualizer
    topbarArea.setDataSource(mixingBus.getMixbusDataSource());

    // instanciate the loop values with what the mixbus has.
    // empty comstructor = request to broacast current value from Mixbus class
    // NOTE: these are here because it's the sole place where we know both classes
    // that handle the loop and the one that display it are initialized and the communication
    // can happen. We could and we probably already initiate the value passing on both ends,
    // but it's a good practice to have the ultimate way that will always work whathever the refactor
    // here. Passing this value through the task many times has no overhead.
    auto getLoopStateTask = std::make_shared<LoopToggleTask>();
    auto getLoopPositionTask = std::make_shared<LoopMovingTask>();
    activityManager.broadcastTask(getLoopStateTask);
    activityManager.broadcastTask(getLoopPositionTask);

    activityManager.getAppState().setExternalAppStateFileHandlers(&arrangementArea, &mixingBus);
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
    juce::LookAndFeel::setDefaultLookAndFeel(&appLookAndFeel);
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
    topbarArea.setBounds(localBounds);
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

    sharedConfig.get() = conf;
}

void MainComponent::mouseMove(const juce::MouseEvent &event)
{
    if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT &&
        resizeHandleArea.contains(event.getPosition()))
    {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }
    else if (activityManager.getAppState().getUiState() == UI_STATE_RESIZE_MAINVIEW)
    {
        arrangementAreaHeight = event.getPosition().getY() - getBounds().getY();
        resized();
        repaint();
    }
}

void MainComponent::mouseUp(const juce::MouseEvent &me)
{
    // note: isLeftButtonDown means it was down before it was lifted
    if (activityManager.getAppState().getUiState() == UI_STATE_RESIZE_MAINVIEW && me.mods.isLeftButtonDown() == true)
    {
        activityManager.getAppState().setUiState(UI_STATE_DEFAULT);
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void MainComponent::mouseDown(const juce::MouseEvent &me)
{
    if (activityManager.getAppState().getUiState() == UI_STATE_DEFAULT && me.mods.isLeftButtonDown() == true &&
        resizeHandleArea.contains(me.getPosition()))
    {
        activityManager.getAppState().setUiState(UI_STATE_RESIZE_MAINVIEW);
    }
}

void MainComponent::mouseDrag(const juce::MouseEvent &me)
{

    if (activityManager.getAppState().getUiState() == UI_STATE_RESIZE_MAINVIEW)
    {
        arrangementAreaHeight = me.getPosition().getY() - (NOTIF_HEIGHT + NOTIF_OUTTER_MARGINS + NOTIF_OUTTER_MARGINS);
        resized();
        repaint();
    }
}

bool MainComponent::taskHandler(std::shared_ptr<Task> task)
{
    auto quitTask = std::dynamic_pointer_cast<QuittingTask>(task);
    if (quitTask != nullptr)
    {
        juce::JUCEApplicationBase::quit();
        quitTask->setCompleted(true);
        return true;
    }

    return false;
}