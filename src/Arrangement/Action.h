#ifndef DEF_ACTION_HPP
#define DEF_ACTION_HPP

#include "Marshalable.h"
class State {};

class Action : public Marshalable {
 public:
  State* oldState;
  std::string Marshal() override final;
  Marshalable* Unmarshal(std::string&) override final;
};

#endif  // DEF_ACTION_HPP