/* @file shader.frag

    Fragment shader. Just do simple presentation.
    SPDX-License-Identifier: WTFPL

*/
#version 450

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform sampler2D OutputImage;

void main() {
    outColor = texelFetch(OutputImage, ivec2(gl_FragCoord.xy), 0);
}
