#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uEyePos;
uniform vec4 uObjectColor;
uniform vec3 uLightDir;

const float AMBIENT = 0.15;
const float DIFFUSE_SCALE = 0.65;
const float SPECULAR_STRENGTH = 0.12;
const float SHININESS = 48.0;

out vec4 fragColor;

void main() {
    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N;

    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uEyePos - vWorldPos);
    vec3 H = normalize(L + V);

    // Half-lambert diffuse — softer shadow transitions.
    float NdotL = dot(N, L);
    float diff = NdotL * 0.5 + 0.5;
    diff = diff * diff;

    // Specular blends with object color to avoid bright white hotspots.
    float spec = pow(max(dot(N, H), 0.0), SHININESS) * SPECULAR_STRENGTH;

    vec3 color = uObjectColor.rgb * (AMBIENT + diff * DIFFUSE_SCALE + spec);
    fragColor = vec4(color, uObjectColor.a);
}
