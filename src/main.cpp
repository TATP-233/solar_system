#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip> // 用于格式化输出
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../include/text_renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// 窗口尺寸
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// 自转与公转速度
float rotationSpeed = 1.0f;
float orbitSpeed = 0.5f;

// 相机控制参数
glm::vec3 cameraPos = glm::vec3(0.0f, 100.0f, 230.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraZoom = 15.0f;

// 默认相机参数（用于重置）
const glm::vec3 DEFAULT_CAMERA_POS = glm::vec3(0.0f, 100.0f, 230.0f);
const glm::vec3 DEFAULT_CAMERA_TARGET = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 DEFAULT_CAMERA_UP = glm::vec3(0.0f, 1.0f, 0.0f);
const float DEFAULT_CAMERA_ZOOM = 15.0f;

// 鼠标控制参数
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool leftMousePressed = false;
bool rightMousePressed = false;
float mouseSpeed = 0.5f;
float scrollSpeed = 2.0f;

// 控制标志
bool showPlanetNames = true;  // 显示行星名称
int currentFont = 0;          // 当前使用的字体

// 轨迹点最大数量
const int MAX_TRAIL_POINTS = 200;

// 字体路径
const char* FONT_PATHS[] = {
    "fonts/Helvetica.ttc",
    "fonts/MarkerFelt.ttc"
};

// 行星数据结构
struct Planet {
    std::string name;      // 行星名称
    float radius;          // 半径
    float distance;        // 与太阳的距离
    float orbitSpeed;      // 公转速度
    float rotationSpeed;   // 自转速度
    float tilt;            // 轴倾角
    float currentOrbitAngle; // 当前公转角度
    float currentRotationAngle; // 当前自转角度
    GLuint textureID;      // 纹理ID
    std::vector<glm::vec3> trailPoints; // 轨迹点
    float baseOrbitSpeed;    // 基础公转速度
    float baseRotationSpeed; // 基础自转速度
};

// 全局变量
GLuint trailShaderProgram;
std::vector<Planet> planets;  // 行星数组改为全局变量
Planet moon;                  // 月球也改为全局变量

// 球体生成函数
void generateSphere(std::vector<float>& vertices, std::vector<float>& normals, 
                   std::vector<float>& texCoords, std::vector<unsigned int>& indices,
                   float radius, unsigned int sectors, unsigned int stacks) {
    // 清空旧数据
    vertices.clear();
    normals.clear();
    texCoords.clear();
    indices.clear();
    
    float x, y, z, xy;                              // 顶点位置
    float nx, ny, nz, lengthInv = 1.0f / radius;    // 顶点法线
    float s, t;                                     // 顶点纹理坐标

    float sectorStep = 2 * M_PI / sectors;
    float stackStep = M_PI / stacks;
    float sectorAngle, stackAngle;

    // 生成球体顶点、法线和纹理坐标
    for (unsigned int i = 0; i <= stacks; ++i) {
        stackAngle = M_PI / 2 - i * stackStep;      // 从pi/2到-pi/2
        xy = radius * cosf(stackAngle);             // r * cos(u)
        z = radius * sinf(stackAngle);              // r * sin(u)

        // 每个stack增加(sectors+1)个顶点
        // 第一个和最后一个顶点具有相同的位置和法线，但纹理坐标不同
        for (unsigned int j = 0; j <= sectors; ++j) {
            sectorAngle = j * sectorStep;           // 从0到2pi

            // 顶点位置 (x, y, z)
            x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // 归一化顶点法线 (nx, ny, nz)
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            normals.push_back(nx);
            normals.push_back(ny);
            normals.push_back(nz);

            // 顶点纹理坐标 (s, t) 范围在[0, 1]
            s = (float)j / sectors;
            t = (float)i / stacks;
            texCoords.push_back(s);
            texCoords.push_back(t);
        }
    }

    // 生成球体三角形索引
    // k1--k1+1
    // |  / |
    // | /  |
    // k2--k2+1
    unsigned int k1, k2;
    for (unsigned int i = 0; i < stacks; ++i) {
        k1 = i * (sectors + 1);     // 当前stack的起始索引
        k2 = k1 + sectors + 1;      // 下一个stack的起始索引

        for (unsigned int j = 0; j < sectors; ++j, ++k1, ++k2) {
            // 每个扇区两个三角形，但第一个和最后一个stack只有一个三角形
            
            // k1 => k2 => k1+1
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            // k1+1 => k2 => k2+1
            if (i != (stacks - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

// 加载着色器代码
std::string loadShaderSource(const char* filePath) {
    std::string shaderCode;
    std::ifstream shaderFile;

    // 确保ifstream对象可以抛出异常
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try {
        // 打开文件
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        
        // 读取文件内容到流中
        shaderStream << shaderFile.rdbuf();
        
        // 关闭文件
        shaderFile.close();
        
        // 将流转换为字符串
        shaderCode = shaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }
    
    return shaderCode;
}

// 加载纹理函数
GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // 加载并生成纹理
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format, internalFormat;
        if (nrChannels == 1) {
            format = GL_RED;
            internalFormat = GL_RED;
        }
        else if (nrChannels == 3) {
            format = GL_RGB;
            internalFormat = GL_RGB8;
        }
        else if (nrChannels == 4) {
            format = GL_RGBA;
            internalFormat = GL_RGBA8;
        }
            
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        std::cout << "Texture loaded: " << path << " (" << width << "x" << height << ", " << nrChannels << " channels)" << std::endl;
    } else {
        std::cout << "Failed to load texture: " << path << std::endl;
    }
    stbi_image_free(data);
    
    return textureID;
}

// 鼠标移动回调函数
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = ypos - lastY;
    lastX = xpos;
    lastY = ypos;
    
    // 左键按下：旋转视角
    if (leftMousePressed) {
        float rotX = -yoffset * mouseSpeed * 0.01f;
        float rotY = -xoffset * mouseSpeed * 0.01f;
        
        // 计算相机到目标的方向向量
        glm::vec3 direction = normalize(cameraPos - cameraTarget);
        
        // 计算相机右向量和上向量
        glm::vec3 right = normalize(cross(cameraUp, direction));
        glm::vec3 up = cross(direction, right);
        
        // 创建旋转矩阵
        glm::mat4 rotMatrix = glm::rotate(glm::mat4(1.0f), rotY, cameraUp);
        rotMatrix = glm::rotate(rotMatrix, rotX, right);
        
        // 应用旋转
        glm::vec4 newPos = rotMatrix * glm::vec4(cameraPos - cameraTarget, 1.0f);
        cameraPos = cameraTarget + glm::vec3(newPos);
    }
    
    // 右键按下：平移视角
    if (rightMousePressed) {
        // 计算相机到目标的方向向量
        glm::vec3 direction = normalize(cameraPos - cameraTarget);
        
        // 计算相机右向量和上向量
        glm::vec3 right = normalize(cross(cameraUp, direction));
        glm::vec3 up = cross(direction, right);
        
        // 计算平移量
        glm::vec3 pan = (-right * xoffset * mouseSpeed + up * yoffset * mouseSpeed) * 0.25f * cameraZoom / 45.0f;
        
        // 应用平移
        cameraPos += pan;
        cameraTarget += pan;
    }
}

// 鼠标按键回调函数
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS)
            leftMousePressed = true;
        else if (action == GLFW_RELEASE)
            leftMousePressed = false;
    }
    
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS)
            rightMousePressed = true;
        else if (action == GLFW_RELEASE)
            rightMousePressed = false;
    }
}

