#version 330 core
layout (location = 0) in vec2 aTexCoords;

out vec2 TexCoords;

void main() {
    gl_Position = vec4(aTexCoords * 2.0f - 1.0f, 0.0, 1.0); 
    TexCoords = aTexCoords;
}