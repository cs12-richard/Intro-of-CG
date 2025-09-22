#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
in vec2 TexCoord; 

uniform vec3 objectColor; 

void main()
{
    vec3 lightPos = vec3(0,100,50);
    vec3 lightColor = vec3(1,1,1);
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
        
    vec3 result = diffuse * objectColor;
    FragColor = vec4(result, 1.0);
} 