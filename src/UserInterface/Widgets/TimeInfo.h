#ifndef DEF_TIME_INFO_HPP
#define DEF_TIME_INFO_HPP

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#include "../../Audio/MixbusDataSource.h"

#include "../FontsLoader.h"

#define TIMEINFO_WIDTH 95

#define TIMEINFO_MIN_NO_CHAR 2
#define TIMEINFO_SEC_NO_CHAR 2
#define TIMEINFO_MS_NO_CHAR 3

/**
 * @brief      This class describes a time information component
 *             timestamp value. It converts the number of audio
 *             samples (in the sense of audio frames) into a
 *             minutes, second, millisec value.
 */
class TimeInfoValue
{
  public:
    // creates an empty timeinfo widget
    TimeInfoValue();
    int getMinutes();
    int getSeconds();
    int getMilliseconds();
    void setFrameValue(int);

  private:
    // the actual values of the timestamp
    int min, sec, ms;
    // the last used frame value
    int lastUsedFrameCount;
};

/**
 * @brief      This class describes a visual component
 *             that shows a timestamp in a minutes-seconds-ms
 *             format.
 */
class TimeInfo : public juce::Component
{
  public:
    /**
     * @brief      Creates a timeinfo widget.
     */
    TimeInfo();

    /**
     * @brief      Paint the area inside the timeinfo widget.
     *
     * @param      g     juce graphics context used to draw
     */
    void paint(juce::Graphics &g) override;

    /**
     * @brief      Sets the data source.
     *
     * @param[in]  pds   The new value
     */
    void setDataSource(std::shared_ptr<PositionDataSource> pds);

  private:
    // value of the timestamp in audio frames
    int frameValue;
    // the value handler (data broken down in min/sec/ms)
    TimeInfoValue value;
    // shared reference to the loaded custom fonts
    juce::SharedResourcePointer<FontsLoader> sharedFonts;

    // the source where we pull values from
    std::shared_ptr<PositionDataSource> positionDataSource;

    // width of a character
    int characterWidth;

    int textWidth;

    ///////////

    /**
     * @brief      Formats an int to a string and ensure
     *             it's the exact width. If higher, truncates,
     *             if lower, pad with zeros to the left.
     *
     * @param[in]  value  The value to convert to string.
     * @param[in]  width  The width of the output string.
     *
     * @return     the value to string with the provided width
     */
    std::string formatToStringWithWidth(int value, int width);

    /**
     * @brief      Paints the backgroud of the component.
     */
    void paintBackground(juce::Graphics &g);

    /**
     * @brief      Will paint the section where the text lies
     *             with the time info, with a different color
     *             for values and units.
     */
    void paintMulticolorTimeText(juce::Graphics &g);
};

#endif // DEF_TIME_INFO_HPP