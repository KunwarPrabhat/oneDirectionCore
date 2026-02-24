# OneDirectionCore

A high-performance C-core library and .NET-based UI for advanced DSP and radar applications.

[![Download Windows](https://img.shields.io/badge/Download-ODC.exe-blue?style=for-the-badge&logo=windows&logoColor=white)](./ODC.exe)


##  Features
Map based radar with heat sense


### Linux
1. Clone the repository.
2. Install dependencies: `libpipewire-0.3`, `glfw3`, `gtk4`.
3. Use Meson to build:
   ```bash
   meson setup build
   ninja -C build
   ```

## Build Requirements

- **C/C++ Compiler** (GCC 11+ or MSVC)
- **Meson Build System**
- **dependencies**: GLFW3, OpenGL, Pipewire (Linux)
