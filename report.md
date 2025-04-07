# Solar System Simulator Technical Report

## 1. Shader Implementation

This project uses two sets of shaders:

### 1.1 Planet Shader

**Vertex Shader (vertex.glsl)**
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

**Fragment Shader (fragment.glsl)**
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
    // Ambient light
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse light
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Get color from texture
    vec4 texColor = texture(texture1, TexCoord);
    
    // Final color
    vec3 result = (ambient + diffuse) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}
```

### 1.2 Trail Shader

Used for rendering planet orbits.

## 2. Planet Parameters

| Planet    | Radius | Distance from Sun | Base Orbit Speed | Base Rotation Speed | Axial Tilt |
|-----------|--------|-------------------|------------------|---------------------|------------|
| Sun       | 3.0    | 0.0               | 0.0              | 0.1                 | 0.0        |
| Mercury   | 0.6    | 4.5               | 4.7              | 0.017               | 0.03       |
| Venus     | 1.2    | 6.0               | 3.5              | 0.004               | 177.3      |
| Earth     | 1.3    | 9.0               | 3.0              | 1.0                 | 23.4       |
| Mars      | 0.7    | 12.0              | 2.4              | 0.97                | 25.2       |
| Jupiter   | 2.5    | 15.0              | 1.3              | 2.4                 | 3.1        |
| Saturn    | 2.3    | 25.0              | 0.97             | 2.2                 | 26.7       |
| Uranus    | 1.8    | 35.0              | 0.68             | 1.4                 | 97.8       |
| Neptune   | 1.8    | 45.0              | 0.54             | 1.5                 | 28.3       |
| Moon      | 0.4    | 2.5*              | 13.0             | 0.1                 | 6.7        |

*Note: Moon's distance is relative to Earth, not the Sun

## 3. Free Camera Setup

The project implements a fully functional free camera system with the following parameters:

```cpp
// Camera control parameters
glm::vec3 cameraPos = glm::vec3(0.0f, 100.0f, 230.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraZoom = 15.0f;

// Default camera parameters (for reset)
const glm::vec3 DEFAULT_CAMERA_POS = glm::vec3(0.0f, 100.0f, 230.0f);
const glm::vec3 DEFAULT_CAMERA_TARGET = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 DEFAULT_CAMERA_UP = glm::vec3(0.0f, 1.0f, 0.0f);
const float DEFAULT_CAMERA_ZOOM = 15.0f;
```

The camera supports three basic operations:
1. **Rotation**: Through left mouse button drag to rotate the view
2. **Translation**: Through right mouse button drag to pan the view
3. **Zoom**: Through mouse wheel to control the field of view (FOV)

## 4. Interaction Logic

### 4.1 Camera Control

```cpp
// Mouse movement callback function
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    // Left button pressed: rotate view
    // Calculate direction vector from camera to target, along with up and right vectors, create rotation matrix
    
    // Right button pressed: pan view
    // Calculate camera translation vector, move both camera position and target position
}

// Mouse wheel callback function
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // Scale view (change field of view)
    cameraZoom -= yoffset * scrollSpeed;
    // Limit zoom range
    if (cameraZoom < 1.0f) cameraZoom = 1.0f;
    if (cameraZoom > 25.0f) cameraZoom = 25.0f;
}
```

### 4.2 Planet Motion Control

Planet motion is divided into rotation and orbit, which can be adjusted using keyboard arrow keys:

```cpp
// Keyboard callback function
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Up arrow key: increase speed
    // Down arrow key: decrease speed
    // Right arrow key: multiply speed
    // Left arrow key: reduce speed
    
    // Update all planet speeds
    updatePlanetSpeeds();
}

// Update all planet speeds
void updatePlanetSpeeds() {
    // Update orbit and rotation speeds for all planets
    for (size_t i = 0; i < planets.size(); i++) {
        planets[i].orbitSpeed = planets[i].baseOrbitSpeed * orbitSpeed;
        planets[i].rotationSpeed = planets[i].baseRotationSpeed * rotationSpeed;
    }
    
    // Update moon's speed
    moon.orbitSpeed = moon.baseOrbitSpeed * orbitSpeed;
    moon.rotationSpeed = moon.baseRotationSpeed * rotationSpeed;
}
```

### 4.3 UI Interaction

```cpp
// Ctrl key toggles planet name display
if ((key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) && action == GLFW_PRESS) {
    showPlanetNames = !showPlanetNames;
}

// F key switches font
if (key == GLFW_KEY_F && action == GLFW_PRESS) {
    currentFont = (currentFont + 1) % 2; // Toggle between two fonts
}

