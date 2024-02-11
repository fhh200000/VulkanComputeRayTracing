/* @file globals.glsl

    global variables needed in compute shader.
    SPDX-License-Identifier: WTFPL

*/
#include "structures.glsl"
#include "textures.glsl"
#define SAMPLES_PER_PIXEL 100
#define MAX_RECURSION_LEVEL 50

const float aspect_ratio = 16.0 / 9.0;

// Camera
const float focal_length = 1.0;
const float viewport_height = 2.0;
const float viewport_width = viewport_height * (aspect_ratio);
const vec3 camera_center = vec3(0, 0, 0);

// Calculate the vectors across the horizontal and down the vertical viewport edges.
const vec3 viewport_u = vec3(viewport_width, 0, 0);
const vec3 viewport_v = vec3(0, -viewport_height, 0);

// Calculate the horizontal and vertical delta vectors from pixel to pixel.
const vec3 pixel_delta_u = viewport_u / 1280;
const vec3 pixel_delta_v = viewport_v / 720;

// Calculate the location of the upper left pixel.
const vec3 viewport_upper_left = camera_center - vec3(0, 0, focal_length) - viewport_u/2 - viewport_v/2;
const vec3 pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

const float infinity = 1. / 0.;

// World.
const sphere world[] = {
    sphere(vec3(0,0,-1), 0.5, vec3(1.0,1.0,1.0),vec3(1.0,0.0,0.0)),
    sphere(vec3(0,-100.5,-1), 100, vec3(1.0,1.0,1.0),vec3(1.0,0.0,0.0))
};
