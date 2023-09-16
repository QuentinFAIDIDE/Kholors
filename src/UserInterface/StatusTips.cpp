#include "StatusTips.h"
#include <mutex>

StatusTips::StatusTips()
{
    statusRepaintComponent = nullptr;
}

void StatusTips::setPositionStatus(std::string newPosStat)
{
    {
        std::scoped_lock<std::mutex> lock(positionStatusLock);
        positionStatus = newPosStat;
    }

    // we may repaint if possible
    if (statusRepaintComponent != nullptr)
    {
        const juce::MessageManagerLock mml;
        statusRepaintComponent->repaint();
    }
}

std::optional<std::string> StatusTips::getPositionStatus()
{
    if (positionStatusLock.try_lock())
    {
        return positionStatus;
    }
    else
    {
        return std::optional<std::string>();
    }
}

void StatusTips::setStatusPainter(juce::Component *statusPainter)
{
    statusRepaintComponent = statusPainter;
}

void StatusTips::unsetStatusPainter()
{
    statusRepaintComponent = nullptr;
}