#ifndef DEF_ARRANGEMENTAREA_HPP
#define DEF_ARRANGEMENTAREA_HPP

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>

#include <memory>
#include <utility>
#include <vector>

#include "../../Arrangement/ActivityManager.h"
#include "../../Arrangement/SampleAreaRectangle.h"
#include "../../Arrangement/TaxonomyManager.h"
#include "../../Audio/MixingBus.h"
#include "../../Config.h"
#include "../../OpenGL/AlphaMaskTextureLoader.h"
#include "../../OpenGL/BackgroundModel.h"
#include "../../OpenGL/SampleGraphicModel.h"
#include "../StatusTips.h"
#include "../ViewPosition.h"
#include "FrequencyGrid.h"
#include "TempoGrid.h"
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

/**
 * @brief      This class describes a sample border, and its direction (depending
 *             on if it's on left (top) or right (bottom) audio channel, it's in
 *             opposite directions).
 */
struct SampleBorder
{
    SampleBorder(int i, Border b, SampleDirection dir) : id(i), border(b), direction(dir)
    {
    }
    int id;
    Border border;
    SampleDirection direction;
};

/**
 * @brief      This class describes a component where the arrangement of the samples
 *             is performed, mostly through posting tasks to activityManager.
 *             It also manages its sub widget for tempo grid and labels.
 */
