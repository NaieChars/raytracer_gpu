#ifndef BVH_H
#define BVH_H


#include "hittable_list.h"
#include "sphere.h"

#include <algorithm>
#include <glm/glm.hpp>

class bvh_node : public hittable
{
    public:
    bvh_node() {}

    bvh_node(std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end)
    {
        int axis = random_int(0, 2);

        auto comparator = (axis == 0) ? box_x_compare
                    : (axis == 1) ? box_y_compare
                                  : box_z_compare;


        size_t object_span = end - start;   // start 和 end 是区间下标 [start, end)，object_span 即为这个节点应该处理的物体个数。

        if (object_span == 1)
        {
            left = right = objects[start];
        }
        else if (object_span == 2)
        {
            left = objects[start];
            right = objects[start + 1];
        }
        else
        {
            std::sort(objects.begin() + start, objects.begin() + end, comparator);

            auto mid = start + object_span / 2;
            // 递归构建树
            left = make_shared<bvh_node>(objects, start, mid);
            right = make_shared<bvh_node>(objects, mid, end);
        }

        aabb box_left, box_right;

        if (  !left->bounding_box (box_left) || !right->bounding_box(box_right))
            std::cerr << "No bounding box in bvh_node constructor.\n";

        bbox = surrounding_box(box_left, box_right);
    }

    // 遍历左右子树求交取最近的交点————现在在 GPU 实现
    bool hit(const vec3& ray_origin, const vec3& ray_dir, double t_min, double t_max, hit_record& rec) const override 
    {
        /*
        if (!bbox.hit(ray_origin, ray_dir, t_min, t_max))
            return false;
        bool hit_left = left->hit(r, t_min, t_max, rec);
        bool hit_right = right->hit(r, t_min, hit_left ? rec.t : t_max, rec);
        return hit_left || hit_right;
        */

        return false;
    }

    bool bounding_box(aabb& output_box) const override 
    { 
        output_box = bbox;
        return true; 
    }

    shared_ptr<hittable> left;
    shared_ptr<hittable> right;
    aabb bbox;

    static bool box_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b, int axis_index)
    {
        aabb box_a;
        aabb box_b;

        if (!a->bounding_box(box_a) || !b->bounding_box(box_b))
        std::cerr << "No bounding box in bvh_node constructor.\n";

        return box_a.min().e[axis_index] < box_b.min().e[axis_index];
    }

    static bool box_x_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b)
    {
        return box_compare(a, b, 0);
    }

    static bool box_y_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) 
    {
        return box_compare(a, b, 1);
    }

    static bool box_z_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) 
    {
        return box_compare(a, b, 2);
    }
};

//======================================================================

struct alignas(16) GPUBVHNode   // 强制整个结构体的对齐方式为 16 字节边界
{
    glm::vec3 aabbMin;          // 包围盒的最小顶点坐标
    int leftChild;              // 内部节点填左孩子下标，叶子填-1
                                // 这个int紧跟在vec3后面，会被塞进vec3的16字节对齐槽里，不会额外占空间

    glm::vec3 aabbMax;
    int rightChild;             // 内部节点填左孩子下标，叶子填图元下标

    int isLeaf;                 // 1 叶子，0 内部节点
    float pad0, pad1, pad2;     // 凑齐16字节，避免下一个node因为对齐产生偏移
};
// 所以最终一个 bvhnode占用48字节
// 为何在CPU端要这样设计节点？
// std430规则下vec3本身要按16字节对齐，正好剩一个4字节空当，编译器会自动把紧跟着的float塞进去，不会浪费也不会错位，这是个很常用的省内存技巧。

struct GPUSphere 
{
    glm::vec3 center;
    float radius;

    int materialId; // Day26要用，先占位填0
    float pad0, pad1, pad2;
};

class BVHFlattener
{
    public:
        std::vector<GPUBVHNode> flatNodes;
        std::vector<GPUSphere> flatSpheres;

        // 入口：传入根节点，返回根节点在flatNodes里的下标
        int flatten(shared_ptr<hittable> root)
        {
            return flattenNode(root);
        }

    private:
        int flattenNode(shared_ptr<hittable> node)
        {
            auto asBVH = std::dynamic_pointer_cast<bvh_node>(node);

            if (asBVH)
            {
                // 如果是内部节点，先递归拍平左右孩子，得到他们在数组中的索引
                int leftIdx = flattenNode(asBVH->left);
                int rightIdx = flattenNode(asBVH->right);

                aabb box = asBVH->bbox;
                GPUBVHNode gpuNode;
                gpuNode.aabbMin = glm::vec3(box.min().x(), box.min().y(), box.min().z());
                gpuNode.aabbMax = glm::vec3(box.max().x(), box.max().y(), box.max().z());
                gpuNode.leftChild = leftIdx;
                gpuNode.rightChild = rightIdx;
                gpuNode.isLeaf = 0;

                flatNodes.push_back(gpuNode);
                return (int)flatNodes.size() - 1;
            }
            else
            {
                // 是叶子：node直接是一个球
                auto s = std::dynamic_pointer_cast<sphere>(node);
                GPUSphere gpuSphere;
                gpuSphere.center = glm::vec3(s->center.x(), s->center.y(), s->center.z());
                gpuSphere.radius = s->radius;
                gpuSphere.materialId = 0;
                flatSpheres.push_back(gpuSphere);
                int sphereIdx = (int)flatSpheres.size() - 1;

                aabb box;
                node->bounding_box(box);
                GPUBVHNode gpuNode;
                gpuNode.aabbMin = glm::vec3(box.min().x(), box.min().y(), box.min().z());
                gpuNode.aabbMax = glm::vec3(box.max().x(), box.max().y(), box.max().z());
                gpuNode.leftChild = -1;
                gpuNode.rightChild = sphereIdx; // leaf节点复用rightChild字段存图元下标
                gpuNode.isLeaf = 1;

                flatNodes.push_back(gpuNode);
                return (int)flatNodes.size() - 1;
            }
        }
};


#endif