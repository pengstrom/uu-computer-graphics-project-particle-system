// Fragment shader
#version 150

in vec2 UV;
in vec3 pos_ws;
in vec4 colour;
in float size;
in float life;

uniform float max_life;
uniform bool show_quads;

out vec4 frag_color;

vec3 color_over_life(float life)
{
    float n_age = 1 - life / max_life;
    float c = 0.1 + n_age * 0.7;
    return vec3(c, c, 1);
}

void main()
{
    if (show_quads) {
        frag_color = vec4(UV, 0, 1);
    } else {
        float n_life = 1-(max_life - life)/max_life;

        vec2 centre = UV - vec2(0.5, 0.5);
        float norm = length(centre) * 2;
        float dxy = fwidth(norm);
        float circle = 1-smoothstep(0.4-dxy, 0.4+dxy, norm);

        vec3 color = color_over_life(life);

        frag_color = vec4(color, circle * n_life * 0.05);
    }
}
