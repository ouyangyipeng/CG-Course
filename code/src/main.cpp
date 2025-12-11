// Software rasterizer for triangle drawing, Gouraud/Phong shading, and ImGui-based UI.

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace
{

enum class LineMode
{
	DDA,
	Bresenham
};

enum class ShadingMode
{
	Flat,
	Gouraud,
	Phong,
	BlinnPhong
};

enum class MeshType
{
	SingleTriangle,
	Cube,
	Tetrahedron
};

struct RenderSettings
{
	LineMode lineMode = LineMode::DDA;
	ShadingMode shading = ShadingMode::BlinnPhong;
	MeshType mesh = MeshType::SingleTriangle;
	bool drawWireframe = false;
	bool autoRotate = true;
	float rotationSpeed = 30.0f; // degrees per second
	float orbitYawDeg = 0.0f;    // horizontal angle around target
	float orbitPitchDeg = -10.0f; // vertical angle (clamped)
	float orbitRadius = 3.5f;    // distance to target
	glm::vec3 clearColor = glm::vec3(0.05f, 0.05f, 0.08f);
	glm::vec3 baseColor = glm::vec3(0.7f, 0.7f, 0.8f);
	glm::vec3 lightColor = glm::vec3(1.0f);
	glm::vec3 lightPos = glm::vec3(2.0f, 2.0f, 2.5f);
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.5f);
	float ambientStrength = 0.1f;
	float diffuseStrength = 1.0f;
	float specularStrength = 0.8f;
	float shininess = 64.0f;
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
};

struct RasterVertex
{
	glm::vec2 screen;   // pixel coordinate
	float depth;        // 0..1 depth
	glm::vec3 color;    // lit color for flat/Gouraud
	glm::vec3 albedo;   // base color for per-pixel shading
	glm::vec3 normal;   // world-space normal
	glm::vec3 worldPos; // world-space position
};

struct Framebuffer
{
	int width = 800;
	int height = 600;
	std::vector<uint8_t> color; // RGBA
	std::vector<float> depth;

	Framebuffer(int w, int h)
		: width(w), height(h), color(static_cast<size_t>(w * h * 4)), depth(static_cast<size_t>(w * h))
	{
	}

	void resize(int w, int h)
	{
		width = w;
		height = h;
		color.assign(static_cast<size_t>(w * h * 4), 0);
		depth.assign(static_cast<size_t>(w * h), std::numeric_limits<float>::max());
	}

	void clear(const glm::vec3 &c)
	{
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				const size_t idx = static_cast<size_t>((y * width + x) * 4);
				color[idx + 0] = static_cast<uint8_t>(std::clamp(c.r, 0.0f, 1.0f) * 255.0f);
				color[idx + 1] = static_cast<uint8_t>(std::clamp(c.g, 0.0f, 1.0f) * 255.0f);
				color[idx + 2] = static_cast<uint8_t>(std::clamp(c.b, 0.0f, 1.0f) * 255.0f);
				color[idx + 3] = 255;
			}
		}
		std::fill(depth.begin(), depth.end(), std::numeric_limits<float>::max());
	}

	void setPixel(int x, int y, const glm::vec3 &c, float z)
	{
		if (x < 0 || x >= width || y < 0 || y >= height)
		{
			return;
		}
		const size_t idxDepth = static_cast<size_t>(y * width + x);
		if (z >= depth[idxDepth])
		{
			return;
		}
		depth[idxDepth] = z;

		const size_t idxColor = static_cast<size_t>(idxDepth * 4);
		color[idxColor + 0] = static_cast<uint8_t>(std::clamp(c.r, 0.0f, 1.0f) * 255.0f);
		color[idxColor + 1] = static_cast<uint8_t>(std::clamp(c.g, 0.0f, 1.0f) * 255.0f);
		color[idxColor + 2] = static_cast<uint8_t>(std::clamp(c.b, 0.0f, 1.0f) * 255.0f);
		color[idxColor + 3] = 255;
	}
};

