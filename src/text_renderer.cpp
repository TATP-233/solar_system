#include "../include/text_renderer.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

// 加载着色器函数
std::string loadShaderSource(const char* filePath);

// 创建着色器程序
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource);

TextRenderer::TextRenderer(unsigned int width, unsigned int height)
{
    // 加载并创建着色器程序
    this->shader = createShaderProgram("shaders/text_vertex.glsl", "shaders/text_fragment.glsl");
    
    // 设置投影矩阵
    this->projection = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
    
    // 设置着色器变量
    glUseProgram(this->shader);
    glUniformMatrix4fv(glGetUniformLocation(this->shader, "projection"), 1, GL_FALSE, glm::value_ptr(this->projection));
    
    // 配置VAO/VBO用于文本四边形
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glBindVertexArray(this->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

TextRenderer::~TextRenderer()
{
    // 清理资源
    for (auto& ch : Characters)
    {
        glDeleteTextures(1, &ch.second.TextureID);
    }
    glDeleteVertexArrays(1, &this->VAO);
    glDeleteBuffers(1, &this->VBO);
    glDeleteProgram(this->shader);
}

bool TextRenderer::Load(std::string font, unsigned int fontSize)
{
    // 清除之前加载的字符
    this->Characters.clear();
    
    // 初始化FreeType库
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return false;
    }
    
    // 加载字体
    FT_Face face;
    if (FT_New_Face(ft, font.c_str(), 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return false;
    }
    
    // 设置字体大小
    FT_Set_Pixel_Sizes(face, 0, fontSize);
    
    // 禁用字节对齐限制
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    // 加载前128个ASCII字符
    for (GLubyte c = 0; c < 128; c++)
    {
        // 加载字符的字形
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYPE: Failed to load Glyph" << std::endl;
            continue;
        }
        
        // 生成纹理
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        
        // 设置纹理选项
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // 存储字符
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<GLuint>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }
    
    // 清理资源
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    
    return true;
}

void TextRenderer::RenderText(std::string text, float x, float y, float scale, glm::vec3 color)
{
    // 激活对应的渲染状态
    glUseProgram(this->shader);
    glUniform3f(glGetUniformLocation(this->shader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(this->VAO);
    
    // 遍历文本中的所有字符
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];
        
        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
        
        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        
        // 为每个字符更新VBO
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        
        // 渲染字形纹理
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        
        // 更新VBO内存
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        // 渲染四边形
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // 更新位置到下一个字形
        x += (ch.Advance >> 6) * scale; // 位偏移是以1/64像素表示的，所以需要除以64
    }
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
} 