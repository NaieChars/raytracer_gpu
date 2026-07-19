#ifndef SCENE_H
#define SCENE_H

#include "bvh.h"

hittable_list createScene()
{
    hittable_list world;


    world.add(
        make_shared<sphere>(
            vec3(0,-1000.8,0),
            1000.0f,
            make_shared<lambertian>(vec3(0.9, 0.9, 0.9))
        )
    );

    world.add(
        make_shared<sphere>(
            vec3(0,0,-1),
            0.3f,
            make_shared<lambertian>(vec3(1.0, 1.0, 0.0))
        )
    );


    world.add(
        make_shared<sphere>(
            vec3(1,0,-2),
            0.6f,
            make_shared<metal>(vec3(0.8, 0.6, 0.2), 0.2)
        )
    );


    world.add(
        make_shared<sphere>(
            vec3(-1.5,0.3,-3),
            1.0f,
            make_shared<dielectric>(1.5)
        )
    );


    return world;
}

#endif