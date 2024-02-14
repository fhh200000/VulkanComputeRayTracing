/* @file SceneGenerator.hpp
 *
 *  Random scene generator for shader, since random is extremely difficult in GLSL.
 *  SPDX-License-Identifier: WTFPL
 *
 */
#include <cstdio>
#include <random>

typedef struct vec3 {
    float x, y, z;
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3 operator-(vec3 in) { return vec3(x - in.x, y - in.y, z - in.z); }
    float length() { return x * x + y * y + z * z; }
} point3;

double random_double() {
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
}

int main(void) {
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());
            if ((center - point3(4, 0.2, 0)).length() < 0.9) {
                continue;
            }
            printf("sphere(vec3(%.2f,%.2f,%.2f), 0.2, ", center.x, center.y,
                         center.z);
            if (choose_mat < 0.8) {
                // diffuse
                printf("vec3(%.2f,%.2f,%.2f), vec3(TEXTURE_LAMBERTIAN,%.2f,0.0)),\n",
                             random_double(), random_double(), random_double(),
                             random_double());
            } else if (choose_mat < 0.95) {
                // metal
                printf("vec3(%.2f,%.2f,%.2f), vec3(TEXTURE_METAL,%.2f,0.0)),\n",
                             random_double(), random_double(), random_double(),
                             random_double());
            } else {
                // glass
                printf("vec3(1.0,1.0,1.0), vec3(TEXTURE_GLASS,1.5,0.0)),\n");
            }
        }
    }
    printf("\n");
    printf("sphere(vec3(0, 1, 0),1.0, vec3(1.0,1.0,1.0), "
           "vec3(TEXTURE_GLASS,1.5,0.0)),\n");
    printf("sphere(vec3(-4, 1, 0),1.0, vec3(0.4, 0.2, 0.1), "
           "vec3(TEXTURE_LAMBERTIAN,1.0,0.0)),\n");
    printf("sphere(vec3(4, 1, 0),1.0, vec3(0.7, 0.6, 0.5), "
           "vec3(TEXTURE_METAL,1.0,0.0)),\n");
}
