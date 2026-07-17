#version 430 core

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D screenTexture;    // 声明一个 2D 纹理采样器 uniform，它是在应用程序中（CPU 端）传入的。

void main()
{
    fragColor = texture(screenTexture, texCoord);   // 在 screenTexture 上，用纹理坐标 texCoord 进行采样，将得到的颜色值直接赋给输出 fragColor。
}