#ifndef DEF_SECTION_HPP
#define DEF_SECTION_HPP

#include <juce_gui_extra/juce_gui_extra.h>

class Section : public juce::Component {
 public:
  Section();
  Section(std::string title);
  ~Section();
  void paint(juce::Graphics &) override;
  void resized() override;
  void setContent(juce::Component *);

 private:
  juce::Component *_content;
  std::string _title;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Section)
};

#endif  // DEF_SECTION_HPP