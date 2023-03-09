#ifndef ACTION_TABS_HPP
#define ACTION_TABS_HPP

#include <juce_gui_extra/juce_gui_extra.h>

// we inherit it just if later we wanna use custom tab components
class ActionTabs : public juce::TabbedComponent {
 public:
  ActionTabs(juce::TabbedButtonBar::Orientation);
};

#endif  // ACTION_TABS_HPP
