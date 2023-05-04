#include <fstream>
#include <iostream>
#include <vector>

#include <SDL3/SDL.h>

#include <gtc/matrix_transform.hpp>

#include "glad.h"

struct vertex {
	glm::vec3 position;
	glm::vec3 normal;
};

struct camera {
	glm::vec3 target;
	float r;
	float t;
	float p;

	int old_x = 0;
	int old_y = 0;

	void update() {
		float x;
		float y;
		auto state = SDL_GetMouseState(&x, &y);

		if (state & SDL_BUTTON_LEFT) {

			int dx = x - old_x;
			int dy = y - old_y;

			t += dx;
			if (p > 359.f) {
				p = 359.f;
			}
			if (p < -0.f) {
				p = 0.f;
			}

			p += dy;
			if (p > 179.f) {
				p = 179.f;
			}
			if (p < -0) {
				p = 1.f;
			}
		}

		old_x = x;
		old_y = y;

	}
};

glm::vec3 position(const camera& camera) {
	float x = camera.r * std::sin(glm::radians(camera.p)) * std::cos(glm::radians(camera.t));
	float y = camera.r * std::sin(glm::radians(camera.p)) * std::sin(glm::radians(camera.t));
	float z = camera.r * std::cos(glm::radians(camera.p));

	return glm::vec3(x, z, y);
}

std::string read(const std::string& filename) {
	std::ifstream stream(filename);
	if (!stream) {
		return "";
	}

	std::string data;
	stream.seekg(0, std::ios::end);
	data.resize(stream.tellg());
	stream.seekg(0, std::ios::beg);
	stream.read(&data[0], data.size());
	return data;
}

std::vector<float> read_map(const std::string& filename) {
	std::ifstream stream(filename, std::ios::binary);
	if (!stream) {
		return std::vector<float>{};
	}

	std::vector<float> buffer(1024 * 1024, 0);
	stream.seekg(0x3c);
	stream.read((char*)buffer.data(), buffer.size() * sizeof(float));

	return buffer;
}

int main(int argc, char* argv[]) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << SDL_GetError();
		return -1;
	}

	int ww = 800;
	int wh = 600;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_Window* window = SDL_CreateWindow("Lockhart", ww, wh, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (window == nullptr) {
		std::cout << SDL_GetError();
		return -1;
	}

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	if (gl_context == nullptr) {
		std::cout << SDL_GetError();
		return -1;
	}

	if (!gladLoadGL()) {
		std::cout << "Failed to initialize OpenGL context";
		return -1;
	}

	std::vector<glm::vec3> vertices(1024 * 1024, glm::vec3(0.f, 0.f, 0.f));

	auto data = read_map("Elevation.ddc");

	for (int i = 0; i < 1024; i++) {
		for (int j = 0; j < 1024; j++) {
			vertices[1024 * i + j] = glm::vec3(2.5f * (i - 512), data[1024 * i + j], 2.5f * (j - 512));
		}
	}

	std::vector<vertex> x(vertices.size() * 6);

	for (int i = 0; i < 1024 - 1; i++) {
		for (int j = 0; j < 1024 - 1; j++) {
			vertex v1;
			vertex v2;
			vertex v3;
			vertex v4;

			v1.position = vertices[1024 * i + j];
			v2.position = vertices[1024 * i + (j + 1)];
			v3.position = vertices[1024 * (i + 1) + j];
			v4.position = vertices[1024 * (i + 1) + (j + 1)];

			// First triangle.
			glm::vec3 v = v2.position - v1.position;
			glm::vec3 w = v3.position - v1.position;
			glm::vec3 n = glm::vec3(v.y * w.z - v.z * w.y, v.z * w.x - v.x * w.z, v.x * w.y - v.y * w.z);
			v1.normal = n;
			v2.normal = n;
			v3.normal = n;

			x.push_back(v1);
			x.push_back(v2);
			x.push_back(v3);

			// Second triangle.
			v = v4.position - v3.position;
			w = v2.position - v3.position;
			n = glm::vec3(v.y * w.z - v.z * w.y, v.z * w.x - v.x * w.z, v.x * w.y - v.y * w.z);
			v3.normal = -n;
			v4.normal = -n;
			v2.normal = -n;

			x.push_back(v3);
			x.push_back(v4);
			x.push_back(v2);
		}
	}

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, x.size() * sizeof(vertex), x.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (const void*)12);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
	glBindVertexArray(1);

	std::string vert_data = read("shader.vs");
	std::string frag_data = read("shader.fs");

	const char* vert_src = vert_data.c_str();
	const char* frag_src = frag_data.c_str();

	GLuint vert_shader_program = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &vert_src);

	GLint status = 0;
	glGetProgramiv(vert_shader_program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLint length = 0;
		glGetProgramiv(vert_shader_program, GL_INFO_LOG_LENGTH, &length);
		std::string message;
		message.resize(length);
		glGetProgramInfoLog(vert_shader_program, length, &length, &message[0]);
		std::cout << message;
	}

	GLuint frag_shader_program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &frag_src);

	glGetProgramiv(frag_shader_program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLint length = 0;
		glGetProgramiv(frag_shader_program, GL_INFO_LOG_LENGTH, &length);
		std::string message;
		message.resize(length);
		glGetProgramInfoLog(frag_shader_program, length, &length, &message[0]);
		std::cout << message;
	}

	GLuint program_pipeline;
	glGenProgramPipelines(1, &program_pipeline);
	glUseProgramStages(program_pipeline, GL_VERTEX_SHADER_BIT, vert_shader_program);
	glUseProgramStages(program_pipeline, GL_FRAGMENT_SHADER_BIT, frag_shader_program);

	camera c;
	c.p = 70.f;
	c.t = -200.f;
	c.r = 2000.f;
	c.target = glm::vec3(0.f, 0.f, 0);

	glm::mat4 projection = glm::perspective(glm::radians(45.f), (float)ww / (float)wh, 1.f, 10000.f);
	glm::mat4 view = glm::lookAt(position(c), c.target, glm::vec3(0, 1, 0));
	glm::mat4 model = glm::mat4(1.f);
	glm::mat4 mvp = projection * view * model;

	GLuint mvp_id = glGetUniformLocation(program_pipeline, "mvp");
	GLuint view_id = glGetUniformLocation(program_pipeline, "view");

	SDL_Event event;
	bool is_running = true;

	glEnable(GL_DEPTH_TEST);

	while (is_running) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				is_running = false;
			}
			else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
				ww = event.window.data1;
				wh = event.window.data2;
				std::cout << ww << " " << wh << "\n";
				projection = glm::perspective(glm::radians(45.f), (float)ww / (float)wh, 1.f, 10000.f);
				glViewport(0, 0, ww, wh);
			}
		}

		c.update();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		view = glm::lookAt(position(c), c.target, glm::vec3(0, 1, 0));
		mvp = projection * view * model;
		glProgramUniformMatrix4fv(program_pipeline, mvp_id, 1, GL_FALSE, &mvp[0][0]);
		glProgramUniformMatrix4fv(program_pipeline, view_id, 1, GL_FALSE, &projection[0][0]);
		glBindProgramPipeline(program_pipeline);

		glBindVertexArray(vao);

		glDrawArrays(GL_TRIANGLES, 0, x.size());

		glBindVertexArray(0);

		SDL_GL_SwapWindow(window);
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
