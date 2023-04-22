#ifndef DEF_ACTION_HPP
#define DEF_ACTION_HPP

#include "Marshalable.h"
class State
{
};

class Action : public Marshalable
{
  public:
    State *oldState;
    std::string Marshal() override final
    {
        return "none";
    };
    Marshalable *Unmarshal(std::string &) override final
    {
        return nullptr;
    };
};

#endif // DEF_ACTION_HPP