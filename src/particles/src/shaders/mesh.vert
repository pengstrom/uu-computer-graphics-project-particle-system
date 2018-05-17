// Vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec3 a_normal;

out vec3 v_model_norm;
out vec3 v_norm;
out vec3 v_to_light;
out vec3 v_to_camera;
out vec3 v_model_pos;
out vec3 v_world_pos;

uniform mat4 u_m;
uniform mat4 u_v;
uniform mat4 u_mv;
uniform mat4 u_mvp;
uniform mat4 u_normal_matrix;
uniform vec3 u_light_pos;
uniform vec3 u_camera_pos;

void main()
{
    vec4 view_pos = u_mv * a_position;
    vec4 world_pos = u_m * a_position;

    v_model_pos = a_position.xyz;
    v_world_pos = world_pos.xyz;

    // v_view_norm = normalize((u_mv * vec4(a_normal, 1.0)).xyz);
    // v_view_light = normalize(view_pos.xyz - (u_v * vec4(u_light_pos,1.0)).xyz);
    // v_view_vector = - normalize(view_pos.xyz);

    //v_norm = normalize((u_m * vec4(a_normal, 1.0)).xyz);
    //v_to_light = normalize(u_light_pos - world_pos.xyz);
    //v_to_camera = normalize(u_camera_pos - world_pos.xyz);

    v_norm = normalize((u_normal_matrix * vec4(a_normal, 1.0)).xyz);
    v_to_light = (u_v * vec4(u_light_pos,1.0)).xyz - view_pos.xyz;
    v_to_camera = - view_pos.xyz;

    v_model_norm = a_normal;

    gl_Position = u_mvp * a_position;
}
