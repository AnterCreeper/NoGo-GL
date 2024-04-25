#version 330
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

out vec4 fragPos;
out vec2 TexCoord;
out vec3 aNormal;

out mat3 TBN;

uniform vec3 camPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {

    gl_Position = projection * view * model * vec4(aPos, 1.0);
    fragPos = model * vec4(aPos, 1.0);

    TexCoord = aTexCoord;
    aNormal = normal;
    
    vec3 T = normalize(vec3(model * vec4(tangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(normal, 0.0)));
 
    // re-orthogonalize T with respect to N
         T = normalize(T - dot(T, N) * N);
    // then retrieve perpendicular vector B with the cross product of T and N
    vec3 B = cross(T, N);

    TBN = mat3(T.x, B.x, N.x,
               T.y, B.y, N.y,
               T.z, B.z, N.z);
               
}