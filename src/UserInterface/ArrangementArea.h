#ifndef DEF_ARRANGEMENTAREA_HPP
#define DEF_ARRANGEMENTAREA_HPP

// CMake builds don't use an AppConfig.h, so it's safe to include juce module
// headers directly. If you need to remain compatible with Projucer-generated
// builds, and have called `juce_generate_juce_header(<thisTarget>)` in your
// CMakeLists.txt, you could `#include <JuceHeader.h>` here instead, to make all
// your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>

#include <memory>
#include <vector>

#include "../Arrangement/ActivityManager.h"
#include "../Arrangement/SampleAreaRectangle.h"
#include "../Arrangement/TaxonomyManager.h"
#include "../Audio/MixingBus.h"
#include "../Config.h"
#include "../OpenGL/BackgroundModel.h"
#include "../OpenGL/SampleGraphicModel.h"
#include "juce_opengl/opengl/juce_gl.h"

enum Border
{
    BORDER_LOWER,
    BORDER_UPPER,
    BORDER_LEFT,
    BORDER_RIGHT
};

enum SampleDirection
{
    LOW_FREQS_TO_BOTTOM,
    LOW_FREQS_TO_TOP,
};

class SampleBorder
{
  public:
    SampleBorder(int i, Border b, SampleDirection dir) : id(i), border(b), direction(dir)
    {
    }
    int id;
    Border border;
    SampleDirection direction;
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class ArrangementArea : public juce::Component,
                        public juce::FileDragAndDropTarget,
                        public juce::DragAndDropTarget,
                        public juce::OpenGLRenderer,
                        public TaskListener
{
  public:
    //==============================================================================
    ArrangementArea(MixingBus &, ActivityManager &);
    ~ArrangementArea();

    //==============================================================================
    bool taskHandler(std::shared_ptr<Task> task) override;
    void paint(juce::Graphics &) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseMove(const juce::MouseEvent &) override;
    bool isInterestedInFileDrag(const juce::StringArray &) override;
    void filesDropped(const juce::StringArray &, int, int) override;
    bool keyPressed(const juce::KeyPress &) override;
    bool keyStateChanged(bool) override;

    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override;

    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
    void itemDropped(const SourceDetails &dragSourceDetails) override;

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

  private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementArea)

    ActivityManager &activityManager;
    TaxonomyManager &taxonomyManager;

    juce::OpenGLContext openGLContext;

    // NOTE: we will draw each sample fft in OpenGL
    // with a square on which we map a texture.
    std::vector<SampleGraphicModel *> samples;
    BackgroundModel backgroundGrid;
    std::unique_ptr<juce::OpenGLShaderProgram> texturedPositionedShader;
    std::unique_ptr<juce::OpenGLShaderProgram> backgroundGridShader;
    bool shadersCompiled;

    float grid0PixelWidth;
    int grid0PixelShift;
    float grid0FrameWidth;

    float grid1PixelWidth;
    int grid1PixelShift;
    float grid1FrameWidth;

    float grid2PixelWidth;
    int grid2PixelShift;
    float grid2FrameWidth;

    // the index in audio frame of the view (relation to seconds depends on
    // framerate)
    int64_t viewPosition;
    // how many audio frames per pixel to display
    int64_t viewScale;
    // tempo in beats per minute
    int tempo;
    // size and position of main content widget
    juce::Rectangle<int> bounds;
    // last mouse coordinates
    int lastMouseX;
    int lastMouseY;

    // position at which the selection area is started
    int startSelectX, startSelectY;
    juce::Rectangle<float> currentSelectionRect;

    // reference to the sample manager in use
    MixingBus &mixingBus;
    // color of the play cursor
    juce::Colour cursorColor;
    int64_t lastPlayCursorPosition;

    bool recentlyDuplicated;

    // selected tracks
    std::set<size_t> selectedTracks;

    // initial position when dragging selected samples.
    // if -1, we are not dragging samples
    int64_t trackMovingInitialPosition;
    int64_t dragLastPosition;

    // buffer and vector for the labels on screen (to be swapped after
    // update)
    std::vector<SampleAreaRectangle> onScreenLabelsPixelsCoordsBuffer, onScreenLabelsPixelsCoords;
    // buffer and vector for the coordinates of the selected samples on screen
    std::vector<SampleAreaRectangle> selectedSamplesCoordsBuffer, selectedSamplesCoords;

    std::map<int, int> dragDistanceMap;
    std::map<int, float> initFiltersFreqs;

    //==============================================================================
    void paintPlayCursor(juce::Graphics &g);
    void paintSelection(juce::Graphics &g);
    void paintLabels(juce::Graphics &g);
    void paintSampleLabel(juce::Graphics &g, juce::Rectangle<float> &, int index);
    void paintSplitLocation(juce::Graphics &g);
    void paintSelectionArea(juce::Graphics &g);

    SampleAreaRectangle addLabelAndPreventOverlaps(std::vector<SampleAreaRectangle> &existingLabels, int x, int y,
                                                   int sampleIndex);
    bool rectangleIntersects(SampleAreaRectangle &, std::vector<SampleAreaRectangle> &);

    void handleMiddleButterDown(const juce::MouseEvent &);
    void handleLeftButtonDown(const juce::MouseEvent &);
    void handleLeftButtonUp(const juce::MouseEvent &);
    int getTrackClicked();
    void deleteSelectedTracks();
    float polylens(float);

    void displaySample(std::shared_ptr<SamplePlayer> sample, std::shared_ptr<SampleCreateTask> task);
    void syncSampleColor(int sampleIndex);

    bool buildShaders();
    bool buildShader(std::unique_ptr<juce::OpenGLShaderProgram> &, std::string, std::string);
    void updateShadersPositionUniforms(bool fromGlThread = false);
    void alterShadersPositions();
    void updateGridPixelValues();

    void initSelectedTracksDrag();
    void updateSelectedTracksDrag(int);

    void addSelectedSamples();

    /*
    Emits tasks when the beginning or the end of
    the selected samples were dragged.
    isBeginning is true if it's the sample start
    that is shifted (dragging the left edge) and false if it's length
    that is changed (dragging the right edge).
     */
    void emitTimeDragTasks(bool isBeginning);

    /*
    Emits tasks when the low pass or the high pass
    filter freqs of selected samples were dragged.
    isBeginning is true if it's the low pass
    that is shifted (dragging the top edge) and false if it's
    the high pass (dragging the bottom edge).
     */
    void emitFilterDragTasks(bool isLowPass);

    int64_t lowestStartPosInSelection();

    bool mouseOverPlayCursor();
    juce::Optional<SampleBorder> mouseOverSelectionBorder();
    void updateMouseCursor();

    void cropSampleEdgeHorizontally(bool cropFront);
    void cropSampleBordersVertically(bool isInnerBorder);

    bool overlapSampleArea(SampleAreaRectangle &, int, int);

    bool updateViewResizing(juce::Point<int> &);

    void refreshSampleOpenGlView(int index);
};

#endif // DEF_ARRANGEMENTAREA_HPP