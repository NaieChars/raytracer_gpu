#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Shader.h"
#include "camera.h"
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

Camera camera;

void mouseCallback(GLFWwindow* window, double xpos, double ypos);

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

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(window, &camera); // 把camera对象"挂"到窗口上,回调里才能取到
    glfwSetCursorPosCallback(window, mouseCallback);

    /*
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // 让错误发生在调用处，便于定位
    glDebugMessageCallback(debugCallback, nullptr);
    */

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
    hittable_list world = createScene1();

    //----------- 构建BVH ---------------
    auto bvh = make_shared<bvh_node>(world.objects, 0, world.objects.size());
    //----------- flatten --------------
    BVHFlattener flattener;
    int rootIndex = flattener.flatten(bvh);
    std::cout << "flatSpheres: " << flattener.flatSpheres.size()
          << ", flatBoxes: " << flattener.flatBoxes.size()
          << ", flatNodes: " << flattener.flatNodes.size() << "\n";
    //----------- 上传 GPU --------------
    createBVHSSBO(flattener.flatNodes);
    createShereSSBO(flattener.flatSpheres);
    createMaterialSSBO(flattener.flatMaterials);
    createLightSSBO(flattener.flatLightIndices, flattener);
    createBoxSSBO(flattener.flatBoxes);


    camera.setFocusTarget(glm::vec3(-0.925f, 0.45f, -0.675f)); // 两个主体球的中点

    uint32_t frameCount = 0;
    float lastFrameTime = 0.0f;
    bool fKeyWasPressed = false;
    while (!glfwWindowShouldClose(window)) 
    {   
        float currentFrameTime = (float)glfwGetTime();
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        camera.processKeyboard(window, deltaTime);
        // F键切换景深开关,边缘触发:只在"刚按下"那一帧生效,避免按住不放时每帧反复横跳
        bool fKeyIsPressed = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS);
        if (fKeyIsPressed && !fKeyWasPressed) {
            camera.toggleDOF();
        }
        fKeyWasPressed = fKeyIsPressed;

        // ] [ 键调节光圈大小
        if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
            camera.aperture += 0.002f;
            camera.markMoved();
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
            camera.aperture = std::max(0.0f, camera.aperture - 0.002f);
            camera.markMoved();
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        

        // ---------- 相机参数,一次调用拿到GPU需要的全部值 ----------
        glm::vec3 origin, lowerLeftCorner, horizontal, vertical, u, v;
        float lensRadius;
        float aspectRatio = float(SCR_WIDTH) / float(SCR_HEIGHT);
        camera.getGPUParams(aspectRatio, origin, lowerLeftCorner, horizontal, vertical, u, v, lensRadius);

        if (camera.consumeMovedFlag()) {
            frameCount = 0;
        }

        raytraceProgram.use();
        glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        raytraceProgram.setVec3("camOrigin", origin.x, origin.y, origin.z);
        raytraceProgram.setVec3("camHorizontal", horizontal.x, horizontal.y, horizontal.z);
        raytraceProgram.setVec3("camVertical", vertical.x, vertical.y, vertical.z);
        raytraceProgram.setVec3("camLowerLeftCorner", lowerLeftCorner.x, lowerLeftCorner.y, lowerLeftCorner.z);
        raytraceProgram.setVec3("camU", u.x, u.y, u.z);
        raytraceProgram.setVec3("camV", v.x, v.y, v.z);
        raytraceProgram.setFloat("lensRadius", lensRadius);
        raytraceProgram.setFloat("focusDist", camera.focusDist);
        raytraceProgram.setFloat("focusRangeTolerance", 0.2f);

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
        //
        compositeProgram.setFloat("exposure", EXPOSURE);      // 先从1.0开始,觉得整体偏暗/偏亮再调
        compositeProgram.setFloat("contrast", CONTRAST);       // 轻微增强对比度,让画面更有"厚重感"
        compositeProgram.setFloat("saturation", SATURATION);    // 轻微提高饱和度,配合"星海"场景的梦幻感
        compositeProgram.setVec3("colorBalance", COLORBALANCE_R, COLORBALANCE_G, COLORBALANCE_B); // 轻微偏冷调,蓝色通道略增强,可选的电影感做法

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

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (cam) cam->processMouseMovement((float)xpos, (float)ypos);
}

