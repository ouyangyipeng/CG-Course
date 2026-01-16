# 计算机图形学

# GLSL极简入门

陶钧

taoj23@mail.sysu.edu.cn

中山大学 计算机学院

国家超级计算广州中心

# 回顾此前介绍的渲染管线（讲义5）

- 显卡是高度并行化的硬件，对所有数据采用同一工作流程进行处理

- GLSL (GLslang)

- OpenGL Shading Language  
- 对着色器（shade）进行编程

- Vertex Shader

- 顶点变换，法向量变换  
- 光照  
- 纹理坐标的产生与变换

- Fragment Shader

- 纹理访问及颜色计算，等

![](images/506cc6860308cef6d5b78e8d20de8f49e789149e772b22fe598f6017025da2ef.jpg)

![](images/e20260a41341eb6b8a085663bdaf8869b0a67b1331f25810e5113302d86126fc.jpg)  
- Vertex Shader的输入输出

![](images/1a9c671b6e10570e038dd10bda831950291b080caa86cddeecaa6e58d6989eb8.jpg)  
- FragmentShader的输入输出

# - Vertex Shader: 齐次坐标顶点  $\rightarrow$  屏幕坐标顶点

# - 同时产生顶点相关的属性（经过插值后将输入至fragmentShader）

```c
varying vec3 normal;   
varying vec3 vertex_to_light_vector;   
void main()   
{ //Transforming The Vertex gl_Position  $=$  gl_MODELViewProjectionMatrix \* gl_Vertex; //Transforming The Normal To ModelView-Space normal  $=$  gl_NormalMatrix \* gl_Normal; //Transforming The Vertex Position To ModelView-Space vec4 vertex_in_modelview_space  $=$  gl_MODELViewMatrix \* gl_Vertex; //Calculating The Vector From The Vertex Position To The Light Position vertex_to_light_vector  $=$  vec3(gl_LightSource[0].position -vertex_in_modelview_space);   
}
```

# $\circ$  Normal matrix

- 将向量  $v$  视为两个顶点的差  $p - q$  则其在 camera space 中为

$\mathbf{M} \cdot p - \mathbf{M} \cdot q = \mathbf{M} \cdot (p - q) = \mathbf{M} \cdot v$  
- 可适用于切向量，但不适用于法向量  
- Nonuniform scaling下，法向量与切向量不再垂直！

- 计算normal matrix

$\cdot n^{T} \cdot t = 0$  
$\cdot (N \cdot n)^{T} \cdot (M \cdot t) = 0$  
-  $n^{T}(N^{T}M)t = 0$  
$\cdot N = (M^{-1})^{T}$

![](images/cbdefa0021a577af26ec6a9cfe188c60a79cc65d93f5da9027796ef923bf4825.jpg)  
图片来自于LearnOpenGL.com

![](images/645c68b104b042d04f38df6e0c140623b99ff3cc1ba8a80a502f56ef97d6aad4.jpg)

# - Fragment Shader: 计算片元颜色

- 计算过程中可能使用顶点属性插值后得到的片元属性

```c
varying vec3 normal;   
varying vec3 vertex_to_light_vector;   
void main()   
{ //Defining The Material Colors const vec4 AmbientColor  $=$  vec4(0.1, 0.0, 0.0, 1.0); const vec4 DiffuseColor  $=$  vec4(1.0, 0.0, 0.0, 1.0); // Scaling The Input Vector To Length 1 vec3 normalized_normal  $=$  normalize(normal); vec3 normalized Vertex_to_light_vector  $=$  normalize(vertex_to_light_vector); //Calculating The Diffuse Term And Clamping It To [0;1] float DiffuseTerm  $=$  clamp.dot(normal, vertex_to_light_vector), 0.0, 1.0); //Calculating The Final Color gl_FragColor  $=$  AmbientColor + DiffuseColor * DiffuseTerm;
```

# - GLSL中的数据类型

