#ifndef MATERIAL_H
#define MATERIAL_H


#include "hittable.h"
#include "texture.h"


class material
{
    public:

    virtual ~material() = default;

    virtual vec3 emitted(double u, double v, const vec3& p) const
    {
        return vec3(0, 0, 0);
    }
};

class lambertian : public material
{
    public:
    shared_ptr<texture> albedo;
    
    lambertian(const vec3& a) : albedo(make_shared<constant_texture>(a)) {}
    lambertian(shared_ptr<texture> a) : albedo(a) {}

};

class metal : public material
{
public:
    vec3 albedo;
    double fuzz;

    metal(const vec3& a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}
};

class dielectric : public material
{
    public:
    double refraction_index;

    dielectric(double refraction_index) : refraction_index(refraction_index) {}
};


class diffuse_light : public material
{
    public:

    shared_ptr<texture> emit;

    diffuse_light(shared_ptr<texture> a) : emit(a) {}

    virtual vec3 emitted(double u, double v, const vec3& p) const
    {
        return emit->value(u, v, p);
    }

    vec3 get_emit_color()
    {
        auto constColor = std::dynamic_pointer_cast<constant_texture>(emit);
        if (constColor)
            return constColor->color;
        return vec3(0, 1, 1);
    }
};

// 各向同性
class isotropic : public material
{
    public:
        shared_ptr<texture> albedo;
        double sigma_t;    // 消光系数，值越大介质越"浓稠",光线走不了多远就会碰撞
        double scatter_albedo; // 单次散射反照率(0~1),碰撞后有多大概率是"散射"而不是"被吸收"
        double g;              // Henyey-Greenstein各向异性参数,0=各向同性散射,正值偏向前散射,负值偏向后散射

        isotropic(shared_ptr<texture> a, double sigma_t_ = 1.0, double scatter_albedo_ = 0.9, double g_ = 0.0) 
            : albedo(a), sigma_t(sigma_t_), scatter_albedo(scatter_albedo_), g(g_) {}
        
        vec3 get_albedo() const { return albedo->value(0, 0, vec3(0,0,0)); }
        double get_sigma_t() const { return sigma_t; }
        double get_scatter_albedo() const { return scatter_albedo; }
        double get_g() const { return g; }
};


#endif