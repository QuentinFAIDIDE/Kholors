#ifndef DEF_FREQVIEW_SHADERS_HPP
#define DEF_FREQVIEW_SHADERS_HPP

#include <string>

std::string sampleVertexShader =
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

std::string sampleFragmentShader =
    R"(
#version 330 core
out vec4 FragColor;
  
in vec4 ourColor; // unused for now
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
    FragColor = texture(ourTexture, TexCoord);
}
)";

std::string gridBackgroundVertexShader =
    R"(
#version 330 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec4 outColor;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, aPos.w);
    outColor = aColor;
}
)";

std::string gridBackgroundFragmentShader =
    R"(
#version 330 core
out vec4 FragColor;
  
in vec4 outColor;

uniform float viewHeightPixels;
uniform int grid0PixelWidth;
uniform int grid0PixelShift;

void main()
{
    if ( abs( (int(gl_FragCoord.x) % grid0PixelWidth) - grid0PixelShift ) < 0.75 ) {
        FragColor = vec4(0.6,0.6,0.6,1.0);
    } else if (abs(gl_FragCoord.y - 0.5 - (viewHeightPixels/2)) < 0.5 ) {
        FragColor = vec4(0.4,0.4,0.4,1.0);
    } else if (gl_FragCoord.y < 0.75 ) {
        FragColor = vec4(0.4,0.4,0.4,1.0);
    } else if (gl_FragCoord.y > viewHeightPixels-0.75) {
        FragColor = vec4(0.4,0.4,0.4,1.0);
    } else {
        FragColor = outColor;
    }
}
)";

#endif  // DEF_FREQVIEW_SHADERS_HPP