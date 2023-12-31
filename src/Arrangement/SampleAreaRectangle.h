#ifndef DEF_SAMPLE_AREA_RECT_HPP
#define DEF_SAMPLE_AREA_RECT_HPP

#include <juce_gui_extra/juce_gui_extra.h>

class SampleAreaRectangle : public juce::Rectangle<float>
{
  public:
    SampleAreaRectangle()
    {
        index = -1;
    }
    void setSampleIndex(int i)
    {
        index = i;
    }
    int getSampleIndex()
    {
        return index;
    }
    void setNumParts(int a) {
      numParts = a;
    }
    int getNumParts() {
      return numParts;
    }
    void setPartId(int id) {
      partId = id;
    }
    int getPartId() {
      return partId;
    }

    SampleAreaRectangle &operator=(juce::Rectangle<float> other)
    {
        setWidth(other.getWidth());
        setHeight(other.getHeight());
        setX(other.getX());
        setY(other.getY());

        return *this;
    }

  private:
    int index;
    int numParts;
    int partId;
};

#endif // DEF_SAMPLE_AREA_RECT_HPP