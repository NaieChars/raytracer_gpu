#ifndef CREATE_TEX_H
#define CREATE_TEX_H

#include <glad/glad.h>

#include <algorithm> 

const int BLOOM_MIP_LEVELS = 5; // 总共生成5级level

void createFloatTexture(GLuint& tex, int width, int height) {
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, width, height);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ---------- 结构体封装 Mip 链 ----------
struct BloomMipChain {
    GLuint textures[BLOOM_MIP_LEVELS + 1]; // 0号不用，1~5有效
    int widths[BLOOM_MIP_LEVELS + 1];
    int heights[BLOOM_MIP_LEVELS + 1];
    int BLOOM_MIP_LEVELS;

    // 构造函数：自动清空指针
    BloomMipChain() {
        for (int i = 0; i <= BLOOM_MIP_LEVELS; i++) {
            textures[i] = 0;
            widths[i] = 0;
            heights[i] = 0;
        }
    }
};

// ---------- 创建 Mip 链的函数 ----------
void createBloomMipChain(BloomMipChain& chain, int baseWidth, int baseHeight) {
    for (int level = 1; level <= BLOOM_MIP_LEVELS; level++) {
        // 1. 计算当前级尺寸
        chain.widths[level]  = std::max(1, baseWidth >> level);
        chain.heights[level] = std::max(1, baseHeight >> level);

        // 2. 生成纹理
        glGenTextures(1, &chain.textures[level]);
        glBindTexture(GL_TEXTURE_2D, chain.textures[level]);

        // 3. 分配显存（单级，不自动生成Mipmap）
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 
                       chain.widths[level], chain.heights[level]);

        // 4. 设置采样参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // 5. 解绑
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

#endif