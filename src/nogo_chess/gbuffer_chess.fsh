#version 330
layout (location = 0) out vec4 FragColor0;
layout (location = 1) out vec4 FragColor1;
layout (location = 2) out vec4 FragColor2;
layout (location = 3) out vec4 FragColor3;

in vec2 TexCoord;
in vec4 fragPos;
in vec3 aNormal;

in mat3 TBN;

uniform int ready;
uniform float whiteblack;

uniform ivec2 map;
uniform vec3 litPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 camPos;

#define Positive(input) max(input, 1E-3)
#define pi 3.1415926

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a     = roughness * roughness;
    float NdotH = Positive(dot(N, H));
	
    float nom   = dot(a, a);
    float denom = (dot(NdotH, NdotH) * (dot(a, a) - 1.0) + 1.0);
		  denom = pi * denom * denom;
		  
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
	
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = Positive(dot(N, V));
    float NdotL = Positive(dot(N, L));
	
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    
	vec3 color = mix(vec3(1.00f), vec3(0.05f), whiteblack);

    float metallic = 0.2f; float roughness = 0.5f;
    vec3 F0 = mix(vec3(0.04), color, metallic);

    vec3 N = normalize(aNormal);
    vec3 V = normalize(camPos - fragPos.xyz);
    vec3 L = normalize(litPos - fragPos.xyz);
    vec3 H = normalize(L + V);

    //Cook-Torrance BRDF
	vec3 F    = FresnelSchlickRoughness(Positive(dot(H, V)), F0, roughness);
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);     

    vec3 nominator    = NDF * G * F;
    float denominator = 4 * dot(N, V) * dot(N, L) + 0.001; 
	
    vec3 specular = nominator / denominator;
    color = (Positive(specular) + kD * color / pi) * Positive(dot(N, L));

    float alpha = 1.0f;
    if (whiteblack > 0.4f && whiteblack < 0.6f) alpha = 0.0f;
    int ready0 = ready;
    if (ready == 2) {
        ready0 = 0;
        color *= vec3(1.0f, 0.2f, 0.2f);
    }

    FragColor0 = vec4(pow(color, vec3(1.0f / 2.2f)), mix(alpha, 0.5f, ready0));
    FragColor1 = vec4((map.x + 1) / 255.0f, (map.y + 1) / 255.0f, 1.0f, 1.0f);
    FragColor2 = vec4(vec3(gl_FragCoord.z / gl_FragCoord.w), alpha);
    FragColor3 = vec4(N * 0.5f + 0.5f, alpha);

}