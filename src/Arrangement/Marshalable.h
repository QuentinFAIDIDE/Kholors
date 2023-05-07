#ifndef DEF_MARSHALABLE_HPP
#define DEF_MARSHALABLE_HPP

#include <string>

class Marshalable
{
    virtual std::string marshal() = 0;
    virtual Marshalable *unmarshal(std::string &) = 0;
};

#endif // DEF_MARSHALABLE_HPP