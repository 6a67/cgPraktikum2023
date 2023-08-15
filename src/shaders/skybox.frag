#version 330 core
precision mediump float;

out vec4 color;

in vec3 texcoord;

uniform samplerCube skybox;

void main()
{    
	color = texture(skybox, texcoord);
}
