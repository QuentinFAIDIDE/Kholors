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
    void setIsTopPart(bool is) {
      isTopPart = is;
    }
    bool isTopPart() {
      return isTopPart;
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
    bool isTopPart;
};