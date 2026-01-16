#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 LightingColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

uniform vec3 lightPos; // View Space
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;

void main()
{
    vec4 viewPos4 = view * model * vec4(aPos, 1.0);
    vec3 FragPos = vec3(viewPos4);
    vec3 Normal = normalize(normalMatrix * aNormal);
    
    // Ambient
    vec3 ambient = ambientStrength * lightColor;
  
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;
    
    // Specular
    vec3 viewDir = normalize(-FragPos); 
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;  
        
    LightingColor = (ambient + diffuse + specular) * objectColor;
    
    gl_Position = projection * viewPos4;
}
