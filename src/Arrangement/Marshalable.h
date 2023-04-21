#ifndef DEF_MARSHALABLE_HPP
#define DEF_MARSHALABLE_HPP

#include <string>

class Marshalable {
  virtual std::string Marshal() = 0;
  virtual Marshalable* Unmarshal(std::string&) = 0;
};

#endif  // DEF_MARSHALABLE_HPP