#ifndef DEF_WAITGROUP_HPP
#define DEF_WAITGROUP_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

/**
 * @brief A simple synchronisation routine
 *        inspired from golang WaitGroups.
 *
 *  Stolen from here: https://github.com/dragonquest/cpp-channels/blob/master/waitgroup.h
 *  Minus the race fix of issue #1
 */
class WaitGroup
{
  public:
    void Add(int i = 1);
    void Done();
    void Wait();

  private:
    std::mutex mu_;
    std::atomic<int> counter_;
    std::condition_variable cv_;
};

#endif // DEF_WAITGROUP_HPP