glm::vec3 computeLighting(const glm::vec3 &worldPos,
						  const glm::vec3 &normal,
						  const glm::vec3 &albedo,
						  const RenderSettings &settings,
						  ShadingMode shadingMode)
{
	glm::vec3 n = glm::normalize(normal);
	glm::vec3 l = glm::normalize(settings.lightPos - worldPos);
	glm::vec3 v = glm::normalize(settings.cameraPos - worldPos);
	glm::vec3 h = glm::normalize(l + v);

	const float diff = std::max(glm::dot(n, l), 0.0f);
	float spec = 0.0f;
	if (diff > 0.0f)
	{
		if (shadingMode == ShadingMode::Phong)
		{
			const glm::vec3 r = glm::reflect(-l, n);
			spec = std::pow(std::max(glm::dot(r, v), 0.0f), settings.shininess);
		}
		else
		{
			spec = std::pow(std::max(glm::dot(n, h), 0.0f), settings.shininess);
		}
	}

	const glm::vec3 ambient = settings.ambientStrength * settings.lightColor * albedo;
	const glm::vec3 diffuse = settings.diffuseStrength * diff * settings.lightColor * albedo;
	const glm::vec3 specular = settings.specularStrength * spec * settings.lightColor;
	return ambient + diffuse + specular;
}

void drawLineDDA(Framebuffer &fb, const glm::ivec2 &a, const glm::ivec2 &b, const glm::vec3 &color)
{
	const int dx = b.x - a.x;
	const int dy = b.y - a.y;
	const int steps = std::max(std::abs(dx), std::abs(dy));
	const float incX = static_cast<float>(dx) / static_cast<float>(steps);
	const float incY = static_cast<float>(dy) / static_cast<float>(steps);
	float x = static_cast<float>(a.x);
	float y = static_cast<float>(a.y);
	for (int i = 0; i <= steps; ++i)
	{
		fb.setPixel(static_cast<int>(std::round(x)), static_cast<int>(std::round(y)), color, 0.0f);
		x += incX;
		y += incY;
	}
}

void drawLineBresenham(Framebuffer &fb, const glm::ivec2 &a, const glm::ivec2 &b, const glm::vec3 &color)
{
	int x0 = a.x;
	int y0 = a.y;
	int x1 = b.x;
	int y1 = b.y;
	const int dx = std::abs(x1 - x0);
	const int dy = -std::abs(y1 - y0);
	const int sx = x0 < x1 ? 1 : -1;
	const int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;
	while (true)
	{
		fb.setPixel(x0, y0, color, 0.0f);
		if (x0 == x1 && y0 == y1)
		{
			break;
		}
		const int e2 = 2 * err;
		if (e2 >= dy)
		{
			err += dy;
			x0 += sx;
		}
		if (e2 <= dx)
		{
			err += dx;
			y0 += sy;
		}
	}
}

struct EdgeStepper
{
	int yStart = 0;
	int yEnd = -1;
	float x = 0.0f;
	float dx = 0.0f;
	float depth = 1.0f;
	float dDepth = 0.0f;
	glm::vec3 color{0.0f};
	glm::vec3 dColor{0.0f};
	glm::vec3 normal{0.0f};
	glm::vec3 dNormal{0.0f};
	glm::vec3 worldPos{0.0f};
	glm::vec3 dWorldPos{0.0f};

	void step()
	{
		x += dx;
		depth += dDepth;
		color += dColor;
		normal += dNormal;
		worldPos += dWorldPos;
	}
};

