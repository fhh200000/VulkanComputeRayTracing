/* @file functions.glsl

    texture functions needed in compute shader.
    SPDX-License-Identifier: WTFPL

*/

// Manual OOP in control-oriented GLSL.

#define TEXTURE_LAMBERTIAN 1
#define TEXTURE_METAL 2
#define TEXTURE_GLASS 3

vec3 random_in_unit_sphere(vec3 seed);
bool modified_refract(const in vec3 v, const in vec3 n, const in float ni_over_nt, out vec3 refracted);
float schlick(float cosine, float ior);
float rand(vec2 co);

void texture_lambertian(hit_record record, inout vec3 colour, inout ray generated_ray) {
    //   direction = -faceforward(direction, global_hit_record.normal, direction);
    vec3 direction = record.normal+random_in_unit_sphere(generated_ray.direction);
    colour = colour*record.colour*record.texture.y;
    generated_ray.origin = record.point;
    generated_ray.direction = direction;
}

void texture_glass(hit_record record, inout vec3 colour, inout ray generated_ray) {
        vec3 outward_normal, refracted;
        vec3 reflected = reflect(generated_ray.direction, record.normal);
        float ni_over_nt, reflect_prob, cosine;

        if (dot(generated_ray.direction, record.normal) > 0.) {
            outward_normal = -record.normal;
            ni_over_nt = record.texture.y;
            cosine = dot(generated_ray.direction, record.normal);
            cosine = sqrt(1. - record.texture.y*record.texture.y*(1.-cosine*cosine));
        } else {
            outward_normal = record.normal;
            ni_over_nt = 1. / record.texture.y;
            cosine = -dot(generated_ray.direction, record.normal);
        }

        if (modified_refract(generated_ray.direction, outward_normal, ni_over_nt, refracted)) {
	        reflect_prob = schlick(cosine, record.texture.y);
        } else {
            reflect_prob = 1.;
        }

        generated_ray.origin = record.point;

        if (rand(record.point.xy) < reflect_prob) {
            generated_ray.direction = reflected;
        } else {
            generated_ray.direction = refracted;
        }
}

void texture_metal(hit_record record, inout vec3 colour, inout ray generated_ray) {
    vec3 direction = reflect(generated_ray.direction,record.normal)+record.texture.y*random_in_unit_sphere(generated_ray.direction);
    colour = colour*record.colour;
    generated_ray.origin = record.point;
    generated_ray.direction = direction;
}

void texture_dispatcher(hit_record record, inout vec3 colour, inout ray generated_ray) {
    switch(int(record.texture.x)) {
        case TEXTURE_LAMBERTIAN:texture_lambertian(record,colour,generated_ray);break;
        case TEXTURE_METAL:texture_metal(record,colour,generated_ray);break;
        case TEXTURE_GLASS:texture_glass(record,colour,generated_ray);break;
    }
}
