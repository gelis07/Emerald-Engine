#version 430 core

layout(location = 0) out vec4 outColor;
uniform sampler2D RenderImage;

in vec2 TexCoords;

void main()
{
    outColor = vec4(texture(RenderImage, TexCoords).rgb, 1.0);
}