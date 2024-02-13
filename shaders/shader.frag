/* @file shader.frag

    Fragment shader. Just do simple presentation.
    SPDX-License-Identifier: WTFPL

*/
#version 450

layout (location = 0) out vec4 outColor;

layout (rgba32f, set = 0, binding = 1) uniform readonly image2D OutputImage;

void main() {
    outColor = imageLoad(OutputImage, ivec2(gl_FragCoord.xy));
}
