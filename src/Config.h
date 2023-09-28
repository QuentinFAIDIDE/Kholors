// sample rate (that we call framrate to not confuse with file based sound
// samples)
#define AUDIO_FRAMERATE 44100

#define MIN_TEMPO 30
#define DEFAULT_TEMPO 120
#define MAX_TEMPO 250

#define FREQTIME_VIEW_HEIGHT 400
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
#define FREQVIEW_MIN_HEIGHT 150

#define FREQVIEW_LABEL_HEIGHT 24
#define FREQVIEW_LABELS_CORNER_ROUNDING 4.0f
#define FREQVIEW_LABELS_BORDER_THICKNESS 1.7f
#define FREQVIEW_LABELS_MARGINS 2.0f
#define FREQVIEW_LABELS_MAX_WIDTH 300.0f
#define FREQVIEW_MIN_SAMPLE_CLICK_INTENSITY 0.008f
// samplerate divided by 2 pow 10 which makes 1024 so around 1ms of signal
#define FREQVIEW_MIN_RESIZE_FRAMES (AUDIO_FRAMERATE >> 10)

#define FREQVIEW_SAMPLE_MASK_PATH "~/Kholors/sample_texture_mask.png"

#define MAINVIEW_RESIZE_HANDLE_HEIGHT 8

// minimum windows height to draw something
#define MIN_SCREEN_WIDTH 350
#define MIN_SCREEN_HEIGHT 600

// notification constants
#define NOTIF_WIDTH 400
#define NOTIF_HEIGHT 100
#define NOTIF_INNER_MARGINS 20
#define NOTIF_OUTTER_MARGINS 20
#define NOTIF_BORDER_RADIUS 8.0
#define NOTIF_ANIMATION_SPEED_PIXEL_MS 0.65
#define NOTIF_TIMEOUT 7000
#define NOTIF_ANIM_FPS 28
#define NOTIF_MAX_QUEUE_SIZE 6

#define SIDEBAR_OUTTER_MARGINS 14
#define SIDEBAR_SECTIONS_INNER_MARGINS 8
#define SIDEBAR_WIDGETS_MARGINS 3

#define SIDEBAR_LEFT_SECTION_WIDTH 360
#define SIDEBAR_RIGHT_SECTION_WIDTH 400

#define SIDEBAR_ICONS_BUTTONS_WIDTH 14
#define SIDEBAR_ICONS_BUTTONS_PADDING 12

#define TABS_HEIGHT 34

// various colors
#define COLOR_BACKGROUND juce::Colour(16, 17, 17)
#define COLOR_BACKGROUND_LIGHTER juce::Colour(16, 17, 17)
#define COLOR_APP_BACKGROUND juce::Colour(16, 17, 17)
#define COLOR_BACKGROUND_HIGHLIGHT juce::Colour(28, 28, 28)
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
#define COLOR_DIALOG_BACKGROUND juce::Colour(30, 30, 30)
#define COLOR_SEPARATOR_LINE juce::Colour(80, 80, 80)
#define COLOR_HIGHLIGHT juce::Colour(56, 110, 155)
#define COLOR_SELECTED_BACKGROUND juce::Colour(35, 36, 38)

#define COLOR_UNITS COLOR_TEXT_DARKER.withAlpha(0.5f)

#define COLOR_BLACK juce::Colour(10, 10, 10)
#define COLOR_WHITE juce::Colour(230, 230, 230)

#define TREEVIEW_ITEM_HEIGHT 24
#define TREEVIEW_INDENT_SIZE 23

#define TAB_BUTTONS_INNER_PADDING 42

#define TAB_PADDING 4
#define TAB_SECTIONS_MARGINS 5

#define POPUP_MENU_IDEAL_HEIGHT 18
#define POPUP_MENU_SEPARATOR_IDEAL_HEIGHT 4
#define TAB_HIGHLIGHT_LINE_WIDTH 2

#define DEFAULT_FONT_SIZE 15
#define SMALLER_FONT_SIZE 13

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
#define SAMPLEPLAYER_DEFAULT_FADE_OUT_MS 10.0
#define SAMPLEPLAYER_DEFAULT_FADE_IN_MS 2.0
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

// How many frequencies we will store for each fft.
#define FFT_STORAGE_SCOPE_SIZE 4096

/**< How much zeros we pad at the end of fft input intensities for each intensity sample */
#define FFT_ZERO_PADDING_FACTOR 4

/**< Number of intensities we send as input (not accounting for zero padding after it). */
#define FFT_INPUT_NO_INTENSITIES 1024 // always choose a power of two!

/***< What is the overlap of subsequent FFT windows. 2 = 50% overlap, 3 = 66.666% overlap, 4=25% ... */
#define FFT_OVERLAP_DIVISION 4

/**< Size of the output, as the number of frequencies bins */
#define FFT_OUTPUT_NO_FREQS (((FFT_INPUT_NO_INTENSITIES * FFT_ZERO_PADDING_FACTOR) >> 1) + 1)

/**< Number of floats we send to forward fft in fftw as input */
#define FFTW_INPUT_SIZE (FFT_INPUT_NO_INTENSITIES * FFT_ZERO_PADDING_FACTOR)

// A and B are used to normalize log10
#define FFT_MAGNIFY_A 0.00009990793
#define FFT_MAGNIFY_B 4.0004
// C is the following power applied
#define FFT_MAGNIFY_C 0.88

#define POLYLENS_ONE_ON_TWO_POW_7_10TH 0.6155722066724582

#define MIN_DB -64.0f
#define MAX_DB 0.0f

