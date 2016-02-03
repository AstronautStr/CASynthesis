#version 330 core
layout(origin_upper_left) in vec4 gl_FragCoord;

uniform float time;

uniform float gridWidth;
uniform float gridHeight;

uniform float screenWidth;
uniform float screenHeight;

uniform float mouseX;
uniform float mouseY;

uniform samplerBuffer gridSampler;

out vec4 color;


void main()
{
    vec2 screenSize = vec2(screenWidth, screenHeight);
    vec2 gridSize = vec2(gridWidth, gridHeight);
    
    ivec2 cellCoord = ivec2(gl_FragCoord.xy / screenSize * gridSize);
    vec4 cell = texelFetch(gridSampler, int(cellCoord.x * gridWidth) + cellCoord.y);
    
    float alive = cell.w;
    float freq = log(cell.y / 20000.0);
    float energy = cell.x;
    
    color = vec4(1.0, freq, 0.0, 1.0) * energy;
}