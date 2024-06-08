#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
in vec2 TexCoords;
flat in int materialID;

uniform vec3 lightPos; 
uniform vec3 viewPos; 

uniform vec3 ambient[100];  // Adjust size as needed
uniform vec3 diffuse[100];  // Adjust size as needed
uniform vec3 specular[100]; // Adjust size as needed
uniform float shininess[100]; // Adjust size as needed

uniform int overrideMaterialID = -1;  // Default value indicating no override

uniform sampler2D ambientTexture;
uniform sampler2D diffuseTexture;
uniform sampler2D alphaTexture;

uniform bool useAlphaMap = false; // Controls if alpha map is used

void main()
{
    // Determine active material ID
    int activeMaterialID = (overrideMaterialID != -1) ? overrideMaterialID : materialID;

    // Sample textures
    vec3 ambientColor = texture(ambientTexture, TexCoords).rgb;
    vec3 diffuseColor = texture(diffuseTexture, TexCoords).rgb;
    float alphaValue = useAlphaMap ? texture(alphaTexture, TexCoords).r : 1.0;
	// float alphaValue = 1.0;

    // Calculate ambient, diffuse, and specular components
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuseLight = diff * diffuse[activeMaterialID] * diffuseColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 16.0);
    vec3 specularLight = shininess[activeMaterialID] * spec * specular[activeMaterialID];

    vec3 ambientLight = ambient[activeMaterialID] * ambientColor;

    vec3 result = ambientLight + diffuseLight + specularLight;

    // Set the final fragment color with the alpha value
    FragColor = vec4(result, alphaValue);
}