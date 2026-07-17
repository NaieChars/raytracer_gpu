#version 430 core

// 不需要顶点缓冲，用顶点ID技巧生成一个全屏三角形
out vec2 texCoord;

void main()
{
    vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);  // 生成屏幕坐标，三顶点依次为 (0,0), (2, 0), (0, 2), gl_VertexID 是 GLSL 的内置整型变量，表示当前顶点在绘制命令中的索引。
    // 当你用 glDrawArrays(GL_TRIANGLES, 0, 3) 绘制 3 个顶点时，gl_VertexID 的值依次为 0、1、2。
    texCoord = pos;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);  // 裁剪坐标
}

// 流程：
// 每个顶点独立计算自己的位置，完全并行，结果直接输入到光栅化器。