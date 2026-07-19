#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Shader.h"
#include "debug.h"
#include "cpu/scene.h"
#include "cpu/gpu_buffer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;

#include <iostream>
#include <vector>


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
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE); // 创建窗口时请求调用上下文

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

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // 让错误发生在调用处，便于定位
    glDebugMessageCallback(debugCallback, nullptr);

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

    Shader raytraceProgram = Shader::computeShader("src/shaders/raytrace.comp");
    Shader quadProgram = Shader::graphicsShader("src/shaders/quad.vert", "src/shaders/quad.frag");


    //----------- 场景构建---------------
    hittable_list world = createScene();

    //----------- 构建BVH ---------------
    auto bvh = make_shared<bvh_node>(world.objects, 0, world.objects.size());
    //----------- flatten --------------
    BVHFlattener flattener;
    int rootIndex = flattener.flatten(bvh);
    //----------- 上传 GPU --------------
    createBVHSSBO(flattener.flatNodes);
    createShereSSBO(flattener.flatSpheres);
    createMaterialSSBO(flattener.flatMaterials);


    uint32_t frameCount = 0;
    while (!glfwWindowShouldClose(window)) 
    {
        raytraceProgram.use();
        glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        // 相机参数
        float aspectRatio = float(SCR_WIDTH) / float(SCR_HEIGHT);
        float viewportHeight = 2.0f;
        float viewportWidth = aspectRatio * viewportHeight;
        float focalLength = 1.0f;

        raytraceProgram.setInt("bvhRootIndex", rootIndex);
        raytraceProgram.setVec3("camOrigin", 0, 0, 0);
        raytraceProgram.setVec3("camHorizontal", viewportWidth, 0, 0);
        raytraceProgram.setVec3("camVertical", 0, viewportHeight, 0);
        raytraceProgram.setVec3("camLowerLeftCorner", -viewportWidth/2, -viewportHeight/2, -focalLength);

        raytraceProgram.setUint("frameCount", (int)frameCount);
        frameCount++;

        // ------------ dispatch ------------------
        GLuint groupsX = (SCR_WIDTH + 15) / 16;                 
        GLuint groupsY = (SCR_HEIGHT + 15) / 16;
        glDispatchCompute(groupsX, groupsY, 1);                
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);   

        glClear(GL_COLOR_BUFFER_BIT);
        quadProgram.use();
        glActiveTexture(GL_TEXTURE0);                                           
        glBindTexture(GL_TEXTURE_2D, outputTexture);
        quadProgram.setInt("screenTexture", 0);    
        quadProgram.setInt("sampleCount", (int)(frameCount + 1)); // 新增:告诉fragment shader目前累积了几帧
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        glfwSwapBuffers(window);
        glfwPollEvents();

        showFPS();
    }

    glfwTerminate();
    return 0;
}