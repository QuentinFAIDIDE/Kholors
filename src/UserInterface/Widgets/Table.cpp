#include "Table.h"

#include "../../Config.h"

Table::Table(std::string tableName, TableSelectionMode selectionType, TableDataFrame &df)
    : selectionMode(selectionType), dataFrame(df), name(tableName), header(dataFrame.getFormat()),
      content(dataFrame.getHeaderFormat())
{
    contentViewport.setViewedComponent(&content, false);

    addAndMakeVisible(header);
    addAndMakeVisible(contentViewport);

    // build a header with column names
    auto colnames = dataFrame.getColumnNames();
    std::vector<TableCell> headerRow;
    for (int i = 0; i < colnames.size(); i++)
    {
        headerRow.push_back(TableCell(colnames[i]));
    }

    header.addRow(headerRow);
}

void Table::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds().reduced(TABLE_OUTTER_MARGINS);

    g.setColour(COLOR_TABLE_BACKGROUND);
    g.fillRoundedRectangle(bounds.toFloat(), TABLE_CORNERS_RADIUS);

    auto titleArea = bounds.removeFromTop(TABLE_TITLE_SECTION_HEIGHT);
    g.setColour(COLOR_TEXT);
    g.drawText(name, titleArea, juce::Justification::centred, true);
}

void Table::resized()
{
    auto bounds = getLocalBounds().reduced(TABLE_OUTTER_MARGINS);

    bounds.removeFromTop(TABLE_TITLE_SECTION_HEIGHT);

    header.setBounds(bounds.removeFromTop(TABLE_ROW_HEIGHT));
    contentViewport.setBounds(bounds);
}
