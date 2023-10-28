#ifndef DEF_VIEW_POSITION_HPP
#define DEF_VIEW_POSITION_HPP

#include <vector>

// NOTE: MIN AND MAX ARE DEFINED IN CONFIG.H

// Default position of the view in audio frames (samples)
#define DEFAULT_VIEW_POSITION 0
// Default scale of the view in frames per pixels
#define DEFAULT_VIEW_SCALE 100
// maximum theorical screen width (we need it to prevent displaying int overflowed area)
#define MAX_THEORICAL_SCREEN_WIDTH 16384

class ViewPositionListener
{
  public:
    /**
     * @brief Called when the view position or scale is updated.
     */
    virtual void viewPositionUpdateCallback() = 0;
};

/**
 * @brief Manages view position and scale (zoom), and allow
 *        to change them, as well as broadcast changes to
 *        ViewPositionListeners.
 *        ONLY USE THIS FROM THE MESSAGE THREAD. I CAN'T STRESS IT
 *        ENOUGHT.
 *
 */
class ViewPosition
{
  public:
    /**
     * @brief Construct a new View Position object
     *
     */
    ViewPosition();

    /**
     * @brief Resets the position and view scale to the defaults.
     *
     */
    void reset();

    /**
     * @brief Move the view to a new position in audio frames (we call "audio frames" the
     *        audio samples to prevent confusion with the samples that are the audio files) and
     *        scale it.
     *        Will call the updating callback of all ViewPositionListener objects attached.
     *
     * @param position the view position in audio samples (frames)
     * @param scale the view scale in samples (audio frames) per pixels
     */
    void updateView(int position, int scale);

    /**
     * @brief Attach the view position listener so that his callback are called when
     *        the view position or scale is updated.
     *
     * @param vpl a pointer ot the object to call callback on.
     */
    void attachViewPositionListener(ViewPositionListener *vpl);

    /**
     * @brief Get the View Position value
     *
     * @return int View position in frames
     */
    int getViewPosition() const;

    /**
     * @brief Get the View Scale value
     *
     * @return int view scale in frames per pixels
     */
    int getViewScale() const;

  private:
    /**
     * @brief Call each VIewPositionListener position callback (viewPositionUpdateCallback).
     */
    void broadcastViewUpdate();

    ///////////////////

    std::vector<ViewPositionListener *> viewPositionListeners; /**< List of listeners for view updatess */

    int viewPosition; /**< Position of the view in audio frames (samples) */
    int viewScale;    /**< Scale of the view in frames per pixels (frames per audio samples) */
};

#endif // DEF_VIEW_POSITION_HPP