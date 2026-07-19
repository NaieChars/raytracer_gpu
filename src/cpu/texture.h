#ifndef TEXTURE_H
#define TEXTURE_H

#include "rtweekend.h"

class texture
{
    public:
    virtual vec3 value(double u, double v, const vec3& p) const = 0;
};

class constant_texture : public texture
{
    public:

    constant_texture() {}
    constant_texture(vec3 c) : color(c) {}

    vec3 value(double u, double v, const vec3& p) const override
    {
        return color;
    }

    public:
    vec3 color;
};


class image_texture : public texture
{
    public:

    unsigned char *data;   // stb_image 按数组方式存储的图像信息，每个像素大小3个字节，行优先存储
    int nx, ny;     // 图像宽度（像素列数），图像高度（像素行数）

    image_texture() {}
    image_texture(unsigned char* pixels, int A, int B) : data(pixels), nx(A), ny(B) {}

    ~image_texture()
    {
        delete data;
    }

    // u, v 是归一化纹理坐标，范围 [0，1]
    vec3 value(double u, double v, const vec3& p) const override
    {
        if (data == nullptr) return vec3(0, 1, 1);  // 调试标记（青色）

        auto i = static_cast<int>(u*nx);    // 映射到实际的像素索引范围 [0, nx]，取整得索引i
        auto j = static_cast<int>(1 - v)*ny;    
        // 这里用 1 - v 是倒转v，因为纹理坐标v一般是从下到上，而图像像素是从上到下排列

        if (i < 0) i = 0;
        if (j < 0) j = 0;
        if (i > nx - 1) i = nx - 1;
        if (j > ny - 1) j = ny - 1;

        // 将像素数组里提取指定像素颜色，并归一化到 [0, 1]
        auto r = static_cast<int>(data[3*i + 3*nx*j + 0]) / 255.0;
        auto g = static_cast<int>(data[3*i + 3*nx*j + 1]) / 255.0;
        auto b = static_cast<int>(data[3*i + 3*nx*j + 2]) / 255.0;

        return vec3(r, g, b);
    }
};


class checker_texture : public texture
{
    public:
    checker_texture(shared_ptr<texture> t0, shared_ptr<texture> t1) : even(t0), odd(t1) {}

    vec3 value(double u, double v, const vec3& p) const override
    {
        auto sines = sin(10*p.x())*sin(10*p.y())*sin(10*p.z());
        if (sines < 0) return odd->value(u, v, p);

        return even->value(u, v, p);
    }

    public:
    shared_ptr<texture> odd;
    shared_ptr<texture> even;
};

/*
class noise_texture : public texture
{
    public:

    noise_texture() {}
    noise_texture(double sc) : scale(sc) {}

    vec3 value(double u, double v, const vec3& p) const override
    {
        // 柏林插值的输出结果有可能是负数，需要映射至 0 到 1
        return vec3(1,1,1) * 0.5 * (1.0 + noise.noise(scale * p));
    }

    public: 

    perlin noise;
    double scale;
};
*/

#endif