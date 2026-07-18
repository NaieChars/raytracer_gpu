#ifndef HITTABLE_H
#define HITTABLE_H

#include "aabb.h"

class material;

class hit_record
{
    public:

    point3 point;
    vec3 normal;
    shared_ptr<material> mat;
    double t;
    bool front_face;
    double u;
    double v;

    void set_face_normal(const vec3& ray_origin, const vec3& ray_dir, const vec3& outward_normal)
    {
        front_face = dot(ray_dir, outward_normal) < 0; // 确保法线始终指向光线射来的那一侧
        normal = front_face ? outward_normal : -outward_normal;
    }
};

// 物体的统一接口
class hittable
{
    public:

    virtual ~hittable() = default;

    virtual bool hit(const vec3& ray_origin, const vec3& ray_dir, double min, double max, hit_record& rec) const = 0;  // 纯虚函数，具体实现由子类实现
    virtual bool bounding_box(aabb& output_box) const = 0;
};

#endif