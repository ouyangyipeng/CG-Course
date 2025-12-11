cmake_minimum_required(VERSION 3.15)
project(CG_HW2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(THIRDPARTY_DIR ${CMAKE_SOURCE_DIR}/../3rdparty)
set(GLFW_DIR ${THIRDPARTY_DIR}/glfw-3.3.8)
set(IMGUI_DIR ${THIRDPARTY_DIR}/imgui-1.90.4)
set(GLM_DIR ${THIRDPARTY_DIR}/glm)
set(GLAD_SOURCE ${GLFW_DIR}/deps/glad_gl.c)

option(GLFW_BUILD_EXAMPLES "Build GLFW examples" OFF)
option(GLFW_BUILD_TESTS "Build GLFW tests" OFF)
option(GLFW_BUILD_DOCS "Build GLFW docs" OFF)
option(GLFW_INSTALL "Enable GLFW install target" OFF)

add_subdirectory(${GLFW_DIR} ${CMAKE_BINARY_DIR}/glfw)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLFW_DIR}/include
    ${GLFW_DIR}/deps
)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD GLFW_INCLUDE_NONE)

add_executable(${PROJECT_NAME}
    src/main.cpp
    ${GLAD_SOURCE}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${GLFW_DIR}/include
    ${GLFW_DIR}/deps
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLM_DIR}
    ${THIRDPARTY_DIR}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GLAD
    GLFW_INCLUDE_NONE
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui
    glfw
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
endif()

if (UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl pthread)
endif()