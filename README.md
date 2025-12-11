# CG-HW2

## 环境与构建
- OS: Ubuntu 22.04 x86_64
- Compiler: GCC 11.4.0, C++17
- Build: CMake 3.22 + Make
- 依赖：GLFW 3.3.8、ImGui 1.90.4、GLM（header-only）、GLAD（来自 GLFW deps）

### 编译运行
```bash
cd /work/CV-HW2
mkdir -p build && cd build
cmake ../code
make -j$(nproc)
./bin/CG_HW2
```

## 功能概览
- CPU 光栅化：DDA / Bresenham 边绘制 + Edge-Walking 扫描线填充，含深度测试。
- 着色模型：Flat（已加光照）、Gouraud、Phong、Blinn-Phong（默认）。
- 模型：单三角形、立方体、正四面体。
- 相机：可控绕轨道视角（yaw/pitch/radius），支持俯视/仰视；可自动旋转模型。
- UI 统计：每帧光栅耗时 (ms) 便于算法/着色对比。

## 控制面板说明
- Edge mode：选择边绘制算法（DDA 或 Bresenham）。
- Shading：Flat / Gouraud / Phong / Blinn-Phong。
- Mesh：Single triangle / Cube / Tetrahedron。
- Draw wireframe：叠加线框验证边与填充。
- Auto rotate & Rotation speed：模型自动绕 Y 轴旋转与速度。
- Orbit yaw / pitch / radius：相机绕原点的水平角、俯仰角、距离，用于俯视/仰视。
- Clear color：背景色。
- Base color：物体基色/反照率（Flat 模式直接用）。
- Light color & Light pos：光源颜色与位置。
- Ambient / Diffuse / Specular / Shininess：环境、漫反射、镜面分量及高光锐利度。
- Render time：当前帧 CPU 光栅化耗时（毫秒）。

## 关键实现
- 扫描线填充：三顶点按 y 排序，长边与短边步进，逐行插值 x/depth/color/normal/worldPos 并写入 Framebuffer。
- 线算法：DDA 采用浮点增量；Bresenham 采用整数误差项。
- 光照：Blinn-Phong/Phong 每像素计算；Gouraud 顶点预光照后插值；Flat 使用统一法线+光照。
- Framebuffer：CPU RGBA + 深度缓冲，上传到 OpenGL 纹理后用 ImGui Image 显示。

## 常见问题
- 无面填充：确保 `Draw wireframe` 关闭即可看到纯填充；已修正扫描线逐行写入问题。
- 光照不明显：提高 `Specular` 或 `Shininess`，调节光源位置以看到高光；或切换到 Blinn-Phong。
- 视角调整：使用 Orbit yaw/pitch/radius 进行俯视/仰视；模型仍可自动旋转。

## TODO（可选扩展）
- OBJ 网格加载与背面裁剪。
- 透视校正插值与纹理采样。
- 性能日志导出用于算法对比。
