// sample rate (that we call framrate to not confuse with file based sound
// samples)
#define AUDIO_FRAMERATE 44100

// How many frequency bins we use on vertical axis
#define FREQTIME_VIEW_Y_RESOLUTION 256
// How many time bins we use on horizontal axis
#define FREQTIME_VIEW_X_RESOLUTION 8192
// How many shades of intensities we display
#define FREQTIME_VIEW_INTENSITY_RESOLUTION_BITS 4
#define FREQTIME_VIEW_INTENSITY_RESOLUTION (2 << (FREQTIME_VIEW_INTENSITY_RESOLUTION_BITS - 1))
// Max number of elements per pixel (used to preallocate cache)
#define FREQTIME_VIEW_MAX_SINGLEPOINT_ELEMEMENTS 8
#define FREQTIME_VIEW_HEIGHT 450
// pixel vertical width of the space better top and bottom of samples
#define FREQTIME_VIEW_INNER_MARGINS 2
// TODO: RENAME THESE ! I START USING FREQVIEW IN THE MIDDLE
// IT SHOULD BE PREFIXED WITH ARRANGEMENT_AREA
// scaling factors for mouse actions
#define FREQVIEW_MOUSE_SCALING_FACTOR 0.05
// minimum number of frames per pixels to have
#define FREQVIEW_MIN_SCALE_FRAME_PER_PIXEL 10
#define FREQVIEW_MAX_SCALE_FRAME_PER_PIXEL 1024
// how many audio frames movement we need to redraw after
#define FREQVIEW_MIN_REDRAW_DISTANCE_FRAMES 64
// the ratio of horizontal to vertical movement needed to trigger actions (min
// and max)
#define FREQVIEW_MIN_HORIZONTAL_MOVE_RATIO 2.0
#define FREQVIEW_MIN_VERTICAL_MOVE_RATIO 0.5
// maximum allowed scaling movement
#define FREQVIEW_MAX_ABSOLUTE_SCALE_MOVEMENT 20
// minimum sizes to draw freqview (arrangement) area
#define FREQVIEW_MIN_WIDTH 300
#define FREQVIEW_MIN_HEIGHT FREQTIME_VIEW_HEIGHT

#define FREQVIEW_LABEL_HEIGHT 24
#define FREQVIEW_LABELS_CORNER_ROUNDING 4.0f
#define FREQVIEW_LABELS_BORDER_THICKNESS 1.7f
#define FREQVIEW_LABELS_MARGINS 2.0f
#define FREQVIEW_LABELS_MAX_WIDTH 300.0f
#define FREQVIEW_MIN_SAMPLE_CLICK_INTENSITY 0.008f
// samplerate divided by 2 pow 10 which makes 1024 so around 1ms of signal
#define FREQVIEW_MIN_RESIZE_FRAMES (AUDIO_FRAMERATE >> 10)

// minimum windows height to draw something
#define MIN_SCREEN_WIDTH 350
#define MIN_SCREEN_HEIGHT 600

// notification constants
#define NOTIF_WIDTH 400
#define NOTIF_HEIGHT 80
#define NOTIF_INNER_MARGINS 20
#define NOTIF_OUTTER_MARGINS 20
#define NOTIF_BORDER_RADIUS 8.0
#define NOTIF_ANIMATION_SPEED_PIXEL_MS 0.7
#define NOTIF_TIMEOUT 3000
#define NOTIF_ANIM_FPS 60
#define NOTIF_MAX_QUEUE_SIZE 6

#define TOPBAR_OUTTER_MARGINS 14
#define TOPBAR_SECTIONS_INNER_MARGINS 8
#define TOPBAR_BORDER_RADIUS 4

#define TOPBAR_LEFT_SECTION_WIDTH 400
#define TOPBAR_RIGHT_SECTION_WIDTH 400

// various colors
#define COLOR_BACKGROUND juce::Colour(20, 20, 20)
#define COLOR_BACKGROUND_LIGHTER juce::Colour(25, 25, 25)
#define COLOR_APP_BACKGROUND juce::Colour(27, 26, 26)
#define COLOR_BACKGROUND_HIGHLIGHT juce::Colour(45, 45, 45)
#define COLOR_SAMPLE_BORDER juce::Colour(140, 140, 180)
#define COLOR_TEXT COLOR_WHITE
#define COLOR_TEXT_DARKER juce::Colour(195, 195, 195)
#define COLOR_NOTIF_BAR_TOP juce::Colour(62, 49, 49)
#define COLOR_NOTIF_BAR_BOTTOM juce::Colour(10, 10, 10)
#define COLOR_LABELS_BORDER juce::Colour(210, 210, 220)
#define COLOR_SPLIT_PLACEHOLDER juce::Colour(210, 30, 30)
#define COLOR_SELECT_AREA juce::Colour(54, 210, 210)
#define COLOR_OPAQUE_BICOLOR_LIST_1 COLOR_APP_BACKGROUND
#define COLOR_OPAQUE_BICOLOR_LIST_2 COLOR_APP_BACKGROUND

#define COLOR_BLACK juce::Colour(10, 10, 10)
#define COLOR_WHITE juce::Colour(230, 230, 230)

#define TREEVIEW_ITEM_HEIGHT 22
#define TREEVIEW_INDENT_SIZE 20

#define DEFAULT_FONT_SIZE 15

