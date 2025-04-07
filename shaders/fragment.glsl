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
uniform bool isSun;

void main()
{
    // 直接从纹理中获取颜色
    vec4 texColor = texture(texture1, TexCoord);
    
    // 如果是太阳，直接使用纹理颜色并增强亮度
    if(isSun) {
        // 使太阳发光，增强亮度
        vec3 sunColor = texColor.rgb * 1.5;
        FragColor = vec4(sunColor, texColor.a);
    }
    else {
        // 环境光
        vec3 ambient = ambientStrength * lightColor;
        
        // 漫反射
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        
        // 最终颜色
        vec3 result = (ambient + diffuse) * texColor.rgb;
        FragColor = vec4(result, texColor.a);
    }
} 