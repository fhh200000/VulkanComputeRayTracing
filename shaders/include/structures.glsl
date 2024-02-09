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

float hit_sphere (const sphere s, ray r) {
    vec3 oc = r.origin - s.center;
    float a = dot(r.direction,r.direction);
    float half_b = dot(oc, r.direction);
    float c = dot(oc,oc) - s.radius*s.radius;
    float discriminant = half_b*half_b - a*c;
    if (discriminant < 0) {
        return -1.0;
    } else {
        return (-half_b - sqrt(discriminant) ) / a;
    }

}

vec3 ray_color(const ray r) {
    float t = hit_sphere(sphere(vec3(0,0,-1), 0.5), r);
    if (t > 0.0) {
        vec3 N = normalize(r.origin + t*r.direction - vec3(0,0,-1));
        return 0.5*vec3(N.x+1, N.y+1, N.z+1);
    }
    vec3 unit_direction = normalize(r.direction);
    float a = 0.5*(unit_direction.y + 1.0);
    return (1.0-a)*vec3(1.0, 1.0, 1.0) + a*vec3(0.5, 0.7, 1.0);
}
