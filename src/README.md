# CplusIntercept

`CplusIntercept` is a C++17 OpenGL project built with CMake. The current program opens a GLFW window and renders a generated colored sphere using modern OpenGL, GLAD, and GLM. The sphere will have lines that will interact with the sphere as if they are objects with a velocity > 25000 mph and the sphere has the mass of the earth. This is a project to help myself learn more about c++ and rendering.

## Dependencies

CMake downloads the project libraries automatically on first configure:

- GLFW for window creation and input
- GLAD for loading OpenGL functions
- GLM for vector and matrix math

You still need a C++ compiler, CMake, Git, and your platform's OpenGL
development support.

## macOS

Install the command-line tools and CMake:

```bash
xcode-select --install
brew install cmake git
```

Build and run from the project root:

```bash
cmake -S . -B build-opengl
cmake --build build-opengl
./build-opengl/CplusIntercept
```

## Windows

Install:

- Visual Studio 2022 with the "Desktop development with C++" workload
- CMake
- Git

Then open PowerShell in the project root and run:

```powershell
cmake -S . -B build-opengl
cmake --build build-opengl --config Debug
.\build-opengl\Debug\CplusIntercept.exe
```

For a release build:

```powershell
cmake --build build-opengl --config Release
.\build-opengl\Release\CplusIntercept.exe
```

## Linux

On Ubuntu or Debian, install the compiler and OpenGL/X11 development packages:

```bash
sudo apt update
sudo apt install build-essential cmake git libgl1-mesa-dev xorg-dev
```

Build and run from the project root:

```bash
cmake -S . -B build-opengl
cmake --build build-opengl
./build-opengl/CplusIntercept
```

## Controls

- `W` rotates the sphere upward
- `S` rotates the sphere downward
- `A` rotates the sphere left
- `D` rotates the sphere right
- `Esc` closes the window

## Notes

The first `cmake -S . -B build-opengl` command requires internet access because
CMake fetches GLFW, GLAD, and GLM from GitHub.

If an older `build/` directory exists, prefer `build-opengl/` for this OpenGL
version. That avoids stale CMake cache issues from earlier experiments.
