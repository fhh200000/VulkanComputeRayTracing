/* @file structures.glsl

    Structures needed in compute shader.
    SPDX-License-Identifier: WTFPL

*/

struct sphere {
    vec3 center;
    float radius;
};

struct ray {
    vec3 origin;
    vec3 direction;
};

struct hit_record {
    vec3 point;
    vec3 normal;
    float t;
};

