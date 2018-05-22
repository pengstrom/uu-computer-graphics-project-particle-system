// Fragment shader
#version 150

in vec2 UV;
in vec4 colour;

out vec4 frag_color;

void main()
{
    frag_color = colour;
}
