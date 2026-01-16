# 计算机图形学 HW3 实验报告

## 一、实验原理

### 1. Phong Shading
Phong Shading（冯氏着色）是一种在片元着色器（Fragment Shader）中进行光照计算的方法。与 Gouraud Shading 在顶点着色器中计算光照并插值颜色不同，Phong Shading 在顶点着色器中输出法向量和顶点位置，由光栅化阶段进行插值，然后在片元着色器中对每个像素使用插值后的法向量进行光照计算。

Phong 光照模型包含三个分量：
1.  **环境光 (Ambient)**: 模拟全局光照，$I_{amb} = k_a \cdot I_{light}$。
2.  **漫反射 (Diffuse)**: 模拟粗糙表面的反射，遵循 Lambert 定律，$I_{diff} = k_d \cdot I_{light} \cdot \max(N \cdot L, 0)$。
3.  **镜面反射 (Specular)**: 模拟光滑表面的高光，$I_{spec} = k_s \cdot I_{light} \cdot \max(R \cdot V, 0)^{\alpha}$。

最终颜色为三者之和。

### 2. VBO (Vertex Buffer Object)
VBO 是 OpenGL 中用于在显卡内存（显存）中存储顶点数据的机制。
- **传统立即模式 (`glBegin`/`glEnd`)**: 每一帧都需要 CPU 将顶点数据逐个发送给 GPU，带宽消耗大，性能低。
- **VBO**: 将顶点数据一次性上传到 GPU 显存。绘制时，CPU 只需发送绘制指令（如 `glDrawArrays` 或 `glDrawElements`），GPU 直接从显存读取数据，极大提高了渲染效率。

## 二、实现细节

### 1. Shader 实现 (Phong Shading)

**Vertex Shader (`phong.vert`)**:
负责将顶点位置和法向量变换到观察空间（View Space），并传递给 Fragment Shader。
```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main()
{
    // 变换到观察空间
    vec4 viewPos4 = view * model * vec4(aPos, 1.0);
    FragPos = vec3(viewPos4);
    
    // 法向量变换（使用 Normal Matrix 维持垂直性）
    Normal = normalize(normalMatrix * aNormal);
    
    gl_Position = projection * viewPos4;
}
```

**Fragment Shader (`phong.frag`)**:
在片元阶段计算 Phong 光照。
```glsl
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos; // View Space
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;

void main()
{
    // Ambient
    vec3 ambient = ambientStrength * lightColor;
  
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;
    
    // Specular (Blinn-Phong or Phong)
    // 这里使用标准 Phong 反射模型
    vec3 viewDir = normalize(-FragPos); // View Space 中视点为原点
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;  
        
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
```

### 2. VBO 与球体生成 (C++)

使用经纬度划分法生成球体顶点和索引，并绑定到 VBO/EBO。

```cpp
// 球体生成核心代码
void createSphere(float radius, int sectors, int stacks, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    // ... (省略具体数学计算，见源码)
    // 生成顶点位置 (x,y,z) 和法向量 (nx,ny,nz)
    // 生成索引 (indices) 用于 glDrawElements
}

// VBO 配置
unsigned int VBO, VAO, EBO;
glGenVertexArrays(1, &VAO);
glGenBuffers(1, &VBO);
glGenBuffers(1, &EBO);

glBindVertexArray(VAO);

glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

// 设置顶点属性指针
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // Pos
glEnableVertexAttribArray(0);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // Normal
glEnableVertexAttribArray(1);
```

## 三、问题解决

1.  **GLM 头文件找不到**:
    - **问题**: 编译时报错 `fatal error: glm/glm.hpp: No such file or directory`。
    - **原因**: `CMakeLists.txt` 中 `GLM_DIR` 指向了 `glm` 库的根目录，但代码中使用 `#include <glm/glm.hpp>` 需要父目录在 include path 中。
    - **解决**: 在 `CMakeLists.txt` 的 `target_include_directories` 中添加 `${THIRDPARTY_DIR}`。

2.  **Shader 文件读取失败**:
    - **问题**: 运行时报错 `ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ`。
    - **原因**: 可执行文件在 `build/bin` 目录，而 Shader 源码在 `code/shaders`。程序默认在当前工作目录寻找 Shader。
    - **解决**: 将 `shaders` 文件夹复制到 `build/bin` 目录中，确保程序能找到文件。

## 四、结果对比与讨论

### 1. Phong Shading vs Gouraud Shading
-   **Gouraud Shading**: 光照计算在顶点着色器进行。对于低多边形模型，高光（Specular）会显得不自然，呈现出明显的三角形插值痕迹，且高光可能因为顶点未处于高光中心而消失。
-   **Phong Shading**: 光照计算在片元着色器进行。法向量在三角形内部平滑插值，因此能产生非常平滑的高光效果，视觉质量远高于 Gouraud Shading，但计算开销稍大。

### 2. VBO vs 立即模式 (Immediate Mode)
-   **立即模式 (`glBegin`/`glEnd`)**: 每次绘制都需要 CPU 循环遍历所有顶点并调用 API 发送数据。当顶点数量达到数万甚至数百万时，CPU 与 GPU 之间的带宽成为瓶颈，且函数调用开销巨大。
-   **VBO**: 数据预先上传至显存。绘制时，CPU 仅需发送一条绘制命令，GPU 即可全速读取显存数据进行渲染。
-   **性能差异**: 在渲染高精度球体（如细分级别很高）时，VBO 的 FPS 远高于立即模式。虽然本实验使用 Core Profile 无法直接运行立即模式，但理论和过往经验表明 VBO 是现代图形渲染的基石。

### 3. Index Array (EBO) 的优势
-   本实验使用了 EBO (`glDrawElements`)。
-   如果不使用 EBO，绘制两个相邻三角形（共用 2 个顶点）需要存储 6 个顶点数据。
-   使用 EBO，只需存储 4 个唯一顶点，并通过索引引用它们。这显著减少了显存占用（顶点数据通常比索引数据大得多）和顶点着色器的执行次数（GPU 的 Post-Transform Cache 机制）。
