// sample rate (that we call framrate to not confuse with file based sound samples)
#define AUDIO_FRAMERATE 44100

// How many frequency bins we use on vertical axis
#define FREQTIME_VIEW_Y_RESOLUTION 256
// How many time bins we use on horizontal axis
#define FREQTIME_VIEW_X_RESOLUTION 8192
// How many shades of intensities we display
#define FREQTIME_VIEW_INTENSITY_RESOLUTION_BITS 4
#define FREQTIME_VIEW_INTENSITY_RESOLUTION \
  (2 << (FREQTIME_VIEW_INTENSITY_RESOLUTION_BITS - 1))
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
#define FREQVIEW_MIN_REDRAW_DISTANCE_FRAMES 128
// the ratio of horizontal to vertical movement needed to trigger actions (min
// and max)
#define FREQVIEW_MIN_HORIZONTAL_MOVE_RATIO 2.0
#define FREQVIEW_MIN_VERTICAL_MOVE_RATIO 0.5
// maximum allowed scaling movement
#define FREQVIEW_MAX_ABSOLUTE_SCALE_MOVEMENT 20
// minimum sizes to draw freqview (arrangement) area
#define FREQVIEW_MIN_WIDTH 300
#define FREQVIEW_MIN_HEIGHT FREQTIME_VIEW_HEIGHT

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

// various colors
#define COLOR_NOTIF_BACKGROUND juce::Colour(20, 20, 20)
#define COLOR_APP_BACKGROUND juce::Colour(32, 29, 29)
#define COLOR_NOTIF_TEXT juce::Colour(230, 230, 230)

// the gain smoothing when the value is changed
#define DSP_GAIN_SMOOTHING_RAMP_SEC 0.03f
// the headroom is positive
#define DSP_DEFAULT_MASTER_LIMITER_HEADROOM_DB 0.6f
#define DSP_DEFAULT_MASTER_LIMITER_RELEASE_MS 50.0

// maximum number of seconds above which we refuse to load it
#define SAMPLE_MAX_DURATION_SEC 10
// the maximum number of elements a song can have
#define SAMPLE_MAX_PLAYERS_USED 16383
// how many 64bits blocks the bitmask has
#define SAMPLE_BITMASK_SIZE int(SAMPLE_MAX_PLAYERS_USED>>6)+1

#define SAMPLE_MASKING_DISTANCE_SEC 2*SAMPLE_MAX_DURATION_SEC
#define SAMPLE_MASKING_DISTANCE_FRAMES AUDIO_FRAMERATE*SAMPLE_MASKING_DISTANCE_SEC

#define SAMPLEPLAYER_BORDER_RADIUS 4.0
#define SAMPLEPLAYER_BORDER_COLOR juce::Colour(230, 230, 230)
#define SAMPLEPLAYER_BORDER_WIDTH 2.5

#define PLAYCURSOR_WIDTH 6
#define PLAYCURSOR_GRAB_WIDTH 10

#define KEYMAP_DRAG_MODE "d"
#define KEYMAP_DELETE_SELECTION "x"

#define FREQVIEW_SAMPLE_FFT_ORDER 10
#define FREQVIEW_SAMPLE_FFT_SIZE (1<<FREQVIEW_SAMPLE_FFT_ORDER)