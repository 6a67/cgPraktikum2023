// #version 330 core
// 
// // layout (location = 0) in vec3 position;
// // layout (location = 1) in vec3 color;
// // layout (location = 2) in vec2 texcoord;
// 
// Out vec3 v2f_color;
// Out vec2 v2f_texcoord;
// Out vec3 v2f_modelSpaceNormal;
// 
// Void main()
// {
// 	// creates a single triangle that fills up the whole screen
// 	vec2 vertices[3]=vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1, 3));
// 
// 	// TODO: Check if this z value is sensible
// 	gl_Position = vec4(vertices[gl_VertexID],1,1);
// 
//     //v2f_color = color;
//     v2f_texcoord = 0.5 * gl_Position.xy + vec2(0.5);
// 	v2f_modelSpaceNormal = gl_Position.xyz;
// }


#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
} 
