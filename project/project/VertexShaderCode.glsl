#version 330 core

layout(location=0) in vec3 position;
layout(location=1) in vec2 vertexUV;
layout(location=2) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec3 normalWorld;
out vec3 vertexPositionWorld;

void main()
{
    vec4 v = vec4(position, 1.0);
    vec4 newPosition = model * v;
    vec4 out_position = projection * view * newPosition;
    gl_Position = out_position;
    
    TexCoord = vertexUV;
    
    vec4 normal_temp = model * vec4(normal, 0);
    normalWorld = normal_temp.xyz;
    
    vertexPositionWorld = newPosition.xyz;
}
