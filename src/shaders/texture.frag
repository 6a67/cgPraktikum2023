// #version 330 core
// 
// out vec4 color;
//   
// in vec3 v2f_color;
// in vec2 v2f_texcoord;
// in vec3 v2f_modelSpaceNormal;
// 
// // uniform sampler2D ourTexture;
// 
// uniform samplerCube cubemap; 
// 
// void main()
// {
// 	gl_FragDepth = 0.99;
//     color = vec4(1.0,0,0,1); 
//     color = texture(cubemap, vec3(v2f_texcoord.xy,1.0));
// 	color = vec4(v2f_texcoord.xy,1,1);
// }

#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    FragColor = vec4(1,0,0,1);
	FragColor = texture(skybox, TexCoords);
}
