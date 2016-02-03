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
    
    /*vec2 cellSize = screenSize / gridSize;
    vec2 mouse = vec2(mouseX, mouseY);
    ivec2 mouseCellCoord = ivec2(mouse.xy / screenSize * gridSize);
    float brushSize = length(cellSize);
    float highlight = 1.0 - clamp(length(gl_FragCoord.xy - ((mouseCellCoord + 0.5) * cellSize)) / brushSize, 0.0, 1.0);
    */
    float alive = cell.w;
    
    color = vec4(alive, 0.0, 0.0, alive);
}