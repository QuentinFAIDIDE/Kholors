#ifndef DEF_GENERATORS_TAB_HPP
#define DEF_GENERATORS_TAB_HPP

#include "../Section.h"
#include "../../Config.h"

#define GENERATOR_TAB_SELECTOR_WIDTH 400

/**
Component that sits in the generator tab and allow
to use Generators. @see Generators.
*/
class GeneratorsTab : public juce::Component 
{
  public:
    /**
    Create the generator tab
    */
    GeneratorsTab();

    /**
    Delete the generator tab
    */
    ~GeneratorsTab();

    /**
    Repaint the inside of the component
    */
    void paint(juce::Graphics&);
};

#endif // DEF_GENERATORS_TAB_HPP