// R key resets camera view
if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    cameraPos = DEFAULT_CAMERA_POS;
    cameraTarget = DEFAULT_CAMERA_TARGET;
    cameraUp = DEFAULT_CAMERA_UP;
    cameraZoom = DEFAULT_CAMERA_ZOOM;
    firstMouse = true;
}
```

## 5. Additional Features

### 5.1 Planet Trail System

The system records and renders the movement trail for each planet (except the Sun), with a maximum of 200 trail points per planet. Newer trail points have lower opacity.

```cpp
// Add trail point
void addTrailPoint(Planet& planet, const glm::vec3& position) {
    // If trail points exceed maximum, remove oldest point
    if (planet.trailPoints.size() >= MAX_TRAIL_POINTS) {
        planet.trailPoints.erase(planet.trailPoints.begin());
    }
    
    // Add new trail point
    planet.trailPoints.push_back(position);
}

// Draw trail
void drawTrail(const Planet& planet, const glm::mat4& view, const glm::mat4& projection) {
    // Use trail shader to render trail lines
}
```

### 5.2 Moon System

The simulator not only includes the nine planets of the solar system but also simulates the Moon orbiting around Earth:

```cpp
// Special handling: Earth's Moon
if (i == 3) {  // Earth is the fourth planet (index 3)
    glm::mat4 moonModel = glm::mat4(1.0f);
    
    // Earth's position
    moonModel = glm::rotate(moonModel, planets[i].currentOrbitAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    moonModel = glm::translate(moonModel, glm::vec3(planets[i].distance, 0.0f, 0.0f));
    
    // Moon orbits around Earth
    moonModel = glm::rotate(moonModel, moon.currentOrbitAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    moonModel = glm::translate(moonModel, glm::vec3(moon.distance, 0.0f, 0.0f));
    
    // Save Moon position (for displaying name and drawing trail)
    moonPosition = glm::vec3(moonModel[3]);
    
    // Add Moon trail point
    addTrailPoint(moon, moonPosition);
    
    // Moon rotation
    moonModel = glm::rotate(moonModel, glm::radians(moon.tilt), glm::vec3(0.0f, 0.0f, 1.0f));
    moonModel = glm::rotate(moonModel, moon.currentRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Set Moon size
    moonModel = glm::scale(moonModel, glm::vec3(moon.radius));
    
    // Pass model matrix to shader
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(moonModel));
    
    // Bind Moon texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, moon.textureID);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
    
    // Draw Moon
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    
    // Update Moon angles
    moon.currentOrbitAngle += moon.orbitSpeed * 0.01f;
    moon.currentRotationAngle += moon.rotationSpeed * 0.01f;
}
```

### 5.3 Text Rendering System

The simulator implements a text rendering system to display planet names and control information. It supports two fonts that can be toggled with the F key.

## 6. Rendering Techniques

### 6.1 Lighting Model

Uses a simplified Phong lighting model including ambient and diffuse lighting, with the light source positioned at the Sun:

```glsl
// Ambient light
vec3 ambient = ambientStrength * lightColor;

// Diffuse light
vec3 norm = normalize(Normal);
vec3 lightDir = normalize(lightPos - FragPos);
float diff = max(dot(norm, lightDir), 0.0);
vec3 diffuse = diff * lightColor;

// Final color
vec3 result = (ambient + diffuse) * texColor.rgb;
```

### 6.2 Texture Mapping

Loads individual textures for all planets, mapped to sphere surfaces using spherical texture coordinates:

```cpp
// Load texture function
GLuint loadTexture(const char* path) {
    // Set texture parameters
    // Load and generate texture
    // Return texture ID
}
```

### 6.3 3D Coordinate Transformation

Implements 3D world coordinate to 2D screen coordinate conversion for displaying planet names:

```cpp
// Convert 3D world coordinates to 2D screen coordinates
glm::vec2 world3DToScreen2D(const glm::vec3& worldPos, const glm::mat4& view, const glm::mat4& projection, const glm::vec4& viewport) {
    glm::vec4 clipSpacePos = projection * view * glm::vec4(worldPos, 1.0f);
    
    // Perspective division
    clipSpacePos.x /= clipSpacePos.w;
    clipSpacePos.y /= clipSpacePos.w;
    clipSpacePos.z /= clipSpacePos.w;
    
    // NDC coordinates to screen coordinates
    glm::vec2 screenPos;
    screenPos.x = (clipSpacePos.x + 1.0f) * 0.5f * viewport.z + viewport.x;
    screenPos.y = (clipSpacePos.y + 1.0f) * 0.5f * viewport.w + viewport.y;
    
    return screenPos;
}
```

## Summary

The Solar System Simulator uses OpenGL to implement a lighting system, text rendering, free camera control, and planet motion simulation. Through a carefully designed interaction system, users can easily control the view and planet motion speed. The project also implements planet trail rendering, parameterized planet system, and physically-based moon motion simulation.