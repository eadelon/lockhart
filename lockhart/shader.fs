#version 330 core

in vec3 normal0;
out vec4 color;

struct light {
    vec3 color;
    vec3 direction;
    float ambient_intensity;
    float diffuse_intensity;
};

void main() {
    light l;
    l.color = vec3(1.f, 1.f, 1.f);
    l.direction = vec3(-300.f, -500.f, -300.f);
    l.ambient_intensity = 0.1f;
    l.diffuse_intensity = 0.9f;

    vec4 ambiant_color = vec4(l.color, 1.f) * l.ambient_intensity;

    float diffuse = dot(normalize(normal0), normalize(-l.direction));
    vec4 diffuse_color;

    if (diffuse > 0) {
        diffuse_color = vec4(l.color * l.diffuse_intensity * diffuse, 1.f);
    }
    else {
        diffuse_color = vec4(0.f, 0.f, 0.f, 0.f);
    }

    color = vec4(0.8f, 0.8f, 0.8f, 1.f) * (ambiant_color + diffuse_color);
}
