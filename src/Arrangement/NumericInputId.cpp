#include "NumericInputId.h"

NumericInputManager::NumericInputManager()
{
    // iterate over static input ids and store em
    for (int i = 0; i < noFixedInputs; i++)
    {
        numericInputIds.insert(i);
        numericInputDescriptions[i] = defaultNumericInputIdsDescriptions[i];
    }
    lowestUnallocatedInputId = noFixedInputs;
}

int NumericInputManager::allocateId(std::string description)
{
    numericInputIds.insert(lowestUnallocatedInputId);
    numericInputDescriptions[lowestUnallocatedInputId] = description;
    lowestUnallocatedInputId++;
    return lowestUnallocatedInputId - 1;
}

void NumericInputManager::freeId(int i)
{
    numericInputIds.erase(i);
    numericInputDescriptions.erase(i);
}