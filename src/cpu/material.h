#ifndef MATERIAL_H
#define MATERIAL_H


#include "hittable.h"
#include "texture.h"


class material
{
    public:

    virtual ~material() = default;

    // 各个派生类的scatter均在GPU里计算
    virtual bool scatter(const vec3& rayOrigin, const vec3& in_rayDir, const hit_record& rec, vec3& attenuation, const vec3& rayPoint, const vec3& out_rayDir) const
    {
        return false;
    }

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
    
    bool scatter(const vec3& rayOrigin, const vec3& in_rayDir, const hit_record& rec, vec3& attenuation, const vec3& rayPoint, const vec3& out_rayDir) const override
    {
        /*
        vec3 scatter_direction = rec.normal + random_unit_vector();
        
        // 有极小概率随机向量为法线负方向
        if (scatter_direction.near_zero())
        scatter_direction = rec.normal;
        
        scattered = ray(rec.p, scatter_direction, r_in.time()); // 把入射光线的时间传递给散射光线，保证入射与散射光线时间戳相同
        attenuation = albedo->value(rec.u, rec.v, rec.p);
        */
        return true;
    }

};

class metal : public material
{
public:
    vec3 albedo;
    double fuzz;

    metal(const vec3& a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}

    virtual bool scatter(const vec3& rayOrigin, const vec3& in_rayDir, const hit_record& rec, vec3& attenuation, const vec3& rayPoint, const vec3& out_rayDir) const override
    {
        /*
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        reflected = unit_vector(reflected);
        scattered = ray(rec.p, reflected + fuzz * random_unit_vector());
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
        */
       return true;
    }
};

class dielectric : public material
{
    public:
    double refraction_index;

    dielectric(double refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const vec3& rayOrigin, const vec3& in_rayDir, const hit_record& rec, vec3& attenuation, const vec3& rayPoint, const vec3& out_rayDir) const override
    {
        /*
        attenuation = vec3(1.0, 1.0, 1.0);
        double ri = rec.front_face ? 1.0f / refraction_index : refraction_index;

        vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0f);
        double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

        // 全反射
        bool cannot_refract = ri * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, ri) > random_double()) // 重要性采样的体现：反射率越高，光线反射概率就越高*
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, ri);

        scattered = ray(rec.p, direction);
        */
        return true;
    }


    // Schilick 近似式，计算菲涅尔反射率。在给定入射角下，有多少反射，有多少折射
    static double reflectance(double cos, double refraction_index)
    { 
        // 垂直入射反射率
        auto r0 = (1 - refraction_index) / (1 + refraction_index);
        r0 = r0*r0;

        // 近似式
        return r0 + (1 - r0) * std::pow((1 - cos), 5);
    }
};

/*
class diffuse_light : public material
{
    public:

    shared_ptr<texture> emit;

    diffuse_light(shared_ptr<texture> a) : emit(a) {}

    bool scatter(const vec3& rayOrigin, const vec3& in_rayDir, const hit_record& rec, vec3& attenuation, const vec3& rayPoint, const vec3& out_rayDir) const override
    {
        return false;
    }

    virtual vec3 emitted(double u, double v, const vec3& p) const
    {
        return emit->value(u, v, p);
    }
};

// 各向同性
class isotropic : public material
{
    public:
        shared_ptr<texture> albedo;

        isotropic(shared_ptr<texture> a) : albedo(a) {}

        bool scatter(const vec3& rayOrigin, const vec3& in_rayDir, const hit_record& rec, vec3& attenuation, const vec3& rayPoint, const vec3& out_rayDir) const override
        {
            scattered = ray(rec.p, random_unit_vector(), r_in.time());
            attenuation = albedo->value(rec.u, rec.v, rec.p);
            return true;
        }
};
*/

#endif