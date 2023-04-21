#ifndef DEF_Marshalable_HPP
#define DEF_Marshalable_HPP

class Marshalable {
    virtual std::string Marshal() = 0;
    virtual Marshalable* Unmarshal() = 0;
};

#endif // DEF_Marshalable_HPP