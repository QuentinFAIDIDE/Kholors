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
  
in vec4 ourColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
    FragColor = vec4(ourColor.x, ourColor.y, ourColor.z, texture(ourTexture, TexCoord).a);
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
uniform float grid0PixelWidth;
uniform int grid0PixelShift;
uniform float grid1PixelWidth;
uniform int grid1PixelShift;
uniform float grid2PixelWidth;
uniform int grid2PixelShift;

void main()
{
    float grid0position = (gl_FragCoord.x - float(grid0PixelShift)) / grid0PixelWidth;
    float grid1position = (gl_FragCoord.x - float(grid1PixelShift)) / grid1PixelWidth;
    float grid2position = (gl_FragCoord.x - float(grid2PixelShift)) / grid2PixelWidth;

    // horizontal bars
    if (abs(gl_FragCoord.y - 0.5 - (viewHeightPixels/2)) < 0.5 ) {
        FragColor = vec4(0.4,0.4,0.4,1.0);
    } else if (gl_FragCoord.y < 0.75 ) {
        FragColor = vec4(0.4,0.4,0.4,1.0);
    } else if (gl_FragCoord.y > viewHeightPixels-0.75) {
        FragColor = vec4(0.4,0.4,0.4,1.0);
    
    // vertical grid tempo bars
    } else if ( abs( grid0position - round(grid0position) )*grid0PixelWidth < 1.5 && grid0PixelWidth > 25 ) {
        FragColor = vec4(0.4,0.4,0.4,1.0);
    } else if ( abs( grid1position - round(grid1position) )*grid1PixelWidth < 1.5 && grid1PixelWidth > 25 ) {
        FragColor = vec4(0.2,0.2,0.2,1.0);
    } else if ( abs( grid2position - round(grid2position) )*grid2PixelWidth < 1.5 && grid2PixelWidth > 15 ) {
        FragColor = vec4(0.15,0.15,0.15,1.0);

    // background
    } else {
        FragColor = outColor;
    }
}
)";

#endif  // DEF_FREQVIEW_SHADERS_HPP