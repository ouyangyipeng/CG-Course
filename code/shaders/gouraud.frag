#version 330 core
out vec4 FragColor;

in vec3 LightingColor;

void main()
{
    FragColor = vec4(LightingColor, 1.0);
}
