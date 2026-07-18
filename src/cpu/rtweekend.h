#ifndef RTWEEKEND_H
#define RTWEEKEND_H

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>

using std::make_shared;
using std::shared_ptr;

const double infinity = std::numeric_limits<double>::infinity();
constexpr double pi = 3.1415926535897932385;

// 工具函数

/**
 * @return 随机数[0, 1)
 */
inline double random_double()
{
    return std::rand() / (RAND_MAX + 1.0f);
}

inline double random_double(double min, double max)
{
    return min + (max - min) * random_double();
}

inline int random_int(int min, int max)
{
    // 随机整数 [min, max]
    return int(random_double(min, max+1));
}

inline double degrees_to_radians(double degrees)
{
    return degrees * pi / 180.0f;
}

inline double clamp(double x, double min, double max) 
{
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

#include "vec3.h"

#endif