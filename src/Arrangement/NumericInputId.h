#ifndef DEF_NUMERIC_INPUT_ID_HPP
#define DEF_NUMERIC_INPUT_ID_HPP

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

// these are special numerical inputs ids which are always
// fixed
enum FixedNumericInputId
{
    NUM_INPUT_ID_TEMPO = 0
};

/**
 * @brief      This class describes a numeric input manager.
 *             It allocates unique random input ids
 *             for numeric inputs, and also initialize
 *             itself with the fixed one (eg: tempo).
 */
class NumericInputManager
{
  public:
    /**
     * @brief      Constructs a new instance.
     */
    NumericInputManager();

    /**
     * @brief      assign an id for a new numeric input.
     *
     * @param[in]  description  The description of the new input we allocate.
     *
     * @return     the identifier allocated
     */
    int allocateId(std::string description);

    /**
     * @brief      Let the manager know this id is not used anymore and can be
     *             assigned to new inputs.
     *
     * @param[in]  i     the id to free
     */
    void freeId(int i);

  private:
    // the lowest unallocated numeric input id
    int lowestUnallocatedInputId;

    // the set of allocated numeric input ids.
    std::set<int> numericInputIds;

    // description of the numerical inputs to modify.
    // To be used by the api later on.
    std::map<int, std::string> numericInputDescriptions;
};

#endif // DEF_NUMERIC_INPUT_ID_HPP