// 鼠标滚轮回调函数
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // 缩放视角（改变视野角度）
    cameraZoom -= yoffset * scrollSpeed;
    
    // 限制缩放范围
    if (cameraZoom < 1.0f)
        cameraZoom = 1.0f;
    if (cameraZoom > 25.0f)
        cameraZoom = 25.0f;
}

// 更新所有行星速度
void updatePlanetSpeeds() {
    // 更新所有行星的公转和自转速度
    for (size_t i = 0; i < planets.size(); i++) {
        planets[i].orbitSpeed = planets[i].baseOrbitSpeed * orbitSpeed;
        planets[i].rotationSpeed = planets[i].baseRotationSpeed * rotationSpeed;
    }
    
    // 更新月球的速度
    moon.orbitSpeed = moon.baseOrbitSpeed * orbitSpeed;
    moon.rotationSpeed = moon.baseRotationSpeed * rotationSpeed;
}

// 键盘回调函数
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    // 增加/减少旋转速度 (上下方向键)
    if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        rotationSpeed += 0.1f;
        orbitSpeed += 0.05f;
        updatePlanetSpeeds();  // 更新所有行星速度
    }
    if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        rotationSpeed -= 0.1f;
        orbitSpeed -= 0.05f;
        if (rotationSpeed < 0.1f) rotationSpeed = 0.1f;
        if (orbitSpeed < 0.05f) orbitSpeed = 0.05f;
        updatePlanetSpeeds();  // 更新所有行星速度
    }
    
    // 左右方向键控制运动速度
    if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        rotationSpeed *= 1.2f;
        orbitSpeed *= 1.2f;
        updatePlanetSpeeds();  // 更新所有行星速度
    }
    if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        rotationSpeed *= 0.8f;
        orbitSpeed *= 0.8f;
        if (rotationSpeed < 0.05f) rotationSpeed = 0.05f;
        if (orbitSpeed < 0.025f) orbitSpeed = 0.025f;
        updatePlanetSpeeds();  // 更新所有行星速度
    }
    
    // Ctrl键控制显示/隐藏行星名称
    if ((key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) && action == GLFW_PRESS) {
        showPlanetNames = !showPlanetNames;
    }
    
    // F键切换字体
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        currentFont = (currentFont + 1) % 2; // 在两种字体间切换
    }
    
    // R键重置相机视角
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        cameraPos = DEFAULT_CAMERA_POS;
        cameraTarget = DEFAULT_CAMERA_TARGET;
        cameraUp = DEFAULT_CAMERA_UP;
        cameraZoom = DEFAULT_CAMERA_ZOOM;
        firstMouse = true;
    }
}

