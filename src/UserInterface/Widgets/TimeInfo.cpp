#include "TimeInfo.h"

#include "../../Config.h"
#include <stdexcept>
#include <string>

TimeInfoValue::TimeInfoValue()
{
    min = 0;
    sec = 0;
    ms = 0;
    lastUsedFrameCount = 0;
}

int TimeInfoValue::getMinutes()
{
    return min;
}

int TimeInfoValue::getSeconds()
{
    return sec;
}

int TimeInfoValue::getMilliseconds()
{
    return ms;
}

void TimeInfoValue::setFrameValue(int v)
{
    // do nothing if last timestamp had same value
    if (v == lastUsedFrameCount)
    {
        return;
    }
    lastUsedFrameCount = v;
    // compute ms value
    ms = v / (AUDIO_FRAMERATE / 1000);
    // populate seconds value and remove it from ms
    sec = ms / 1000;
    ms = ms - (sec * 1000);
    // populate minutes, and remove em from seconds
    min = sec / 60;
    sec = sec - (min * 60);
}

/////////////////////////////////////

TimeInfo::TimeInfo() : frameValue(0)
{
    characterWidth = sharedFonts->monospaceFont.withHeight(DEFAULT_FONT_SIZE).getStringWidth(" ");
    textWidth = characterWidth * (TIMEINFO_MIN_NO_CHAR + 2 + TIMEINFO_SEC_NO_CHAR + 2 + TIMEINFO_MS_NO_CHAR + 2);
}

void TimeInfo::paint(juce::Graphics &g)
{
    // if position data source is set fetch the value
    if (positionDataSource != nullptr)
    {
        auto pos = positionDataSource->getPosition();
        if (pos.hasValue())
        {
            value.setFrameValue(*pos);
        }
    }

    paintBackground(g);

    paintMulticolorTimeText(g);
}

void TimeInfo::paintBackground(juce::Graphics &g)
{
    // We do not draw background anymore for timeinfo
}

void TimeInfo::paintMulticolorTimeText(juce::Graphics &g)
{

    auto textArea = g.getClipBounds();
    g.setFont(sharedFonts->monospaceFont.withHeight(DEFAULT_FONT_SIZE));

    if (textArea.getWidth() <= textWidth)
    {
        return;
    }
    else
    {
        int diff = textArea.getWidth() - textWidth;
        textArea.removeFromLeft((diff / 2));
    }

    // centering text area to perfectly fit text
    int centeringShift = (textArea.getWidth() - textWidth) >> 1;
    textArea.setWidth(textWidth);
    textArea.setX(textArea.getX() + centeringShift);

    // BUG: if the time info widget is longer than 99 minutes
    // the minutes will get truncated by formatToStringWithWidth

    std::string minStr = formatToStringWithWidth(value.getMinutes(), TIMEINFO_MIN_NO_CHAR);
    std::string secStr = formatToStringWithWidth(value.getSeconds(), TIMEINFO_SEC_NO_CHAR);
    std::string msStr = formatToStringWithWidth(value.getMilliseconds(), TIMEINFO_MS_NO_CHAR);

    auto txtSectionArea = textArea.removeFromLeft(TIMEINFO_MIN_NO_CHAR * characterWidth);
    g.setColour(COLOR_TEXT_DARKER);
    g.drawText(minStr, txtSectionArea, juce::Justification::centredLeft);

    txtSectionArea = textArea.removeFromLeft(2 * characterWidth);
    g.setColour(COLOR_UNITS);
    g.drawText("m", txtSectionArea, juce::Justification::centredLeft);

    txtSectionArea = textArea.removeFromLeft(TIMEINFO_SEC_NO_CHAR * characterWidth);
    g.setColour(COLOR_TEXT_DARKER);
    g.drawText(secStr, txtSectionArea, juce::Justification::centredLeft);

    txtSectionArea = textArea.removeFromLeft(2 * characterWidth);
    g.setColour(COLOR_UNITS);
    g.drawText("s", txtSectionArea, juce::Justification::centredLeft);

    txtSectionArea = textArea.removeFromLeft(TIMEINFO_MS_NO_CHAR * characterWidth);
    g.setColour(COLOR_TEXT_DARKER);
    g.drawText(msStr, txtSectionArea, juce::Justification::centredLeft);

    txtSectionArea = textArea.removeFromLeft(2 * characterWidth);
    g.setColour(COLOR_UNITS);
    g.drawText("ms", txtSectionArea, juce::Justification::centredLeft);
}

std::string TimeInfo::formatToStringWithWidth(int intval, int width)
{
    auto output = std::to_string(intval);

    // if we have >= width characters, we trim.
    // if it doesn't work everything is okay.
    try
    {
        output = output.substr(0, width);
    }
    catch (std::out_of_range &err)
    {
        //
    }

    // now we eventually fill the left part with 0
    while (output.length() < width)
    {
        output = "0" + output;
    }

    return output;
}

void TimeInfo::setDataSource(std::shared_ptr<PositionDataSource> pds)
{
    positionDataSource = pds;
}
