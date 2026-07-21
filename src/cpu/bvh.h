#ifndef BVH_H
#define BVH_H

#include "hittable_list.h"
#include "sphere.h"
#include "material.h"

#include <algorithm>
#include <unordered_map>
#include <glm/glm.hpp>

class bvh_node : public hittable
{
    public:
    shared_ptr<hittable> left;
    shared_ptr<hittable> right;
    aabb bbox;

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
    bool hit(const vec3& ray_origin, const vec3& ray_dir, double t_min, double t_max, hit_record& rec) const override {return false;}

    bool bounding_box(aabb& output_box) const override 
    { 
        output_box = bbox;
        return true; 
    }

    static bool box_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b, int axis_index)
    {
        aabb box_a;
        aabb box_b;

        if (!a->bounding_box(box_a) || !b->bounding_box(box_b))
        std::cerr << "No bounding box in bvh_node constructor.\n";

        return box_a.min().e[axis_index] < box_b.min().e[axis_index];
    }

    static bool box_x_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) {return box_compare(a, b, 0);}
    static bool box_y_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) {return box_compare(a, b, 1);}
    static bool box_z_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) {return box_compare(a, b, 2);}
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
static_assert(sizeof(GPUBVHNode) == 48, "GPUBVHNode must be 48 bytes");

struct GPUSphere 
{
    glm::vec3 center;
    float radius;

    int materialId;
    float pad0, pad1, pad2;
};
static_assert(sizeof(GPUSphere)  == 32, "GPUSphere must be 32 bytes");

struct GPUMaterial
{
    glm::vec3 albedo;
    float fuzz;         // metal专用:模糊度 / isotropic专用(新用法):单次散射反照率scatter_albedo
    float ir;           // dielectric绿光通道折射率 / isotropic专用(新用法):消光系数sigma_t
    int type;
    float iorR;         // dielectric红光通道折射率 / isotropic专用(新用法):各向异性参数g
    float iorB;         // dielectric蓝光通道折射率(isotropic不使用,保留)
};
static_assert(sizeof(GPUMaterial) == 32, "GPUMaterial size mismatch, check alignment");

class BVHFlattener
{
    public:
        std::vector<GPUBVHNode> flatNodes;
        std::vector<GPUSphere> flatSpheres;
        std::vector<GPUMaterial> flatMaterials; 
        std::vector<int> flatLightIndices;  // 新增:记录哪些下标的球是发光体

        int flatten(shared_ptr<hittable> root)
        {
            return flattenNode(root);
        }

    private:
        // 材质去重：同一个material对象可能被多个球共用，避免重复存储；materialCache是哈希表，存一个键值对，key:材质内存地址，value:一个int,代表这个材质在最终连续数组里的下标
        std::unordered_map<material*, int> materialCache;

        // 拍平材质
        int flattenMaterial(shared_ptr<material> mat)
        {
            // 去重核心，查找缓存
            auto it = materialCache.find(mat.get());    // mat.get()取智能指针里包裹的原始内存地址，用这个地址去查哈希表
            if (it != materialCache.end()) return it->second;

            GPUMaterial gpuMat{};   // {}将结构体内成员初始化为0
            // 利用 dynamic_pointer_cast 向下转型，判断具体是哪种材质，并填充 gpuMat
            if (auto lamb = std::dynamic_pointer_cast<lambertian>(mat))
            {
                gpuMat.type = 0;
                if (auto colorTex = std::dynamic_pointer_cast<constant_texture>(lamb->albedo))
                {
                    gpuMat.albedo = glm::vec3(colorTex->color.x(), colorTex->color.y(), colorTex->color.z());
                }
            }
            else if (auto met = std::dynamic_pointer_cast<metal>(mat))
            {
                gpuMat.type = 1;
                gpuMat.albedo = glm::vec3(met->albedo.x(), met->albedo.y(), met->albedo.z());
                gpuMat.fuzz = (float)met->fuzz;
            }
            else if (auto diel = std::dynamic_pointer_cast<dielectric>(mat))
            {
                gpuMat.type = 2;
                float baseIor = (float)diel->refraction_index;

                // 色散强度:数值越大,彩虹分离效果越明显,这是艺术化近似,不是严格物理值
                float dispersionStrength = 0.02f;
                gpuMat.ir   = baseIor;                              // 绿光通道,基准折射率
                gpuMat.iorR = baseIor - dispersionStrength;          // 红光偏折最少,折射率最低
                gpuMat.iorB = baseIor + dispersionStrength * 1.5f;   // 蓝光偏折最多,折射率最高(现实中蓝紫光确实偏折更多)
            }
            else if (auto light = std::dynamic_pointer_cast<diffuse_light>(mat))
            {
                gpuMat.type = 3;
                gpuMat.albedo = glm::vec3(light->get_emit_color().x(), light->get_emit_color().y(), light->get_emit_color().z());
            }
            else if (auto iso = std::dynamic_pointer_cast<isotropic>(mat))
            {
                gpuMat.type = 4;
                gpuMat.albedo = glm::vec3(iso->get_albedo().x(), iso->get_albedo().y(), iso->get_albedo().z());
                gpuMat.fuzz = (float)iso->get_scatter_albedo(); // 复用字段,存散射反照率
                gpuMat.ir   = (float)iso->get_sigma_t();          // 复用字段,存消光系数
                gpuMat.iorR = (float)iso->get_g();                 // 复用字段,存各向异性参数
                gpuMat.iorB = (float)iso->get_ior(); // 新增:边界折射率
            }

            flatMaterials.push_back(gpuMat);
            int idx = (int)flatMaterials.size() - 1;
            materialCache[mat.get()] = idx;
            return idx;
        }

        int flattenNode(shared_ptr<hittable> node)
        {
            auto asBVH = std::dynamic_pointer_cast<bvh_node>(node);

            if (asBVH)
            {
                // 如果是内部节点，先递归拍平左右孩子，得到他们在数组中的索引
                int leftIdx = flattenNode(asBVH->left);
                int rightIdx = flattenNode(asBVH->right);

                aabb box = asBVH->bbox;
                GPUBVHNode gpuNode {};
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
                GPUSphere gpuSphere {};
                gpuSphere.center = glm::vec3(s->center.x(), s->center.y(), s->center.z());
                gpuSphere.radius = s->radius;
                gpuSphere.materialId = flattenMaterial(s->mat); // 把材质拍平记录下标
                flatSpheres.push_back(gpuSphere);
                int sphereIdx = (int)flatSpheres.size() - 1;

                // 若是发光球体
                if (flatMaterials[gpuSphere.materialId].type == 3)  flatLightIndices.push_back(sphereIdx);

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