- 向量: vec2, vec3, vec4, vec2, vec3, vec4, bvec2, bvec3, bvec4  
- 矩阵: mat2, mat3, mat4  
- 纹理：sampler1D, sampler2D, sampler3D, samplerCube, sampler1Dshadow, sampler2Dshadow

# GLSL中的数据修饰词

- uniform: 对所有顶点而言为常量，不因顶点而异（如光源位置）  
- attribute: 因顶点而异, 只读, 只能在vertex Shader中使用  
- varying: vertex Shader的输出, fragmentShader的输入, 传输过程中进行插值  
-in, out: 表明变量为输入或输出

# - GLSL中的内置变量

- Vertex Shader中的内置attribute

• gl_Normal, gl_Color, gl_MultiTexCoordX

- 内置uniform

- gl_MODELViewMatrix, gl_MODELViewProjectionMatrix, gl_NormalMatrix

- Shader输出

vertex Shader: gl_Position  
- fragment Shader: gl_FragColor, gl_FragDepth

# 编译GLSL着色器程序

```c
GLuint vertexShader = glCreateShader(GLvertexShader);  
GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);  
glShaderSource(vertexShader, 1, &vsource, 0);  
glShaderSource(fragmentShader, 1, &fsource, 0);  
glCompileShader(vertexShader);  
glCompileShader(fragmentShader);  
GLuint program = glCreateProgram();  
glAttachShader(program, vertexShader);  
glAttachShader(program, fragmentShader);  
glLinkProgram(program);
```

# - 使用GLSL着色器程序

-glUseProgram(program): 指明在接下来的绘制中使用program所代表的着色器程序  
- gIUseProgram(0): 使用默认着色器

# 设置程序中uniform变量的值

- 获取uniform变量在程序中的位置

• glGetUniformLocation(program, "variable_name")

- 设置uniform变量在程序中的值

• glUniform{a}{b}{c} (location, value);  
•  $\{a\} : 1, 2, 3, 4$  
•  $\{b\} : f, i, u i$  
•  $\{\mathrm{c}\} : / , \mathrm{v}$

# - 此前，我们的绘制方式是

- 使用glVertex, glColor, glNormal指明顶点位置，颜色，法向量等信息

- 慢：对每个顶点的每个属性都需要调用一次相应的函数，将数据传至 OpenGL的buffer中，此后在绘制时，将buffer中内容传至显存进行绘制

- 使用GLSL中的内置变量gl_Vertex, gl_Normal等引用这些信息

```c
out vec3 normal;   
void main()   
{ gl_Position  $=$  gl_MODELViewProjectionMatrix \* gl_Ne rTEX; normal  $=$  gl_NormalMatrix \* gl_Normal;
```

- 当前趋势使用generic attributes，而非gl_Netex等内置变量

# - 使用VBO将需要绘制的内容放在编程人员创建的buffer中

- 创建VBO: void glGenBuffers(GLsizei n, GLuint* ids)  
- 绑定VBO: void glBindBuffer(GLenum target, GLuint id)  
- 拷贝数据至VBO: void glBufferData(GLenum target, GLsizei size, const void* data, GLenum usage)

• usage: GL_STATIC_DRAW, GL_STATIC_READ, GL_STATIC_copy, GL_DYNAMIC_DRAW, GL_DYNAMIC_READ, GL_DYNAMIC_copy, GL_STREAM_DRAW, GL_STREAM_READ, GL_STREAM copying  
- STATIC: 拷贝一次后不再改变  
DYNAMIC: 程序运行过程中可能发生改变  
- STREAM: 绘制过程中的每帧都发生改变

- 删除VBO: void glDeleteBuffers(GLsizei n, const GLuint* ids)

# - 使用VBO将需要绘制的内容放在编程人员创建的buffer中 - 完整示例