class ArrangementArea : public juce::Component,
                        public juce::FileDragAndDropTarget,
                        public juce::DragAndDropTarget,
                        public juce::OpenGLRenderer,
                        public TaskListener,
                        public Marshalable,
                        public ViewPositionListener
{
  public:
    ArrangementArea(MixingBus &, ActivityManager &);
    ~ArrangementArea();

    /**
     * @brief      Reset the state of the arrangement area view.
     */
    void resetArrangement();

    /**
     * @brief      Handle the tasks that this component is managing (has to perform filtering).
     *
     * @param[in]  task  The task
     *
     */
    bool taskHandler(std::shared_ptr<Task> task) override;

    /**
     * @brief      Paint the content below and inside the opengl context.
     *             Inherited from Juce component.
     */
    void paint(juce::Graphics &) override;

    /**
     * @brief      Paints components, but on top of children widgets.
     *             Useful to keep labels on top of grids.
     *
     */
    void paintOverChildren(juce::Graphics &) override;

    /**
     * @brief     Inherited from juce component, called when this
     *            components is created or resied to position child components.
     */
    void resized() override;

    /**
     * @brief    Called when mouse is clicked
     */
    void mouseDown(const juce::MouseEvent &) override;

    /**
     * @brief      Called when mouse button is released
     */
    void mouseUp(const juce::MouseEvent &) override;

    /**
     * @brief     Called when user is initiating a drag movement (click+move)
     */
    void mouseDrag(const juce::MouseEvent &) override;

    /**
     * @brief      Called when mouse is moved, and I seem to recall, not dragged (no mouse button clicked).
     */
    void mouseMove(const juce::MouseEvent &) override;

    void mouseExit(const juce::MouseEvent &) override;

    /**
     * @brief      Determines whether the specified file is eligible for file drag.
     *
     * @param[in]  file    the file string path
     *
     * @return     True if interested in this file drag, False otherwise.
     */
    bool isInterestedInFileDrag(const juce::StringArray &file) override;

    /**
     * @brief      Called when files are dropped on the compoennt.
     *
     * @param[in]  filePath  The file path
     * @param[in]  x         x coordinates in pixels
     * @param[in]  y         y coordiantes in pixel
     */
    void filesDropped(const juce::StringArray &filePath, int x, int y) override;

    /**
     * @brief     Called when a key is pressed.
     */
    bool keyPressed(const juce::KeyPress &) override;

    /**
     * @brief     Called when any key is pressed or released.
     */
    bool keyStateChanged(bool) override;

    /**
     * @brief      Called when wheel is rolled.
     */
    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override;

    /**
     * @brief      Determines whether the specified drag source details is eligible for drag.
     *
     * @param[in]  dragSourceDetails  The drag source details
     *
     * @return     True if the specified drag source details is eligible for drag source, False otherwise.
     */
    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;

    /**
     * @brief      Called when items are dropped over hte cmponent
     *
     * @param[in]  dragSourceDetails  The drag source details
     */
    void itemDropped(const SourceDetails &dragSourceDetails) override;

    /**
     * @brief      Called when opengl context is created.
     */
    void newOpenGLContextCreated() override;

    /**
     * @brief      Renders openGL context
     */
    void renderOpenGL() override;

    /**
     * @brief      Called when opengl context is closed.
     */
    void openGLContextClosing() override;

    /**
     * @brief      Dump a JSON formatted string representing UI state
     *
     * @return     A JSON string representing ui state.
     */
    std::string marshal() override;

    /**
     * @brief      Parse the json formatted string to restore the state
     *             it describes.
     *
     * @param      s     a JSON formatted string that was generated by marshal of this class
     */
    void unmarshal(std::string &s) override;

    /**
     * @brief Called when the view position in audio frames is updated.
     *
     * @param int the position of the view in audio frames (samples).
     */
    void viewPositionUpdateCallback() override;

  private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementArea)

    ActivityManager &activityManager;
    TaxonomyManager &taxonomyManager;

    juce::OpenGLContext openGLContext;

    TempoGrid tempoGrid;
    FrequencyGrid frequencyGrid;

    // object that loads the fragment shader second texture
    // used as a sample alpha mask texture (ie add grain / stars)
    AlphaMaskTextureLoader alphaMaskTextureLoader;

    // NOTE: we will draw each sample fft in OpenGL
    // with a rectangle on which we map a texture.
    std::vector<std::shared_ptr<SampleGraphicModel>> samples;
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

    // Selected tracks gain ramps saved for the final SampleTimeCrop tasks
    // First component is fade in in frame length, second is fade out.
    std::map<int, std::pair<int, int>> selectedSamplesFrameGainRamps;

    // initial position when dragging selected samples.
    // if -1, we are not dragging samples
    int64_t trackMovingInitialPosition;
    int64_t dragLastPosition;

    // buffer and vector for the labels on screen (to be swapped after
    // update)
    std::vector<SampleAreaRectangle> onScreenLabelsPixelsCoordsBuffer, onScreenLabelsPixelsCoords;
    // buffer and vector for the coordinates of the selected samples on screen
    std::vector<SampleAreaRectangle> selectedSamplesCoordsBuffer, cropSelectedSamplesPosition;

    std::map<int, int> dragDistanceMap;
    std::map<int, float> initFiltersFreqs;

    juce::SharedResourcePointer<StatusTips> sharedTips;

    juce::SharedResourcePointer<TextureManager> textureManager;

    juce::SharedResourcePointer<ViewPosition> viewPositionManager;

    //==============================================================================
    void paintPlayCursor(juce::Graphics &g);
    void paintSelection(juce::Graphics &g);
    void paintLabels(juce::Graphics &g);
    void paintSampleLabel(juce::Graphics &g, juce::Rectangle<float> &, int index);
    void paintSplitLocation(juce::Graphics &g);
    void paintSelectionArea(juce::Graphics &g);

    /**
     * @brief      Adds label to vector of sample areas and prevent overlaps.
     *
     * @param                                      existingLabels  The existing labels
     * @param[in]  sampleBeginPixelCoords          x coordinates of the sample beginning on screen in pixels
     * @param[in]  sampleEndPixelCoords            x coordinates of where the sample ends on screen in pixels
     * @param[in]  sampleIndex                     The sample index
     *
     * @return     The sample area rectangle.
     */
    SampleAreaRectangle addLabelAndPreventOverlaps(std::vector<SampleAreaRectangle> &existingLabels,
                                                   int sampleBeginPixelCoords, int sampleEndPixelCoords,
                                                   int sampleIndex);

    /**
     * @brief      Test if the rectangle intersect with the others.
     */
    bool rectangleIntersects(SampleAreaRectangle &, std::vector<SampleAreaRectangle> &);

    void handleMiddleButterDown(const juce::MouseEvent &);
    void handleLeftButtonDown(const juce::MouseEvent &);
    void handleLeftButtonUp(const juce::MouseEvent &);

    /**
     * @brief      Used saved mouse position to determined which track is undermouse cursor
     *
     * @return     The id of the sample clicked.
     */
    int getSampleIdUnderCursor();

    /**
     * @brief Get the Texture Intensity if exists at mouse position.
     *
     * @return float the texture intensity between 0 and 1, being 0 if nothing is below the mouse cursor.
     */
    float getTextureIntensityUnderCursor();

    /**
     * @brief      Will copy the selected track ids, and iteratively remove them by posting the deletion task.
     *             Will group the tasks for efficiency.
     */
    void deleteSelectedTracks();

    /**
     * @brief      Create the sample on the openGL model side and against the taxonomy.
     *
     * @param[in]  sample  The sample id
     * @param[in]  task    The task that describe the sample creation process (duplication, restoring, new, etc...)
     */
    void createNewSampleMeshAndTaxonomy(std::shared_ptr<SamplePlayer> sample, std::shared_ptr<SampleCreateTask> task);

    /**
     * @bref       Take the taxonomy color for this sample and send it to openGL.
     *
     * @param[in]  sampleIndex  The sample index
     */
    void setSampleColorFromTaxonomy(int sampleIndex);

    /**
     * @brief      Called by arrangement area to build all openGL shader programs;
     *
     * @return     True on sucess.
     */
    bool buildAllShaders();

    /**
     * @brief      Builds a shader. Called by buildAllShaders
     *
     * @param      shader             The shader
     * @param[in]  vertexShaderTxt    The vertex shader source code text
     * @param[in]  fragmentShaderTxt  The fragment shader source code text
     *
     * @return     True on sucess.
     */
    bool buildShader(std::unique_ptr<juce::OpenGLShaderProgram> &shader, std::string vertexShaderTxt,
                     std::string fragmentShaderTxt);

    /**
     * @brief      Will call openGL shaders uniforms updates from the current thread if fromGlThread is true,
     *             or will lock the Gl thread to do it if fromGlThread is false.
     */
    void shaderUniformUpdateThreadWrapper(bool fromGlThread = false);

    /**
     * @brief      Update view position and grid variables before sending them to the openGL
     *             shaders as uniforms. Reminder: uniform are variables that are used inside
     *             opengl shaders.
     */
    void updateShadersViewAndGridUniforms();

    /**
     * @brief      Calculates the shaders grid uniforms variables.
     */
    void computeShadersGridUniformsVars();

    /**
     * @brief      Initializes dragging of the selected tracks.
     */
    void initSelectedTracksDrag();

    /**
     * @brief      Update currently performed selected tracks dragging.
     */
    void updateSelectedTracksDrag(int);

    /**
     * @brief      Add samples under the selection area to selection.
     */
    void addToSelectionFromSelectionArea();

    /**
     * @brief      Saves initial selection gain ramps (fade in and fade out)
     *             for current selection. This is required as the final sample crop
     *             tasks will need to save potentially altered fades to be restored
     *             after reversion..
     */
    void saveInitialSelectionGainRamps();

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

    /**
     * @brief    Will get the lowest track
     *           position in audio frames from the selected tracks. It returns 0
     *           if no track is selected.
     *
     * @return   Audio frame position of the leftmost sample.
     */
    int64_t lowestStartPosInSelection();

    bool mouseOverPlayCursor();

    juce::Optional<SampleBorder> mouseOverSelectionBorder();

    /**
     * @brief      Change cursor image based on ui state and cursor position.
     */
    void updateMouseCursor();

    /**
     * @brief      Will iterate over selected tracks and try moving the left or right edge.
     *
     * @param[in]  cropFront  Are we cropping the beginning or the end ?
     */
    void cropSelectedSamplesPos(bool cropFront);

    /**
     * @brief      Iterate over selection and change filters freqs based on side cropped
     *             and mouse cursor position.
     *
     * @param[in]  isInnerBorder  Indicates if dragging the inner border (low pass) or not.
     */
    void cropSelectedSamplesFreqs(bool isInnerBorder);

    /**
     * @brief      Test if the rectangles overlaps the sample area with some margin allowed.
     *
     * @param      rectangleToTest  The rectangle to test in pixel coordinates
     * @param[in]  sampleIdToTest   The sample identifier to test
     * @param[in]  marginsAllowed   The margins allowed (added to the sample pixel coordinates)
     *
     * @return     true if overlaps, false if not
     */
    bool overlapSampleArea(SampleAreaRectangle &rectangleToTest, int sampleIdToTest, int marginsAllowed);

    /**
     * @brief      Compute the new view scale and view position based on mouse movements.
     *
     * @param      The new mouse position
     *
     * @return     true if viewPosition and viewScale changed and we would need a repaint, false if not.
     */
    bool updateViewResizing(juce::Point<int> &newMousePosition);

    /**
     * @brief      Get the samplePlayer object from mixbus for this sample and pass it to the openGL
     *             mesh so that it can update the properties displayed on screen.
     *
     * @param[in]  index  The index
     */
    void refreshSampleOpenGlView(int index);

    /**
     * @brief      Called when a task is received to recolor a group. It will get all the group sample ids
     *             and call the function that pulls the taxonomy color in the openGL mesh.
     *
     * @param[in]  task   the SampleGroupRecolor task that was received and that instruct which group gets
     *                    recolored.
     */
    void recolorSelection(std::shared_ptr<SampleGroupRecolor> task);

    /**
    Makes a copy of the set of selected tracks and broadcast it
    as a completed task so that it can be picked by mixbus.
    It's either from within a running task and will be broadcasted
    now, or from another process and will trigger a new task broadcating
    process.
    */
    void copyAndBroadcastSelection(bool fromWithinTask);

    /**
     * @brief Set the cursor position tips on the shared status tip object.
     *
     */
    void emitPositionTip();
};

#endif // DEF_ARRANGEMENTAREA_HPP