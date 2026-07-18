#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <iostream>
#include <vector>

class vec3
{
public:

    double e[3];

    vec3() : e{0, 0, 0} {}
    vec3(double a, double b, double c) : e{a, b, c} {}

    double x() const {return e[0];}
    double y() const {return e[1];}
    double z() const {return e[2];}

    vec3 operator-() const {return vec3(-e[0], -e[1], -e[2]);}
    double operator[](int i) const {return e[i];}
    double& operator[](int i) {return e[i];}

    vec3& operator+=(const vec3& v) 
    {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    vec3& operator*=(double t) 
    {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    vec3& operator/=(double t) 
    {
        return *this *= 1/t;
    }

    double length() const
    {
        return std::sqrt(length_squared());
    }

    double length_squared() const 
    {
        return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }

    // 向量各个分量都几乎为0时，返回 true
    bool near_zero() const 
    {
        auto s = 1e-8;
        return (std::abs(e[0]) < s) && (std::abs(e[1]) < s) && (std::abs(e[2]) < s);
    }

    static vec3 random()
    {
        return vec3(random_double(), random_double(), random_double());
    }

    static vec3 random(double min, double max) 
    {
        return vec3(random_double(min,max), random_double(min,max), random_double(min,max));
    }


    void write_color(std::vector<unsigned char>& buffer, int samples_per_pixel) 
    {
        // 类 MSAA 抗锯齿
        // gamma 校正，此处使用的是 gamma 2 空间
        auto scale = 1.0f / samples_per_pixel;
        auto r = sqrt(scale * e[0]);
        auto g = sqrt(scale * e[1]);
        auto b = sqrt(scale * e[2]);

        buffer.push_back(static_cast<unsigned char>(255.999 * clamp(r, 0.0, 0.999)));
        buffer.push_back(static_cast<unsigned char>(255.999 * clamp(g, 0.0, 0.999)));
        buffer.push_back(static_cast<unsigned char>(255.999 * clamp(b, 0.0, 0.999)));
    }

};

using point3 = vec3;

// 向量工具函数

inline vec3 operator+(const vec3& u, const vec3& v)
{
    return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

inline vec3 operator-(const vec3& u, const vec3& v) 
{
    return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

// 元素相乘，不是点乘，返回的是 vec3
inline vec3 operator*(const vec3& u, const vec3& v) 
{
    return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

inline vec3 operator*(double t, const vec3& v) 
{
    return vec3(t*v.e[0], t*v.e[1], t*v.e[2]);
}

inline vec3 operator*(const vec3& v, double t) 
{
    return t * v;
}

inline vec3 operator/(const vec3& v, double t) 
{
    return (1/t) * v;
}

inline std::ostream& operator<<(std::ostream& out, const vec3& v) 
{
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

inline double dot(const vec3& u, const vec3& v) 
{
    return u.e[0] * v.e[0]
         + u.e[1] * v.e[1]
         + u.e[2] * v.e[2];
}

inline vec3 cross(const vec3& u, const vec3& v) 
{
    return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
                u.e[2] * v.e[0] - u.e[0] * v.e[2],
                u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

inline vec3 unit_vector(const vec3& v) 
{
    return v / v.length();
}

/**
 * @brief 在单位圆盘上生成一个均匀分布的随机二维点，从一个单位圆盘射出光线
 */
inline vec3 random_in_unit_disk()
{
    while (true)
    {
        auto p = vec3(random_double(-1, 1), random_double(-1, 1), 0);
        if (p.length_squared() < 1) return p;
    }
}

/**
 * @brief 生成单位球面上均匀分布的随机方向
 */ 
inline vec3 random_unit_vector()
{
    while (true)
    {
        auto p = vec3::random(-1, 1);
        auto lensq = p.length_squared();
        if (1e-160 < lensq && lensq <= 1.0f)
            return p / sqrt(lensq); // 归一化到单位球面
    }
}

/**
 * @brief 以 normal 为法线方向的半球面上均匀地随机选取一个方向。
 */
inline vec3 random_on_hemisphere(const vec3& normal)
{
    vec3 on_unit_sphere = random_unit_vector();
    if (dot(on_unit_sphere, normal) > 0)
        return on_unit_sphere;
    else
        return -on_unit_sphere; // 将向量翻转至 normal 所在的半球
}

/** 
 * @param v 入射光线，指向表面
 * @param n 单位法向量 
 * @return 返回反射光线，指向外
 */
inline vec3 reflect(const vec3& v, const vec3& n)
{
    return v - 2 * dot(v, n) * n;
}

/**
 * @param uv 入射光线，指向表面
 * @param etai_over_etat 入射介质折射率与透射介质折射率比值
 * @return 折射光线方向，指向离开表面方向
 */
inline vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat)
{
    auto cos_theta = std::fmin(dot(-uv, n), 1.0f);
    vec3 r_out_perp = etai_over_etat * (uv + cos_theta*n); // 折射光线切向分量
    vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())) * n; // 折射光线法向分量
    return r_out_perp + r_out_parallel;
}


#endif