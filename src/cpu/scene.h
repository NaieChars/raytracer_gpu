#ifndef SCENE_H
#define SCENE_H

#include "bvh.h"

hittable_list createScene()
{
    hittable_list world;

    world.add(
        make_shared<sphere>(
            vec3(0,-1001,0),
            1000.0f,
            make_shared<lambertian>(vec3(0.9, 0.9, 0.9))
        )
    );

    // 大量小玻璃珠,随机分布在一片椭球空间里,营造悬浮的星尘感
    int numGlassBeads = 150;
    for (int i = 0; i < numGlassBeads; i++) {
        float x = random_double(-4.0, 4.0);
        float y = random_double(-1.0, 2.5);
        float z = random_double(-5.0, -1.0);
        float radius = random_double(0.02, 0.12);
        world.add(make_shared<sphere>(
            vec3(x, y, z), radius, make_shared<dielectric>(1.5)));
    }

    // 顶部大光源
    vec3 WholeLight = vec3(1.0, 0.88, 0.65)*3.9f;
    world.add(make_shared<sphere>(vec3(-10, 20, -25), 4.0f, make_shared<diffuse_light>(make_shared<constant_texture>(WholeLight))));

    // 掺杂的暖黄色小光球,数量少一些,起点缀星光的作用
    // 颜色乘了个4倍增强发光强度,因为场景里没有环境光/天空光源,
    // 全靠这些小球自己发光撑起整个画面亮度,不增强的话太暗
    int numLights = 20;
    for (int i = 0; i < numLights; i++) {
        float x = random_double(-4.0, 4.0);
        float y = random_double(-1.0, 2.5);
        float z = random_double(-5.0, -1.0);
        if (x >= -0.6f && x <= 0.6f && z >= -2.8f && z <= -1.6f && y >= 0.9f && y <= 0.3f) continue;
        float radius = random_double(0.01, 0.03);
        vec3 warmYellow = vec3(1.0, 0.85, 0.4);
        world.add(make_shared<sphere>(
            vec3(x, y, z), radius,
            make_shared<diffuse_light>(make_shared<constant_texture>(warmYellow))));
    }

    // 金属球周边的发光粒子
    int numspectical = 15;
    for (int i = 0; i < numspectical; i++) {
        float x = random_double(-1.0, 1.0);
        float y = random_double(-1.0, -0.9);
        float z = random_double(-3.0, -1.5);
        float radius = random_double(0.005, 0.01);
        vec3 warmYellow = vec3(1.0, 0.85, 0.4) * 8.0;
        world.add(make_shared<sphere>(
            vec3(x, y, z), radius,
            make_shared<diffuse_light>(make_shared<constant_texture>(warmYellow))));
    }

    // 衬托主体球的光
    vec3 backLight = vec3(1.0, 0.65, 0.2);
    world.add(make_shared<sphere>(vec3(-0.56, -0.8, -1.6), 0.15f, make_shared<diffuse_light>(make_shared<constant_texture>(backLight))));


    // 视觉锚点:一个大金属球,近似对焦点位置,方便待会儿观察景深虚化效果
    world.add(make_shared<sphere>(vec3(0, -0.4, -2.2), 0.5f,make_shared<metal>(vec3(0.9, 0.9, 0.95), 0.05)));

    // 一个发光雾团玻璃珠
    world.add(make_shared<sphere>(vec3(0, -0.8, -1.8), 0.1f,make_shared<isotropic>(
        make_shared<constant_texture>(vec3(1.0, 0.549, 0.0)), // 淡粉色雾
        0.5,                                                // sigma_t: 消光系数,数值越大雾越"浓",光线穿透越费劲
        1.7,                                               // scatter_albedo: 散射反照率,越接近1雾越亮越透,越接近0越像吸光的暗色玻璃
        0.3,                                                // g: 各向异性,先用0(各向同性)测试,之后可以试试0.3左右看前向散射的效果差异
        1.8                                                 // ior: 边界折射率,数值越高表面反光越强
    )
));

    return world;
}

/*
hittable_list createScene1()
{
    hittable_list world;



    // ------------------ 地面 ----------------
    auto ground = make_shared<lambertion>(make_shared<constant_texture>(vec3(0.96, 0.87, 0.7)));

    const int boxes_per_side = 20;
    for (int i = 0; i < boxes_per_side; i++) {
        for (int j = 0; j < boxes_per_side; j++) {
            auto w = 100.0;
            auto x0 = -1000.0 + i*w;
            auto z0 = -1000.0 + j*w;
            auto y0 = 0.0;
            auto x1 = x0 + w;
            auto y1 = random_double(1,101);
            auto z1 = z0 + w;

            world.add(make_shared<box>(vec3(x0,y0,z0), vec3(x1,y1,z1), ground));
        }
    }

    return world;
}
    */

#endif