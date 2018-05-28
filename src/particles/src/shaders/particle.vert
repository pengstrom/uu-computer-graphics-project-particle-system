// Vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec3 billboard_vert_pos;
layout(location = 1) in vec3 part_pos_ws;
layout(location = 2) in float part_size;
layout(location = 3) in float part_life;
layout(location = 4) in vec4 particle_colour;
layout(location = 5) in float part_max_life;

out vec2 UV;
out vec3 pos_ws;
out vec4 colour;
out float size;
out float life;
out float max_life;

uniform vec3 camera_up;
uniform vec3 camera_right;
uniform mat4 vp;

vec4 billboard_position() {
    vec3 pos  = part_pos_ws;
         pos += camera_up * billboard_vert_pos.y * part_size * (0.5/0.9);
         pos += camera_right * billboard_vert_pos.x * part_size * (0.5/0.9);

    return vp * vec4(pos, 1);
}

void main()
{
    UV = billboard_vert_pos.xy + vec2(0.5, 0.5);
    colour = particle_colour;
    pos_ws = part_pos_ws;
    size = part_size;
    life = part_life;

    max_life = part_max_life;

    gl_Position = billboard_position();
}
