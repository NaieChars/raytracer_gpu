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
    int numGlassBeads = 200;
    for (int i = 0; i < numGlassBeads; i++) {
        float x = random_double(-4.0, 4.0);
        float y = random_double(-1.0, 2.5);
        float z = random_double(-5.0, -1.0);
        float radius = random_double(0.04, 0.12);
        world.add(make_shared<sphere>(
            vec3(x, y, z), radius, make_shared<dielectric>(1.5)));
    }

    // 掺杂的暖黄色小光球,数量少一些,起点缀星光的作用
    // 颜色乘了个4倍增强发光强度,因为场景里没有环境光/天空光源,
    // 全靠这些小球自己发光撑起整个画面亮度,不增强的话太暗
    int numLights = 20;
    for (int i = 0; i < numLights; i++) {
        float x = random_double(-4.0, 4.0);
        float y = random_double(-1.0, 2.5);
        float z = random_double(-5.0, -1.0);
        float radius = random_double(0.03, 0.07);
        vec3 warmYellow = vec3(1.0, 0.85, 0.4) * 32.0;
        world.add(make_shared<sphere>(
            vec3(x, y, z), radius,
            make_shared<diffuse_light>(make_shared<constant_texture>(warmYellow))));
    }

    // 视觉锚点:一个大金属球,近似对焦点位置,方便待会儿观察景深虚化效果
    world.add(make_shared<sphere>(
        vec3(0, -0.3, -2.2), 0.6f,
        make_shared<metal>(vec3(0.9, 0.9, 0.95), 0.05)));



    return world;
}

#endif