#version 330 core

precision mediump float;

in vec2 texcoords;
out vec4 FragColor;
in vec3 texcoord;


uniform int window_width;
uniform int window_height;
uniform float iTime;                 // shader playback time (in seconds)

uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, TexCoords);
}
