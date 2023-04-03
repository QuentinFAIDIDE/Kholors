#ifndef DEF_RECTANGLE_MODEL
#define DEF_RECTANGLE_MODEL

#include "GraphicModel.h"

class RectangleModel : public GraphicModel {
 public:
  RectangleModel(juce::Colour, float, float, float, float);
  ~RectangleModel();
  void registerGlObjects() override;
  void drawGlObjects() override;
  void disable() override;
};

#endif  // DEF_RECTANGLE_MODEL