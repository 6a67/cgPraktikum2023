#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 texcoord;

uniform mat4 projection;
uniform mat4 view;
uniform bool offsetSkybox;

void main()
{
    texcoord = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
	if(offsetSkybox) {
	  // set depth value so that we can look into the cube the outside
	  gl_Position = vec4(pos.x, pos.y, -pos.z + 5, pos.w);
	}else{
	  gl_Position = pos.xyww;
	}
} 
