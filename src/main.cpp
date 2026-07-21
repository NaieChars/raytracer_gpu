#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Shader.h"
#include "debug.h"
#include "cpu/scene.h"
#include "cpu/gpu_buffer.h"
#include "create_tex.h"
#include "Config.h"

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

    // ----------------- 创建纹理-------------------------
    GLuint outputTexture, resolvedTexture, brightTexture, finalTexture;

    createFloatTexture(outputTexture, SCR_WIDTH, SCR_HEIGHT);
    createFloatTexture(resolvedTexture, SCR_WIDTH, SCR_HEIGHT);
    createFloatTexture(brightTexture, SCR_WIDTH, SCR_HEIGHT);
    createFloatTexture(finalTexture, SCR_WIDTH, SCR_HEIGHT);

    BloomMipChain bloomMips;
    createBloomMipChain(bloomMips, SCR_WIDTH, SCR_HEIGHT);

    // --------------------------------- 编译着色器 ------------------------------------------
    Shader raytraceProgram  = Shader::computeShader("src/shaders/raytrace.comp");
    Shader quadProgram      = Shader::graphicsShader("src/shaders/quad.vert", "src/shaders/quad.frag");
    Shader resolveProgram   = Shader::computeShader("src/shaders/resolve.comp");
    Shader downsampleProgram = Shader::computeShader("src/shaders/downsample.comp");
    Shader upsampleProgram   = Shader::computeShader("src/shaders/upsample.comp");
    Shader compositeProgram = Shader::computeShader("src/shaders/composite.comp");


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
    createLightSSBO(flattener.flatLightIndices, flattener);

    uint32_t frameCount = 0;
    while (!glfwWindowShouldClose(window)) 
    {   
        // 相机参数
        float aspectRatio = float(SCR_WIDTH) / float(SCR_HEIGHT);
        // 相机的位置与朝向,可以根据场景自由调整
        glm::vec3 lookFrom(0.0f, 0.5f, 1.0f);
        glm::vec3 lookAt(0.0f, 0.0f, -2.2f);            // 对准金属锚点球所在位置
        glm::vec3 vup(0.0f, 1.0f, 0.0f);
        float fovDegrees = 50.0f;                       // 视场角,数值越大看到的范围越广
        float aperture = 0.2f;                         // 光圈大小,数值越大虚化越强
        float focusDist = glm::length(lookFrom - lookAt); // 对焦距离:自动对准lookAt所在平面
        // 相机的三个正交轴,w是"往后看"的方向,u是右方向,v是上方向
        glm::vec3 w = glm::normalize(lookFrom - lookAt);
        glm::vec3 u = glm::normalize(glm::cross(vup, w));
        glm::vec3 v = glm::cross(w, u);
        
        float theta = glm::radians(fovDegrees);
        float halfHeight = tan(theta / 2.0f);
        float halfWidth = aspectRatio * halfHeight;
        
        glm::vec3 origin = lookFrom;
        glm::vec3 lowerLeftCorner = origin - halfWidth * focusDist * u - halfHeight * focusDist * v - focusDist * w;
        glm::vec3 horizontal = 2.0f * halfWidth * focusDist * u;
        glm::vec3 vertical = 2.0f * halfHeight * focusDist * v;
        float lensRadius = aperture / 2.0f;
        
        // ---------------------- 光追累计采样 --------------------------
        raytraceProgram.use();
        glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        raytraceProgram.setVec3("camOrigin", origin.x, origin.y, origin.z);
        raytraceProgram.setVec3("camHorizontal", horizontal.x, horizontal.y, horizontal.z);
        raytraceProgram.setVec3("camVertical", vertical.x, vertical.y, vertical.z);
        raytraceProgram.setVec3("camLowerLeftCorner", lowerLeftCorner.x, lowerLeftCorner.y, lowerLeftCorner.z);
        raytraceProgram.setVec3("camU", u.x, u.y, u.z);
        raytraceProgram.setVec3("camV", v.x, v.y, v.z);
        raytraceProgram.setFloat("lensRadius", lensRadius);

        raytraceProgram.setInt("bvhRootIndex", rootIndex);
        raytraceProgram.setUint("frameCount", (int)frameCount);
        frameCount++;
        raytraceProgram.setInt("lightCount", (int)flattener.flatLightIndices.size());

        // ------------ dispatch ------------
        GLuint groupsX = (SCR_WIDTH + 15) / 16;                 
        GLuint groupsY = (SCR_HEIGHT + 15) / 16;
        glDispatchCompute(groupsX, groupsY, 1);                
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);   

        // ---------- 阶段2: 求平均 + 提取高亮部分 ----------
        resolveProgram.use();
        glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(1, resolvedTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glBindImageTexture(2, brightTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        resolveProgram.setInt("sampleCount", (int)(frameCount + 1));
        resolveProgram.setFloat("bloomThreshold", BLOOMTHRESHOLD);
        glDispatchCompute(groupsX, groupsY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        // ---------- 阶段3: 逐级降采样,构建mip链 ----------
        downsampleProgram.use();
        GLuint srcTex = brightTexture;
        int srcW = SCR_WIDTH, srcH = SCR_HEIGHT;

        for (int level = 1; level <= BLOOM_MIP_LEVELS; level++)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, srcTex);
            downsampleProgram.setInt("srcTex", 0);
            downsampleProgram.setVec2("srcTexelSize", 1.0f / srcW, 1.0f / srcH);
            glBindImageTexture(0, bloomMips.textures[level], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

            GLuint gx = (bloomMips.widths[level] + 15) / 16;
            GLuint gy = (bloomMips.heights[level] + 15) / 16;
            glDispatchCompute(gx, gy, 1);
            // 注意:这里barrier要同时包含TEXTURE_FETCH,因为下一轮要用sampler2D采样这次写入的结果,
            // 光有IMAGE_ACCESS_BARRIER只保证image2D读写顺序,不保证texture()采样能看到最新数据
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

            srcTex = bloomMips.textures[level];
            srcW = bloomMips.widths[level];
            srcH = bloomMips.heights[level];
        }

        // ---------- 阶段4: 逐级升采样叠加,从最小一级往回叠加,最终叠回brightTexture本身 ----------
        upsampleProgram.use();
        for (int level = BLOOM_MIP_LEVELS; level >= 1; level--)
        {
            GLuint dstTex = (level == 1) ? brightTexture : bloomMips.textures[level - 1];
            int dstW = (level == 1) ? SCR_WIDTH  : bloomMips.widths[level - 1];
            int dstH = (level == 1) ? SCR_HEIGHT : bloomMips.heights[level - 1];
            float layerWeight = pow(BLOOM_DECAY_FACTOR, (float)(level - 1)); 
            upsampleProgram.setFloat("decay", layerWeight);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, bloomMips.textures[level]);
            upsampleProgram.setInt("srcTex", 0);
            upsampleProgram.setVec2("srcTexelSize", 1.0f / bloomMips.widths[level], 1.0f / bloomMips.heights[level]);
            upsampleProgram.setFloat("bloomRadius", BLOOMRADIUS); 

            glBindImageTexture(0, dstTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

            GLuint gx = (dstW + 15) / 16;
            GLuint gy = (dstH + 15) / 16;
            glDispatchCompute(gx, gy, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }

        // ---------- 阶段5: 叠加bloom + gamma校正 ----------
        compositeProgram.use();
        glBindImageTexture(0, resolvedTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(1, brightTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(2, finalTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        compositeProgram.setFloat("bloomIntensity", BLOOMINTENSITY); 
        glDispatchCompute(groupsX, groupsY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);



        glClear(GL_COLOR_BUFFER_BIT);
        quadProgram.use();
        glActiveTexture(GL_TEXTURE0);                                           
        glBindTexture(GL_TEXTURE_2D, finalTexture);
        quadProgram.setInt("screenTexture", 0);    
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        glfwSwapBuffers(window);
        glfwPollEvents();

        showFPS();
    }

    glfwTerminate();
    return 0;
}