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

uniform vec3 point_ambientLight_planet;
uniform vec3 point_lightPos_planet;

uniform vec3 spot_ambientLight;
uniform vec3 spotPos;
uniform vec3 spotDir;
uniform float spotCutOff;
uniform float spotOuterCutOff;
uniform int spotOn;

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
    
    
    //planet
    v = point_lightPos_planet - vertexPositionWorld;
    d = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    fd = 1.0/(1.0 + 0.007 * d + 0.0002*d*d);
    
    //env_diffuse
    point_lightVectorWorld = normalize(point_lightPos_planet - vertexPositionWorld);
    point_brightness = dot(point_lightVectorWorld, normal);
    point_diffuseLight = vec3(point_brightness, point_brightness, point_brightness);
    //point_specular
    point_reflectedLightVectorWorld = reflect(-point_lightVectorWorld, normal);
    eyeVectorWorld = normalize(eyePositionWorld - vertexPositionWorld);
    point_s = clamp(dot(point_reflectedLightVectorWorld, eyeVectorWorld), 0, 1);
    point_s = pow(point_s, 50);
    point_specularLight = vec3(point_s, point_s, point_s);
    
    vec4 point_color_planet = fd * vec4(1.0f, 1.0f, 0.2f, 1.0f) * vec4(temp_texture * (point_ambientLight + clamp(point_diffuseLight, 0, 1) + point_specularLight), 1.0f);
    
    
    //spot
    vec3 lightDir = normalize(spotPos - vertexPositionWorld);
    vec4 spot_color;
    // Check if lighting is inside the spotlight cone
    float theta = dot(lightDir, normalize(-spotDir));
    if(theta > spotCutOff)
    {
        vec3 v_spot = spotPos - vertexPositionWorld;
        float d_spot = sqrt(v_spot.x*v_spot.x + v_spot.y*v_spot.y + v_spot.z*v_spot.z);
        float fd_spot = 1.0/(1.0 + 0.0014 * d_spot + 0.000007*d_spot*d_spot);
        
        vec3 spot_ambient = temp_texture * spot_ambientLight;
        //env_diffuse
        vec3 spot_lightVectorWorld = lightDir;
        float spot_brightness = dot(spot_lightVectorWorld, normalize(normalWorld));
        vec3 spot_diffuseLight = vec3(spot_brightness, spot_brightness, spot_brightness);
        //point_specular
        vec3 spot_reflectedLightVectorWorld = reflect(-spot_lightVectorWorld, normalWorld);
        float spot_s = clamp(dot(spot_reflectedLightVectorWorld, eyeVectorWorld), 0, 1);
        spot_s = pow(spot_s, 50);
        vec3 spot_specularLight = vec3(spot_s, spot_s, spot_s);
        
        float epsilon = (spotCutOff - spotOuterCutOff);
        float intensity = clamp((theta - spotOuterCutOff) / epsilon, 0.0, 1.0);
        
        spot_color = fd_spot * vec4(1.0f, 1.0f, 0.0f, 1.0f) * vec4(temp_texture * (spot_ambient + (1-intensity) * (clamp(spot_diffuseLight, 0, 1) + spot_specularLight)), 1.0f);
    }
    else {
        spot_color = vec4(0.0f);
    }
    
    
    Color = spotOn > 0 ? (0.4* point_color_planet + spot_color):(point_color + point_color_planet);
}
