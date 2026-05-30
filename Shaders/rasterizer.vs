#version 430 core

layout (location = 0) in vec3 iPos;
layout (location = 1) in vec3 iTexCoords;

uniform mat4 uMvp;

void main()
{
    gl_Position = uMvp * vec4(iPos, 1.0);
}