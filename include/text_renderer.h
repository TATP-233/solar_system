#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <map>
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <ft2build.h>
#include FT_FREETYPE_H

// 存储字符的结构体
struct Character {
    GLuint TextureID;   // 字符纹理ID
    glm::ivec2 Size;    // 字符大小
    glm::ivec2 Bearing; // 从基线到字符左侧/顶部的偏移量
    GLuint Advance;     // 到下一个字符的水平偏移量
};

class TextRenderer {
public:
    TextRenderer(unsigned int width, unsigned int height);
    ~TextRenderer();
    
    // 加载字体
    bool Load(std::string font, unsigned int fontSize);
    
    // 渲染文本
    void RenderText(std::string text, float x, float y, float scale, glm::vec3 color = glm::vec3(1.0f));
    
private:
    // 着色器程序
    GLuint shader;
    
    // 投影矩阵
    glm::mat4 projection;
    
    // 字符映射表
    std::map<char, Character> Characters;
    
    // VAO和VBO
    GLuint VAO, VBO;
};

#endif // TEXT_RENDERER_H 