EdgeStepper makeEdge(const RasterVertex &a, const RasterVertex &b)
{
	EdgeStepper e;
	const RasterVertex *top = &a;
	const RasterVertex *bottom = &b;
	if (top->screen.y > bottom->screen.y)
	{
		std::swap(top, bottom);
	}

	const float dy = bottom->screen.y - top->screen.y;
	if (std::abs(dy) < 1e-5f)
	{
		e.yStart = 0;
		e.yEnd = -1;
		return e;
	}

	e.yStart = static_cast<int>(std::ceil(top->screen.y));
	e.yEnd = static_cast<int>(std::ceil(bottom->screen.y)) - 1;
	const float yOffset = static_cast<float>(e.yStart) - top->screen.y;
	const float invDy = 1.0f / dy;

	e.dx = (bottom->screen.x - top->screen.x) * invDy;
	e.dDepth = (bottom->depth - top->depth) * invDy;
	e.dColor = (bottom->color - top->color) * invDy;
	e.dNormal = (bottom->normal - top->normal) * invDy;
	e.dWorldPos = (bottom->worldPos - top->worldPos) * invDy;

	e.x = top->screen.x + e.dx * yOffset;
	e.depth = top->depth + e.dDepth * yOffset;
	e.color = top->color + e.dColor * yOffset;
	e.normal = top->normal + e.dNormal * yOffset;
	e.worldPos = top->worldPos + e.dWorldPos * yOffset;

	return e;
}

glm::vec3 shadePixel(const glm::vec3 &albedo,
					 const glm::vec3 &normal,
					 const glm::vec3 &worldPos,
					 const RenderSettings &settings,
					 ShadingMode mode)
{
	if (mode == ShadingMode::Flat)
	{
		return computeLighting(worldPos, normal, albedo, settings, ShadingMode::BlinnPhong);
	}
	if (mode == ShadingMode::Gouraud)
	{
		return albedo; // already lit at vertices
	}
	return computeLighting(worldPos, normal, albedo, settings, mode);
}

void drawScanline(Framebuffer &fb,
			  EdgeStepper left,
			  EdgeStepper right,
			  int y,
			  const RenderSettings &settings,
			  ShadingMode shading)
{
	if (y < 0 || y >= fb.height)
	{
		return;
	}
	if (left.x > right.x)
	{
		std::swap(left, right);
	}

	int xStart = static_cast<int>(std::ceil(left.x));
	int xEnd = static_cast<int>(std::floor(right.x));
	const float span = right.x - left.x;
	if (span == 0.0f)
	{
		return;
	}

	glm::vec3 colorStep = (right.color - left.color) / span;
	glm::vec3 normalStep = (right.normal - left.normal) / span;
	glm::vec3 worldStep = (right.worldPos - left.worldPos) / span;
	float depthStep = (right.depth - left.depth) / span;

	const float offset = static_cast<float>(xStart) - left.x;
	glm::vec3 color = left.color + colorStep * offset;
	glm::vec3 normal = left.normal + normalStep * offset;
	glm::vec3 worldPos = left.worldPos + worldStep * offset;
	float depth = left.depth + depthStep * offset;

	for (int x = xStart; x <= xEnd; ++x)
	{
		const glm::vec3 lit = shadePixel(color, normal, worldPos, settings, shading);
		fb.setPixel(x, y, lit, depth);
		color += colorStep;
		normal += normalStep;
		worldPos += worldStep;
		depth += depthStep;
	}
}

void rasterizeTriangle(Framebuffer &fb,
					   const std::array<RasterVertex, 3> &v,
					   const RenderSettings &settings)
{
	// Sort by y to build top/middle/bottom vertices.
	std::array<RasterVertex, 3> tri = v;
	std::sort(tri.begin(), tri.end(), [](const RasterVertex &a, const RasterVertex &b) {
		return a.screen.y < b.screen.y;
	});

	EdgeStepper longEdge = makeEdge(tri[0], tri[2]);
	EdgeStepper upperEdge = makeEdge(tri[0], tri[1]);
	EdgeStepper lowerEdge = makeEdge(tri[1], tri[2]);

	for (int y = upperEdge.yStart; y <= upperEdge.yEnd; ++y)
	{
		drawScanline(fb, longEdge, upperEdge, y, settings, settings.shading);
		longEdge.step();
		upperEdge.step();
	}

	for (int y = lowerEdge.yStart; y <= lowerEdge.yEnd; ++y)
	{
		drawScanline(fb, longEdge, lowerEdge, y, settings, settings.shading);
		longEdge.step();
		lowerEdge.step();
	}

	if (settings.drawWireframe)
	{
		const glm::vec3 edgeColor(1.0f, 0.9f, 0.2f);
		const glm::ivec2 a{static_cast<int>(std::round(v[0].screen.x)), static_cast<int>(std::round(v[0].screen.y))};
		const glm::ivec2 b{static_cast<int>(std::round(v[1].screen.x)), static_cast<int>(std::round(v[1].screen.y))};
		const glm::ivec2 c{static_cast<int>(std::round(v[2].screen.x)), static_cast<int>(std::round(v[2].screen.y))};
		if (settings.lineMode == LineMode::DDA)
		{
			drawLineDDA(fb, a, b, edgeColor);
			drawLineDDA(fb, b, c, edgeColor);
			drawLineDDA(fb, c, a, edgeColor);
		}
		else
		{
			drawLineBresenham(fb, a, b, edgeColor);
			drawLineBresenham(fb, b, c, edgeColor);
			drawLineBresenham(fb, c, a, edgeColor);
		}
	}
}

