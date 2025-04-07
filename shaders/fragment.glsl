#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform float ambientStrength;

void main()
{
    // 环境光
    vec3 ambient = ambientStrength * lightColor;
    
    // 漫反射
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 直接从纹理中获取颜色
    vec4 texColor = texture(texture1, TexCoord);
    
    // 最终颜色
    vec3 result = (ambient + diffuse) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
} 