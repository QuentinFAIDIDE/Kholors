#ifndef DEF_ACTION_HPP
#define DEF_ACTION_HPP

class Action {
    virtual Action() = 0;
    virtual ~Action();
    virtual std::string Marshall();
    virtual Unmarshall(std::string &);
};

#endif // DEF_ACTION_HPP