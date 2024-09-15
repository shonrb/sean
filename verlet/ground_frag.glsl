#version 420 core
precision highp float;
out vec4 fragColor;

in vec2 v_tex_coords;

uniform sampler2D tex_sampler;

void main() 
{
    fragColor = texture(tex_sampler, v_tex_coords);
}