std::vector<std::array<Vertex, 3>> buildSingleTriangle()
{
	std::vector<std::array<Vertex, 3>> tris;
	tris.push_back({Vertex{glm::vec3(-0.6f, -0.45f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)},
				   Vertex{glm::vec3(0.6f, -0.45f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
				   Vertex{glm::vec3(0.0f, 0.7f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f)}});
	return tris;
}

std::vector<std::array<Vertex, 3>> buildCube()
{
	const std::array<glm::vec3, 8> p = {
		glm::vec3(-0.6f, -0.6f, 0.6f),  glm::vec3(0.6f, -0.6f, 0.6f),
		glm::vec3(0.6f, 0.6f, 0.6f),    glm::vec3(-0.6f, 0.6f, 0.6f),
		glm::vec3(-0.6f, -0.6f, -0.6f), glm::vec3(0.6f, -0.6f, -0.6f),
		glm::vec3(0.6f, 0.6f, -0.6f),   glm::vec3(-0.6f, 0.6f, -0.6f)};

	const std::array<glm::vec3, 6> n = {
		glm::vec3(0, 0, 1), glm::vec3(0, 0, -1), glm::vec3(-1, 0, 0),
		glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, -1, 0)};

	const glm::vec3 faceColors[6] = {
		{0.9f, 0.1f, 0.1f}, {0.1f, 0.9f, 0.2f}, {0.1f, 0.2f, 0.9f},
		{0.9f, 0.9f, 0.1f}, {0.1f, 0.9f, 0.9f}, {0.9f, 0.4f, 0.9f}};

	const int idx[] = {
		0, 1, 2, 2, 3, 0,       // front
		5, 4, 7, 7, 6, 5,       // back
		4, 0, 3, 3, 7, 4,       // left
		1, 5, 6, 6, 2, 1,       // right
		3, 2, 6, 6, 7, 3,       // top
		4, 5, 1, 1, 0, 4};      // bottom

	std::vector<std::array<Vertex, 3>> tris;
	tris.reserve(12);
	for (int f = 0; f < 6; ++f)
	{
		glm::vec3 normal = n[f];
		glm::vec3 color = faceColors[f];
		for (int t = 0; t < 2; ++t)
		{
			int base = f * 6 + t * 3;
			tris.push_back({Vertex{p[idx[base + 0]], normal, color},
						   Vertex{p[idx[base + 1]], normal, color},
						   Vertex{p[idx[base + 2]], normal, color}});
		}
	}
	return tris;
}

std::vector<std::array<Vertex, 3>> buildTetrahedron()
{
	const std::array<glm::vec3, 4> p = {
		glm::vec3(0.0f, 0.8f, 0.0f),
		glm::vec3(-0.7f, -0.6f, 0.7f),
		glm::vec3(0.7f, -0.6f, 0.7f),
		glm::vec3(0.0f, -0.6f, -0.8f)};

	const glm::vec3 colors[4] = {
		{0.9f, 0.3f, 0.3f}, {0.3f, 0.9f, 0.3f}, {0.3f, 0.3f, 0.9f}, {0.9f, 0.9f, 0.3f}};

	const int idx[] = {
		0, 1, 2,
		0, 2, 3,
		0, 3, 1,
		1, 3, 2};

	std::vector<std::array<Vertex, 3>> tris;
	tris.reserve(4);
	for (int f = 0; f < 4; ++f)
	{
		glm::vec3 a = p[idx[f * 3 + 0]];
		glm::vec3 b = p[idx[f * 3 + 1]];
		glm::vec3 c = p[idx[f * 3 + 2]];
		glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
		glm::vec3 color = colors[f % 4];
		tris.push_back({Vertex{a, normal, color}, Vertex{b, normal, color}, Vertex{c, normal, color}});
	}
	return tris;
}

RasterVertex toRaster(const Vertex &v,
					  const glm::mat4 &model,
					  const glm::mat4 &view,
					  const glm::mat4 &proj,
					  const Framebuffer &fb,
					  const RenderSettings &settings)
{
	RasterVertex out{};
	const glm::vec4 world = model * glm::vec4(v.position, 1.0f);
	const glm::vec4 clip = proj * view * world;
	const glm::vec3 ndc = glm::vec3(clip) / clip.w;
	out.screen.x = (ndc.x * 0.5f + 0.5f) * static_cast<float>(fb.width - 1);
	out.screen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(fb.height - 1);
	out.depth = (ndc.z + 1.0f) * 0.5f;
	out.worldPos = glm::vec3(world);
	out.normal = glm::normalize(glm::mat3(model) * v.normal);
	out.albedo = v.color;

	if (settings.shading == ShadingMode::Gouraud)
	{
		out.color = computeLighting(out.worldPos, out.normal, v.color, settings, ShadingMode::BlinnPhong);
	}
	else
	{
		out.color = v.color;
	}

	if (settings.shading == ShadingMode::Flat)
	{
		out.color = settings.baseColor;
		out.albedo = settings.baseColor;
	}
	return out;
}

void renderMesh(Framebuffer &fb,
				const std::vector<std::array<Vertex, 3>> &mesh,
				const glm::mat4 &model,
				const glm::mat4 &view,
				const glm::mat4 &proj,
				const RenderSettings &settings)
{
	for (const auto &tri : mesh)
	{
		std::array<RasterVertex, 3> rv{
			toRaster(tri[0], model, view, proj, fb, settings),
			toRaster(tri[1], model, view, proj, fb, settings),
			toRaster(tri[2], model, view, proj, fb, settings)};

		rasterizeTriangle(fb, rv, settings);
	}
}

GLuint createTexture(int width, int height)
{
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);
	return tex;
}

