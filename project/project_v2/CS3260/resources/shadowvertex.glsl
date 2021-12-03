#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 lightSpaceMatrix;
uniform mat4 model;
void main()
{
    mat4 lightMVP = lightSpaceMatrix * model;
    gl_Position = lightMVP * vec4(aPos, 1.0);
}
