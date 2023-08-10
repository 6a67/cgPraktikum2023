#version 330

precision mediump float;

// texcoords are in the normalized [0,1] range for the viewport-filling quad part of the triangle
in vec2 texcoords;

void main() {
    gl_FragColor = vec4(0.0, texcoords.xy, 1.0);
}

