#version 430 core

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D screenTexture;
uniform int sampleCount;    // 目前累计多少帧

void main()
{
    vec3 sum = texture(screenTexture, texCoord).rgb;
    vec3 color = sum / float(sampleCount);  // 取平均

    color = sqrt(max(color, vec3(0.0)));    // gamma 校正，2.0空间

    fragColor = vec4(color, 1.0);
}