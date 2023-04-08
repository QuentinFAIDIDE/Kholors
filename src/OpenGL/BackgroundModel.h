#ifndef DEF_BACKGROUND_MODEL
#define DEF_BACKGROUND_MODEL

#include "GraphicModel.h"

class BackgroundModel : public GraphicModel {
 public:
  BackgroundModel();
  ~BackgroundModel();
  void registerGlObjects() override final;
  void drawGlObjects() override final;
  void disable() override final;
};

#endif  // DEF_BACKGROUND_MODEL