#ifndef DEF_TABLE_HPP
#define DEF_TABLE_HPP

#include <memory>
#include <set>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

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
class TableCell
{
  public:
    /**
     * @brief      Sets as text. One of the function that set value and
     *             determines type of the cell.
     */
    TableCell(std::string s);

    /**
     * @brief      Sets as integer. One of the function that set value and
     *             determines type of the cell.
     */
    TableCell(int i);

    /**
     * @brief      Sets as float. One of the function that set value and
     *             determines type of the cell.
     */
    TableCell(float f);

    /**
     * @brief      Sets as component. One of the function that set value and
     *             determines type of the cell.
     */
    TableCell(std::shared_ptr<juce::Component> c);

    TableType type;
    std::string textValue;
    int integerValue;
    float floatValue;
    std::shared_ptr<juce::Component> component;
};

/**
 * @brief      This class manages data frames. Children classes are free to buffer
 *             and load rows as convenient, based on their needs and the corresponding table
 *             loading settings.
 */
class TableDataFrame
{
  public:
    /**
     * @brief      Gets the maximum number of rows this data loader delivers.
     *             We can assume at some point we will allow to return -1 to indicate it's infinite
     *             or not known.
     *
     * @return     The maximum row number.
     */
    virtual int getMaxRowNumber() = 0;

    /**
     * @brief      Gets the row at index. Index as in table line index starting at 0.
     *
     * @param[in]  n     0 starting row number of the row.
     *
     * @return     The row.
     */
    virtual std::vector<TableCell> &getRow(int n) = 0;

    /**
     * @brief      Gets the format of the dataframe, as a vector of
     *             column types paired with their alignment.
     *
     * @return     The format of the dataframe
     */
    virtual std::vector<std::pair<TableType, TableColumnAlignment>> &getFormat() = 0;

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
};

/**
 * @brief      Class that displays a set of rows for a table. It can be used
 *             for both headers and content and holds one component per cell.
 */
class TableRowsPainter : public juce::Component
{
  public:
    TableRowsPainter(std::vector<std::pair<TableType, TableColumnAlignment>> &format);
    void paint(juce::Graphics &g);
    void clear();
    void addRow(std::vector<TableCell> &row);

  private:
    std::vector<int> columnsWidth;
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
    Table(std::string tableName, TableSelectionMode selectionType, TableDataFrame &df);

    /**
     * @brief      Paints the component. Called by the juce library.
     */
    void paint(juce::Graphics &g) override;

    /**
     * @brief      Called when the component and its childs are resized. Called by the juce library.
     */
    void resized() override;

  private:
    TableSelectionMode selectionMode;
    TableDataFrame &dataFrame;
    std::vector<int> columnsWidth;

    juce::Viewport contentViewport;
    TableRowsPainter header;
    TableRowsPainter content;
};

#endif // DEF_TABLE_HPP