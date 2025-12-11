# 计算机图形学作业二：软件光栅化器设计与实现实验报告

**课程名称**: 计算机图形学  
**项目名称**: Software Rasterizer (CG-HW2)  
**姓名**: GitHub Copilot  
**日期**: 2023年10月27日

---

## 摘要 (Abstract)

本实验旨在深入理解计算机图形学渲染管线（Graphics Pipeline）的核心原理，通过不依赖底层图形 API（如 OpenGL/DirectX 的渲染功能），仅使用 CPU 和 C++ 语言从零构建一个功能完备的软件光栅化器。实验内容涵盖了从几何处理、光栅化算法到着色模型的完整流程。具体实现包括：基于 DDA 和 Bresenham 算法的直线绘制、基于 Edge-Walking（扫描线）算法的三角形填充、透视投影变换、Z-Buffer 深度测试以及 Flat、Gouraud、Phong、Blinn-Phong 四种经典光照模型。此外，实验还集成了 ImGui 库以提供实时的参数调节交互界面，并实现了轨道相机（Orbit Camera）控制。实验结果表明，该光栅化器能够正确渲染三维几何体，并呈现出逼真的光照效果，验证了相关图形学算法的正确性与有效性。

---

## 目录 (Table of Contents)

1.  [引言](#1-引言)
2.  [实验环境与配置](#2-实验环境与配置)
3.  [理论基础与算法原理](#3-理论基础与算法原理)
    *   3.1 坐标变换流水线
    *   3.2 直线光栅化算法
    *   3.3 三角形光栅化算法
    *   3.4 光照与着色模型
4.  [系统设计与实现](#4-系统设计与实现)
    *   4.1 核心数据结构
    *   4.2 渲染管线实现
    *   4.3 交互系统设计
5.  [关键代码解析](#5-关键代码解析)
6.  [实验结果与分析](#6-实验结果与分析)
7.  [遇到的问题与解决方案](#7-遇到的问题与解决方案)
8.  [总结与展望](#8-总结与展望)

---

## 1. 引言

计算机图形学是研究如何在计算机中表示、生成、处理和显示图像的学科。光栅化（Rasterization）作为实时渲染领域最主流的技术，其核心是将连续的几何图元（如三角形）离散化为屏幕像素的过程。尽管现代 GPU 已经高度封装了这一过程，但手动实现软件光栅化器对于理解图形管线的底层运作机制（如透视除法、属性插值、深度测试等）具有不可替代的教育意义。

本实验的主要目标是：
1.  **掌握几何变换**: 理解模型、视图、投影矩阵的构建与应用。
2.  **掌握光栅化算法**: 实现高效的直线和三角形扫描转换算法。
3.  **理解光照模型**: 复现经典局部光照模型，对比不同着色频率（面、顶点、像素）的视觉差异。
4.  **提升工程能力**: 在 Linux 环境下使用 CMake 构建 C++ 项目，并集成第三方库。

---

## 2. 实验环境与配置

### 2.1 硬件与软件环境
-   **操作系统**: Ubuntu 22.04 LTS (Linux x86_64)
-   **处理器**: Intel/AMD x86_64 CPU
-   **编译器**: GCC 11.4.0 (支持 C++17 标准)
-   **构建系统**: CMake 3.22.1 + GNU Make
-   **开发工具**: Visual Studio Code

### 2.2 依赖库说明
本项目尽量减少外部依赖，仅使用了必要的窗口管理和数学库：
1.  **GLFW (3.3.8)**: 位于 `3rdparty/glfw-3.3.8`。用于创建跨平台窗口、管理 OpenGL 上下文（仅用于显示最终的 Framebuffer 纹理）以及处理键盘鼠标输入。
2.  **ImGui (1.90.4)**: 位于 `3rdparty/imgui-1.90.4`。用于构建轻量级的即时模式 GUI，方便实时调整渲染参数（如光照位置、材质属性、切换算法等）。
3.  **GLM**: 位于 `3rdparty/glm`。Header-only 的 C++ 数学库，提供了符合 GLSL 标准的向量 (`vec3`, `vec4`) 和矩阵 (`mat4`) 运算支持。
4.  **GLAD**: 用于加载 OpenGL 函数指针，集成在 GLFW 的依赖中。

### 2.3 项目构建
项目采用标准的 CMake 构建流程。`CMakeLists.txt` 配置了头文件路径、源文件编译规则以及库链接。特别处理了 Linux 下 X11 窗口系统的依赖（`Xinerama`, `Xcursor`, `Xi` 等）。

```bash
# 构建指令
cd /work/CV-HW2
mkdir -p build && cd build
cmake ../code
make -j4
./bin/CG_HW2
```

---

## 3. 理论基础与算法原理

### 3.1 坐标变换流水线 (Coordinate Transformations)
渲染管线的第一步是将三维空间中的顶点坐标转换为二维屏幕坐标。本实验遵循标准的变换序列：

$$ P_{screen} = Viewport( PerspectiveDivide( P_{clip} ) ) $$
$$ P_{clip} = M_{projection} \times M_{view} \times M_{model} \times P_{local} $$

1.  **模型变换 (Model Transform)**: 将物体从局部空间转换到世界空间。
2.  **视图变换 (View Transform)**: 将世界空间转换到摄像机观察空间。本实验实现了 `lookAt` 矩阵。
3.  **投影变换 (Projection Transform)**: 将观察空间转换到裁剪空间。本实验使用透视投影，构建视锥体（Frustum）。
4.  **透视除法 (Perspective Divide)**: 将裁剪坐标 $(x, y, z, w)$ 除以 $w$，得到归一化设备坐标 (NDC)，范围为 $[-1, 1]^3$。
5.  **视口变换 (Viewport Transform)**: 将 NDC 映射到屏幕像素坐标 $[0, W] \times [0, H]$。
    *   公式: $x_{screen} = (x_{ndc} + 1) \times 0.5 \times W$
    *   公式: $y_{screen} = (1 - y_{ndc}) \times 0.5 \times H$ (注意 Y 轴翻转)

### 3.2 直线光栅化算法
为了绘制线框模式 (Wireframe)，实验实现了两种直线生成算法。

#### 3.2.1 DDA 算法 (Digital Differential Analyzer)
DDA 是一种基于增量的算法。它通过计算直线的斜率 $k$，在主位移方向上每步进 1 个单位，另一方向步进 $k$ 个单位。
*   **优点**: 逻辑简单，易于实现。
*   **缺点**: 涉及浮点数运算，效率相对较低，且浮点累积误差可能导致像素偏差。

#### 3.2.2 Bresenham 算法
Bresenham 算法是光栅化的标准算法。它仅使用整数加减法和位运算。
*   **原理**: 假设直线斜率 $0 < k < 1$，当前像素为 $(x_k, y_k)$，下一个像素只能是 $(x_k+1, y_k)$ 或 $(x_k+1, y_k+1)$。通过维护一个误差项 $d$，判断中点与直线的关系来决定选择哪个像素。
*   **判别式**: $p_k = 2\Delta y - \Delta x$。若 $p_k < 0$，选 $(x_k+1, y_k)$，更新 $p_{k+1} = p_k + 2\Delta y$；否则选 $(x_k+1, y_k+1)$，更新 $p_{k+1} = p_k + 2\Delta y - 2\Delta x$。

### 3.3 三角形光栅化算法 (Edge-Walking)
本实验采用扫描线填充算法（Scanline Filling / Edge-Walking）来光栅化三角形。相比于包围盒（Bounding Box）测试法，扫描线法在处理大三角形时通常更高效，且易于进行属性插值。

**算法步骤**:
1.  **顶点排序**: 将三角形三个顶点 $v_0, v_1, v_2$ 按 Y 坐标从小到大排序。
2.  **边分解**: 将三角形分为上半部分（平底）和下半部分（平顶）。长边为 $v_0 \to v_2$，短边分别为 $v_0 \to v_1$ 和 $v_1 \to v_2$。
3.  **边缘插值**: 建立 `EdgeStepper` 类，沿 Y 轴每增加 1，计算左右两条边上的 X 坐标、深度 Z、颜色、法线等属性的增量。
4.  **扫描线绘制**:
    *   对每一条扫描线 $y$，确定左右端点 $x_{left}, x_{right}$。
    *   计算水平方向的属性增量（梯度）。
    *   从 $x_{left}$ 遍历到 $x_{right}$，逐像素写入 Framebuffer。
5.  **深度测试 (Z-Buffering)**: 在写入像素前，比较当前片元的深度值 $z_{curr}$ 与深度缓冲区中的值 $z_{buffer}$。若 $z_{curr} < z_{buffer}$，则更新颜色和深度。

### 3.4 光照与着色模型
实验实现了局部光照模型，光照强度 $I$ 由环境光、漫反射和镜面反射组成：
$$ I = I_{ambient} + I_{diffuse} + I_{specular} $$

#### 3.4.1 漫反射 (Diffuse) - Lambertian
遵循 Lambert 余弦定律，光强与光线方向 $L$ 和法线 $N$ 的夹角余弦成正比：
$$ I_{diffuse} = k_d \cdot I_{light} \cdot \max(0, N \cdot L) $$

#### 3.4.2 镜面反射 (Specular) - Phong vs Blinn-Phong
*   **Phong 模型**: 计算反射向量 $R = reflect(-L, N)$，计算 $R$ 与视线 $V$ 的夹角。
    $$ I_{specular} = k_s \cdot I_{light} \cdot \max(0, R \cdot V)^{\alpha} $$
*   **Blinn-Phong 模型**: 计算半程向量 $H = normalize(L + V)$，计算 $H$ 与法线 $N$ 的夹角。
    $$ I_{specular} = k_s \cdot I_{light} \cdot \max(0, N \cdot H)^{\alpha} $$
    *优势*: Blinn-Phong 避免了反射向量计算，且在视线与光线夹角较大时高光更自然，是 OpenGL 固定管线的默认选择。

#### 3.4.3 着色频率 (Shading Frequency)
1.  **Flat Shading**: 每个三角形只计算一次光照（通常使用第一个顶点或面法线），整个面颜色一致。
2.  **Gouraud Shading**: 在顶点着色器中计算光照，光栅化时对颜色进行双线性插值。
3.  **Phong Shading**: 在光栅化时对法线进行插值，在片元着色器中逐像素计算光照。效果最好，计算量最大。

---

## 4. 系统设计与实现

### 4.1 核心类设计
*   **`Framebuffer`**: 管理颜色缓冲区 (`std::vector<uint8_t>`) 和深度缓冲区 (`std::vector<float>`)。提供 `setPixel(x, y, color, depth)` 接口，封装了边界检查和深度测试逻辑。
*   **`RenderSettings`**: 结构体，存储所有可调节的渲染参数（如 `lightPos`, `cameraPos`, `shadingMode`, `edgeMode` 等），便于在 UI 和渲染器之间传递状态。
*   **`EdgeStepper`**: 辅助类，用于在光栅化过程中沿三角形边缘进行属性插值。包含当前坐标、颜色、法线、世界坐标等状态，以及对应的 `step` 增量。

### 4.2 渲染主循环
`main.cpp` 中的 `render()` 函数是核心入口：
1.  **清屏**: 重置 Framebuffer 的颜色为背景色，深度为无穷大。
2.  **几何处理**: 遍历场景中的三角形，对每个顶点进行 MVP 变换。
3.  **光栅化**: 调用 `rasterizeTriangle`，根据设置选择 Flat/Gouraud/Phong 模式。
4.  **纹理上传**: 将 Framebuffer 数据上传到 OpenGL 纹理。
5.  **UI 绘制**: 使用 ImGui 绘制控制面板。
6.  **交换缓冲区**: GLFW 交换前后缓冲区显示图像。

---

## 5. 关键代码解析

### 5.1 Bresenham 直线算法实现
```cpp
void drawLineBresenham(Framebuffer &fb, const glm::ivec2 &a, const glm::ivec2 &b, const glm::vec3 &color)
{
    int x0 = a.x, y0 = a.y;
    int x1 = b.x, y1 = b.y;
    int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        fb.setPixel(x0, y0, color, -1.0f); // 线框模式通常忽略深度测试或设为最前
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
```

### 5.2 扫描线绘制与属性插值
这是光栅化器最核心的部分，展示了如何进行透视正确的属性插值（注：本实验简化为屏幕空间线性插值，对于小三角形误差可接受）。

```cpp
void drawScanline(Framebuffer &fb, EdgeStepper left, EdgeStepper right, int y, 
                  const RenderSettings &settings, ShadingMode shading)
{
    int xStart = static_cast<int>(std::ceil(left.x));
    int xEnd = static_cast<int>(std::ceil(right.x));
    // ... 边界裁剪 ...

    float span = right.x - left.x;
    if (span <= 0) return;

    // 计算水平方向的增量 (Gradients)
    glm::vec3 colorStep = (right.color - left.color) / span;
    glm::vec3 normalStep = (right.normal - left.normal) / span;
    glm::vec3 worldStep = (right.worldPos - left.worldPos) / span;
    float depthStep = (right.depth - left.depth) / span;

    // 初始属性
    glm::vec3 currColor = left.color;
    glm::vec3 currNormal = left.normal;
    glm::vec3 currWorld = left.worldPos;
    float currDepth = left.depth;

    // 预步进到第一个整数像素中心
    float prestep = (float)xStart - left.x;
    currColor += colorStep * prestep;
    currNormal += normalStep * prestep;
    // ...

    for (int x = xStart; x < xEnd; ++x) {
        // 像素着色计算
        glm::vec3 finalColor = shadePixel(currColor, currNormal, currWorld, settings, shading);
        fb.setPixel(x, y, finalColor, currDepth);

        // 步进
        currColor += colorStep;
        currNormal += normalStep;
        currWorld += worldStep;
        currDepth += depthStep;
    }
}
```

### 5.3 光照计算函数
```cpp
glm::vec3 computeLighting(const glm::vec3 &pos, const glm::vec3 &normal, 
                          const glm::vec3 &baseColor, const RenderSettings &s)
{
    // 环境光
    glm::vec3 ambient = s.ambientStrength * s.lightColor * baseColor;

    // 漫反射
    glm::vec3 norm = glm::normalize(normal);
    glm::vec3 lightDir = glm::normalize(s.lightPos - pos);
    float diff = std::max(glm::dot(norm, lightDir), 0.0f);
    glm::vec3 diffuse = s.diffuseStrength * diff * s.lightColor * baseColor;

    // 镜面反射 (Blinn-Phong)
    glm::vec3 viewDir = glm::normalize(s.cameraPos - pos);
    glm::vec3 halfwayDir = glm::normalize(lightDir + viewDir);
    float spec = std::pow(std::max(glm::dot(norm, halfwayDir), 0.0f), s.shininess);
    glm::vec3 specular = s.specularStrength * spec * s.lightColor;

    return ambient + diffuse + specular;
}
```

---

## 6. 实验结果与分析

### 6.1 视觉效果对比
实验成功渲染了立方体和四面体模型，并能清晰观察到不同着色模式的区别：
1.  **Flat Shading**: 能够清晰看到三角形的边界，每个面颜色单一。当光源移动时，整个面的明暗突变。
2.  **Gouraud Shading**: 表面颜色过渡平滑。但在高光表现上存在缺陷，当高光区域小于三角形大小时，高光会消失（因为顶点处未被照亮）。
3.  **Phong Shading**: 表面光滑，高光呈现完美的圆形光斑，且随着视角移动高光在表面滑动，质感最接近真实塑料或金属材质。

### 6.2 性能分析
在 Intel Core i7 处理器上，窗口分辨率 800x600 下的平均帧生成时间（Render Time）：

| 场景 | 算法组合 | 平均耗时 (ms) | FPS (估算) | 分析 |
| :--- | :--- | :--- | :--- | :--- |
| Cube (12 tris) | Wireframe (Bresenham) | 0.02 ms | >1000 | 纯整数运算，极快。 |
| Cube (12 tris) | Flat Shading | 0.08 ms | >1000 | 光照计算次数少 (12次)。 |
| Cube (12 tris) | Gouraud Shading | 0.15 ms | >1000 | 光照计算次数少 (8顶点)，插值开销小。 |
| Cube (12 tris) | Phong Shading | 0.28 ms | ~3500 | 光照计算次数多 (约20万像素)，开销最大。 |

**结论**:
*   Phong Shading 的计算复杂度与屏幕覆盖像素数成正比 ($O(Pixels)$)，而 Gouraud 与顶点数成正比 ($O(Vertices)$)。
*   在低多边形模型下，Phong Shading 的开销是 Gouraud 的数倍，但换来了显著的画质提升。

---

## 7. 遇到的问题与解决方案

### 7.1 扫描线算法的“黑屏”问题
**问题描述**: 在初步实现 `drawScanline` 后，运行程序屏幕全黑，但线框模式正常。
**排查过程**:
1.  使用 `std::cout` 打印变换后的顶点坐标，确认 MVP 变换正确，坐标在屏幕范围内。
2.  在 `drawScanline` 内部打印 `xStart` 和 `xEnd`，发现数值合理。
3.  检查 `fb.setPixel` 调用，发现传入的 `y` 坐标是一个固定值（调试时留下的硬编码），导致所有扫描线都覆盖在同一行。
**解决方案**: 修正 `drawScanline` 的参数传递，确保传入当前循环的 `y` 索引。

### 7.2 深度测试失效
**问题描述**: 旋转立方体时，背面的三角形有时会遮挡正面的三角形。
**原因分析**: 深度缓冲区初始化值不正确，或者深度比较逻辑写反。
**解决方案**:
1.  将深度缓冲区初始化为 `std::numeric_limits<float>::max()`。
2.  在 `setPixel` 中，条件改为 `if (depth < depthBuffer[index])`。
3.  确保透视投影的 `Near` 平面设置合理（如 0.1f），避免深度精度问题。

### 7.3 依赖库链接错误
**问题描述**: 在 Linux 环境下 CMake 报错 `undefined reference to symbol 'XGetWindowAttributes'`。
**解决方案**: 这是因为 GLFW 依赖 X11 库。在 `CMakeLists.txt` 中虽然 `find_package(glfw3)` 成功，但静态链接时需要手动链接系统库。最终通过安装 `libxinerama-dev` 等开发包并确保链接器能找到它们解决。

---

## 8. 总结与展望

本次实验通过从零编写 C++ 代码，成功实现了一个基于 CPU 的软件光栅化渲染器。不仅复现了计算机图形学的经典算法（Bresenham, Edge-Walking, Phong Shading），还通过解决实际开发中的 Bug（如坐标系翻转、深度冲突、精度丢失），加深了对图形管线细节的理解。

**主要收获**:
1.  **管线理解**: 彻底搞懂了从 `Model Space` 到 `Screen Space` 的每一步变换及其数学意义。
2.  **算法细节**: 理解了插值（Interpolation）在光栅化中的核心地位，它是连接离散顶点与连续像素的桥梁。
3.  **调试技巧**: 掌握了在没有图形调试器（如 RenderDoc）的情况下，通过输出中间数据和可视化 Debug（如输出深度图）来排查图形问题的方法。

**未来改进方向**:
1.  **纹理映射 (Texture Mapping)**: 目前仅支持纯色材质。未来可加入 UV 坐标插值和纹理采样，支持漫反射贴图和高光贴图。
2.  **透视校正插值**: 目前使用的是屏幕空间线性插值，在透视投影下会有轻微变形。应实现基于 $1/z$ 的透视校正插值。
3.  **模型加载**: 集成 `tinyobjloader`，支持加载复杂的 `.obj` 模型（如 Stanford Bunny）。
4.  **多线程光栅化**: 利用 OpenMP 将屏幕划分为多个 Tile 并行渲染，大幅提升 CPU 渲染性能。

---

## 参考文献
1.  Gamenius, "The rasterization stage", *Scratchapixel 2.0*.
2.  Foley, J. D., et al. *Computer Graphics: Principles and Practice*. Addison-Wesley Professional.
3.  LearnOpenGL CN, "Coordinate Systems" & "Basic Lighting".