// 创建着色器程序
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    // 读取着色器代码
    std::string vertexCode = loadShaderSource(vertexPath);
    std::string fragmentCode = loadShaderSource(fragmentPath);
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    
    // 创建并编译顶点着色器
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vShaderCode, NULL);
    glCompileShader(vertexShader);
    
    // 检查顶点着色器是否编译成功
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // 创建并编译片段着色器
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
    glCompileShader(fragmentShader);
    
    // 检查片段着色器是否编译成功
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // 创建着色器程序并链接着色器
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // 检查链接是否成功
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    // 删除着色器对象
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

// 将3D世界坐标转换为2D屏幕坐标
glm::vec2 world3DToScreen2D(const glm::vec3& worldPos, const glm::mat4& view, const glm::mat4& projection, const glm::vec4& viewport) {
    glm::vec4 clipSpacePos = projection * view * glm::vec4(worldPos, 1.0f);
    
    // 透视除法
    clipSpacePos.x /= clipSpacePos.w;
    clipSpacePos.y /= clipSpacePos.w;
    clipSpacePos.z /= clipSpacePos.w;
    
    // NDC坐标转换为屏幕坐标
    glm::vec2 screenPos;
    screenPos.x = (clipSpacePos.x + 1.0f) * 0.5f * viewport.z + viewport.x;
    screenPos.y = (clipSpacePos.y + 1.0f) * 0.5f * viewport.w + viewport.y;
    
    return screenPos;
}

// 计算行星名称的位置，使其与行星旋转方向一致
glm::vec3 calculateNamePosition(const Planet& planet, const glm::vec3& planetPos) {
    // 在行星正上方显示文字
    return glm::vec3(planetPos.x, planetPos.y, planetPos.z - planet.radius * 1.0f);
}

// 添加轨迹点
void addTrailPoint(Planet& planet, const glm::vec3& position) {
    // 如果轨迹点数量超过最大值，移除最旧的点
    if (planet.trailPoints.size() >= MAX_TRAIL_POINTS) {
        planet.trailPoints.erase(planet.trailPoints.begin());
    }
    
    // 添加新的轨迹点
    planet.trailPoints.push_back(position);
}

