#ifndef DEF_GRIDLEVEL_HPP
#define DEF_GRIDLEVEL_HPP

typedef struct GridLevel
{
    // which number to divide full bar height to get this one
    float subdivisions;
    // which amound of while over 255 to use
    int shade;
} GridLevel;

#endif // DEF_GRIDLEVEL_HPP