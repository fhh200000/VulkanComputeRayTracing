/* @file structures.glsl

    Structures needed in compute shader.
    SPDX-License-Identifier: WTFPL

*/

precision mediump float;

struct sphere {
    vec3 center;
    float radius;
    vec3 colour;
    vec3 texture;   // texture.x: texture type (diffuse/metal/glass)
                    // texture.y: texture param1(reflect ratio in diffuse, fuzzness in metal, eta in glass)
};

struct ray {
    vec3 origin;
    vec3 direction;
};

struct hit_record {
    vec3 point;
    vec3 normal;
    float min_t;
    float max_t;
};

