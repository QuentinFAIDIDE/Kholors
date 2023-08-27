#ifndef DEF_TABLE_HPP
#define DEF_TABLE_HPP

#include "juce_core/system/juce_PlatformDefs.h"
#include <memory>
#include <set>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#define TABLE_TITLE_SECTION_HEIGHT 35
#define TABLE_OUTTER_MARGINS 5
#define TABLE_CORNERS_RADIUS 8
#define TABLE_ROW_HEIGHT 30
#define TABLE_CELL_INNER_MARGINS 12
#define TABLE_HEADER_AND_CONTENT_INNER_MARGINS 8
#define TABLE_CONTENT_ALPHA 0.75f

enum TableType
{
    TABLE_COLUMN_TYPE_TEXT,
    TABLE_COLUMN_TYPE_INT,
    TABLE_COLUMN_TYPE_FLOAT,
    TABLE_COLUMN_TYPE_COMPONENT
};

enum TableColumnAlignment
{
    TABLE_COLUMN_ALIGN_LEFT,
    TABLE_COLUMN_ALIGN_RIGHT,
    TABLE_COLUMN_ALIGN_CENTER
};

enum TableSelectionMode
{
    TABLE_SELECTION_NONE,
    TABLE_SELECTION_ONE,
    TABLE_SELECTION_MANY
};

class Table;

/**
 * @brief      This class describes a table selection listener. It registers to the Table
 *             object that will call it back whenever the selection is changed.
 */
class TableSelectionListener
{
  public:
    virtual void receiveTableSelection(std::set<int> selectedRowIndexes) = 0;
};

/**
 * @brief      This class describes a table cell container that can hold many
 *             types and that builds the appropriate component (if its type is not component).
 */
class TableCell : public juce::Component
{
  public:
    /**
     * @brief      Sets as text. One of the function that set value and
     *             determines type of the cell.
     */
    TableCell(std::string s, TableColumnAlignment align);

    /**
     * @brief      Sets as integer. One of the function that set value and
     *             determines type of the cell.
     */
    TableCell(int i, TableColumnAlignment align);

    /**
     * @brief      Sets as float. One of the function that set value and
     *             determines type of the cell.
     */
    TableCell(float f, TableColumnAlignment align);

    /**
     * @brief      Sets as component. One of the function that set value and
     *             determines type of the cell.
     */
    TableCell(std::shared_ptr<juce::Component> c);

    /**
     * @brief      Gets the juce UI component.
     *
     * @return     The component.
     */
    std::shared_ptr<juce::Component> getComponent();

    /**
     * @brief      Juce painting callback.
     */
    void paint(juce::Graphics &g) override;

    /**
     * @brief      Sets the text color.
     *
     * @param[in]  col   The new value
     */
    void setTextColor(juce::Colour col);

    /**
     * @brief      juce resize component (that we use for placing components)
     */
    void resized() override;

  private:
    TableType type;
    TableColumnAlignment alignment;
    std::string content;
    std::shared_ptr<juce::Component> subComponent;
    juce::Justification justification;
    juce::Colour textColor;

    /**
     * @brief      Sets the juce justification for text based on alignment.
     */
    void setJustification();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TableCell)
};

/**
 * @brief      This class manages data frames. Children classes are free to buffer
 *             and load rows as convenient, based on their needs and the corresponding table
 *             loading settings.
 */
class TableDataFrame
{
  public:
    TableDataFrame();

    /**
     * @brief      Gets the maximum index of row this data loader delivers.
     *             We can assume at some point we will allow to return -1 to indicate it's infinite
     *             or not known.
     *
     * @return     The maximum row index.
     */
    virtual int getMaxRowIndex() = 0;

    /**
     * @brief      Gets the row at index. Index as in table line index starting at 0.
     *
     * @param[in]  n     0 starting row number of the row.
     *
     * @return     The row.
     */
    virtual std::vector<std::shared_ptr<TableCell>> getRow(int n) = 0;

