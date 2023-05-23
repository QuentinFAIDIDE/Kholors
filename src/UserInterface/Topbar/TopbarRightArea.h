#ifndef DEF_TOPBAR_RIGHT_AREA
#define DEF_TOPBAR_RIGHT_AREA

#include "../../Config.h"
#include "../Widgets/VuMeter.h"
#include "ColorPicker.h"

#include <juce_gui_extra/juce_gui_extra.h>

class TopbarRightArea : public juce::Component
{
  public:
    TopbarRightArea(ActivityManager &);
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
    // the outer bounds, minus the inner margins.
    juce::Rectangle<float> bounds;
    ColorPicker colorPicker;
    VuMeter selectionGainVu;
};

#endif // DEF_TOPBAR_RIGHT_ARE