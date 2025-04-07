
# Solar System Simulator

This is a solar system simulator implemented using OpenGL, featuring the nine planets of the solar system and the moon.

## Controls

### Camera Controls
- **Left Mouse Button Drag**: Rotate view
- **Right Mouse Button Drag**: Pan view
- **Mouse Wheel**: Zoom in/out
- **R Key**: Reset camera to default position

### Planet Motion Controls
- **Up Arrow Key**: Increase planet rotation and orbit speed
- **Down Arrow Key**: Decrease planet rotation and orbit speed
- **Right Arrow Key**: Multiply planet motion speed (×1.2)
- **Left Arrow Key**: Reduce planet motion speed (×0.8)

### Interface Controls
- **Ctrl Key**: Show/hide planet names
- **F Key**: Switch font display
- **Esc Key**: Exit program

## Compilation and Execution

Make sure you have the following dependencies installed:
- OpenGL
- GLEW
- GLFW
- GLM
- FreeType

Compile the project using CMake:

```bash
mkdir build
cd build
cmake ..
make
```

Run the program:

```bash
./solar_system
```
