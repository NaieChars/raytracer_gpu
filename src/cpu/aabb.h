#ifndef AABB_H
#define AABB_H

#include "rtweekend.h"

class aabb
{
    public:

    vec3 _min, _max;

    aabb() {}
    aabb(const vec3& a, const vec3& b) {_min = a; _max = b;}

    vec3 min() const {return _min;}
    vec3 max() const {return _max;}

    // aabb 包围盒求交，现在在GPU里面实现
    bool hit(const vec3& ray_origin, const vec3& ray_dir, double t_enter, double t_exit) const
    {
        for (int i = 0; i < 3; i++)
        {
            auto invD = 1.0f / ray_dir[i];
            auto t0 = (min()[i] - ray_origin[i]) * invD;
            auto t1 = (max()[i] - ray_origin[i]) * invD;

            if (t0 > t1) std::swap(t0, t1);

            t_enter = t0 > t_enter ? t0 : t_enter;
            t_exit = t1 < t_exit ? t1 : t_exit;

            if (t_enter > t_exit) return false;
        }
        return true;
    }

};

// 返回能同时包住 box0 和 box1 的最小包围盒
inline aabb surrounding_box(const aabb& box0, const aabb& box1) 
{
    vec3 small(
        fmin(box0.min().x(), box1.min().x()),
        fmin(box0.min().y(), box1.min().y()),
        fmin(box0.min().z(), box1.min().z())
    );
    vec3 big(
        fmax(box0.max().x(), box1.max().x()),
        fmax(box0.max().y(), box1.max().y()),
        fmax(box0.max().z(), box1.max().z())
    );
    return aabb(small, big);
}

#endif
