cmake_minimum_required(VERSION 3.10)
project(solar_system)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Freetype REQUIRED)

# 源文件
set(SOURCES
    src/main.cpp
    src/text_renderer.cpp
    src/stb_image_impl.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# 添加include目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
    OpenGL::GL
    GLEW::GLEW
    glfw
    ${FREETYPE_LIBRARIES}
)

# 将着色器文件和纹理复制到构建目录
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/texture DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/fonts DESTINATION ${CMAKE_BINARY_DIR}) 