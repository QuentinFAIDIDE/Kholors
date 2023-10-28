#include "ViewPosition.h"
#include "../Config.h"
#include <climits>

ViewPosition::ViewPosition() : viewPosition(DEFAULT_VIEW_POSITION), viewScale(DEFAULT_VIEW_SCALE)
{
}

void ViewPosition::reset()
{
    viewPosition = DEFAULT_VIEW_POSITION;
    viewScale = DEFAULT_VIEW_SCALE;

    broadcastViewUpdate();
}

void ViewPosition::updateView(int position, int scale)
{
    bool positionOrScaleChanged = false;

    int maximumPosition = INT_MAX - (viewScale * MAX_THEORICAL_SCREEN_WIDTH);
    if (position >= 0 && position < maximumPosition)
    {
        positionOrScaleChanged = true;
        viewPosition = position;
    }

    int maxScale = ((INT_MAX - viewPosition) / MAX_THEORICAL_SCREEN_WIDTH) - 1;
    maxScale = std::min(maxScale, FREQVIEW_MAX_SCALE_FRAME_PER_PIXEL);
    if (scale >= FREQVIEW_MIN_SCALE_FRAME_PER_PIXEL && scale <= maxScale)
    {
        positionOrScaleChanged = true;
        viewScale = scale;
    }

    if (positionOrScaleChanged)
    {
        broadcastViewUpdate();
    }
}

void ViewPosition::attachViewPositionListener(ViewPositionListener *vpl)
{
    viewPositionListeners.push_back(vpl);
}

void ViewPosition::broadcastViewUpdate()
{
    for (size_t i = 0; i < viewPositionListeners.size(); i++)
    {
        viewPositionListeners[i]->viewPositionUpdateCallback();
    }
}

int ViewPosition::getViewPosition() const
{
    return viewPosition;
}

int ViewPosition::getViewScale() const
{
    return viewScale;
}