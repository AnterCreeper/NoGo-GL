#version 330 core
layout (location = 0) out vec4 FragColor0;
layout (location = 1) out vec4 FragColor1;

in vec2 TexCoords;

uniform mat4 projectioninverse;
uniform float viewWidth;
uniform float viewHeight;
uniform sampler2D screenTexture;
uniform sampler2D depthTexture;
uniform sampler2D normalTexture;

vec4 GetColorTex(sampler2D tex, vec2 coord){
	return pow(texture(tex, coord), vec4(2.2f));
}

vec4 texture_Bicubic(in sampler2D tex, in vec2 coord) {
	vec2 res = vec2(viewWidth, viewHeight);
	coord = coord * res + 0.5f;
	vec2 i = floor(coord);
	vec2 f = fract(coord);
	f = f * f * (3.0f - 2.0f * f);
	coord = i + f;
	coord = (coord - 0.5f) / res;
	return texture(tex, coord);
}

const float offset[9] = float[] (0.0, 1.4896, 3.4757, 5.4619, 7.4482, 9.4345, 11.421, 13.4075, 15.3941);
const float weight[9] = float[] (0.066812, 0.129101, 0.112504, 0.08782, 0.061406, 0.03846, 0.021577, 0.010843, 0.004881);

vec3 fetchHigh(sampler2D image, vec2 uv) {
	float brightness = dot(texture_Bicubic(image, uv).rgb, vec3(0.2126, 0.7152, 0.0722));
	return texture_Bicubic(image, uv).rgb * pow(brightness, 4.0);
}

vec3 blur(sampler2D image, vec2 uv, vec2 direction) {
	vec3 color = fetchHigh(image, uv).rgb * weight[0];
	for(int i = 1; i < 9; i++) {
		color += fetchHigh(image, uv + direction * offset[i]).rgb * weight[i];
		color += fetchHigh(image, uv - direction * offset[i]).rgb * weight[i];
	}
	return color;
}

float rand(float x){
    return fract(sin(dot(vec2(x), vec2(12.9898,78.233))) * 43758.5453);
}

float hash(vec2 pos) {
	vec2 i = floor(pos);
	vec2 f = fract(pos);
	vec2 u = f * f * (3.0 - 2.0 * f);
	float n = i.x + i.y * 57.0;
	return mix(mix(rand(n + 0.0) , rand(n + 1.0),  u.x), 
	           mix(rand(n + 57.0), rand(n + 58.0), u.x), u.y);
}

vec2 rotateDirection(vec2 angle, vec2 rotation) {
	return vec2(angle.x * rotation.x - angle.y * rotation.y,
                angle.x * rotation.y + angle.y * rotation.x);
}

vec2 CalculateNoisePattern(vec2 offset, float size) {
	vec2 coord = TexCoords.st;
		 coord *= vec2(viewWidth, viewHeight);
		 coord  = mod(coord + offset, vec2(size));
		 coord /= vec2(viewWidth, viewHeight);

	return vec2(hash(coord.st), hash(coord.ts + vec2(1.0f)));
}

vec4 GetViewSpacePosition(in vec2 coord) {
	vec4 fragpos = projectioninverse * vec4(coord * 2.0f - 1.0f, texture(depthTexture, coord).x * 2.0f - 1.0f, 1.0f);
	return fragpos / fragpos.w;
}

#define pi 3.1415926

float getAO(in vec2 coord, in vec3 viewNormal) {

	const int numSampleSteps     = 6;
	const int samplingDirections = 4;
	const float tangentBias      = 0.2;
	const float samplingStep     = 0.001;
	const float samplingRadius   = 0.6;

	float total = 0.0;
	float sampleDirectionIncrement = 2.0 * 3.1415926 / float(samplingDirections);

	vec3 viewPosition   = GetViewSpacePosition(coord).xyz;
	vec2 randomRotation = CalculateNoisePattern(vec2(0.0f), 3.0).xy;

	for(int i = 0; i < samplingDirections; i++) {
	
		//Holding off jittering as long as possible.
		float samplingAngle = i * sampleDirectionIncrement; //Azimuth angle theta in the paper.

		//Random Rotation Defined by NVIDIA
		vec2 sampleDirection = rotateDirection(vec2(cos(samplingAngle), sin(samplingAngle)), randomRotation.xy);
		sampleDirection.y = -sampleDirection.y;

		float tangentAngle = acos(dot(vec3(sampleDirection, 0.0), viewNormal)) - (0.5 * pi) + tangentBias;
		float horizonAngle = tangentAngle;
		vec3 lastDiff = vec3(0.0);

		for(int j = 0; j < numSampleSteps; j++) {
			//Marching Time
			vec2 sampleOffset = float(j + 1) * samplingStep * sampleDirection;
			vec2 offset = coord + sampleOffset;

			vec3 offsetViewPosition = GetViewSpacePosition(offset).xyz;
			vec3 differential = offsetViewPosition - viewPosition;

			if(length(differential) < samplingRadius) { //skip samples outside local sample space
				lastDiff = differential;
				float elevationAngle = atan(differential.z / length(differential.xy));
				horizonAngle = max(horizonAngle, elevationAngle);
			}
		}
		
		float normalDiff = length(lastDiff) / samplingRadius;
		float atten = 1.0 - pow(normalDiff, 2);

		float AO = clamp(atten * (sin(horizonAngle) - sin(tangentAngle)), 0.0, 1.0);
		total += 1.0 - AO;
		
	}
	
	total /= samplingDirections;
	return total;

}

void main() {
	vec3 N = texture(normalTexture, TexCoords.st).rgb * 2.0f - 1.0f;
    FragColor0 = vec4(texture(screenTexture, TexCoords.st).rgb, getAO(TexCoords.st, N));
	FragColor1 = vec4(pow(blur(screenTexture, TexCoords.st, vec2(0.0, 1.0) / vec2(viewWidth, viewHeight)), vec3(1.0f / 2.2f)), 1.0f);
}