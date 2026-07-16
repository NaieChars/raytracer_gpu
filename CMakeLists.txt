cmake_minimum_required(VERSION 3.15)
project(raytracer_gpu)

# 强制 UTF-8
if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------- GLFW ----------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw)

# ---------- GLAD ----------
add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

# ---------- GLM ----------
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${CMAKE_SOURCE_DIR}/third_party)

# ---------- stb ----------
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_SOURCE_DIR}/third_party/stb)

# ---------- 主程序 ----------
add_executable(raytracer_gpu src/main.cpp)
target_link_libraries(raytracer_gpu PRIVATE glfw glad glm stb)


if(WIN32)
    target_link_libraries(raytracer_gpu PRIVATE opengl32)
endif()