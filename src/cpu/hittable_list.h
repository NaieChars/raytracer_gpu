#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"

#include <vector>

class hittable_list : public hittable
{
    public:

    std::vector<shared_ptr<hittable>> objects;

    hittable_list() {}
    hittable_list(shared_ptr<hittable> object) { add(object);}

    void clear() {objects.clear();}

    void add(shared_ptr<hittable> object)
    {
        objects.push_back(object);
    }

    // 核心函数，求在所有物体中离光线最远的点，是多态的核心————现在已经在 GPU 实现
    bool hit(const vec3& ray_origin, const vec3& ray_dir, double min, double max, hit_record& rec) const override
    {
        hit_record temp_rec;
        bool hit_anything = false;
        auto closest_so_far = max;

        for (const auto& object : objects)
        {
            if (object->hit(ray_origin, ray_dir, min, closest_so_far, temp_rec))
            {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        return hit_anything;
    }

    // 求容器内全部元素整体包围盒
    bool bounding_box(aabb& output_box) const override
    {
        if (objects.empty()) return false;

        aabb temp_box;
        bool first_box = true;

        for (const auto& object : objects)
        {
            if (!object->bounding_box(temp_box)) return false;
            output_box = first_box ? temp_box : surrounding_box(output_box,temp_box);
            first_box = false;
        }

        return true;
    }
};


/*
class xy_rect : public hittable
{
    public:
    shared_ptr<material> mp;
    double x0, x1, y0, y1, k;

    xy_rect() {}
    xy_rect(double _x0, double _x1, double _y0, double _y1, double _k, shared_ptr<material> mat)
    : x0(_x0), x1(_x1), y0(_y0), y1(_y1), k(_k), mp(mat) {}

    // 判断光线是否击中矩形光源
    bool hit(const ray& r, double t0, double t1, hit_record& rec) const override
    {
        auto t = (k-r.origin().z()) / r.direction().z();
        if (t < t0 || t > t1)
            return false;
        auto x = r.origin().x() + t*r.direction().x();
        auto y = r.origin().y() + t*r.direction().y();
        if (x < x0 || x > x1 || y < y0 || y > y1)
            return false;

        rec.u = (x-x0)/(x1-x0);
        rec.v = (y-y0)/(y1-y0);
        rec.t = t;
        
        vec3 outward_normal = vec3(0, 0, 1);
        rec.set_face_normal(r, outward_normal);
        rec.mat = mp;
        rec.p = r.at(t);
        return true;
    }

    // 为矩形光源 aabb 增添一定厚度
    bool bounding_box(double t0, double t1, aabb& output_box) const override
    {
        output_box = aabb(vec3(x0,y0, k-0.0001), vec3(x1, y1, k+0.0001));
        return true;
    }
};

class xz_rect: public hittable 
{
        public:
            xz_rect() {}

            xz_rect(double _x0, double _x1, double _z0, double _z1, double _k, shared_ptr<material> mat)
                : x0(_x0), x1(_x1), z0(_z0), z1(_z1), k(_k), mp(mat) {};

            bool hit(const ray& r, double t0, double t1, hit_record& rec) const override
            {
                auto t = (k-r.origin().y()) / r.direction().y();
                if (t < t0 || t > t1)
                    return false;
                auto x = r.origin().x() + t*r.direction().x();
                auto z = r.origin().z() + t*r.direction().z();
                if (x < x0 || x > x1 || z < z0 || z > z1)
                    return false;
                rec.u = (x-x0)/(x1-x0);
                rec.v = (z-z0)/(z1-z0);
                rec.t = t;
                vec3 outward_normal = vec3(0, 1, 0);
                rec.set_face_normal(r, outward_normal);
                rec.mat = mp;
                rec.p = r.at(t);
                return true;
            }

            virtual bool bounding_box(double t0, double t1, aabb& output_box) const 
            {
                output_box =  aabb(vec3(x0,k-0.0001,z0), vec3(x1, k+0.0001, z1));
                return true;
            }

        public:
            shared_ptr<material> mp;
            double x0, x1, z0, z1, k;
};

class yz_rect: public hittable 
{
    public:
        yz_rect() {}

        yz_rect(double _y0, double _y1, double _z0, double _z1, double _k, shared_ptr<material> mat)
            : y0(_y0), y1(_y1), z0(_z0), z1(_z1), k(_k), mp(mat) {};

        bool hit(const ray& r, double t0, double t1, hit_record& rec) const override
        {
            auto t = (k-r.origin().x()) / r.direction().x();
            if (t < t0 || t > t1)
                return false;
            auto y = r.origin().y() + t*r.direction().y();
            auto z = r.origin().z() + t*r.direction().z();
            if (y < y0 || y > y1 || z < z0 || z > z1)
                return false;
            rec.u = (y-y0)/(y1-y0);
            rec.v = (z-z0)/(z1-z0);
            rec.t = t;
            vec3 outward_normal = vec3(1, 0, 0);
            rec.set_face_normal(r, outward_normal);
            rec.mat = mp;
            rec.p = r.at(t);
            return true;
        }

        virtual bool bounding_box(double t0, double t1, aabb& output_box) const 
        {
            output_box =  aabb(vec3(k-0.0001, y0, z0), vec3(k+0.0001, y1, z1));
            return true;
        }

    public:
        shared_ptr<material> mp;
        double y0, y1, z0, z1, k;
};

// 翻转矩形
class flip_face : public hittable 
{
    public:
        flip_face(shared_ptr<hittable> p) : ptr(p) {}

        bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override
        {
            if (!ptr->hit(r, t_min, t_max, rec))
                return false;

            rec.front_face = !rec.front_face;
            return true;
        }

        bool bounding_box(double t0, double t1, aabb& output_box) const override
        {
            return ptr->bounding_box(t0, t1, output_box);
        }

    public:
        shared_ptr<hittable> ptr;
};

// 立方体实例
class box : public hittable
{
    public:
        vec3 box_min;
        vec3 box_max;
        hittable_list sides;

        box() {}
        box(const vec3& p0, const vec3& p1, shared_ptr<material> ptr) : box_min(p0), box_max(p1)
        {
            sides.add(make_shared<xy_rect>(p0.x(), p1.x(), p0.y(), p1.y(), p1.z(), ptr));
            sides.add(make_shared<flip_face>(make_shared<xy_rect>(p0.x(), p1.x(), p0.y(), p1.y(), p0.z(), ptr)));

            sides.add(make_shared<xz_rect>(p0.x(), p1.x(), p0.z(), p1.z(), p1.y(), ptr));
            sides.add(make_shared<flip_face>(make_shared<xz_rect>(p0.x(), p1.x(), p0.z(), p1.z(), p0.y(), ptr)));

            sides.add(make_shared<yz_rect>(p0.y(), p1.y(), p0.z(), p1.z(), p1.x(), ptr));
            sides.add(make_shared<flip_face>(make_shared<yz_rect>(p0.y(), p1.y(), p0.z(), p1.z(), p0.x(), ptr)));
        }

        bool hit(const ray& r, double t0, double t1, hit_record& rec) const override
        {
            return sides.hit(r, t0, t1, rec);
        }

        bool bounding_box(double t0, double t1, aabb& output_box) const override
        {
            output_box = aabb(box_min, box_max);
            return true;
        }
};

// 平移变换：在光追里我们不会对物体进行实际的移动，而是通过移动光线的源点求出交点信息，然后再平移回光线。
// 例如，想让物体向左平移2，那就相当于是把光线源点向右平移2，求出hit_record后再-2返回到正确位置，这就相当于让物体平移了
class translate : public hittable
{
    public:
        translate(shared_ptr<hittable> p, const vec3& displacement) : ptr(p), offset(displacement) {}

        bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override
        {
            ray moved_r(r.origin() - offset, r.direction(), r.time());
            if (!ptr->hit(moved_r, t_min, t_max, rec))
                return false;

            rec.p += offset;
            rec.set_face_normal(moved_r, rec.normal);

            return true;
        }
        
        bool bounding_box(double t0, double t1, aabb& output_box) const override
        {
            if (!ptr->bounding_box(t0, t1, output_box))
                return false;

            output_box = aabb(
            output_box.min() + offset,
            output_box.max() + offset);

            return true;
        }

        shared_ptr<hittable> ptr;   // 被平移的物体
        vec3 offset;                // 平移的方向
};

// 绕y轴旋转
class rotate_y : public hittable 
{
    public:
        rotate_y(shared_ptr<hittable> p, double angle) : ptr(p)
        {
            auto radians = degrees_to_radians(angle);
            sin_theta = sin(radians);
            cos_theta = cos(radians);
            hasbox = ptr->bounding_box(0, 1, bbox);

            vec3 min( infinity,  infinity,  infinity);
            vec3 max(-infinity, -infinity, -infinity);

            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    for (int k = 0; k < 2; k++) {
                        auto x = i*bbox.max().x() + (1-i)*bbox.min().x();
                        auto y = j*bbox.max().y() + (1-j)*bbox.min().y();
                        auto z = k*bbox.max().z() + (1-k)*bbox.min().z();

                        auto newx =  cos_theta*x + sin_theta*z;
                        auto newz = -sin_theta*x + cos_theta*z;

                        vec3 tester(newx, y, newz);

                        for (int c = 0; c < 3; c++) {
                            min[c] = fmin(min[c], tester[c]);
                            max[c] = fmax(max[c], tester[c]);
                        }
                    }
                }
            }
            bbox = aabb(min, max);
        }

        bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override
        {
            vec3 origin = r.origin();
            vec3 direction = r.direction();

            origin[0] = cos_theta*r.origin()[0] - sin_theta*r.origin()[2];
            origin[2] = sin_theta*r.origin()[0] + cos_theta*r.origin()[2];

            direction[0] = cos_theta*r.direction()[0] - sin_theta*r.direction()[2];
            direction[2] = sin_theta*r.direction()[0] + cos_theta*r.direction()[2];

            ray rotated_r(origin, direction, r.time());

            if (!ptr->hit(rotated_r, t_min, t_max, rec))
                return false;

            vec3 p = rec.p;
            vec3 normal = rec.normal;

            p[0] =  cos_theta*rec.p[0] + sin_theta*rec.p[2];
            p[2] = -sin_theta*rec.p[0] + cos_theta*rec.p[2];

            normal[0] =  cos_theta*rec.normal[0] + sin_theta*rec.normal[2];
            normal[2] = -sin_theta*rec.normal[0] + cos_theta*rec.normal[2];

            rec.p = p;
            rec.set_face_normal(rotated_r, normal);

            return true;
        }

        bool bounding_box(double t0, double t1, aabb& output_box) const override
        {
            output_box = bbox;
            return hasbox;
        }

    public:
        shared_ptr<hittable> ptr;
        double sin_theta;
        double cos_theta;
        bool hasbox;
        aabb bbox;
};

// 体积体
class constant_medium : public hittable
{
    public:
        shared_ptr<hittable> boundary;
        shared_ptr<material> phase_function;
        double neg_inv_density;

        constant_medium(shared_ptr<hittable> b, double d, shared_ptr<texture> a)
        : boundary(b), neg_inv_density(-1/d)
        {
            phase_function = make_shared<isotropic>(a);
        }

        bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override
        {
            // 调试技巧，偶尔输出数据
            const bool enableDebug = false;
            const bool debugging = enableDebug && random_double() < 0.00001;

            hit_record rec1, rec2;

            if (!boundary->hit(r, -infinity, infinity, rec1))   return false;
            if (!boundary->hit(r, rec1.t+0.0001, infinity, rec2))   return false;
            if (debugging) std::cerr << "\nt0=" << rec1.t << ", t1=" << rec2.t << '\n';

            if (rec1.t < t_min) rec1.t = t_min;
            if (rec2.t > t_max) rec2.t = t_max;

            if (rec1.t >= rec2.t) return false;
            if (rec1.t < 0) rec1.t = 0;

            // 计算光线在截止内穿行的实际距离
            const auto ray_length = r.direction().length();
            const auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;

            // 比尔-朗伯定律，光线在参与介质中传播距离的随机采样
            const auto hit_distance = neg_inv_density * log(random_double());
            if (hit_distance > distance_inside_boundary) return false;

            rec.t = rec1.t + hit_distance / ray_length;
            rec.p = r.at(rec.t);

            if (debugging) 
            {
                std::cerr << "hit_distance = " <<  hit_distance << '\n'
                        << "rec.t = " <<  rec.t << '\n'
                        << "rec.p = " <<  rec.p << '\n';
            }

            rec.normal = vec3(1,0,0);  // arbitrary
            rec.front_face = true;     // also arbitrary
            rec.mat = phase_function;

            return true;
        }

        bool bounding_box(double t0, double t1, aabb& output_box) const override
        {
            return boundary->bounding_box(t0, t1, output_box);
        }
};
*/

#endif