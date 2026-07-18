#ifndef SCENE_H
#define SCENE_H

#include "bvh.h"

hittable_list createScene()
{
    hittable_list world;


    world.add(
        make_shared<sphere>(
            vec3(0,0,-1),
            0.5f
        )
    );


    world.add(
        make_shared<sphere>(
            vec3(1,0,-2),
            0.5f
        )
    );


    world.add(
        make_shared<sphere>(
            vec3(-1,0,-2),
            0.5f
        )
    );


    return world;
}

#endif