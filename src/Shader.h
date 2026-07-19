#ifndef SHADER_H
#define SHADER_H

#include "glad/glad.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <initializer_list>

class Shader
{
    public:
        GLuint program;

        // compute shader 专用
        static Shader computeShader(const std::string& path)
        {
            GLuint cs = compile(GL_COMPUTE_SHADER, readFile(path));
            return Shader(link({cs}));
        }

        // vertex+fragment组合
        static Shader graphicsShader(const std::string& vertPath, const std::string& fragPath) 
        {
            GLuint vs = compile(GL_VERTEX_SHADER, readFile(vertPath));
            GLuint fs = compile(GL_FRAGMENT_SHADER, readFile(fragPath));
            return Shader(link({vs, fs}));
        }

        // 激活着色器可执行程序
        void use() const { glUseProgram(program); }

        void setInt(const std::string& name, int v) const 
        {
            glUniform1i(glGetUniformLocation(program, name.c_str()), v);
        }

        void setFloat(const std::string& name, float v) const 
        {
            glUniform1f(glGetUniformLocation(program, name.c_str()), v);
        }

        void setVec3(const std::string& name, float x, float y, float z) const 
        {
            glUniform3f(glGetUniformLocation(program, name.c_str()), x, y, z);
        }

        void setUint(const std::string& name, unsigned int v) const 
        {
        glUniform1ui(glGetUniformLocation(program, name.c_str()), v);
        }

    private:
        Shader(GLuint prog) : program(prog) {}

        //-------------------- 读取、编译、链接三件套--------------------

        static std::string readFile(const std::string& path) 
        {
            std::ifstream file(path);
            if (!file.is_open()) { std::cerr << "Failed to open: " << path << "\n"; return ""; }
            std::stringstream ss; ss << file.rdbuf();
            return ss.str();
        }

        static GLuint compile(GLenum type, const std::string& src) 
        {
            GLuint shader = glCreateShader(type);
            const char* s = src.c_str();
            glShaderSource(shader, 1, &s, nullptr);
            glCompileShader(shader);
            GLint success;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                char log[1024];
                glGetShaderInfoLog(shader, 1024, nullptr, log);
                std::cerr << "Shader compile error:\n" << log << "\n";
            }
            return shader;
        }

        static GLuint link(std::initializer_list<GLuint> shaders) 
        {
            GLuint prog = glCreateProgram();
            for (GLuint s : shaders) glAttachShader(prog, s);
            glLinkProgram(prog);
            GLint success;
            glGetProgramiv(prog, GL_LINK_STATUS, &success);
            if (!success) {
                char log[1024];
                glGetProgramInfoLog(prog, 1024, nullptr, log);
                std::cerr << "Program link error:\n" << log << "\n";
            }
            for (GLuint s : shaders) glDeleteShader(s);
            return prog;
        }
};


#endif