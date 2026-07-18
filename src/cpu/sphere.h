#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "glm/glm.hpp"

class sphere : public hittable
{
    public:

    sphere(const point3& static_center, double radius, shared_ptr<material> mat) : center(static_center), radius(radius), mat(mat) {}
    //sphere(const point3& center1, const point3& center2, double radius, shared_ptr<material> mat) : center(center1, center2 - center1), radius(radius), mat(mat) {}
    sphere(const vec3& c, float r) : center(c), radius(r) {}

    bool hit(const vec3& ray_origin, const vec3& ray_dir, double min, double max, hit_record& rec) const override
    {
        /*
        point3 current_center = center.at(r.time());    // current_center 就是光线击中球的一瞬间，球心所在坐标
        vec3 oc = r.origin() - current_center;
        auto a = r.direction().length_squared();
        auto b = 2.0f * dot(oc, r.direction());
        auto c = oc.length_squared() - radius * radius;

        auto disc = b * b - 4.0f * a * c;

        if (disc < 0) return false;
        
        auto sqrt_disc = std::sqrt(disc);

        // 找到合法且最近的根
        auto root = (-b - sqrt_disc) * 0.5f / a;
        if (!(root > min && root < max))
        {
            root = (-b + sqrt_disc) * 0.5f / a;
            if (!(root > min && root < max))
                return false;
        }

        // 保存最近的交点信息
        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - current_center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.mat = mat;
        */
        return true;
    }

    bool bounding_box(aabb& output_box) const override
    {
        vec3 min = center - vec3(radius, radius, radius);
        vec3 max = center + vec3(radius, radius, radius);
        output_box = aabb(min, max);
        return true;
    }

    public:
    vec3 center;
    double radius;
    shared_ptr<material> mat;

    static void get_sphere_uv(const vec3& point, double& u, double& v)
    {
        // 图形学与数学的坐标系转换
        auto phi = atan2(point.z(), point.x()); // 返回范围 -pi 到 pi
        auto theta = asin(point.y());       // 返回范围 -pi/2  到 pi/2
        u = 1 - (phi + pi) / 2 * pi;    // 用 1 来减是让u从1减小到0，逆时针环绕
        v = (theta + pi/2) / pi;
    }
};


#endif