// How many frequency bins we use on vertical axis
#define FREQTIME_VIEW_Y_RESOLUTION 256
// How many time bins we use on horizontal axis
#define FREQTIME_VIEW_X_RESOLUTION 8192
// How many shades of intensities we display
#define FREQTIME_VIEW_INTENSITY_RESOLUTION_BITS 4
#define FREQTIME_VIEW_INTENSITY_RESOLUTION (2<<(FREQTIME_VIEW_INTENSITY_RESOLUTION_BITS-1))
// Max number of elements per pixel (used to preallocate cache)
#define FREQTIME_VIEW_MAX_SINGLEPOINT_ELEMEMENTS 8
#define FREQTIME_VIEW_HEIGHT 450
// scaling factors for mouse actions
#define FREQVIEW_MOUSE_SCALING_FACTOR 0.1
// minimum number of frames per pixels to have
#define FREQVIEW_MIN_SCALE_FRAME_PER_PIXEL 10
#define FREQVIEW_MAX_SCALE_FRAME_PER_PIXEL 10000
// the ratio of horizontal to vertical movement needed to trigger actions (min and max)
#define FREQVIEW_MIN_HORIZONTAL_MOVE_RATIO 2.0
#define FREQVIEW_MIN_VERTICAL_MOVE_RATIO 0.5
// maximum allowed scaling movement
#define FREQVIEW_MAX_ABSOLUTE_SCALE_MOVEMENT 20

// minimum windows height to draw something
#define MIN_SCREEN_WIDTH 350
#define MIN_SCREEN_HEIGHT 600

#define AUDIO_FRAMERATE 44100