// Fragment shader
#version 150

in vec2 UV;
in vec3 pos_ws;
in vec4 colour;
in float size;
in float life;

uniform float max_life;

out vec4 frag_color;

vec3 color_over_life(float life)
{
    float age = max_life - life;
    if (age < 0.1 * max_life) {
        return vec3(1,1,1);
    }
    if (age < 0.4 * max_life) {
        return vec3(1, 0, 0);
    }
    
    return vec3 (0.1,0.1,0.1);
}

void main()
{
    float n_life = 1-(max_life - life)/max_life;

    vec2 centre = UV - vec2(0.5, 0.5);
    float norm = length(centre) * 2;
    float dxy = fwidth(norm);
    float circle = 1-smoothstep(0.4-dxy, 0.4+dxy, norm);

    vec3 color = color_over_life(life);
    frag_color = vec4(color, circle * n_life);
}
