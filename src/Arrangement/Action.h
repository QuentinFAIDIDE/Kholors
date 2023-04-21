#ifndef DEF_ACTION_HPP
#define DEF_ACTION_HPP

class State {};

class Action: public Marshalable {
public:
    State* oldState;
};

#endif // DEF_ACTION_HPP