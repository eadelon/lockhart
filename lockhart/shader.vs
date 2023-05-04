#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 mvp;
uniform mat4 view;

out vec3 normal0;

void main() {
    gl_Position = mvp * vec4(position, 1);
    normal0 = (view * vec4(normal, 0.f)).xyz;
}
