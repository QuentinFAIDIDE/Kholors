#ifndef DEF_VERTEX_HPP
#define DEF_VERTEX_HPP

// a structure to pass data to openGL as buffers
struct Vertex
{
    float position[2];
    float colour[4];
    float texturePosition[2];
};

#endif // DEF_VERTEX_HPP