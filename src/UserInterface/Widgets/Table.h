#ifndef DEF_TABLE_HPP
#define DEF_TABLE_HPP

#include <set>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

enum TableColumnType
{
    TABLE_COLUMN_TYPE_TEXT,
    TABLE_COLUMN_TYPE_INT,
    TABLE_COLUMN_TYPE_FLOAT,
    TABLE_COLUMN_TYPE_COMPONENT
};

enum TableSelectionSetting
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
  protected:
    virtual void receiveTableSelection(std::set<int> selectedRowIndexes) = 0;
};

/**
 * @brief      Table component to display data frames with eventually widgets.
 */
class Table : public juce::Component
{
    /**
     * @brief      Constructs a new instance.
     */
    Table(std::string tableName, TableSelectionSetting selectionType);

    /**
     * @brief      Adds a column to the row.
     *
     * @param[in]  columnName  The column name
     * @param[in]  columnType  The column type
     */
    void addColumn(std::string columnName, TableColumnType columnType);
};

#endif // DEF_TABLE_HPP