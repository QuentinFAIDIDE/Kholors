#ifndef DEF_SAMPLE_GRAPHIC_MODEL
#define DEF_SAMPLE_GRAPHIC_MODEL

#include <vector>

#include "../Audio/SamplePlayer.h"
#include "TexturedModel.h"
#include "Vertex.h"
#include "juce_opengl/opengl/juce_gl.h"

class SampleGraphicModel : public TexturedModel {
 public:
  SampleGraphicModel(SamplePlayer*);
  void move(int64_t position);
  void initDrag();
  void updateDrag(int);

 private:
  void _transformIntensity(float&);
  int _transformFrequencyLocation(int);
  int _dragStartPosition;
  int _lastWidth;

  void uploadVerticesToGpu();
};

#endif  // DEF_SAMPLE_GRAPHIC_MODEL