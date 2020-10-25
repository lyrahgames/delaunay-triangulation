#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
//
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
using namespace gl;
//
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
//
#include <glm/glm.hpp>
//
#include <glm/ext.hpp>
//
#include <delaunay/delaunay.hpp>

struct vertex {
  float x, y;
  float r, g, b;
};

float width = 500;
float height = 500;
glm::vec2 fov{0.5f * width / height, 0.5f};

int main() {
  using namespace std;

  mt19937 rng{random_device{}()};
  uniform_real_distribution<float> dist{-1, 1};

  constexpr size_t samples = 2000;
  vector<glm::vec2> points(samples);
  for (auto& p : points) p = glm::vec2{dist(rng), dist(rng)};
  points[0].x = -1;
  points[0].y = -1;
  points[1].x = 1;
  points[1].y = -1;
  points[2].x = 1;
  points[2].y = 1;
  points[3].x = -1;
  points[3].y = 1;

  delaunay::triangulation triangulation{};
  const auto elements = triangulation.triangle_data(points);
  // vector<uint32_t> elements{0, 1, 2, 0, 1, 3};

  std::vector<vertex> vertices(elements.size());
  for (size_t i = 0; i < vertices.size(); i += 3) {
    // const auto r = 0.5 * (dist(rng) + 1);
    // const auto g = 0.5 * (dist(rng) + 1);
    // const auto b = 0.5 * (dist(rng) + 1);
    const auto b = (dist(rng) + 2) / 3;
    for (size_t j = 0; j < 3; ++j) {
      vertices[i + j].x = points[elements[i + j]].x;
      vertices[i + j].y = points[elements[i + j]].y;
      vertices[i + j].r = 0.3 * b;
      vertices[i + j].g = 0.6 * b;
      vertices[i + j].b = b;
    }
  }

  // Run the program.
  glfwSetErrorCallback([](int error, const char* description) {
    throw runtime_error{"GLFW Error " + to_string(error) + ": " + description};
  });
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  auto window = glfwCreateWindow(width, height, "Delaunay Triangulation Mosaic",
                                 nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glbinding::initialize(glfwGetProcAddress);
  glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int w, int h) {
    width = w;
    height = h;
    fov.x = fov.y * width / height;
  });
  glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode,
                                int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
      glfwSetWindowShouldClose(window, GLFW_TRUE);
  });

  GLuint vertex_array;
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);

  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * vertices.size(),
               vertices.data(), GL_STATIC_DRAW);

  auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  const char* vertex_shader_text =
      "#version 330\n"
      "uniform mat4 MVP;"
      "attribute vec2 vPos;"
      "attribute vec3 vCol;"
      "out vec3 color;"
      "void main() {"
      "  gl_Position = MVP * vec4(vPos, 0.0, 1.0);"
      "  color = vCol;"
      "}";
  glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
  glCompileShader(vertex_shader);
  auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  const char* fragment_shader_text =
      "#version 330\n"
      "in vec3 color;"
      "void main() {"
      "  gl_FragColor = vec4(color.x, color.y, color.z, 1.0);"
      "}";
  glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
  glCompileShader(fragment_shader);
  auto program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  auto line_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  const char* line_fragment_shader_text =
      "#version 330\n"
      "in vec3 color;"
      "void main() {"
      "  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);"
      "}";
  glShaderSource(line_fragment_shader, 1, &line_fragment_shader_text, NULL);
  glCompileShader(line_fragment_shader);
  auto line_program = glCreateProgram();
  glAttachShader(line_program, vertex_shader);
  glAttachShader(line_program, line_fragment_shader);
  glLinkProgram(line_program);

  auto mvp_location = glGetUniformLocation(program, "MVP");
  auto vpos_location = glGetAttribLocation(program, "vPos");
  auto vcol_location = glGetAttribLocation(program, "vCol");

  glEnableVertexAttribArray(vpos_location);
  glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(vertex),
                        (void*)0);

  glEnableVertexAttribArray(vcol_location);
  glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),
                        (void*)8);

  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glPointSize(5.0f);
  glLineWidth(1.2f);

  while (!glfwWindowShouldClose(window)) {
    glViewport(0, 0, width, height);
    glm::mat4x4 m{1.0f};
    const auto v = glm::translate(glm::mat4(1.0f), {0, 0, -1});
    const auto p = glm::ortho(-0.5f * fov.x, 0.5f * fov.x, -0.5f * fov.y,
                              0.5f * fov.y, 0.1f, 100.f);
    glm::mat4 mvp = p * v * m;

    glClear(GL_COLOR_BUFFER_BIT);

    // Draw triangle interior.
    glUseProgram(program);
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    // Draw triangle lines.
    glUseProgram(line_program);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
}