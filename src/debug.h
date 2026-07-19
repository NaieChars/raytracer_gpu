#ifndef DEBUG_H
#define DEBUG_H

#include <glad/glad.h>
#include <iostream>
#include <iomanip> 

void GLAPIENTRY debugCallback(GLenum source,
                              GLenum type,
                              GLuint id,
                              GLenum severity,
                              GLsizei length,
                              const GLchar* message,
                              const void* userParam)
{
    // 忽略非错误类消息（你也可以根据需要调整）
    if (type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR ||
        type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR || type == GL_DEBUG_TYPE_PORTABILITY ||
        type == GL_DEBUG_TYPE_PERFORMANCE)
    {
        std::cerr << "OpenGL Debug [" << id << "] ";
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:   std::cerr << "[HIGH] "; break;
            case GL_DEBUG_SEVERITY_MEDIUM: std::cerr << "[MED] "; break;
            case GL_DEBUG_SEVERITY_LOW:    std::cerr << "[LOW] "; break;
            default: break;
        }
        std::cerr << message << std::endl;
    }
}

// 输出帧数
void showFPS() {
    static int frameCount = 0;
    static double lastTime = glfwGetTime();

    frameCount++;
    double currentTime = glfwGetTime();
    double elapsed = currentTime - lastTime;

    if (elapsed >= 1.0) {
        double fps = frameCount / elapsed;

        // \r 回到行首，不换行；输出固定宽度避免残留
        std::cout << "\rFPS: " << std::fixed << std::setprecision(1) 
                  << std::setw(6) << fps << std::flush;

        frameCount = 0;
        lastTime = currentTime;
    }
}

#endif