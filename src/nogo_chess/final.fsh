#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float isInit;
uniform float isBlack;
uniform float viewWidth;
uniform float viewHeight;
uniform sampler2D screenTexture0;
uniform sampler2D screenTexture1;
uniform sampler2D menuTexture0;

float Luminance(vec3 color) {
	return dot(color.rgb,vec3(0.2125f, 0.7154f, 0.0721f));
}

const float offset[9] = float[] (0.0, 1.4896, 3.4757, 5.4619, 7.4482, 9.4345, 11.421, 13.4075, 15.3941);
const float weight[9] = float[] (0.066812, 0.129101, 0.112504, 0.08782, 0.061406, 0.03846, 0.021577, 0.010843, 0.004881);

vec3 blur(sampler2D image, vec2 uv, vec2 direction) {
	vec3 color = pow(texture(image, uv).rgb, vec3(2.2f)) * weight[0];
	for(int i = 1; i < 9; i++) {
		color += pow(texture(image, uv + direction * offset[i]).rgb, vec3(2.2f)) * weight[i];
		color += pow(texture(image, uv - direction * offset[i]).rgb, vec3(2.2f)) * weight[i];
	}
	return color;
}

void doBloom(inout vec3 color) {
	vec3 bloom = blur(screenTexture1, TexCoords.st, vec2(1.0, 0.0) / vec2(viewWidth, viewHeight));
    color.rgb += bloom * 2.95f;
}

void doTonemapping(inout vec3 color) {
	const float A = 2.51f;
	const float B = 0.03f;
	const float C = 2.43f;
	const float D = 0.59f;
	const float E = 0.14f;
	color = (color * (A * color + B)) / (color * (C * color + D) + E);
	color = pow(color.rgb, vec3(1.0f / 2.2f));
}

void doVignette(inout vec3 color) {
    float power = 0.6f;
    float dist  = distance(TexCoords.st, vec2(0.5f)) * 2.0f;
          dist /= 1.5142f;
          dist  = pow(dist, 1.1f);
    color.rgb *= 1.0f - dist * power;
}

void doColorProcess(inout vec3 color) {
	float gamma		 = 0.88;
	float exposure	 = 1.30;
	float saturation = 1.05;
	float contrast	 = 0.94;

	float luma = Luminance(color.rgb);
	color = pow(color, vec3(gamma)) * exposure;
	color = (((color - luma) * saturation) + luma) / luma * pow(luma, contrast);
}

vec4 GetBlurAO(in sampler2D tex, in vec2 coord) {
	vec2 res = vec2(viewWidth, viewHeight);
	coord = coord * res + 0.5f;
	vec2 i = floor(coord);
	vec2 f = fract(coord);
	f = f * f * (3.0f - 2.0f * f);
	coord = i + f;
	coord = (coord - 0.5f) / res;
	return texture(tex, coord);
}

void main() {
	vec3 color = pow(texture(screenTexture0, TexCoords).rgb, vec3(2.2f)) * pow(GetBlurAO(screenTexture0, TexCoords).a, 4.0f);
	vec4 menu = texture(menuTexture0, TexCoords);
	doBloom(color);
	color *= 1.0f - isBlack;
	color = mix(color, pow(menu.rgb * menu.a, vec3(2.2f)), menu.a * mix(1.0f - isBlack, 1.0f, isInit));
	doColorProcess(color);
    doTonemapping(color);
    FragColor = vec4(color, 1.0f);
}