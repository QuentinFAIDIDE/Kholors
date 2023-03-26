#ifndef DEF_FREQVIEW_SHADERS_HPP
#define DEF_FREQVIEW_SHADERS_HPP

#include <string>

std::string freqviewVertexShader =
    R"(
#version 330 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec4 ourColor;
out vec2 TexCoord;

uniform float viewPosition;
uniform float viewWidth;

void main()
{
    gl_Position = vec4( (2.0*((aPos.x-viewPosition)/viewWidth))-1.0, aPos.y, aPos.z, aPos.w);
    ourColor = aColor;
    TexCoord = aTexCoord;
}
)";

std::string freqviewFragmentShader =
    R"(
#version 330 core
out vec4 FragColor;
  
in vec4 ourColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
    FragColor = texture(ourTexture, TexCoord);
}
)";

#endif  // DEF_FREQVIEW_SHADERS_HPP