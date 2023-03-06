#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec3 objectColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

uniform float aV = 0.5;
uniform float dV = 0.5;
uniform float sV = 0.5;
uniform float sE = 1.0;
uniform float constantAt = 0.5;
uniform float linearAt = 0.5;
uniform float squareAt = 0.5;

void main()
{
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), sE);
    

    vec3 ambiental = (lightColor * aV);
    vec3 diffuse = lightColor * dV * max(dot(norm, lightDir), 0.0);
    vec3 specularar = sV * spec * lightColor;

    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (constantAt + linearAt + squareAt * distance * distance);

    FragColor = vec4(ambiental + attenuation * (diffuse + specularar), 1.0) * vec4(objectColor, 1.0);
}