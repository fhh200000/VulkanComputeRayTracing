/* @file functions.glsl

    texture functions needed in compute shader.
    SPDX-License-Identifier: WTFPL

*/

// Manual OOP in control-oriented GLSL.

#define TEXTURE_LAMBERTIAN 1
#define TEXTURE_METAL 2
#define TEXTURE_GLASS 3

float rand(vec2 co);