// the gain smoothing when the value is changed
#define DSP_GAIN_SMOOTHING_RAMP_SEC 0.03f
// the headroom is positive
#define DSP_DEFAULT_MASTER_LIMITER_HEADROOM_DB 0.6f
#define DSP_DEFAULT_MASTER_LIMITER_RELEASE_MS 50.0

// maximum number of seconds above which we refuse to load it
#define SAMPLE_MAX_DURATION_SEC 10
// minimum number of audio samples (frame in our taxonomy) a sample must have
#define SAMPLE_MIN_DURATION_FRAMES 1024
// the maximum number of elements a song can have
#define SAMPLE_MAX_PLAYERS_USED 16383
// how many 64bits blocks the bitmask has
#define SAMPLE_BITMASK_SIZE int(SAMPLE_MAX_PLAYERS_USED >> 6) + 1

#define SAMPLE_MASKING_DISTANCE_SEC 2 * SAMPLE_MAX_DURATION_SEC
#define SAMPLE_MASKING_DISTANCE_FRAMES AUDIO_FRAMERATE *SAMPLE_MASKING_DISTANCE_SEC

#define SAMPLEPLAYER_BORDER_RADIUS 4.0
#define SAMPLEPLAYER_BORDER_COLOR juce::Colour(230, 230, 230)
#define SAMPLEPLAYER_BORDER_WIDTH 2.5
#define SAMPLEPLAYER_MAX_FILTER_REPEAT 4
#define SAMPLEPLAYER_FILTER_SNAP_RATIO 0.999
#define SAMPLEPLAYER_MIN_FILTER_FREQ 10
#define SAMPLEPLAYER_DEFAULT_FADE_MS 30.0
#define SAMPLEPLAYER_MAX_FADE_MS 1000.0
#define SAMPLEPLAYER_MIN_FREQ_DISTANCE_FACTOR 0.06

#define PLAYCURSOR_WIDTH 3
#define PLAYCURSOR_GRAB_WIDTH 4

#define KEYMAP_DRAG_MODE "d"
#define KEYMAP_DELETE_SELECTION "x"
#define KEYMAP_SPLIT_SAMPLE_AT_FREQUENCY "shift + s"
#define KEYMAP_SPLIT_SAMPLE_AT_TIME "s"
#define KEYMAP_UNDO "ctrl + z"
#define KEYMAP_REDO "ctrl + shift + z"

#define FREQVIEW_SAMPLE_FFT_ORDER 10
// total number of fft frequency values. Given that the real part of the result is half of it
// the effective number of frequency bins from 1 to AUDIO_FRAMERATE (samplerate)
// is half that.
#define FREQVIEW_SAMPLE_FFT_SIZE (1 << FREQVIEW_SAMPLE_FFT_ORDER)
// How many frequencies we will store for each fft.
#define FREQVIEW_SAMPLE_FFT_SCOPE_SIZE 4096
#define FREQVIEW_SAMPLE_FFT_RESOLUTION_PIXELS 3

// A and B are used to normalize log10
#define FFT_MAGNIFY_A 0.00009990793
#define FFT_MAGNIFY_B 4.0004
// C is the following power applied
#define FFT_MAGNIFY_C 0.88

#define POLYLENS_ONE_ON_TWO_POW_7_10TH 0.6155722066724582

#define MIN_DB -100.0f
#define MAX_DB 0.0f

#define LIBRARY_IDEAL_SEARCH_SIZE_PROPORTION 0.45
#define LIBRARY_MIN_SEARCH_SIZE 365

#define ACTIVITY_HISTORY_RING_BUFFER_SIZE 4096

#ifndef DEF_CONFIG_HPP
#define DEF_CONFIG_HPP

#include <yaml-cpp/yaml.h>

#include <string>
#include <vector>

class Config
{
  public:
    Config(std::string);

    bool isInvalid() const;
    std::string getProfileName() const;
    int getNumAudioLibs() const;
    std::string getAudioLibName(unsigned long) const;
    std::string getAudioLibPath(unsigned long) const;
    bool audioLibIgnoreCount(unsigned long) const;
    std::string getErrMessage() const;
    std::string getDataFolderPath() const;
    int getBufferSize() const;

  private:
    bool invalid;
    std::string errMsg;
    std::string profile;
    std::vector<std::string> audioLibNames;
    std::vector<std::string> audioLibPaths;
    std::vector<bool> audioLibIgnoreCounts;
    std::string configDirectoryPath;
    std::string dataLibraryPath;
    int bufferSize;

    void checkMandatoryParameters(YAML::Node &);
    void checkApiVersion(YAML::Node &);
    void checkIfFieldScalarAndExists(YAML::Node &, std::string);
    void parseAudioLibraryLocations(YAML::Node &);
    void parseAudioLibLocationPath(YAML::Node &);
    void parseAudioLibLocationName(YAML::Node &);
    void parseAudioLibLocationIgnoreCount(YAML::Node &);
    void parseProfileName(YAML::Node &);
    void parseBufferSize(YAML::Node &);

    void getConfigDirectory(YAML::Node &);
    void getDataDirectory(YAML::Node &);
    std::string getProvidedOrDefaultPath(YAML::Node &, std::string, std::string);

    void createFolderIfNotExists(std::string);

    static std::vector<std::string> mandatoryParameters;
};

#endif // DEF_CONFIG_HPP