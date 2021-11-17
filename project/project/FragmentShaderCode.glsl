#version 330 core

in vec2 TexCoord;
in vec3 normalWorld;
in vec3 vertexPositionWorld;

uniform sampler2D Texture;
uniform sampler2D NormalMapping;
uniform sampler2D Gold;

uniform int normalMapping_flag;

uniform vec3 eyePositionWorld;

uniform vec3 point_ambientLight;
uniform vec3 point_lightPos;

out vec4 Color;

void main()
{
    vec3 normal = normalize(normalWorld);
    
    if(normalMapping_flag == 1)
    {
        //Obtain normal from normal map in range [0, 1]
        normal = texture(NormalMapping, TexCoord).rgb;
        //Transform normal vector to range [-1, 1]
        normal = normalize(normal * 2.0 - 1.0);
    }
    
    //texture
    vec3 temp_texture = vec3(texture(Texture, TexCoord));
    
    //point_ambient
    vec3 v = point_lightPos - vertexPositionWorld;
    float d = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    float fd = 1.0/(1.0 + 0.0014 * d + 0.000007*d*d);
    
    //env_diffuse
    vec3 point_lightVectorWorld = normalize(point_lightPos - vertexPositionWorld);
    float point_brightness = dot(point_lightVectorWorld, normal);
    vec3 point_diffuseLight = vec3(point_brightness, point_brightness, point_brightness);
    //point_specular
    vec3 point_reflectedLightVectorWorld = reflect(-point_lightVectorWorld, normal);
    vec3 eyeVectorWorld = normalize(eyePositionWorld - vertexPositionWorld);
    float point_s = clamp(dot(point_reflectedLightVectorWorld, eyeVectorWorld), 0, 1);
    point_s = pow(point_s, 50);
    vec3 point_specularLight = vec3(point_s, point_s, point_s);
    
    vec4 point_color = fd * vec4(1.0f, 1.0f, 1.0f, 1.0f) * vec4(temp_texture * (point_ambientLight + clamp(point_diffuseLight, 0, 1) + point_specularLight), 1.0f);
    
    
    Color = point_color;
}
