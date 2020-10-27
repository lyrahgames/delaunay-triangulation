#include <fstream>
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
#include <delaunay/geometry.hpp>
//
extern "C" {
#include "stb_image.h"
}

struct vertex {
  float x, y;
  float r, g, b;
  float u, v;
};

struct accum {
  float color[4]{};
  unsigned int count{};
};

float width = 500;
float height = 500;
glm::vec2 fov{width / height, 1.0f};

int main(int argc, char** argv) {
  using namespace std;

  int image_w, image_h, image_channels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char* image_data =
      stbi_load(argv[1], &image_w, &image_h, &image_channels, 0);
  if (image_data) {
    cout << "resolution = " << image_w << " x " << image_h << " x "
         << image_channels << "\n";
  } else {
    throw runtime_error("Could not load the given image!");
  }

  width = image_w;
  height = image_h;
  fov.x = width / height;

  mt19937 rng{random_device{}()};
  uniform_real_distribution<float> dist{0, 1};

  constexpr size_t samples = 1000;
  vector<glm::vec2> points(samples);
  for (auto& p : points) p = glm::vec2{fov.x * dist(rng), dist(rng)};
  points[0].x = 0;
  points[0].y = 0;
  points[1].x = fov.x;
  points[1].y = 0;
  points[2].x = fov.x;
  points[2].y = 1;
  points[3].x = 0;
  points[3].y = 1;

  delaunay::triangulation triangulation{};
  const auto elements = triangulation.triangle_data(points);

  vector<accum> accum_buffer(elements.size() / 3);
  for (int i = 0; i < image_h; ++i) {
    for (int j = 0; j < image_w; ++j) {
      const auto x = float(j) / (image_w - 1) * fov.x + 0.5f / image_h;
      const auto y = float(i) / (image_h - 1) + 0.5f / image_h;

      size_t index = 0;
      for (size_t k = 0; k < elements.size(); k += 3, ++index) {
        if (geometry::intersection(
                {{{points[elements[k + 0]].x, points[elements[k + 0]].y},
                  {points[elements[k + 1]].x, points[elements[k + 1]].y},
                  {points[elements[k + 2]].x, points[elements[k + 2]].y}}},
                {x, y})) {
          break;
        }
      }
      if (index == elements.size() / 3) continue;

      accum_buffer[index].count += 1;
      for (int k = 0; k < image_channels; ++k) {
        accum_buffer[index].color[k] +=
            float(image_data[image_channels * (i * image_w + j) + k]) / 255.0f;
      }
    }
  }

  vector<vertex> vertices(elements.size());
  int index = 0;
  for (size_t i = 0; i < vertices.size(); i += 3, ++index) {
    for (size_t j = 0; j < 3; ++j) {
      vertices[i + j].x = points[elements[i + j]].x;
      vertices[i + j].y = points[elements[i + j]].y;

      vertices[i + j].r =
          accum_buffer[index].color[0] / accum_buffer[index].count;
      vertices[i + j].g =
          accum_buffer[index].color[1] / accum_buffer[index].count;
      vertices[i + j].b =
          accum_buffer[index].color[2] / accum_buffer[index].count;

      vertices[i + j].u = vertices[i + j].x / fov.x;
      vertices[i + j].v = vertices[i + j].y;
    }
  }

  // Generate SVG.
  fstream svg_file{"output.svg", ios::out};
  svg_file << "<svg height=\"" << image_h << "\" width=\"" << image_w << "\">";
  index = 0;
  for (size_t i = 0; i < vertices.size(); i += 3, ++index) {
    svg_file << "<polygon points=\"";
    for (size_t j = 0; j < 3; ++j) {
      svg_file << (vertices[i + j].x * image_h) << ','
               << ((1.0f - vertices[i + j].y) * image_h) << ' ';
    }
    svg_file << "\" style=\"fill:rgb(" << (vertices[i].r * 255.0f) << ", "
             << (vertices[i].g * 255.0f) << ", " << (vertices[i].b * 255.0f)
             << ");stroke:rgb(" << (vertices[i].r * 255.0f) << ", "
             << (vertices[i].g * 255.0f) << ", " << (vertices[i].b * 255.0f)
             << ");stroke-width:0.1\" />";
  }
  svg_file << "</svg>" << flush;

  // Run the program.
  glfwSetErrorCallback([](int error, const char* description) {
    throw runtime_error{"GLFW Error " + to_string(error) + ": " + description};
  });
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  auto window = glfwCreateWindow(width, height, "Delaunay Image Tessellation",
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

  // GLuint texture;
  // glGenTextures(1, &texture);
  // glBindTexture(GL_TEXTURE_2D, texture);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // switch (image_channels) {
  //   case 3:
  //     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_w, image_h, 0, GL_RGB,
  //                  GL_UNSIGNED_BYTE, image_data);
  //     break;

  //   case 4:
  //     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_w, image_h, 0, GL_RGBA,
  //                  GL_UNSIGNED_BYTE, image_data);
  //     break;
  // }
  // glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(image_data);

  auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  const char* vertex_shader_text =
      "#version 330\n"
      "uniform mat4 MVP;"
      "attribute vec2 vPos;"
      "attribute vec3 vCol;"
      "attribute vec2 vTex;"
      "out vec3 color;"
      "out vec2 texuv;"
      "void main() {"
      "  gl_Position = MVP * vec4(vPos, 0.0, 1.0);"
      "  color = vCol;"
      "  texuv = vTex;"
      "}";
  glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
  glCompileShader(vertex_shader);
  auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  const char* fragment_shader_text =
      "#version 330\n"
      "in vec3 color;"
      "in vec2 texuv;"
      // "uniform sampler2D tex;"
      "void main() {"
      "  gl_FragColor = vec4(color,1.0);"
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
  auto vtex_location = glGetAttribLocation(program, "vTex");

  glEnableVertexAttribArray(vpos_location);
  glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(vertex),
                        (void*)0);

  glEnableVertexAttribArray(vcol_location);
  glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),
                        (void*)8);

  glEnableVertexAttribArray(vtex_location);
  glVertexAttribPointer(vtex_location, 2, GL_FLOAT, GL_FALSE, sizeof(vertex),
                        (void*)20);

  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glPointSize(5.0f);
  glLineWidth(0.5f);

  // glBindTexture(GL_TEXTURE_2D, texture);

  while (!glfwWindowShouldClose(window)) {
    glViewport(0, 0, width, height);
    glm::mat4x4 m{1.0f};
    const auto v = glm::translate(glm::mat4(1.0f), {0, 0, -1});
    const auto p = glm::ortho(0.0f, fov.x, 0.0f, 1.0f, 0.1f, 100.f);
    glm::mat4 mvp = p * v * m;

    glClear(GL_COLOR_BUFFER_BIT);

    // Draw triangle interior.
    glUseProgram(program);
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    // Draw triangle lines.
    // glUseProgram(line_program);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
    // glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
}