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

out vec4 frag_color;

vec4 colour_over_life(float life)
{
    float n_age = 1 - life / max_life;
    return (1-n_age) * init_col + n_age * final_col;
}

void main()
{
    if (show_quads) {
        frag_color = vec4(UV, 0, alpha);
    } else {
        float n_life = 1-(max_life - life)/max_life;

        vec2 centre = UV - vec2(0.5, 0.5);
        float norm = length(centre) * 2;
        float dxy = fwidth(norm);
        float circle = 1-smoothstep(0.4-dxy, 0.4+dxy, norm);

        vec4 colour = colour_over_life(life);

        frag_color = vec4(colour.rgb, circle * colour.a * alpha);
    }
}