void uploadTexture(GLuint tex, const Framebuffer &fb)
{
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fb.width, fb.height, GL_RGBA, GL_UNSIGNED_BYTE, fb.color.data());
	glBindTexture(GL_TEXTURE_2D, 0);
}

void setupImGui(GLFWwindow *window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
}

void shutdownImGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void drawUI(RenderSettings &settings, double renderMs)
{
	ImGui::Begin("Controls");
	const char *lineLabels[] = {"DDA", "Bresenham"};
	int lineMode = settings.lineMode == LineMode::DDA ? 0 : 1;
	if (ImGui::Combo("Edge mode", &lineMode, lineLabels, 2))
	{
		settings.lineMode = lineMode == 0 ? LineMode::DDA : LineMode::Bresenham;
	}

	const char *shadeLabels[] = {"Flat", "Gouraud", "Phong", "Blinn-Phong"};
	int shadeMode = static_cast<int>(settings.shading);
	if (ImGui::Combo("Shading", &shadeMode, shadeLabels, 4))
	{
		settings.shading = static_cast<ShadingMode>(shadeMode);
	}

	const char *meshLabels[] = {"Single triangle", "Cube", "Tetrahedron"};
	int meshMode = static_cast<int>(settings.mesh);
	if (ImGui::Combo("Mesh", &meshMode, meshLabels, 3))
	{
		settings.mesh = static_cast<MeshType>(meshMode);
	}

	ImGui::Checkbox("Draw wireframe", &settings.drawWireframe);
	ImGui::Checkbox("Auto rotate", &settings.autoRotate);
	ImGui::SliderFloat("Rotation speed (deg/s)", &settings.rotationSpeed, 0.0f, 180.0f);
	ImGui::SliderFloat("Orbit yaw (deg)", &settings.orbitYawDeg, -180.0f, 180.0f);
	ImGui::SliderFloat("Orbit pitch (deg)", &settings.orbitPitchDeg, -85.0f, 85.0f);
	ImGui::SliderFloat("Orbit radius", &settings.orbitRadius, 1.5f, 8.0f);

	ImGui::ColorEdit3("Clear color", &settings.clearColor.x);
	ImGui::ColorEdit3("Base color", &settings.baseColor.x);
	ImGui::ColorEdit3("Light color", &settings.lightColor.x);
	ImGui::DragFloat3("Light pos", &settings.lightPos.x, 0.05f, -10.0f, 10.0f);
	ImGui::DragFloat("Ambient", &settings.ambientStrength, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Diffuse", &settings.diffuseStrength, 0.01f, 0.0f, 2.0f);
	ImGui::DragFloat("Specular", &settings.specularStrength, 0.01f, 0.0f, 2.0f);
	ImGui::DragFloat("Shininess", &settings.shininess, 1.0f, 1.0f, 128.0f);

	ImGui::Text("Render time: %.3f ms", renderMs);
	ImGui::Text("Use this panel to compare DDA vs Bresenham edges and Gouraud vs Phong shading.");
	ImGui::End();
}

} // namespace

