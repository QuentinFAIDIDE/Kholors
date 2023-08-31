#include "Table.h"

#include "../../Config.h"
#include <memory>
#include <stdexcept>
#include <string>

Table::Table(std::string tableName, TableSelectionMode selectionType, TableDataFrame &df, int bufSize)
    : selectionMode(selectionType), dataFrame(df), name(tableName),
      header(dataFrame.getHeaderFormat(), TableSelectionMode::TABLE_SELECTION_NONE),
      content(dataFrame.getFormat(), selectionType), bufferingSize(bufSize)
{
    contentViewport.setScrollBarsShown(true, false);
    contentViewport.setViewedComponent(&content, false);
    contentViewport.setScrollListener(this);

    addAndMakeVisible(header);
    addAndMakeVisible(contentViewport);

    header.showLoadPlaceholder(false);

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

    // instanciate the actual data frame rows
    content.setTextColor(COLOR_TEXT.withAlpha(TABLE_CONTENT_ALPHA));
    int maxRowIndex = df.getMaxRowIndex();
    int noShownRows = std::min(maxRowIndex + 1, bufferingSize);
    // if there will be more rows that can be loaded, we show the loading placeholder
    content.showLoadPlaceholder(noShownRows != maxRowIndex + 1);
    // add initial rows
    for (int i = 0; i < noShownRows; i++)
    {
        content.addRow(df.getRow(i));
    }
}

void Table::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds().reduced(TABLE_OUTTER_MARGINS);

    g.setColour(COLOR_BACKGROUND);
    g.fillAll();

    // unfortunately we had to make this color match the tableRowPaint paintOverChildren one :(
    // so beware
    g.setColour(COLOR_BACKGROUND_LIGHTER);
    g.fillRoundedRectangle(bounds.toFloat(), TABLE_CORNERS_RADIUS);

    g.setColour(COLOR_TABLE_BORDERS);
    g.drawRoundedRectangle(bounds.toFloat(), TABLE_CORNERS_RADIUS, TABLE_BORDERS_LINE_WIDTH);

    auto titleArea = bounds.removeFromTop(TABLE_TITLE_SECTION_HEIGHT);
    g.setColour(COLOR_TEXT);
    g.drawText(name, titleArea, juce::Justification::centred, true);
}

void Table::resized()
{
    auto bounds = getLocalBounds().reduced(TABLE_OUTTER_MARGINS);

    bounds.removeFromTop(TABLE_TITLE_SECTION_HEIGHT);

    bounds.reduce(TABLE_HEADER_AND_CONTENT_INNER_MARGINS, 0);

    auto headerBounds = bounds.removeFromTop(TABLE_ROW_HEIGHT);
    // we reduce the header size by the scrollbar width here and right after when we get columns width
    headerBounds.setWidth(headerBounds.getWidth() - contentViewport.getScrollBarThickness());
    header.setBounds(headerBounds);

    contentViewport.setBounds(bounds);

    auto columnsWidth = dataFrame.getColumnsWidth(bounds.getWidth() - contentViewport.getScrollBarThickness());
    header.setColumnsWidth(columnsWidth);
    content.setColumnsWidth(columnsWidth);
}

