#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
in vec2 TexCoords;
flat in int materialID;

uniform vec3 lightPos; 
uniform vec3 viewPos; 

uniform vec3 ambient[6];  // Adjust size as needed
uniform vec3 diffuse[6];  // Adjust size as needed
uniform vec3 specular[6]; // Adjust size as needed
uniform float shininess[6]; // Adjust size as needed

uniform int overrideMaterialID = -1;  // Default value indicating no override

uniform sampler2D ourTexture;

void main()
{
    // Determine active material ID
    int activeMaterialID = (overrideMaterialID != -1) ? overrideMaterialID : materialID;

    // ambient
    float ambientStrength = 0.1;
    vec3 ambientLight = ambientStrength * ambient[activeMaterialID];
    
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuseLight = diff * diffuse[activeMaterialID];
    
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 16.0);
    vec3 specularLight = shininess[activeMaterialID] * spec * specular[activeMaterialID];  
    
    // Combine lighting components
    vec3 result = (ambientLight + diffuseLight + specularLight);
    FragColor = vec4(result, 1.0);
}