/* @file functions.glsl

    common functions needed in compute shader.
    SPDX-License-Identifier: WTFPL

*/

#include "globals.glsl"

float rand(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

bool hit_sphere(const sphere s, ray r, inout hit_record global_hit_record) {
    vec3 oc = r.origin - s.center;
    float a = dot(r.direction,r.direction);
    float half_b = dot(oc, r.direction);
    float c = dot(oc,oc) - s.radius*s.radius;
    float discriminant = half_b*half_b - a*c;
    if (discriminant < 0) {
        return false;
    }

    // Find the nearest root that lies in the acceptable range.
    float sqrtd = sqrt(discriminant);
    float root = (-half_b - sqrtd) / a;
    if (root <= global_hit_record.min_t || global_hit_record.max_t <= root) {
        root = (-half_b + sqrtd) / a;
    if (root <= global_hit_record.min_t || global_hit_record.max_t <= root)
        return false;
    }

    global_hit_record.max_t = root;
    global_hit_record.point = root*r.direction+r.origin;
    vec3 normal = (global_hit_record.point - s.center) / s.radius;
    global_hit_record.normal = normal;//faceforward(normal, normal, r.direction);
    return true;
}

vec3 random_in_unit_sphere(vec3 seed) {
    return normalize(vec3(rand(seed.xy),rand(seed.xz),rand(seed.yz)));
}


vec3 ray_color(ray r) {

    hit_record global_hit_record;
    global_hit_record.max_t = infinity;
    vec3 color = vec3(1.0,1.0,1.0);
    bool t;

    // Non-recursion version ray-tracing WA because GLSL does not allow recursion.
    for(int pass=0;pass<MAX_RECURSION_LEVEL;pass++) {
        t = false;
        global_hit_record.max_t = infinity;
        global_hit_record.min_t = 0.001;
        for(int i=0;i<world.length();i++) {
            if(hit_sphere(world[i], r, global_hit_record)) {
                t = true;
            }
        }
        if (t) {
            //   direction = -faceforward(direction, global_hit_record.normal, direction);
            vec3 direction = global_hit_record.normal+random_in_unit_sphere(r.direction);
            color *= 0.5;
            r.origin = global_hit_record.point;
            r.direction = direction;
        }
        else { // Hit sky.
                vec3 unit_direction = normalize(r.direction);
                float a = 0.5*(unit_direction.y + 1.0);
                color *= mix(vec3(1),vec3(.5,.7,1), a);
            return color;
        }
    }
}
