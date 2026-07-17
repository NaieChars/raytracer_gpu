#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;

// ---- 工具函数：读取shader文件文本 ----
std::string readFile(const char* path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open shader file: "  << path << "\n";
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();  // file.rdbuf() 返回文件流的底层缓冲区指针，ss << 把整个文件缓冲区的剩余内容一次性导入ss字符串流中
    return ss.str();
}

// ---- 工具函数：编译单个shader，返回着色器ID，带错误检查 ----
GLuint compileShader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);   // 创建一个空的着色器对象，并返回它的句柄
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr); // 给着色器对象内部拷贝一份源码副本
    glCompileShader(shader);    // 编译

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "Shader compile error:\n" << infoLog << "\n";
    }
    return shader;
}

// ---- 工具函数：链接program，返回GPU可执行程序，带错误检查 ----
GLuint linkProgram(std::initializer_list<GLuint> shaders)
{
    GLuint program = glCreateProgram();
    for (GLuint s : shaders) glAttachShader(program, s);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) 
    {
        char infoLog[1024];
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        std::cerr << "Program link error:\n" << infoLog << "\n";
    }
    for (GLuint s : shaders) glDeleteShader(s);  // 链接完清空shaders
    return program;
}

int main()
{
    if (!glfwInit()) 
    {
        std::cerr << "GLFW init failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Compute Shader Raytracer", nullptr, nullptr);
    if (!window) 
    {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
    {
        std::cerr << "GLAD init failed\n";
        return -1;
    }


    // ----------------- 没有绑定VAO的 glDrawArrays不会执行，所以我们仍要绑定一个 VAO，哪怕根本不用顶点属性
    GLuint dummyVAO;
    glGenVertexArrays(1, &dummyVAO);
    glBindVertexArray(dummyVAO);

    // ----------------- 创建输出图像（compute shader写入的目标）-------------------------
    GLuint outputTexture;
    glGenTextures(1, &outputTexture);                                      // 生成一个纹理对象ID，并保存到outputTexture
    glBindTexture(GL_TEXTURE_2D, outputTexture);                           // 绑定对象到 GL_TEXTURE_2D目标，因为 OpenGL 是状态机，所以后续对 GL_TEXTURE_2D的操作都会作用到这个纹理上
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT);   // glTexStorage2D 而不是 glTexImage2D —— 这是创建"不可变存储"的纹理，compute shader写入这种纹理效率更好，也是目前推荐用法。
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);                                       // 解绑 GL_TEXTURE_2D

    GLuint computeShader = compileShader(GL_COMPUTE_SHADER, readFile("src/shaders/hello.comp"));    // 相对路径，相对于运行exe时的工作目录，
    GLuint computeProgram = linkProgram({computeShader});

    GLuint vertShader = compileShader(GL_VERTEX_SHADER, readFile("src/shaders/quad.vert"));
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, readFile("src/shaders/quad.frag"));
    GLuint quadProgram = linkProgram({vertShader, fragShader});


    while (!glfwWindowShouldClose(window)) 
    {
        // 派发compute shader，把结果写进 outputTexture
        glUseProgram(computeProgram);
        glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);   // glBindImageTexture(0... 这里的 0 要和.comp里 binding = 0 对应上, 这是CPU和GPU之间的接口约定

        GLuint groupsX = (SCR_WIDTH + 15) / 16;                 // 整数向上取整
        GLuint groupsY = (SCR_HEIGHT + 15) / 16;
        glDispatchCompute(groupsX, groupsY, 1);                 // 派发 Compute Shader 工作：告诉 GPU 启动 groupsX × groupsY × 1 个工作组。(z=1)
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);    // 确保屏障之前的所有纹理图像写入（imageStore）对屏障之后的同一图像单元访问可见。

        // 把结果画到屏幕
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(quadProgram);
        glActiveTexture(GL_TEXTURE0);                                           // 激活 0 号纹理
        glBindTexture(GL_TEXTURE_2D, outputTexture);
        glUniform1i(glGetUniformLocation(quadProgram, "screenTexture"), 0);     // 设置着色器程序 quadProgram 中的 uniform 采样器 screenTexture 的值。glGetUniformLocation 查询 shader 中 "screenTexture" 的位置。值 0 表示它从纹理单元 0 采样
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;

}