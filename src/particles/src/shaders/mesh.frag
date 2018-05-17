// Fragment shader
#version 150

in vec3 v_norm;
in vec3 v_to_light;
in vec3 v_to_camera;
in vec3 v_model_norm;
in vec3 v_model_pos;
in vec3 v_world_pos;

out vec4 frag_color;

uniform vec3 u_amb_col;
uniform vec3 u_dif_col;
uniform vec3 u_spc_col;
uniform float u_spc_pow;
uniform vec3 u_light_col;
uniform bool u_draw_norm;
uniform bool u_gamma_correct;
uniform float u_time;

uniform samplerCube u_cubemap;

float aaline(float x, float w) {
    x = abs(x-0.5);
    float dxy = fwidth(x);
    float halfwidth = 0.5*w*dxy;
    float xmin = 0.0;
    float xmax = xmin + halfwidth;
    float z = 1-smoothstep(xmax, xmax + dxy, x);
    return z;
}

vec3 ambient() {
    return u_amb_col;
}

vec3 diffuse(vec3 N, vec3 L) {
    return u_dif_col * u_light_col * clamp(dot(N,L),0,1);
}

vec3 specular(vec3 N, vec3 H) {
    float normalization = (u_spc_pow + 8)/8;
    float spec = pow(dot(N,H),u_spc_pow);
    return  normalization * u_spc_col * u_light_col * spec;
}

float drive(float x) {
    return fract(x * 10);
}

float grid(float x) {
    float a = aaline(x, 1.0);
    return a;
}

vec4 gridZ() {
    float a = grid(fract(v_model_pos.z*20));
    return vec4(vec3(0.0, a, 0.0), a);
}

vec4 gridY() {
    float a = grid(drive(v_model_pos.y));
    return vec4(vec3(0.0, a, 0.0), a);
}

vec4 gridX() {
    float a = grid(fract(v_model_pos.x*20));
    return vec4(vec3(0.0, a, 0.0), a);
}

void main()
{

    if (u_draw_norm) {
        vec3 N = normalize(v_model_norm);
        frag_color = vec4(N.xyz, 1.0);
    } else {
        vec3 N = normalize(v_norm);
        vec3 L = normalize(v_to_light);
        vec3 V = normalize(v_to_camera);
        vec3 H = normalize(L+V);

        vec3 i_amb = ambient();

        vec3 i_dif = diffuse(N, L);

        vec3 i_spc = specular(N, H);

        vec3 I = i_amb + i_dif + i_spc;

        if (u_gamma_correct) {
            frag_color = vec4(pow(I, vec3(1/2.2)), 0.5);
        } else {
            frag_color = vec4(I, 0.5);
        }

        vec3 R = reflect(-V,N);
        //frag_color = vec4(texture(u_cubemap, R).rgb, 1.0);

        //frag_color = vec4(fwidth(v_model_pos.x*8), fwidth(v_model_pos.y*8), fwidth(v_model_pos.z*8), 1.0);

        //frag_color = vec4(vec3(fwidth(fract(v_model_pos.y*10))*4), 1.0);
        frag_color = gridY() + gridZ() + gridX();
    }
}
