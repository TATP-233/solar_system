# 太阳系模拟器技术报告

## 1. 着色器 (Shader) 实现

本项目使用了两组着色器：

### 1.1 行星着色器

**顶点着色器 (vertex.glsl)**
```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
}
```

**片段着色器 (fragment.glsl)**
```glsl
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
```

### 1.2 轨迹着色器

用于渲染行星轨迹的着色器,显示行星的历史轨迹。

## 2. 行星参数表

| 行星   | 半径  | 与太阳距离 | 公转速度基值 | 自转速度基值 | 轴倾角 |
|--------|-------|------------|------------|------------|--------|
| 太阳   | 3.0   | 0.0        | 0.0        | 0.1        | 0.0    |
| 水星   | 0.6   | 4.5        | 4.7        | 0.017      | 0.03   |
| 金星   | 1.2   | 6.0        | 3.5        | 0.004      | 177.3  |
| 地球   | 1.3   | 9.0        | 3.0        | 1.0        | 23.4   |
| 火星   | 0.7   | 12.0       | 2.4        | 0.97       | 25.2   |
| 木星   | 2.5   | 15.0       | 1.3        | 2.4        | 3.1    |
| 土星   | 2.3   | 25.0       | 0.97       | 2.2        | 26.7   |
| 天王星 | 1.8   | 35.0       | 0.68       | 1.4        | 97.8   |
| 海王星 | 1.8   | 45.0       | 0.54       | 1.5        | 28.3   |
| 月球   | 0.4   | 2.5*       | 13.0       | 0.1        | 6.7    |

*注：月球距离是相对于地球而非太阳

## 3. 自由相机设置

本项目实现了功能完整的自由相机系统，具体参数如下：

```cpp
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
```

相机支持三种基本操作：
1. **旋转**：通过鼠标左键拖动实现视角旋转
2. **平移**：通过鼠标右键拖动实现视角平移
3. **缩放**：通过鼠标滚轮控制视野角度（FOV）

## 4. 交互逻辑

### 4.1 相机控制

```cpp
// 鼠标移动回调函数
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    // 左键按下：旋转视角
    // 通过计算相机到目标的方向向量，以及上向量和右向量，创建旋转矩阵
    
    // 右键按下：平移视角
    // 计算相机平移向量，同时移动相机位置和目标位置
}

// 鼠标滚轮回调函数
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // 缩放视角（改变视野角度）
    cameraZoom -= yoffset * scrollSpeed;
    // 限制缩放范围
    if (cameraZoom < 1.0f) cameraZoom = 1.0f;
    if (cameraZoom > 25.0f) cameraZoom = 25.0f;
}
```

### 4.2 行星运动控制

行星的运动分为自转和公转，可以通过键盘方向键调整速度：

```cpp
// 键盘回调函数
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // 上方向键：增加速度
    // 下方向键：减少速度
    // 右方向键：倍增速度
    // 左方向键：减缓速度
    
    // 更新所有行星速度
    updatePlanetSpeeds();
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
```

### 4.3 UI交互

```cpp
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
```

## 5. 附加功能

### 5.1 行星轨迹系统

系统会为每个行星（除太阳外）记录移动轨迹并渲染出来，每个行星最多保留200个轨迹点。轨迹点越新，透明度越低。

```cpp
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
    // 使用轨迹着色器渲染轨迹线
}
```

### 5.2 月球系统

本模拟器不仅包含太阳系的九大行星，还模拟了月球围绕地球运转的情景：

```cpp
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
```

### 5.3 文本渲染系统

模拟器中实现了文本渲染系统，用于显示行星名称和控制信息。支持两种字体，可通过F键切换。

## 6. 渲染技术

### 6.1 光照模型

使用简化的Phong光照模型，包含环境光和漫反射，光源位于太阳的位置：

```glsl
// 环境光
vec3 ambient = ambientStrength * lightColor;

// 漫反射
vec3 norm = normalize(Normal);
vec3 lightDir = normalize(lightPos - FragPos);
float diff = max(dot(norm, lightDir), 0.0);
vec3 diffuse = diff * lightColor;

// 最终颜色
vec3 result = (ambient + diffuse) * texColor.rgb;
```

### 6.2 纹理映射

为所有行星加载独立的纹理，通过球面纹理坐标映射到球体表面：

```cpp
// 加载纹理函数
GLuint loadTexture(const char* path) {
    // 设置纹理参数
    // 加载并生成纹理
    // 返回纹理ID
}
```

### 6.3 3D坐标转换

实现了3D世界坐标到2D屏幕坐标的转换，用于显示行星名称：

```cpp
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
```

## 总结

太阳系模拟器使用OpenGL实现，包含光照系统、文本渲染、自由相机控制和行星运动模拟。通过精心设计的交互系统，用户可以轻松控制视角和行星运动速度。项目中还实现了行星轨迹渲染、参数化的行星系统，以及基于物理的月球运动模拟。