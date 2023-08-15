#version 330

// texcoords are in the normalized [0,1] range for the viewport-filling quad part of the triangle
out vec2 texcoords;

uniform vec3 viewRotation;
uniform vec3 origin;

out vec3 v2f_viewRotation;
out vec3 v2f_origin;


void main() {
        // creates a single triangle that fills up the whole screen
        vec2 vertices[3]=vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1, 3));

		// TODO: Check if this z value is sensible
        gl_Position = vec4(vertices[gl_VertexID],0.1,1);
        texcoords = 0.5 * gl_Position.xy + vec2(0.5);

		v2f_origin = origin;
		v2f_viewRotation = viewRotation;
}
