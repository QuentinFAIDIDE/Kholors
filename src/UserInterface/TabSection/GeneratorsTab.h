#ifndef DEF_GENERATORS_TAB_HPP
#define DEF_GENERATORS_TAB_HPP

#include "../Section.h"

/**
Component that sits in the generator tab and allow
to use Generators. @see Generators.
*/
class GeneratorsTab : public juce::Component 
{
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