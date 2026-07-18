#ifndef DEBUG_H
#define DEBUG_H

#include <glad/glad.h>
#include <iostream>

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

#endif