#include "SidebarArea.h"

#define SECTION_COMPONENTS_PADDING 6

SidebarArea::SidebarArea(ActivityManager &am)
    : playButton(am), stopButton(am), loopButton(am), colorPicker(am), trackProperties(am), sampleProperties(am),
      selectionGainVu("SELECTION", VUMETER_ID_SELECTED), masterGainVu("MASTER", VUMETER_ID_MASTER), activityManager(am)
{
    setFramesPerSecond(NOTIF_ANIM_FPS);

    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(loopButton);
    addAndMakeVisible(colorPicker);
    addAndMakeVisible(trackProperties);
    addAndMakeVisible(sampleProperties);
    addAndMakeVisible(selectionGainVu);
    addAndMakeVisible(masterGainVu);
}

SidebarArea::~SidebarArea()
{
}

void SidebarArea::setDataSource(std::shared_ptr<MixbusDataSource> ds)
{
    selectionGainVu.setDataSource(ds);
    masterGainVu.setDataSource(ds);
    trackProperties.setDataSource(ds);
}

bool SidebarArea::taskHandler(std::shared_ptr<Task>)
{
    // NOTE: Used to pick up notifications, not anymore !
    return false;
}

void SidebarArea::paint(juce::Graphics &g)
{
    g.setColour(COLOR_BACKGROUND_LIGHTER);
    g.fillAll();
    g.setColour(COLOR_TEXT);

    std::string projectName = "New Project";
    auto optionalRepo = activityManager.getAppState().getRepoDirectory();
    if (optionalRepo.has_value())
    {
        projectName = optionalRepo.value().getFileName().toStdString();
    }

    g.setFont(DEFAULT_FONT_SIZE);
    g.drawText(projectName, projectTitleArea, juce::Justification::centredLeft, true);
}

void SidebarArea::resized()
{
    // this is the component bounds that we will remove area from
    // to assign it to other components
    auto remainingBounds = getLocalBounds();

    remainingBounds.reduce(SIDEBAR_OUTTER_MARGINS, 0);

    // top bar with app title and play/stop buttons
    auto topLine = remainingBounds.removeFromTop(TABS_HEIGHT);
    loopButton.setBounds(topLine.removeFromRight(SIDEBAR_ICONS_BUTTONS_WIDTH));
    topLine.removeFromRight(SIDEBAR_ICONS_BUTTONS_PADDING);
    stopButton.setBounds(topLine.removeFromRight(SIDEBAR_ICONS_BUTTONS_WIDTH));
    topLine.removeFromRight(SIDEBAR_ICONS_BUTTONS_PADDING);
    playButton.setBounds(topLine.removeFromRight(SIDEBAR_ICONS_BUTTONS_WIDTH));
    topLine.removeFromRight(SIDEBAR_ICONS_BUTTONS_PADDING);
    // the remaining area is where we draw the project title
    projectTitleArea = topLine;

    // we now proceed to add vu meters to the bottom
    auto vuMetersSection = remainingBounds.removeFromBottom(SIDEBAR_VU_METERS_AREA_HEIGHT);
    masterGainVu.setBounds(vuMetersSection.removeFromLeft(vuMetersSection.getWidth() >> 1));
    selectionGainVu.setBounds(vuMetersSection);

    // we finally start adding the main sections
    auto mainArea = remainingBounds;
    mainArea.removeFromTop(SIDEBAR_MAIN_SECTION_TOP_PADDING);
    colorPicker.setBounds(mainArea.removeFromTop(colorPicker.getIdealHeight()));
    mainArea.removeFromTop(SECTION_COMPONENTS_PADDING);
    trackProperties.setBounds(mainArea.removeFromTop(trackProperties.getIdealHeight()));
    mainArea.removeFromTop(SECTION_COMPONENTS_PADDING);
    sampleProperties.setBounds(mainArea.removeFromTop(sampleProperties.getIdealHeight()));
};

void SidebarArea::update()
{
}