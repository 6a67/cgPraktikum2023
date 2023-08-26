
#version 330

precision mediump float;

in vec3  v2f_normal;
in vec2  v2f_tex;
in vec3  v2f_view;
in vec3  v2f_color;
in vec4  v2f_pos;

uniform float reflectiveness;

uniform bool   use_lighting;
uniform bool   use_texture;
uniform bool   use_srgb;
uniform bool   use_vertex_color;
uniform vec3   front_color;
uniform vec3   back_color;
uniform float  ambient;
uniform float  diffuse;
uniform float  specular;
uniform float  shininess;
uniform float  alpha;
uniform vec3   light1;
uniform vec3   light2;

uniform vec3 cam_direction;

uniform int window_width;
uniform int window_height;
uniform float iTime;

// uniform sampler2D mytexture;
uniform samplerCube cubetexture;


out vec4 f_color;

void main()
{
    vec3 color = use_vertex_color ? v2f_color : (gl_FrontFacing ? front_color : back_color);

	float alive = color == vec3(1.0, 1.0, 1.0) ? 0.0 : 1.0;

    vec3 rgb;

    if (use_lighting)
    {
        vec3 L1 = normalize(light1);
        vec3 L2 = normalize(light2);
        vec3 V  = normalize(v2f_view);
        vec3 N  = gl_FrontFacing ? normalize(v2f_normal) : -normalize(v2f_normal);
        vec3 R;
        float NL, RV;

        rgb = ambient * 0.1 * color;

        NL = dot(N, L1);
        if (NL > 0.0)
        {
            rgb += diffuse * NL * color;
            R  = normalize(-reflect(L1, N));
            RV = dot(R, V);
            if (RV > 0.0)
            {
                // rgb += vec3( specular * pow(RV, shininess) );
            }
        }


         NL = dot(N, L2);
         if (NL > 0.0)
         {
             rgb += diffuse * NL * color;
             R  = normalize(-reflect(L2, N));
             RV = dot(R, V);
             if (RV > 0.0)
             {
                //  rgb += vec3( specular * pow(RV, shininess) );
             }
         }

		 vec3 Re  = reflect(V, N);
		 vec4 reflection = texture(cubetexture,vec3(Re.x, Re.y, Re.z));
		 // rgb = mix(rgb, reflection.xyz, 0.3 * alive);
		 rgb = mix(rgb, reflection.xyz, reflectiveness);
    }

    // do not use lighting
    else
    {
        rgb = color;
    }

    rgb += shininess * 0.00001 + specular * 0.00001;

    // if (use_texture) rgb *= texture(mytexture, v2f_tex).xyz;
    if (use_srgb)    rgb  = pow(clamp(rgb, 0.0, 1.0), vec3(0.45));

    f_color = vec4(rgb, alpha);
}
