// Fragment shader
#version 150

in vec2 UV;
in vec3 pos_ws;
in vec4 colour;
in float size;
in float life;
in float max_life;

uniform bool show_quads;
uniform float alpha;
uniform vec4 init_col;
uniform vec4 final_col;
uniform float init_fuzz;
uniform float final_fuzz;

out vec4 frag_color;

float age = 1 - life / max_life;

vec4 colour_over_life(float life)
{
    return (1-age) * init_col + age * final_col;
}

float fuzz_circle(vec2 centre, float radius, float fuzzyness)
{
    vec2 diff = UV - centre;
    float norm = length(diff) * 2;
    float dxy = fwidth(norm);
    float circle = 1-smoothstep((1-fuzzyness)*radius-dxy, radius+dxy, norm);
    return circle;
}
    

void main()
{
    if (show_quads) {
        frag_color = vec4(UV, 0, alpha);
    } else {
        float n_life = 1-(max_life - life)/max_life;

        float fuzz = (1-age) * init_fuzz + age * final_fuzz;
        float circle = fuzz_circle(vec2(0.5, 0.5), 0.9, fuzz);

        vec4 colour = colour_over_life(life);

        frag_color = vec4(colour.rgb, circle * colour.a * alpha);
    }
}