    /**
     * @brief      Gets the format of the dataframe, as a vector of
     *             column types paired with their alignment.
     *
     * @return     The format of the dataframe
     */
    virtual std::vector<std::pair<TableType, TableColumnAlignment>> &getFormat() = 0;

    /**
     * @brief      Gets the format of the dataframe header, it's a vector of string the size
     *             of the regular format basically.
     *
     * @return     The format of the dataframe
     */
    virtual std::vector<std::pair<TableType, TableColumnAlignment>> &getHeaderFormat() = 0;

    /**
     * @brief      Gets the ordering, as a pair of column id and a boolean
     *             indicating if ascending. It can return nothing, meaning there are no ordering
     *             available.
     *
     * @return     The ordering.
     */
    virtual std::optional<std::pair<int, bool>> &getOrdering() = 0;

    /**
     * @brief      Tried to set the specified ordering. If suceeding, the
     *             ids of the rows will be reset and you should clear the table content
     *             and query the rows again.
     *
     * @param[in]  ordering  The ordering as a pair a column id and boolean
     *             indicating if ascending.
     *
     * @return     true if suceeded, false if failed.
     */
    virtual bool trySettingOrdering(std::pair<int, bool> ordering) = 0;

    /**
     * @brief      Gets the header as a list of column names.
     *
     * @return     The vector of string column names.
     */
    virtual std::vector<std::string> &getColumnNames() = 0;

    /**
     * @brief      Gets the columns width for specific total width.
     *
     * @return     The columns width.
     */
    virtual std::vector<int> getColumnsWidth(int totalWidth) = 0;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TableDataFrame)
};

/**
 * @brief      Class that displays a set of rows for a table. It can be used
 *             for both headers and content and holds one component per cell.
 */
class TableRowsPainter : public juce::Component
{
  public:
    TableRowsPainter(std::vector<std::pair<TableType, TableColumnAlignment>> &format, TableSelectionMode rowSelectMode);
    void paint(juce::Graphics &g) override;
    void clear();
    void addRow(std::vector<std::shared_ptr<TableCell>> row);
    void setColumnsWidth(std::vector<int> columnsWdith);
    int getRowCount();
    int getWidth();
    void updateSize();
    void setTextColor(juce::Colour col);
    void mouseEnter(const juce::MouseEvent &me) override;
    void mouseMove(const juce::MouseEvent &me) override;
    void mouseExit(const juce::MouseEvent &me) override;
    void mouseDown(const juce::MouseEvent &me) override;

  private:
    int noColumns;
    std::vector<std::vector<std::shared_ptr<TableCell>>> rows;
    std::vector<int> columnsWidth;
    std::vector<juce::Rectangle<int>> rowCellsPositions;
    juce::Colour textColor;
    TableSelectionMode rowSelectionMode;
    int mouseOverRow;

    void refreshRowCellsPositions();
    void updateMouseRowHover(const juce::MouseEvent &me);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TableRowsPainter)
};

/**
 * @brief      Table component to display data frames with eventually widgets.
 */
class Table : public juce::Component
{
  public:
    /**
     * @brief      Constructs a new instance.
     */
    Table(std::string tableName, TableSelectionMode selectionType, TableDataFrame &df, int bufferingSize);

    /**
     * @brief      Paints the component, the title and the card appearance. Called by the juce library.
     */
    void paint(juce::Graphics &g) override;

    /**
     * @brief      Called when the component and its childs are resized. Called by the juce library.
     */
    void resized() override;

  private:
    TableSelectionMode selectionMode; /**< tell if we allow to select one, many, or none rows **/
    TableDataFrame &dataFrame;        /**< object that will give us rows content **/
    std::string name;                 /**< name of the table **/

    juce::Viewport contentViewport; /**< the scrollable are where the content rows are displayed **/
    TableRowsPainter header;        /**< header section **/
    TableRowsPainter content;       /**< component that displayed the list of rows **/

    int bufferingSize; /**< How many rows are loaded per bulk **/

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Table)
};

#endif // DEF_TABLE_HPP