int main()
{
	if (!glfwInit())
	{
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(1280, 720, "CG HW2 - Rasterizer", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (gladLoadGL(glfwGetProcAddress) == 0)
	{
		glfwTerminate();
		return -1;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	setupImGui(window);

	Framebuffer fb(960, 720);
	GLuint framebufferTex = createTexture(fb.width, fb.height);

	RenderSettings settings;
	double lastTime = glfwGetTime();
	float rotationDeg = 0.0f;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		const double now = glfwGetTime();
		const double dt = now - lastTime;
		lastTime = now;
		if (settings.autoRotate)
		{
			rotationDeg = std::fmod(rotationDeg + settings.rotationSpeed * static_cast<float>(dt), 360.0f);
		}

		fb.clear(settings.clearColor);

		const glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(rotationDeg), glm::vec3(0.0f, 1.0f, 0.0f));

		const float yawRad = glm::radians(settings.orbitYawDeg);
		const float pitchRad = glm::radians(glm::clamp(settings.orbitPitchDeg, -85.0f, 85.0f));
		const float cp = std::cos(pitchRad);
		const float sp = std::sin(pitchRad);
		const float cy = std::cos(yawRad);
		const float sy = std::sin(yawRad);
		glm::vec3 camPos = glm::vec3(cp * cy, sp, cp * sy) * settings.orbitRadius;
		settings.cameraPos = camPos;

		const glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::mat4 proj = glm::perspective(glm::radians(45.0f), static_cast<float>(fb.width) / static_cast<float>(fb.height), 0.1f, 20.0f);

		std::vector<std::array<Vertex, 3>> mesh;
		switch (settings.mesh)
		{
		case MeshType::SingleTriangle:
			mesh = buildSingleTriangle();
			break;
		case MeshType::Cube:
			mesh = buildCube();
			break;
		case MeshType::Tetrahedron:
			mesh = buildTetrahedron();
			break;
		}

		const double renderStart = glfwGetTime();
		renderMesh(fb, mesh, model, view, proj, settings);
		const double renderEnd = glfwGetTime();
		const double renderMs = (renderEnd - renderStart) * 1000.0;

		uploadTexture(framebufferTex, fb);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		drawUI(settings, renderMs);

		ImGui::Begin("Rasterized Output");
		ImGui::Image(reinterpret_cast<void *>(static_cast<intptr_t>(framebufferTex)), ImVec2(static_cast<float>(fb.width), static_cast<float>(fb.height)));
		ImGui::End();

		int displayW, displayH;
		glfwGetFramebufferSize(window, &displayW, &displayH);
		glViewport(0, 0, displayW, displayH);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	shutdownImGui();
	glDeleteTextures(1, &framebufferTex);
	glfwTerminate();
	return 0;
}
