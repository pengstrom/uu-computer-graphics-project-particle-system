// Vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec3 billboard_vert_pos;
layout(location = 1) in vec4 posSize;
layout(location = 2) in vec4 particle_colour;

out vec2 UV;
out vec4 colour;

uniform vec3 camera_up;
uniform vec3 camera_right;
uniform mat4 vp;

vec4 billboard_position() {
    float size = posSize.w;
    vec3 part_pos_ws = posSize.xyz;

    vec3 pos  = part_pos_ws;
         pos += camera_up * billboard_vert_pos.y * size;
         pos += camera_right * billboard_vert_pos.x * size;

    return vp * vec4(pos, 1);
}

void main()
{
    UV = billboard_vert_pos.xy + vec2(0.5, 0.5);
    colour = particle_colour;

    gl_Position = billboard_position();
    // gl_Position = vec4(0.5*billboard_vert_pos, 1);
}