```txt
GLuint vbold; // ID of VBO  
GLfloat* vertices = new GLfloat[vCount*3]; // create vertex array  
// generate a new VBO and get the associated ID  
glGenBuffers(1, &vbold);  
// bind VBO in order to use  
glBindBuffer(GL_ARRAYbuffer, vbold);  
// upload data to VBO  
glBufferData(GL_ARRAYbuffer, dataSize, vertices, GL_STATIC_DRAW);  
// it is safe to delete after copying data to VBO  
delete [] vertices;  
...  
// delete VBO when program terminated  
glDeleteBuffers(1, &vbold);
```

# - 使用VBO将需要绘制的内容放在编程人员创建的buffer中

# - 拷贝部分数据至VBO

- void glBufferSubData(GLenum target, GLint offset, GLsizei size, void* data)

# - 在VBO之间拷贝数据

- void glCopyBufferSubData(GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size)

# - 修改VBO内的数据

```c
float data[] = {0.5f, 1.0f, -0.35f, [...]};  
glBindBuffer(GL_ARRAY_BUFFER, buffer);  
// get pointer  
void *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);  
// now modify data: for example, copy data into memory  
memcpy(ptr, data, sizeof(data));  
// make sure to tell OpenGL we're done with the pointer  
glUnmapBuffer(GL_ARRAY_BUFFER);
```

# - 使用VBO进行绘制

- 指明每个属性在VBO中的位置

- glVertexPointer, glColorPointer, glNormalPointer, glTexCoordinate  
- Generic: glVertexAttribPointer

- 开启/关闭相应属性的使用

- glEnableClientState/glDisableClientState

- GL Veronica_ARRAY, GL_COLOR_ARRAY, GL_NORMAL_ARRAY, GLTEXCOORD_ARRAY

- Generic: glEnableVertexAttribArray/glDisableVertexAttribArray

- 绘制函数

- glDrawArrays, glMultiDrawArrays, glDrawElements, glMultiDrawElements, glDrawRangeElements

# - 使用VBO进行绘制（使用内置变量）

// bind VBOs for vertex array and index array

glBindBuffer(GL_ARRAYbuffer, vbold1); // for vertex attributes

glBindBuffer(GL_element_ARRAY_buffer, vbold2); // for indices

glEnableClientState(GLvertex_ARRAY); // activate vertex position array

glEnableClientState(GL_NORMAL_ARRAY); // activate vertex normal array

glEnableClientState(GL-textURECOORD_ARRAY); // activate texture coord array

// do same as vertex array except pointer

glVertexPointer(3, GL_FLOAT, stride, offset1); // last param is offset, not ptr

glNormalPointer(GL_FLOAT, stride, offset2);

glTexCoordPointer(2, GL_FLOAT, stride, offset3);

// draw 6 faces using offset of index array

glDrawElements(GL_TRIANGLE, 36, GL_UNSIGNED_BYTE, 0);

glDisableClientState(GLvertex_ARRAY); // deactivate vertex position array

glDisableClientState(GL_NORMAL_ARRAY); // deactivate vertex normal array

glDisableClientState(GL-textURECOORD_ARRAY); // deactivate vertex tex coord array

// bind with 0, so, switch back to normal pointer operation

glBindBuffer(GL_ARRAY_buffer, 0);

glBindBuffer(GL_element_ARRAY_buffer, 0);

# - 使用VBO进行绘制（使用generic属性）

// bind VBOs for vertex array and index array

glBindBuffer(GL_ARRAY_BUFFERER, vbold1); // for vertex coordinates

glBindBuffer(GL_element_ARRAY_buffer, vbold2); // for indices

glEnableVertexAttribArray(attribVertex); // activate vertex position array

glEnableVertexAttribArray(attribNormal); // activate vertex normal array

glEnableVertexAttribArray(attribTexCoord); // activate texture coords array

// set vertex arrays with generic API

glVertexAttribPointer(attribVertex, 3, GL_FLOAT, false, stride, offset1);

glVertexAttribPointer(attribNormal, 3, GL_FLOAT, false, stride, offset2);

