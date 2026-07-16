cmake_minimum_required(VERSION 3.15)  
project(raytracer_gpu)                 

set(CMAKE_CXX_STANDARD 17)             # 强制使用 C++17 标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)    # 如果编译器不支持 C++17 则直接报错，不会降级

include(FetchContent)    

# GLFW 自动下载并编译
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git  # 从 GitHub 克隆源码
  GIT_TAG 3.4                                      # 指定 3.4 版本（稳定版）
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)       # 不生成文档，加快编译
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)      # 不生成测试程序
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)   # 不生成示例程序
FetchContent_MakeAvailable(glfw)                   # 执行下载、配置、编译 glfw 的全部流程、

# GLM 自动下载
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG v1.0.1                                   # 稳定版本，也可以换 master 或最新 tag
)
FetchContent_MakeAvailable(glm)

# GLAD 使用本地文件手动编译
add_library(glad STATIC external/glad/src/glad.c)              # 将本地的 glad.c 编译成静态库，后续 glad.c 将编译进最终的可执行文件
target_include_directories(glad PUBLIC external/glad/include)  # 让链接者自动包含 external/glad/include 目录

# 生成可执行文件并链接
add_executable(raytracer_gpu src/main.cpp)              # 源码入口在 src/main.cpp
target_link_libraries(raytracer_gpu PRIVATE glfw glad glm)  # 链接 glfw 、 glm 和 glad 的库目标

if(WIN32)                                          
  target_link_libraries(raytracer_gpu PRIVATE opengl32) # 如果是 Windows 系统，必须显式链接 Windows 自带的 OpenGL 库
endif()