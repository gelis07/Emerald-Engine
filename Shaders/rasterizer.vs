#version 430 core

layout (location = 0) in vec3 iVertex;
uniform mat4 uMvp;
void main()
{
    gl_Position = uMvp * vec4(iVertex, 1.0);
}