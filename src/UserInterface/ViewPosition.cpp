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

    broadcastViewPosition();
    broadcastViewScale();
}

void ViewPosition::updateAppPosition(int position)
{
    int maximumPosition = INT_MAX - (viewScale * MAX_THEORICAL_SCREEN_WIDTH);
    if (position >= 0 && position < maximumPosition)
    {
        viewPosition = position;
        broadcastViewPosition();
    }
}

void ViewPosition::updateAppScale(int scale)
{
    int maxScale = ((INT_MAX - viewPosition) / MAX_THEORICAL_SCREEN_WIDTH) - 1;
    maxScale = std::min(maxScale, FREQVIEW_MAX_SCALE_FRAME_PER_PIXEL);

    if (scale >= FREQVIEW_MIN_SCALE_FRAME_PER_PIXEL && scale <= maxScale)
    {
        viewScale = scale;
        broadcastViewScale();
    }
}

void ViewPosition::attachViewPositionListener(ViewPositionListener *vpl)
{
    viewPositionListeners.push_back(vpl);
}

void ViewPosition::broadcastViewPosition()
{
    for (size_t i = 0; i < viewPositionListeners.size(); i++)
    {
        viewPositionListeners[i]->viewPositionUpdateCallback(viewPosition);
    }
}

void ViewPosition::broadcastViewScale()
{
    for (size_t i = 0; i < viewPositionListeners.size(); i++)
    {
        viewPositionListeners[i]->viewScaleUpdateCallback(viewScale);
    }
}
