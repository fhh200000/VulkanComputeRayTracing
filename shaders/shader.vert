/* @file shader.vert

    Vertex shader. Just do simple presentation.
    SPDX-License-Identifier: WTFPL

*/
#version 450

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) readonly buffer OutputImage {
	vec4 graphics[ ];
};

vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = graphics[gl_VertexIndex];
}
