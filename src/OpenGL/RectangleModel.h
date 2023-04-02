#ifndef DEF_RECTANGLE_MODEL
#define DEF_RECTANGLE_MODEL

#include "GraphicModel.h"

class RectangleModel : public GraphicModel {
 public:
  virtual ~RectangleModel();
  virtual void registerGlObjects() override;
  virtual void drawGlObjects() override;
  virtual void disable() override;
};

#endif  // DEF_RECTANGLE_MODEL