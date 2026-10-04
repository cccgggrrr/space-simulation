#version 460 core

in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D albedoTexture;
uniform vec3 bodyColor;
uniform bool useTexture;

out vec4 FragColor;

void main()
{
    vec3 albedo = useTexture ? texture(albedoTexture, TexCoord).rgb : bodyColor;

    vec3 lightDirection = normalize(vec3(0.4, 0.6, 1.0));
    float diffuse = max(dot(normalize(Normal), lightDirection), 0.0);
    float light = 0.12 + diffuse * 0.88;

    FragColor = vec4(albedo * light, 1.0);
}