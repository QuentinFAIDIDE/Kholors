#include "NumericInputId.h"

int noFixedInputs = 1;

// description of the default available input ids.
std::vector<std::string> defaultNumericInputIdsDescriptions = {"Master Track Tempo Value"};

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

std::string NumericInputManager::marshal()
{
    // TODO
}

Marshalable *NumericInputManager::unmarshal(std::string &)
{
    // TODO
}