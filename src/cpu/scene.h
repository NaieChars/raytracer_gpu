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

    // 大量小玻璃珠,随机分布在一片椭球空间里
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

    // 掺杂的暖黄色小光球
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
        0.5,                                               
        1.7,                                               
        0.3,                                                
        1.8                                                 
    )
));

    return world;
}

hittable_list createScene1() {
    hittable_list world;

    // 地面
    world.add(make_shared<box>(vec3(-10,-1,-10), vec3(10, 0, 0), make_shared<lambertian>(vec3(1.0, 0.65, 0.2))));

    world.add(make_shared<box>(vec3(0,0,-2), vec3(1,0.5,-1), make_shared<metal>(vec3(0.75, 0.68, 0.55), 0.1)));
    world.add(make_shared<box>(vec3(0,0,-1), vec3(1,0.3,0), make_shared<metal>(vec3(0.75, 0.68, 0.55), 0.1)));
    world.add(make_shared<box>(vec3(1,0,-2), vec3(2,0.6,-1), make_shared<lambertian>(vec3(1.0, 0.65, 0.2))));
    world.add(make_shared<box>(vec3(1,0,-1), vec3(2,0.7,0), make_shared<lambertian>(vec3(1.0, 0.65, 0.2))));
    world.add(make_shared<box>(vec3(2,-1,0), vec3(3,0.7,1), make_shared<lambertian>(vec3(1.0, 0.65, 0.2))));

    world.add(make_shared<box>(vec3(-1,0,-1), vec3(0,0.2,0), make_shared<lambertian>(vec3(0.51, 0.55, 0.82))));
    world.add(make_shared<box>(vec3(-1,0,-2), vec3(0,1,-1), make_shared<metal>(vec3(0.75, 0.68, 0.55), 0.1)));
    world.add(make_shared<box>(vec3(-1,-1,0), vec3(0,0.25,1), make_shared<lambertian>(vec3(0.98, 0.94, 0.82))));
    world.add(make_shared<box>(vec3(0,-1,0), vec3(1,-0.5,1), make_shared<lambertian>(vec3(1.0, 0.65, 0.2))));
    world.add(make_shared<box>(vec3(-2,0,-1), vec3(-1,0.25,0), make_shared<lambertian>(vec3(1.00, 0.45, 0.05))));

    world.add(make_shared<box>(vec3(-2,0,-2), vec3(-1,0.4,-1), make_shared<lambertian>(vec3(1.0, 0.65, 0.2))));
    world.add(make_shared<box>(vec3(-2,0,-3), vec3(-1,0.5,-2), make_shared<lambertian>(vec3(1.0, 0.65, 0.2)))); world.add(make_shared<box>(vec3(-2,1.5,-3), vec3(-1,2.5,-2), make_shared<lambertian>(vec3(1.0, 0.65, 0.2))));
    world.add(make_shared<box>(vec3(-3,0,-2), vec3(-2,2,-1), make_shared<lambertian>(vec3(1.0, 0.65, 0.2))));
    world.add(make_shared<box>(vec3(-2,0,-4), vec3(-1,2,-3), make_shared<lambertian>(vec3(1.0, 0.65, 0.2))));
    world.add(make_shared<box>(vec3(-1,0,-3), vec3(0,1.5,-2), make_shared<metal>(vec3(0.75, 0.68, 0.55), 0.1)));

    world.add(make_shared<box>(vec3(-3,0,-3), vec3(-2,1.5,-2), make_shared<lambertian>(vec3(1.0, 0.65, 0.2))));
    world.add(make_shared<box>(vec3(-3,0,-5), vec3(-2,2,-4), make_shared<lambertian>(vec3(1.0, 0.65, 0.2))));
    world.add(make_shared<box>(vec3(-3,0,-1), vec3(-2,0.7,0), make_shared<lambertian>(vec3(1.00, 0.45, 0.05))));
    world.add(make_shared<box>(vec3(-2,0,0), vec3(-1,0.4,1), make_shared<lambertian>(vec3(1.00, 0.45, 0.05))));
    world.add(make_shared<box>(vec3(-3,0.7,-1), vec3(-2.5,1.2,-0.5), make_shared<lambertian>(vec3(1.00, 0.45, 0.05))));

    world.add(make_shared<box>(vec3(-4,0,-1), vec3(-3,1.6,0), make_shared<lambertian>(vec3(1.00, 0.45, 0.05))));

    // 顶部大光源
    vec3 WholeLight = vec3(1.0, 0.9, 0.9)*3.9f;
    //world.add(make_shared<sphere>(vec3(0, 15, -5), 4.0f, make_shared<diffuse_light>(make_shared<constant_texture>(WholeLight))));
    world.add(make_shared<sphere>(
    vec3(2.0, 4, -4), 0.7f,
    make_shared<diffuse_light>(make_shared<constant_texture>(vec3(1.0, 0.8, 0.5) * 6.0))
));

    // 补光
    world.add(make_shared<sphere>(
    vec3(-5, 1.0, 2), 0.5f,
    make_shared<diffuse_light>(make_shared<constant_texture>(vec3(0.5, 0.7, 1.0) * 4))
));

    //主体球的光
    vec3 backLight = vec3(1.0, 0.65, 0.2);
    world.add(make_shared<sphere>(vec3(-1.5, 0.6, -2.5), 0.1f, make_shared<diffuse_light>(make_shared<constant_texture>(vec3(1.00, 0.60, 0.15)*2.0f))));
    world.add(make_shared<sphere>(vec3(-1.25, 0.5, -0.75), 0.25f, make_shared<diffuse_light>(make_shared<constant_texture>(backLight))));
    //world.add(make_shared<sphere>(vec3(-0.5, 1, 2), 0.25f, make_shared<diffuse_light>(make_shared<constant_texture>(backLight*2.0f))));
    //world.add(make_shared<sphere>(vec3(0.4, -0.4,0.5), 0.06f, make_shared<diffuse_light>(make_shared<constant_texture>(vec3(1.00, 0.60, 0.15)*8.0f))));

    // 金属球周边的发光粒子
    int MAX = 20;
    for (int i = 0; i < MAX; i++) {
        float x = random_double(0, 1);
        float y = random_double(0.31, 0.33);
        float z = random_double(-0.9, 0);
        float radius = random_double(0.005, 0.01);
        vec3 blue = vec3(0.1, 0.82,1) * 4.0;
        world.add(make_shared<sphere>(
            vec3(x, y, z), radius,
            make_shared<diffuse_light>(make_shared<constant_texture>(blue))));
    }

    // 金属球周边的发光粒子
    int numspectical = 10;
    for (int i = 0; i < numspectical; i++) {
        float x = random_double(-0.95, -0.05);
        float y = random_double(0.21, 0.23);
        float z = random_double(-1.0, 0);
        float radius = random_double(0.005, 0.01);
        vec3 warmYellow = vec3(0, 0.82,1) * 3.0;
        world.add(make_shared<sphere>(
            vec3(x, y, z), radius,
            make_shared<diffuse_light>(make_shared<constant_texture>(warmYellow))));
    }

    // 发光球周边的发光粒子
    int num = 5;
    for (int i = 0; i < num; i++) {
        float x = random_double(-2, -1);
        float y = random_double(0.25, 0.26);
        float z = random_double(-1.0, 0);
        float radius = random_double(0.005, 0.01);
        vec3 sun = vec3(1.00, 0.45, 0.05) * 3.0;
        world.add(make_shared<sphere>(
            vec3(x, y, z), radius,
            make_shared<diffuse_light>(make_shared<constant_texture>(sun))));
    }

    // 大量小玻璃珠
    int numGlassBeads = 15;
    for (int i = 0; i < numGlassBeads; i++) {
        float x = random_double(-0.9, 0);
        float y = random_double(0.25, 0.27);
        float z = random_double(0, 1);
        float radius = random_double(0.02, 0.1);
        world.add(make_shared<sphere>(
            vec3(x, y, z), radius, make_shared<dielectric>(1.5)));
    }


    world.add(make_shared<sphere>(vec3(-1.4, 0.35, -0.4), 0.1f,make_shared<isotropic>(
        make_shared<constant_texture>(vec3(0.65, 0.25, 0.03)), // 深蓝
        3,                                                // sigma_t: 消光系数,数值越大光线穿透越费劲
        0.9,                                               // scatter_albedo: 散射反照率,越接近1雾越亮越透
        0.3,                                                // g: 各向异性
        1.8                                                 // ior: 边界折射率
    )));

    world.add(make_shared<sphere>(vec3(-1.1, 0.31, -0.4), 0.06f,make_shared<isotropic>(
        make_shared<constant_texture>(vec3(0.65, 0.25, 0.03)), // 深蓝
        3,                                                
        0.9,                                               
        0.3,                                              
        1.8                                            
    )));

    world.add(make_shared<sphere>(vec3(-0.6, 0.4, -0.6), 0.2f,make_shared<metal>(vec3(0.9, 0.9, 0.95), 0.05)));

    return world;
}


hittable_list createMinimalBoxTest() {
    hittable_list world;
    world.add(make_shared<box>(vec3(-0.2 - 0.15, -0.8 - 0.15, -1.6 - 0.15), vec3(-0.2 + 0.15, -0.8 + 0.15, -1.6 + 0.15), make_shared<lambertian>(vec3(0.8, 0.3, 0.3)) ));

    // 顶部大光源
    vec3 WholeLight = vec3(1.0, 0.88, 0.65)*3.9f;
    world.add(make_shared<sphere>(vec3(-10, 20, -25), 4.0f, make_shared<diffuse_light>(make_shared<constant_texture>(WholeLight))));


    vec3 backLight = vec3(1.0, 0.65, 0.2);
    world.add(make_shared<sphere>(vec3(-0.56, -0.8, -1.6), 0.15f, make_shared<diffuse_light>(make_shared<constant_texture>(backLight))));
    return world;
}

#endif