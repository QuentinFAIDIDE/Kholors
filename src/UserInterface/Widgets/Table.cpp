#include "Table.h"

#include "../../Config.h"
#include <memory>
#include <stdexcept>
#include <string>

Table::Table(std::string tableName, TableSelectionMode selectionType, TableDataFrame &df, int bufSize)
    : selectionMode(selectionType), dataFrame(df), name(tableName), header(dataFrame.getHeaderFormat()),
      content(dataFrame.getFormat()), bufferingSize(bufSize)
{
    contentViewport.setViewedComponent(&content, false);

    addAndMakeVisible(header);
    addAndMakeVisible(contentViewport);

    // build a header with column names
    auto format = dataFrame.getHeaderFormat();
    auto colnames = dataFrame.getColumnNames();
    std::vector<std::shared_ptr<TableCell>> headerRow;
    for (int i = 0; i < colnames.size(); i++)
    {
        auto cellPointer = std::make_shared<TableCell>(colnames[i], format[i].second);
        headerRow.push_back(cellPointer);
    }

    header.addRow(headerRow);

    int maxRowIndex = df.getMaxRowIndex();

    int maxIter = std::min(maxRowIndex + 1, bufferingSize);

    for (int i = 0; i < maxIter; i++)
    {
        content.addRow(df.getRow(i));
    }
}

void Table::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds().reduced(TABLE_OUTTER_MARGINS);

    g.setColour(COLOR_BACKGROUND);
    g.fillAll();

    g.setColour(COLOR_BACKGROUND_LIGHTER);
    g.fillRoundedRectangle(bounds.toFloat(), TABLE_CORNERS_RADIUS);

    g.setColour(COLOR_TEXT.darker(0.6f));
    g.drawRoundedRectangle(bounds.toFloat(), TABLE_CORNERS_RADIUS, 0.5f);

    auto titleArea = bounds.removeFromTop(TABLE_TITLE_SECTION_HEIGHT);
    g.setColour(COLOR_TEXT);
    g.drawText(name, titleArea, juce::Justification::centred, true);
}

void Table::resized()
{
    auto bounds = getLocalBounds().reduced(TABLE_OUTTER_MARGINS);

    bounds.removeFromTop(TABLE_TITLE_SECTION_HEIGHT);

    bounds.reduce(TABLE_HEADER_AND_CONTENT_INNER_MARGINS, 0);

    header.setBounds(bounds.removeFromTop(TABLE_ROW_HEIGHT));
    contentViewport.setBounds(bounds);

    auto columnsWidth = dataFrame.getColumnsWidth(bounds.getWidth());
    header.setColumnsWidth(columnsWidth);
    content.setColumnsWidth(columnsWidth);
}

/////////////////////////////////////////////////////////////////////////

TableDataFrame::TableDataFrame()
{
    // Juce copy/move constructor removal macro require this apparently, otherwise childs can't init
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
        g.setColour(COLOR_TEXT);
        g.drawText(content, getLocalBounds().reduced(TABLE_CELL_INNER_MARGINS, 0), justification, true);
    }
    g.setColour(COLOR_TEXT.withAlpha(0.5f));
    g.drawLine(juce::Line(getLocalBounds().getBottomLeft(), getLocalBounds().getBottomRight()).toFloat(), 0.5f);
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

    // we fill columns width with zeros, it will
    // hide TableCell objects until setColumnsWidth is called
    for (int i = 0; i < noColumns; i++)
    {
        columnsWidth.push_back(0);
    }

    refreshRowCellsPositions();
}

void TableRowsPainter::refreshRowCellsPositions()
{
    rowCellsPositions.clear();

    int lastCellStart = 0;

    for (int i = 0; i < columnsWidth.size(); i++)
    {
        rowCellsPositions.push_back(juce::Rectangle<int>(lastCellStart, 0, columnsWidth[i], TABLE_ROW_HEIGHT));
        lastCellStart += columnsWidth[i];
    }
}

void TableRowsPainter::setColumnsWidth(std::vector<int> cols)
{

    if (cols.size() != noColumns)
    {
        throw std::runtime_error("Received bad number of rows width given expected table format");
    }

    columnsWidth = cols;

    refreshRowCellsPositions();

    // compute position of all cells
    for (int i = 0; i < rows.size(); i++)
    {
        int yOffset = i * TABLE_ROW_HEIGHT;
        for (int j = 0; j < cols.size(); j++)
        {
            rows[i][j]->setBounds(rowCellsPositions[j].withY(yOffset));
        }
    }

    repaint();
}

void TableRowsPainter::paint(juce::Graphics &g)
{
    // TODO: paint stealthy separator lines
}

void TableRowsPainter::clear()
{
    rows.clear();
    deleteAllChildren();
    repaint();
}

void TableRowsPainter::addRow(std::vector<std::shared_ptr<TableCell>> row)
{
    if (row.size() != noColumns)
    {
        throw std::runtime_error("Received bad number of row cells given expected table format");
    }

    rows.push_back({});
    int yOffset = (rows.size() - 1) * TABLE_ROW_HEIGHT;

    for (int i = 0; i < row.size(); i++)
    {
        rows[rows.size() - 1].push_back(row[i]);
        addAndMakeVisible(row[i].get());
        row[i]->setBounds(rowCellsPositions[i].withY(yOffset));
    }
}

int TableRowsPainter::getRowCount()
{
    return rows.size();
}