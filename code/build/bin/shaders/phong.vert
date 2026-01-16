#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix; // Pass this from C++: transpose(inverse(mat3(view * model)))

void main()
{
    // Transform vertex position to view space
    vec4 viewPos4 = view * model * vec4(aPos, 1.0);
    FragPos = vec3(viewPos4);
    
    // Transform normal to view space
    Normal = normalize(normalMatrix * aNormal);
    
    gl_Position = projection * viewPos4;
}