// 绘制轨迹
void drawTrail(const Planet& planet, const glm::mat4& view, const glm::mat4& projection) {
    if (planet.trailPoints.size() < 2) {
        return;
    }
    
    // 使用轨迹着色器
    glUseProgram(trailShaderProgram);
    
    // 设置着色器全局变量
    glUniformMatrix4fv(glGetUniformLocation(trailShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(trailShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // 创建临时VAO和VBO用于轨迹线渲染
    GLuint trailVAO, trailVBO;
    glGenVertexArrays(1, &trailVAO);
    glGenBuffers(1, &trailVBO);
    
    glBindVertexArray(trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
    
    // 计算顶点数组大小
    std::vector<float> trailVertices;
    std::vector<float> trailColors;
    
    // 收集顶点和颜色数据
    for (size_t i = 0; i < planet.trailPoints.size(); ++i) {
        // 添加位置
        trailVertices.push_back(planet.trailPoints[i].x);
        trailVertices.push_back(planet.trailPoints[i].y);
        trailVertices.push_back(planet.trailPoints[i].z);
        
        // 计算透明度，越新的点越不透明
        float alpha = static_cast<float>(i) / planet.trailPoints.size();
        
        // 如果是相对于当前点180度之外的轨迹点，透明度设为0
        if (i < planet.trailPoints.size() / 2) {
            alpha = 0.0f;
        }
        
        // 添加颜色（白色 + alpha）
        trailColors.push_back(1.0f);  // R
        trailColors.push_back(1.0f);  // G
        trailColors.push_back(1.0f);  // B
        trailColors.push_back(alpha); // A
    }
    
    // 分配缓冲区内存
    const size_t vertexDataSize = trailVertices.size() * sizeof(float);
    const size_t colorDataSize = trailColors.size() * sizeof(float);
    
    glBufferData(GL_ARRAY_BUFFER, vertexDataSize + colorDataSize, nullptr, GL_STATIC_DRAW);
    
    // 填充顶点和颜色数据
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexDataSize, trailVertices.data());
    glBufferSubData(GL_ARRAY_BUFFER, vertexDataSize, colorDataSize, trailColors.data());
    
    // 设置顶点属性指针
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)vertexDataSize);
    glEnableVertexAttribArray(1);
    
    // 启用线宽
    glLineWidth(1.5f);
    
    // 绘制线条
    glDrawArrays(GL_LINE_STRIP, 0, planet.trailPoints.size());
    
    // 清理资源
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteVertexArrays(1, &trailVAO);
    glDeleteBuffers(1, &trailVBO);
}

int main() {
    // 初始化GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // 设置OpenGL版本
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System Simulation", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    // 设置回调函数
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    
    // 初始化GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    
    // 打印OpenGL版本信息
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    // 启用深度测试
    glEnable(GL_DEPTH_TEST);
    
    // 启用混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 启用背面剔除
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // 创建文本渲染器
    TextRenderer textRenderer(SCR_WIDTH, SCR_HEIGHT);
    bool fontLoaded = false;
    
    // 尝试加载字体
    for (int i = 0; i < 2; ++i) {
        if (textRenderer.Load(FONT_PATHS[i], 24)) {
            fontLoaded = true;
            currentFont = i;
            break;
        } else {
            std::cerr << "Failed to load font: " << FONT_PATHS[i] << std::endl;
        }
    }
    
    if (!fontLoaded) {
        std::cerr << "Failed to load any fonts!" << std::endl;
    }
    
    // 创建着色器程序
    GLuint shaderProgram = createShaderProgram("shaders/vertex.glsl", "shaders/fragment.glsl");
    
    // 创建轨迹着色器程序
    trailShaderProgram = createShaderProgram("shaders/trail_vertex.glsl", "shaders/trail_fragment.glsl");
    
    // 创建球体数据
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<unsigned int> indices;
    
    generateSphere(vertices, normals, texCoords, indices, 1.0f, 36, 18);
    
    // 创建顶点数组对象和顶点缓冲对象
    GLuint VAO, VBO, EBO, texCoordsVBO, normalsVBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &texCoordsVBO);
    glGenBuffers(1, &normalsVBO);
    
    glBindVertexArray(VAO);
    
    // 顶点位置
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 法线
    glBindBuffer(GL_ARRAY_BUFFER, normalsVBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    
    // 纹理坐标
    glBindBuffer(GL_ARRAY_BUFFER, texCoordsVBO);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float), texCoords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    
    // 索引
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // 加载行星纹理
    
    // 太阳 (增大半径)
    Planet sun;
    sun.name = "Sun";
    sun.radius = 3.0f;   // 增大太阳半径
    sun.distance = 0.0f;
    sun.baseOrbitSpeed = 0.0f;
    sun.baseRotationSpeed = 0.1f;
    sun.orbitSpeed = sun.baseOrbitSpeed * orbitSpeed;
    sun.rotationSpeed = sun.baseRotationSpeed * rotationSpeed;
    sun.tilt = 0.0f;
    sun.currentOrbitAngle = 0.0f;
    sun.currentRotationAngle = 0.0f;
    sun.textureID = loadTexture("texture/sun.jpg");
    planets.push_back(sun);
    
    // 水星 (增大半径)
    Planet mercury;
    mercury.name = "Mercury";
    mercury.radius = 0.6f;  // 增大水星半径
    mercury.distance = 4.5f;
    mercury.baseOrbitSpeed = 4.7f;
    mercury.baseRotationSpeed = 0.017f;
    mercury.orbitSpeed = mercury.baseOrbitSpeed * orbitSpeed;
    mercury.rotationSpeed = mercury.baseRotationSpeed * rotationSpeed;
    mercury.tilt = 0.03f;
    mercury.currentOrbitAngle = 0.0f;
    mercury.currentRotationAngle = 0.0f;
    mercury.textureID = loadTexture("texture/mercury.jpg");
    planets.push_back(mercury);
    
    // 金星 (增大半径)
    Planet venus;
    venus.name = "Venus";
    venus.radius = 1.2f;   // 增大金星半径
    venus.distance = 6.0f;
    venus.baseOrbitSpeed = 3.5f;
    venus.baseRotationSpeed = 0.004f;
    venus.orbitSpeed = venus.baseOrbitSpeed * orbitSpeed;
    venus.rotationSpeed = venus.baseRotationSpeed * rotationSpeed;
    venus.tilt = 177.3f;
    venus.currentOrbitAngle = 0.0f;
    venus.currentRotationAngle = 0.0f;
    venus.textureID = loadTexture("texture/venus.jpg");
    planets.push_back(venus);
    
    // 地球 (增大半径)
    Planet earth;
    earth.name = "Earth";
    earth.radius = 1.3f;   // 增大地球半径
    earth.distance = 9.0f;
    earth.baseOrbitSpeed = 3.0f;
    earth.baseRotationSpeed = 1.0f;
    earth.orbitSpeed = earth.baseOrbitSpeed * orbitSpeed;
    earth.rotationSpeed = earth.baseRotationSpeed * rotationSpeed;
    earth.tilt = 23.4f;
    earth.currentOrbitAngle = 0.0f;
    earth.currentRotationAngle = 0.0f;
    earth.textureID = loadTexture("texture/earth.jpg");
    planets.push_back(earth);
    
    // 月球 (增大半径)
    moon.name = "Moon";
    moon.radius = 0.4f;    // 增大月球半径
    moon.distance = 2.5f;  // 相对于地球的距离
    moon.baseOrbitSpeed = 13.0f;
    moon.baseRotationSpeed = 0.1f;
    moon.orbitSpeed = moon.baseOrbitSpeed * orbitSpeed;
    moon.rotationSpeed = moon.baseRotationSpeed * rotationSpeed;
    moon.tilt = 6.7f;
    moon.currentOrbitAngle = 0.0f;
    moon.currentRotationAngle = 0.0f;
    moon.textureID = loadTexture("texture/moon.jpg");
    
    // 火星 (增大半径)
    Planet mars;
    mars.name = "Mars";
    mars.radius = 0.7f;    // 增大火星半径
    mars.distance = 12.0f;
    mars.baseOrbitSpeed = 2.4f;
    mars.baseRotationSpeed = 0.97f;
    mars.orbitSpeed = mars.baseOrbitSpeed * orbitSpeed;
    mars.rotationSpeed = mars.baseRotationSpeed * rotationSpeed;
    mars.tilt = 25.2f;
    mars.currentOrbitAngle = 0.0f;
    mars.currentRotationAngle = 0.0f;
    mars.textureID = loadTexture("texture/mars.jpg");
    planets.push_back(mars);
    
    // 木星 (增大半径)
    Planet jupiter;
    jupiter.name = "Jupiter";
    jupiter.radius = 2.5f;   // 增大木星半径
    jupiter.distance = 15.0f;
    jupiter.baseOrbitSpeed = 1.3f;
    jupiter.baseRotationSpeed = 2.4f;
    jupiter.orbitSpeed = jupiter.baseOrbitSpeed * orbitSpeed;
    jupiter.rotationSpeed = jupiter.baseRotationSpeed * rotationSpeed;
    jupiter.tilt = 3.1f;
    jupiter.currentOrbitAngle = 0.0f;
    jupiter.currentRotationAngle = 0.0f;
    jupiter.textureID = loadTexture("texture/jupiter.jpg");
    planets.push_back(jupiter);
    
    // 土星 (增大半径)
    Planet saturn;
    saturn.name = "Saturn";
    saturn.radius = 2.3f;    // 增大土星半径
    saturn.distance = 25.0f;
    saturn.baseOrbitSpeed = 0.97f;
    saturn.baseRotationSpeed = 2.2f;
    saturn.orbitSpeed = saturn.baseOrbitSpeed * orbitSpeed;
    saturn.rotationSpeed = saturn.baseRotationSpeed * rotationSpeed;
    saturn.tilt = 26.7f;
    saturn.currentOrbitAngle = 0.0f;
    saturn.currentRotationAngle = 0.0f;
    saturn.textureID = loadTexture("texture/saturn.jpg");
    planets.push_back(saturn);
    
    // 天王星 (增大半径)
    Planet uranus;
    uranus.name = "Uranus";
    uranus.radius = 1.8f;    // 增大天王星半径
    uranus.distance = 35.0f;
    uranus.baseOrbitSpeed = 0.68f;
    uranus.baseRotationSpeed = 1.4f;
    uranus.orbitSpeed = uranus.baseOrbitSpeed * orbitSpeed;
    uranus.rotationSpeed = uranus.baseRotationSpeed * rotationSpeed;
    uranus.tilt = 97.8f;
    uranus.currentOrbitAngle = 0.0f;
    uranus.currentRotationAngle = 0.0f;
    uranus.textureID = loadTexture("texture/uranus.jpg");
    planets.push_back(uranus);
    
    // 海王星 (增大半径)
    Planet neptune;
    neptune.name = "Neptune";
    neptune.radius = 1.8f;    // 增大海王星半径
    neptune.distance = 45.0f;
    neptune.baseOrbitSpeed = 0.54f;
    neptune.baseRotationSpeed = 1.5f;
    neptune.orbitSpeed = neptune.baseOrbitSpeed * orbitSpeed;
    neptune.rotationSpeed = neptune.baseRotationSpeed * rotationSpeed;
    neptune.tilt = 28.3f;
    neptune.currentOrbitAngle = 0.0f;
    neptune.currentRotationAngle = 0.0f;
    neptune.textureID = loadTexture("texture/neptune.jpg");
    planets.push_back(neptune);
    
    // 定义视口参数用于坐标转换
    glm::vec4 viewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    
    // 存储行星位置
    std::vector<glm::vec3> planetPositions(planets.size());
    glm::vec3 moonPosition;

    // 设置光照参数
    glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 0.0f);  // 光源在太阳的位置
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float ambientStrength = 0.3f;

    // 渲染循环
    while (!glfwWindowShouldClose(window)) {
        // 清空颜色和深度缓冲
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 如果需要切换字体
        static int lastFont = currentFont;
        if (lastFont != currentFont) {
            textRenderer.Load(FONT_PATHS[currentFont], 24);
            lastFont = currentFont;
        }
        
        // 更新相机视图矩阵
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(cameraZoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
                
        // 设置着色器全局变量
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), ambientStrength);
        glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
        
        glBindVertexArray(VAO);
        
        // 更新和渲染每个行星
        for (size_t i = 0; i < planets.size(); i++) {
            planets[i].currentOrbitAngle += planets[i].orbitSpeed * 0.01f;
            planets[i].currentRotationAngle += planets[i].rotationSpeed * 0.01f;
            
            glm::mat4 model = glm::mat4(1.0f);
            
            // 进行公转
            model = glm::rotate(model, planets[i].currentOrbitAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::translate(model, glm::vec3(planets[i].distance, 0.0f, 0.0f));
            
            // 保存未旋转的行星位置（用于显示名称和绘制轨迹）
            planetPositions[i] = glm::vec3(model[3]);
            
            // 添加轨迹点
            if (i > 0) { // 太阳不需要轨迹
                addTrailPoint(planets[i], planetPositions[i]);
            }
            
            // 进行自转
            model = glm::rotate(model, glm::radians(planets[i].tilt), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::rotate(model, planets[i].currentRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            
            // 设置行星大小
            model = glm::scale(model, glm::vec3(planets[i].radius));
            
            // 传递模型矩阵到着色器
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            
            // 绑定纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, planets[i].textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
            
            // 绘制行星
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            
            // 特殊处理：地球的月球
            if (i == 3) {  // 地球是第四个行星（索引为3）
                glm::mat4 moonModel = glm::mat4(1.0f);
                
                // 地球的位置
                moonModel = glm::rotate(moonModel, planets[i].currentOrbitAngle, glm::vec3(0.0f, 1.0f, 0.0f));
                moonModel = glm::translate(moonModel, glm::vec3(planets[i].distance, 0.0f, 0.0f));
                
                // 月球围绕地球旋转
                moonModel = glm::rotate(moonModel, moon.currentOrbitAngle, glm::vec3(0.0f, 1.0f, 0.0f));
                moonModel = glm::translate(moonModel, glm::vec3(moon.distance, 0.0f, 0.0f));
                
                // 保存月球位置（用于显示名称和绘制轨迹）
                moonPosition = glm::vec3(moonModel[3]);
                
                // 添加月球轨迹点
                addTrailPoint(moon, moonPosition);
                
                // 月球自转
                moonModel = glm::rotate(moonModel, glm::radians(moon.tilt), glm::vec3(0.0f, 0.0f, 1.0f));
                moonModel = glm::rotate(moonModel, moon.currentRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
                
                // 设置月球大小
                moonModel = glm::scale(moonModel, glm::vec3(moon.radius));
                
                // 传递模型矩阵到着色器
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(moonModel));
                
                // 绑定月球纹理
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, moon.textureID);
                glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
                
                // 绘制月球
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
                
                // 更新月球角度
                moon.currentOrbitAngle += moon.orbitSpeed * 0.01f;
                moon.currentRotationAngle += moon.rotationSpeed * 0.01f;
            }
        }
        
        // 绘制行星轨迹
        for (size_t i = 1; i < planets.size(); i++) {  // 从1开始，太阳没有轨迹
            drawTrail(planets[i], view, projection);
        }
        
        // 绘制月球轨迹
        drawTrail(moon, view, projection);
        
        // 如果需要显示行星名称
        if (showPlanetNames) {
            // 渲染行星名称
            for (size_t i = 0; i < planets.size(); i++) {
                // 计算符合行星旋转方向的文字位置
                glm::vec3 namePos = calculateNamePosition(planets[i], planetPositions[i]);
                
                // 将3D世界坐标转换为2D屏幕坐标
                glm::vec2 screenPos = world3DToScreen2D(namePos, view, projection, viewport);
                
                // 获取文本宽度以便居中显示
                float textWidth = planets[i].name.length() * 12.0f * 0.5f; // 估计文本宽度
                
                // 渲染行星名称
                textRenderer.RenderText(planets[i].name, screenPos.x - textWidth, screenPos.y, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
            }
            
            // 渲染月球名称
            // 计算符合月球旋转方向的文字位置
            glm::vec3 moonNamePos = calculateNamePosition(moon, moonPosition);
            glm::vec2 moonScreenPos = world3DToScreen2D(moonNamePos, view, projection, viewport);
            float moonTextWidth = moon.name.length() * 12.0f * 0.5f; // 估计文本宽度
            textRenderer.RenderText(moon.name, moonScreenPos.x - moonTextWidth, moonScreenPos.y, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
        }
        
        // 渲染控制信息，保留2位小数
        std::stringstream speedStream;
        speedStream << std::fixed << std::setprecision(2) << "Rotation Speed: " << rotationSpeed << " (Up/Down/Left/Right Keys)";
        textRenderer.RenderText(speedStream.str(), 10.0f, 30.0f, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f));
        
        std::string fontInfo = "Current Font: " + std::string(currentFont == 0 ? "Helvetica" : "MarkerFelt") + " (Press F to change)";
        textRenderer.RenderText(fontInfo, 10.0f, 60.0f, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f));
        
        std::string nameInfo = "Planet Names: " + std::string(showPlanetNames ? "Shown" : "Hidden") + " (Press Ctrl to toggle)";
        textRenderer.RenderText(nameInfo, 10.0f, 90.0f, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f));
        
        std::string cameraInfo = "Camera Control: Left-click (Rotate), Right-click (Pan), Scroll (Zoom), R (Reset)";
        textRenderer.RenderText(cameraInfo, 10.0f, 120.0f, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f));
        
        // 交换缓冲并检查事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // 清理资源
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &texCoordsVBO);
    glDeleteBuffers(1, &normalsVBO);
    glDeleteProgram(shaderProgram);
    
    // 释放纹理资源
    for (const auto& planet : planets) {
        glDeleteTextures(1, &planet.textureID);
    }
    glDeleteTextures(1, &moon.textureID);
    
    glfwTerminate();
    return 0;
} 