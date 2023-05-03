#include "ActivityManager.h"

ActivityManager::ActivityManager()
{
    // TODO
}

ActivityManager::~ActivityManager()
{
    // TODO
}

AppState &ActivityManager::getAppState()
{
    return appState;
}

void ActivityManager::broadcastTask(std::shared_ptr<Task> task)
{
    // TODO
}