#ifndef DEF_TOPBAR_LEFT_AREA
#define DEF_TOPBAR_LEFT_AREA

#include "../../Config.h"
#include "../Widgets/VuMeter.h"
#include "TrackProperties.h"

#include <juce_gui_extra/juce_gui_extra.h>

/**
 Widget for the area to the left of the topbar.
 */
class TopbarLeftArea : public juce::Component
{
  public:
    TopbarLeftArea();
    void paint(juce::Graphics &) override;
    void resized() override;

    /**
     * @brief      Sets the data source this VuMeter will pull from.
     *             Note that the data source actually delivers to many
     *             VuMeter based on the requested VuMeterId
     *
     * @param[in]  datasource  Instanciation of VumeterDataSource class.
     */
    void setDataSource(std::shared_ptr<VuMeterDataSource>);

  private:
    // the outer bounds, minus the LeftArea inner margins.
    juce::Rectangle<float> bounds;
    VuMeter masterGainVu;
    TrackProperties trackProperties;
};

#endif // DEF_TOPBAR_LEFT_AREA