#include "Table.h"

#include "../../Config.h"
#include <string>

Table::Table(std::string tableName, TableSelectionMode selectionType, TableDataFrame &df)
    : selectionMode(selectionType), dataFrame(df), name(tableName), header(dataFrame.getFormat()),
      content(dataFrame.getHeaderFormat())
{
    contentViewport.setViewedComponent(&content, false);

    addAndMakeVisible(header);
    addAndMakeVisible(contentViewport);

    // build a header with column names
    auto format = dataFrame.getFormat();
    auto colnames = dataFrame.getColumnNames();
    std::vector<TableCell> headerRow;
    for (int i = 0; i < colnames.size(); i++)
    {
        headerRow.push_back(TableCell(colnames[i], format[i].second));
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

/////////////////////////////////////////////////////////////////////////

TableCell::TableCell(std::string s, TableColumnAlignment align) : justification(juce::Justification::centred)
{
    content = s;
    alignment = align;
    type = TableType::TABLE_COLUMN_TYPE_TEXT;
    setJustification();
}

TableCell::TableCell(int i, TableColumnAlignment align) : justification(juce::Justification::centred)
{
    content = std::to_string(i);
    alignment = align;
    type = TableType::TABLE_COLUMN_TYPE_INT;
    setJustification();
}

TableCell::TableCell(float f, TableColumnAlignment align) : justification(juce::Justification::centred)
{
    alignment = align;
    type = TableType::TABLE_COLUMN_TYPE_FLOAT;
    setJustification();
}

TableCell::TableCell(std::shared_ptr<juce::Component> c) : justification(juce::Justification::centred)
{
    subComponent = c;
    type = TableType::TABLE_COLUMN_TYPE_COMPONENT;
    addAndMakeVisible(subComponent.get());
}

void TableCell::setJustification()
{
    if (alignment == TABLE_COLUMN_ALIGN_LEFT)
    {
        justification = juce::Justification::centredLeft;
    }

    else if (alignment == TABLE_COLUMN_ALIGN_RIGHT)
    {
        justification = juce::Justification::centredRight;
    }

    else
    {
        justification = juce::Justification::centred;
    }
}

void TableCell::paint(juce::Graphics &g)
{
    if (type != TableType::TABLE_COLUMN_TYPE_COMPONENT)
    {
        g.drawText(content, getLocalBounds(), justification, true);
    }
}

void TableCell::resized()
{
    if (type == TableType::TABLE_COLUMN_TYPE_COMPONENT)
    {
        subComponent->setBounds(getLocalBounds());
    }
}

/////////////////////////////////////////////////////////////////////////////////

TableRowsPainter::TableRowsPainter(std::vector<std::pair<TableType, TableColumnAlignment>> &format)
{
    noColumns = format.size();
}

void paint(juce::Graphics &g)
{
}

void clear()
{
}

void addRow(std::vector<TableCell> &row)
{
}