void Table::receiveViewportUpdate(const juce::Rectangle<int> &newVisibleArea,
                                  const juce::Rectangle<int> childComponentArea)
{
    juce::Rectangle<int> childArea = childComponentArea; /** copying the const member because we modify it */
    auto loadingPlaceholder = childArea.removeFromBottom(TABLE_ROW_HEIGHT);
    if (newVisibleArea.intersects(loadingPlaceholder))
    {
        int remainingRowsToLoad = dataFrame.getMaxRowIndex() + 1 - content.getRowCount();
        if (remainingRowsToLoad > 0)
        {
            int startIndex = content.getRowCount();
            int noRowsToLoad = std::min(bufferingSize, remainingRowsToLoad);
            for (int i = 0; i < noRowsToLoad; i++)
            {
                content.addRow(dataFrame.getRow(startIndex + i));
            }

            // if no more rows to load, simply stop loading
            if (content.getRowCount() == dataFrame.getMaxRowIndex() + 1)
            {
                content.showLoadPlaceholder(false);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////

TableDataFrame::TableDataFrame()
{
    // Juce copy/move constructor removal macro require this apparently, otherwise childs can't init
}

/////////////////////////////////////////////////////////////////////////

TableCell::TableCell(std::string s, TableColumnAlignment align) : justification(juce::Justification::centred)
{
    setInterceptsMouseClicks(false, false);
    content = s;
    alignment = align;
    type = TableType::TABLE_COLUMN_TYPE_TEXT;
    setJustification();
    textColor = COLOR_TEXT;
}

TableCell::TableCell(int i, TableColumnAlignment align) : justification(juce::Justification::centred)
{
    setInterceptsMouseClicks(false, false);
    content = std::to_string(i);
    alignment = align;
    type = TableType::TABLE_COLUMN_TYPE_INT;
    setJustification();
    textColor = COLOR_TEXT;
}

TableCell::TableCell(float f, TableColumnAlignment align) : justification(juce::Justification::centred)
{
    setInterceptsMouseClicks(false, false);
    content = std::to_string(f);
    alignment = align;
    type = TableType::TABLE_COLUMN_TYPE_FLOAT;
    setJustification();
    textColor = COLOR_TEXT;
}

TableCell::TableCell(std::shared_ptr<juce::Component> c) : justification(juce::Justification::centred)
{
    setInterceptsMouseClicks(false, false);
    subComponent = c;
    type = TableType::TABLE_COLUMN_TYPE_COMPONENT;
    addAndMakeVisible(subComponent.get());
    textColor = COLOR_TEXT;
}

void TableCell::setTextColor(juce::Colour col)
{
    textColor = col;
    repaint();
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
        g.setColour(textColor);
        g.drawText(content, getLocalBounds().reduced(TABLE_CELL_INNER_MARGINS, 0), justification, true);
    }
    g.setColour(COLOR_TABLE_SEPARATOR_LINE);
    g.drawLine(juce::Line(getLocalBounds().getBottomLeft(), getLocalBounds().getBottomRight()).toFloat(),
               TABLE_ROW_SEPARATOR_LINE_WIDTH);
}

void TableCell::resized()
{
    if (type == TableType::TABLE_COLUMN_TYPE_COMPONENT)
    {
        subComponent->setBounds(getLocalBounds());
    }
}

/////////////////////////////////////////////////////////////////////////////////

TableRowsPainter::TableRowsPainter(std::vector<std::pair<TableType, TableColumnAlignment>> &format,
                                   TableSelectionMode selectionM)
    : rowSelectionMode(selectionM), hoverRowIndex(-1), clickedRowIndex(-1), showingLoadPlaceholder(true)
{

    // TODO: implement hitTest instead so that we can pass clicks to component inside cells
    setInterceptsMouseClicks(true, false);

    noColumns = format.size();

    // we fill columns width with zeros, it will
    // hide TableCell objects until setColumnsWidth is called
    for (int i = 0; i < noColumns; i++)
    {
        columnsWidth.push_back(0);
    }

    textColor = COLOR_TEXT;

    refreshRowCellsPositions();
    updateComponentHeight();
}

void TableRowsPainter::setTextColor(juce::Colour col)
{
    textColor = col;

    for (int i = 0; i < rows.size(); i++)
    {
        for (int j = 0; j < rows[i].cells.size(); j++)
        {
            rows[i].cells[j]->setTextColor(textColor);
        }
    }
    repaint();
}

void TableRowsPainter::updateComponentHeight()
{
    setSize(getWidth(), (getRowCount() + int(showingLoadPlaceholder)) * TABLE_ROW_HEIGHT);
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

int TableRowsPainter::getWidth()
{
    int width = 0;
    for (int i = 0; i < columnsWidth.size(); i++)
    {
        width += columnsWidth[i];
    }
    return width;
}

void TableRowsPainter::setColumnsWidth(std::vector<int> cols)
{

    if (cols.size() != noColumns)
    {
        throw std::runtime_error("Received bad number of rows width given expected table format");
    }

    columnsWidth = cols;

    // refresh the position of the cells
    refreshRowCellsPositions();

    // compute position of all cells in the row
    for (int i = 0; i < rows.size(); i++)
    {
        int yOffset = i * TABLE_ROW_HEIGHT;
        for (int j = 0; j < cols.size(); j++)
        {
            rows[i].cells[j]->setBounds(rowCellsPositions[j].withY(yOffset));
        }
    }

    updateComponentHeight();
    repaint();
}

void TableRowsPainter::paint(juce::Graphics &g)
{
    if (rowSelectionMode != TableSelectionMode::TABLE_SELECTION_NONE && clickedRowIndex != -1 &&
        greyedOutRowIndexes.find(clickedRowIndex) == greyedOutRowIndexes.end())
    {
        juce::Rectangle<int> rowRectangle(getLocalBounds().getWidth(), TABLE_ROW_HEIGHT);
        rowRectangle.setPosition(0, hoverRowIndex * TABLE_ROW_HEIGHT);

        g.setColour(COLOR_TABLE_CLICKED_ROW);
        g.fillRect(rowRectangle);
    }
    else if (rowSelectionMode != TableSelectionMode::TABLE_SELECTION_NONE && hoverRowIndex != -1 &&
             greyedOutRowIndexes.find(hoverRowIndex) == greyedOutRowIndexes.end())
    {
        juce::Rectangle<int> rowRectangle(getLocalBounds().getWidth(), TABLE_ROW_HEIGHT);
        rowRectangle.setPosition(0, hoverRowIndex * TABLE_ROW_HEIGHT);

        g.setColour(COLOR_TABLE_ROW_HOVER);
        g.fillRect(rowRectangle);
    }

    if (showingLoadPlaceholder)
    {
        juce::Rectangle<int> rowRectangle(getLocalBounds().getWidth(), TABLE_ROW_HEIGHT);
        rowRectangle.setPosition(0, getRowCount() * TABLE_ROW_HEIGHT);
        g.setColour(COLOR_TEXT.withAlpha(TABLE_LOADING_PLACEHOLDER_ALPHA));
        g.drawText("Loading...", rowRectangle, juce::Justification::centred, true);
    }

    g.setColour(COLOR_TABLE_SELECTED_ROW);
    auto selectIter = selectedRowIndexes.begin();
    juce::Rectangle<int> rowRectangle(getLocalBounds().getWidth(), TABLE_ROW_HEIGHT);
    while (selectIter != selectedRowIndexes.end())
    {
        g.fillRect(rowRectangle.withY((*selectIter) * TABLE_ROW_HEIGHT));

        selectIter++;
    }
}

void TableRowsPainter::paintOverChildren(juce::Graphics &g)
{
    for (auto it = greyedOutRowIndexes.begin(); it != greyedOutRowIndexes.end(); it++)
    {
        // unfortunately, we can't alter cells content colour to make them alpha since
        // they should be able to display child component that have no way to do that inherit from component class.
        // We will try to do this ugly draw over for now :(
        // The color of the rectangle matches the table paint method background one
        juce::Rectangle<int> rowRectangle(getLocalBounds().getWidth(), TABLE_ROW_HEIGHT);
        g.setColour(COLOR_TABLE_DISABLED_ROW_DRAWOVER);
        g.fillRect(rowRectangle.withY(TABLE_ROW_HEIGHT * (*it)).reduced(0, 1));
    }
}

void TableRowsPainter::clear()
{
    rows.clear();
    selectedRowIndexes.clear();
    greyedOutRowIndexes.clear();
    removeAllChildren();
    updateComponentHeight();
    repaint();
}

void TableRowsPainter::addRow(DataframeRow row)
{
    if (row.cells.size() != noColumns)
    {
        throw std::runtime_error("Received bad number of row cells given expected table format");
    }

    rows.push_back(DataframeRow({}));
    int yOffset = (rows.size() - 1) * TABLE_ROW_HEIGHT;

    for (int i = 0; i < row.cells.size(); i++)
    {
        rows[rows.size() - 1].cells.push_back(row.cells[i]);
        addAndMakeVisible(row.cells[i].get());
        row.cells[i]->setBounds(rowCellsPositions[i].withY(yOffset));
        row.cells[i]->setTextColor(textColor);
    }

    if (row.greyedOut)
    {
        greyedOutRowIndexes.insert(rows.size() - 1);
    }

    updateComponentHeight();
}

int TableRowsPainter::getRowCount()
{
    return rows.size();
}

void TableRowsPainter::mouseEnter(const juce::MouseEvent &me)
{
    updateMouseRowHover(me);
}

void TableRowsPainter::mouseMove(const juce::MouseEvent &me)
{
    updateMouseRowHover(me);
}

void TableRowsPainter::mouseDown(const juce::MouseEvent &me)
{
    int oldClickedRow = clickedRowIndex;
    clickedRowIndex = (me.position.y) / TABLE_ROW_HEIGHT;
    if (clickedRowIndex >= getRowCount())
    {
        clickedRowIndex = -1;
    }

    if (oldClickedRow != clickedRowIndex)
    {
        if (rowSelectionMode != TableSelectionMode::TABLE_SELECTION_NONE && clickedRowIndex != -1 &&
            greyedOutRowIndexes.find(clickedRowIndex) == greyedOutRowIndexes.end())
        {

            if (selectedRowIndexes.find(clickedRowIndex) == selectedRowIndexes.end())
            {

                if (rowSelectionMode == TableSelectionMode::TABLE_SELECTION_ONE)
                {
                    selectedRowIndexes.clear();
                }
                selectedRowIndexes.insert(clickedRowIndex);
            }
            else
            {
                selectedRowIndexes.erase(clickedRowIndex);
            }
        }

        repaint();
    }
}

void TableRowsPainter::mouseUp(const juce::MouseEvent &me)
{
    int oldClickedRow = clickedRowIndex;
    clickedRowIndex = -1;
    if (oldClickedRow != clickedRowIndex)
    {
        repaint();
    }

    // TODO: row select handling
}

void TableRowsPainter::mouseExit(const juce::MouseEvent &me)
{
    int oldMouseOver = hoverRowIndex;
    int oldClickedRow = clickedRowIndex;

    hoverRowIndex = -1;
    clickedRowIndex = -1;

    if (oldMouseOver != hoverRowIndex || oldClickedRow != clickedRowIndex)
    {
        repaint();
    }
}

void TableRowsPainter::updateMouseRowHover(const juce::MouseEvent &me)
{
    int oldMouseOver = hoverRowIndex;

    hoverRowIndex = (me.position.y) / TABLE_ROW_HEIGHT;
    if (hoverRowIndex >= getRowCount())
    {
        hoverRowIndex = -1;
    }

    if (oldMouseOver != hoverRowIndex)
    {
        repaint();
    }
}

void TableRowsPainter::showLoadPlaceholder(bool isPlaceholderShown)
{
    if (showingLoadPlaceholder != isPlaceholderShown)
    {
        showingLoadPlaceholder = isPlaceholderShown;
        updateComponentHeight();
        repaint();
    }
}