glVertexAttribPointer(attribTexCoord, 2, GL_FLOAT, false, stride, offset3);

// draw 6 faces using offset of index array

glDrawElements(GL_TRIANGLE, 36, GL_UNSIGNED_BYTE, 0);

glDisableVertexAttribArray(attribVertex); // deactivate vertex position

glDisableVertexAttribArray(attribNormal); // deactivate vertex normal

glDisableVertexAttribArray(attribTexCoord); // deactivate texture coords

// bind with 0, so, switch back to normal pointer operation

glBindBuffer(GL_ARRAY_buffer, 0);

glBindBuffer(GL_element_ARRAY_buffer, 0);

# - 使用内置变量 vs 使用generic属性

# - 内置变量

```c
out vec3 normal;   
void main()   
{ gl_Position  $=$  gl_MODELViewProjectionMatrix \* gl_Ne rTEX; normal  $=$  gl_NormalMatrix \* gl_Normal;
```

# - generic属性

```c
layout (location  $= 0$  ) in vec3 vertex_attribute;   
layout (location  $= 1$  ) in vec3 normal_attribute;   
uniform mat4 model;   
uniform mat4 view;   
uniform mat4 projection;   
out vec3 normal;   
void main()   
{ gl_Position  $\equiv$  projection\*view\*model\*vertex_attribute; normal  $\equiv$  mat3(transposeinverse_view\*model))\*normal_attribute; }
```

# - 使用generic属性

- 手动指明（推荐）

```txt
//GLSL layout location  $= 0$  ) in vec3 position; layout (location  $= 1$  ) in vec2 texcoord;
```

```javascript
//CPU  
glEnableVertexAttribArray(0);  
glEnableVertexAttribArray(1);
```

- 动态获取（旧写法，不使用layout）

```c
//GLSL   
in vec3 position;   
in vec2 texcoord;   
//CPU   
GLuint locPos  $=$  glGetAttribLocation(program, "position");   
GLuint locUV  $=$  glGetAttribLocation(program, "texcoord");   
glEnableVertexAttribArray(locPos);   
glEnableVertexAttribArray(locUV);
```

# - GLSL着色器程序举例：2D twisting

$$
- \left( \begin{array}{c} x ^ {\prime} \\ y ^ {\prime} \end{array} \right) = \left( \begin{array}{c c} \cos \left(t \sqrt {x ^ {2} + y ^ {2}}\right) & - \sin \left(t \sqrt {x ^ {2} + y ^ {2}}\right) \\ \sin \left(t \sqrt {x ^ {2} + y ^ {2}}\right) & \cos \left(t \sqrt {x ^ {2} + y ^ {2}}\right) \end{array} \right) \left( \begin{array}{c} x \\ y \end{array} \right)
$$

```txt
uniform float twisting;   
void main()   
{ float angle  $=$  twisting \* length(gl_VeRTex.xy); float s  $=$  sin(angle); float c  $=$  cos(angle); gl_Position.x  $=$  c\* glVRTex.x -s\* glVRTex.y; gl_Position.y  $=$  s\* glVRTex.x +c\* glVRTex.y; gl_Position.z  $= 0.0$  . gl_Position.w  $= 1.0$  .
```

![](images/5659b722dd036d039ce13716930917c54cbe451129ff7f5c52c4b9e2c98dfa60.jpg)

# - GLSL着色器程序举例：根据空间位置

```txt
out vec3 color;  
void main()  
{  
    ...  
    color = (vec3(gl_Position.xyz) + vec3(1, 1, 1)) * 0.5;
```

- 此处使用的是什么空间中的位置？  
- 作业中可尝试使用不同空间中的位置，并比较其效果

![](images/0c35342ea4e20cb2962bb96003ed15eacb4cc14e2c2cb5847979e13283974e64.jpg)

# Questions?

![](images/599e584a7848bd3f5602f21a750d33235faf81945a842e71608fff0f567a9306.jpg)