#define LIBRARY_IDEAL_SEARCH_SIZE_PROPORTION 0.45
#define LIBRARY_MIN_SEARCH_SIZE 365

#define ACTIVITY_HISTORY_RING_BUFFER_SIZE 4096

// TODO: move that into main config once we're sure of the color
#define DIALOG_FOOTER_AREA_HEIGHT 48
#define DIALOG_FOOTER_BUTTONS_SPACING 12
#define DIALOG_FOOTER_BUTTONS_WIDTH 90
#define DIALOG_FOOTER_BUTTONS_HEIGHT 28
#define DIALOG_TEXT_ENTRY_HEIGHT 30
#define DIALOG_TEXT_ENTRY_TOP_PADDING 8

#define REPO_NAME_VALIDATION_REGEX "[A-Za-zà-üÀ-Ü0-9_-]{3,30}"
#define MAIL_REGEX "^[A-Za-z0-9_\\.-]{2,40}@[A-Za-z-_]{2,40}.[a-z]{2,4}$"
#define NAME_REGEX "[A-Za-z à-üÀ-Ü]{3,30}"
#define GIT_COMMIT_REGEX "[A-Za-z à-ü0-9À-Ü _%,;\\.:-]{3,130}"

#define DATETIME_FORMAT_1 "%Y-%m-%d %H:%M:%S"

#define JSON_STATE_SAVING_INDENTATION 4

#ifndef DEF_CONFIG_HPP
#define DEF_CONFIG_HPP

#include <yaml-cpp/yaml.h>

#include <string>
#include <vector>

/**
 * @brief      Config class that parses a YAML config file and store
 *             its value for access. It can be either constructed with
 *             a path to a config file or initilized empty and assigned
 *             using the copy constructor of another config object that
 *             parsed the config file.
 *
 */
class Config
{
  public:
    /**
     * @brief      Contructs a new instance that will set its value
     *             by parsing the config file.
     *
     * @param[in]  path    The path to the config file to be parsed.
     */
    Config(std::string);

    /**
     * @brief      Contructs an empty instance of the class. It needs
     *             to be populated with the assignment operator = from
     *             a COnfig object initialized wiith a config file path
     *             to be usable.
     */
    Config();

    /**
     * @brief      Determines if invalid.
     *
     * @return     True if invalid, False otherwise.
     */
    bool isInvalid() const;

    /**
     * @brief      Gets the profile name.
     *
     * @return     The profile name.
     */
    std::string getProfileName() const;

    /**
     * @brief      Gets the number audio libs user set up.
     *
     * @return     The number audio libs.
     */
    int getNumAudioLibs() const;

    /**
     * @brief      Gets the audio library name for this index.
     * @return     The audio library name.
     */
    std::string getAudioLibName(unsigned long) const;

    /**
     * @brief      Gets the audio library path for this index.
     *
     * @return     The audio library path.
     */
    std::string getAudioLibPath(unsigned long) const;

    /**
     * @brief      return a boolean indicating if the library at this index
     *             is set up to ignore usage count on samples inisde of it.
     */
    bool audioLibIgnoreCount(unsigned long) const;

    /**
     * @brief      Gets the error message of the failed config parsing.
     *
     * @return     The error message.
     */
    std::string getErrMessage() const;

    /**
     * @brief      Gets the path to the data folder. This is the
     *             ~/Kholors one that lives in the user home, as opposed
     *             to the config folder that is prefixed with a . and is in lowercap: ~/.kholors
     *
     * @return     The data folder path.
     */
    std::string getDataFolderPath() const;

    /**
     * @brief      Get the size of the audio buffer user picked.
     *
     * @return     The buffer size.
     */
    int getBufferSize() const;

    /**
     * @brief      Gets the mail.
     *
     * @return     The mail.
     */
    std::string getMail() const;

    /**
     * @brief      Gets the name.
     *
     * @return     The name.
     */
    std::string getName() const;

  private:
    bool invalid;
    std::string errMsg;
    std::string profile;
    std::vector<std::string> audioLibNames;
    std::vector<std::string> audioLibPaths;
    std::vector<bool> audioLibIgnoreCounts;
    std::string configDirectoryPath;
    std::string dataLibraryPath;
    std::string name;
    std::string mail;
    int bufferSize;

    void checkMandatoryParameters(YAML::Node &);
    void checkApiVersion(YAML::Node &);
    void checkIfFieldScalarAndExists(YAML::Node &, std::string);
    void parseAudioLibraryLocations(YAML::Node &);
    void parseAudioLibLocationPath(YAML::Node &);
    void parseAudioLibLocationName(YAML::Node &);
    void parseAudioLibLocationIgnoreCount(YAML::Node &);
    void parseProfileName(YAML::Node &);
    void parseName(YAML::Node &);
    void parseMail(YAML::Node &);
    void parseBufferSize(YAML::Node &);
    void parseConfigDirectory(YAML::Node &);
    void parseDataDirectory(YAML::Node &);

    /**
     * @brief      Return the path based on parameter name paramName in config
     *             or return the home folder followed by defaultPathFromHome.
     *
     * @param      config                 YAML config path to try to find value in
     * @param[in]  paramName              Parameter to look for in the config.
     * @param[in]  defaultPathFromHome    Path to add to home and return if paramName not set in config
     *
     * @return     The provided or default path.
     */
    std::string findConfigPathOrProvideDefault(YAML::Node &config, std::string paramName,
                                               std::string defaultPathFromHome);

    void createFolderIfNotExists(std::string);

    static std::vector<std::string> mandatoryParameters;
};

#endif // DEF_CONFIG_HPP