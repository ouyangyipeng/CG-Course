# CG-HW2 Report

## Environment
- OS: Ubuntu 22.04 (x86_64)
- Compiler: GCC 11.4.0
- Build: CMake 3.22, Ninja/Make (tested with Make), C++17
- Libraries: GLFW 3.3.8 (3rdparty), ImGui 1.90.4, GLM (header-only), GLAD loader from GLFW deps

## Build & Run
1. `cd /work/CV-HW2 && mkdir -p build && cd build`
2. `cmake ../code`
3. `make -j$(nproc)`
4. Execute: `./bin/CG_HW2`

## Core Implementations
### 1. Rasterization (Triangle task)
- **Edge drawing**: Implemented both DDA and Bresenham line algorithms; selectable via ImGui. Edges draw in wireframe overlay for visual verification.
- **Edge-walking fill**: Triangles are sorted by Y, long edge reused; per-scanline interpolation steps for x/depth/color/normal/world position. Fills interior with depth testing and optional wire overlay.
- **Coloring**: Flat color mode uses user-controlled base color to satisfy fixed-color fill requirement.

### 2. Shading (Lighting task)
- **Gouraud shading**: Per-vertex Blinn–Phong lighting, colors interpolated across scanlines.
- **Phong/Blinn-Phong (per-pixel)**: Interpolate normals/world positions per pixel and compute lighting (ambient + diffuse + specular). Specular model toggled by shading mode.
- **Lighting params**: Light position/color, ambient/diffuse/specular weights, and shininess are editable in UI. Camera at (0,0,3.5) with look-at origin.

### 3. Meshes & Interaction
- Mesh options: single triangle (task 1), cube, tetrahedron (task 2). Model auto-rotates; speed adjustable or rotation can be disabled.
- CPU framebuffer: Software rasterization renders into CPU buffer, uploaded each frame to an OpenGL texture and displayed via ImGui `Image`.
- Performance: Per-frame render time measured with `glfwGetTime()` and shown in UI (ms) to compare algorithms/shading cost.

## Issues & Fixes
- `FindOpenGL` missing GL dev headers: avoided by relying on GLFW + GLAD; removed direct `find_package(OpenGL)`.
- GLFW Xinerama headers missing: installed `libxinerama-dev` (plus `libxcursor-dev`, `libxi-dev`).
- `GLFW_INCLUDE_NONE` needed for ImGui backend to avoid `<GL/gl.h>` include; added compile definition.
- GLM include path: added 3rdparty root to include directories so `<glm/glm.hpp>` resolves.

## Results
- **Triangle rasterization**: DDA + edge-walking fills a single triangle correctly; Bresenham available for comparison.
- **Shading demos**: Cube and tetrahedron show Gouraud vs Phong/Blinn-Phong differences under the same light direction; adjust light to observe ambient/diffuse/specular components.
- **Performance observation**: Render time in UI allows empirical comparison of DDA vs Bresenham (edges) and Gouraud vs Phong/Blinn-Phong (shading) on different face counts.

## Screenshots (placeholders)
- Triangle rasterization (DDA + edge-walking): `TODO: triangle.png`
- Cube Gouraud shading: `TODO: cube_gouraud.png`
- Cube Phong shading: `TODO: cube_phong.png`
- Tetrahedron Blinn-Phong shading: `TODO: tetra_blinn.png`

## How to Extend
- Add OBJ loader to test arbitrary meshes.
- Add back-face culling and perspective-correct interpolation for more accurate results.
- Export frame timings to CSV for more rigorous performance plots.
