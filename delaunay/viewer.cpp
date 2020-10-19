#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
using namespace gl;
//
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
//
#include <glm/glm.hpp>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
//
#include <glm/ext.hpp>
//
#include <delaunay/delaunay.hpp>

float width = 800;
float height = 450;
glm::vec2 fov{M_PI_4 * width / height, M_PI_4};
glm::vec3 origin{};
glm::vec3 up{0, 1, 0};
glm::vec3 camera{10, 0.0f, M_PI_2};

int main(void) {
  using namespace std;

  // Generate random points in sphere.
  mt19937 rng{random_device{}()};
  uniform_real_distribution<float> dist{0.0f, 1.0f};
  const size_t samples = 10000;
  vector<glm::vec3> points(samples);
  for (auto& p : points) {
    const auto u = 2 * dist(rng) - 1;
    const auto phi = 2 * M_PI * dist(rng);
    // const auto r = pow(dist(rng), 1 / 3.0f);
    const auto r = 1.0f;
    const auto v = sqrt(1 - u * u);
    p = r * glm::vec3{cos(phi) * v, sin(phi) * v, u};
    const auto rr = dist(rng);
    if (rr <= 0.25f)
      p += glm::vec3{3, 1.5, 1.5};
    else if (rr <= 0.5f)
      p += glm::vec3{1.5, 3, 1.5};
    else if (rr <= 0.75f)
      p += glm::vec3{1.5, 1.5, 3};
    else
      p += glm::vec3{2.45, 2.45, 2.45};
  }

  // Compute pareto front.
  vector<glm::vec3> pareto_points{};
  for (const auto& p : points) {
    bool pareto_dominated = false;
    for (const auto& q : points) {
      if (pareto_dominated = ((q.x <= p.x) && (q.y <= p.y) && (q.z <= p.z) &&
                              ((q.x < p.x) || (q.y < p.y) || (q.z < p.z))))
        break;
    }
    if (!pareto_dominated) pareto_points.push_back(p);
  }

  // Project points to plane with normal = (1,1,1) and get Delaunay points.
  glm::vec3 pu{1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f};
  glm::vec3 pv{-1.0f / sqrt(6.0f), -1.0f / sqrt(6.0f), 2.0f / sqrt(6.0f)};
  vector<glm::vec3> projected_points = pareto_points;
  vector<delaunay::point> delaunay_points{};
  for (auto& p : projected_points) {
    p = dot(pu, p) * pu + dot(pv, p) * pv;
    delaunay_points.push_back({dot(pu, p), dot(pv, p)});
  }

  // Triangulate Pareto front.
  delaunay::triangulation triangulation{};
  auto elements = triangulation.triangle_data(delaunay_points);

  float mean_distance = 0;
  for (uint32_t i = 0; i < elements.size(); i += 3) {
    mean_distance +=
        length(pareto_points[elements[i]] - pareto_points[elements[i + 1]]);
    mean_distance +=
        length(pareto_points[elements[i + 1]] - pareto_points[elements[i + 2]]);
    mean_distance +=
        length(pareto_points[elements[i + 2]] - pareto_points[elements[i]]);
  }
  mean_distance /= elements.size();
  cout << "mean distance = " << mean_distance << '\n';

  constexpr auto sq = [](auto&& x) { return x * x; };

  float var_distance = 0;
  for (uint32_t i = 0; i < elements.size(); i += 3) {
    var_distance +=
        sq(length(pareto_points[elements[i]] - pareto_points[elements[i + 1]]) -
           mean_distance);
    var_distance += sq(length(pareto_points[elements[i + 1]] -
                              pareto_points[elements[i + 2]]) -
                       mean_distance);
    var_distance +=
        sq(length(pareto_points[elements[i + 2]] - pareto_points[elements[i]]) -
           mean_distance);
  }
  var_distance /= elements.size();
  cout << "var distance = " << var_distance << '\n'
       << "stddev distance = " << sqrt(var_distance) << '\n';

  vector<uint32_t> tmp{};
  tmp.swap(elements);
  for (uint32_t i = 0; i < tmp.size(); i += 3) {
    if (max(max(length(pareto_points[tmp[i]] - pareto_points[tmp[i + 1]]),
                length(pareto_points[tmp[i + 1]] - pareto_points[tmp[i + 2]])),
            length(pareto_points[tmp[i + 2]] - pareto_points[tmp[i]])) <
        mean_distance + 2.0f * sqrt(var_distance)) {
      elements.push_back(tmp[i]);
      elements.push_back(tmp[i + 1]);
      elements.push_back(tmp[i + 2]);
    }
  }

  glfwSetErrorCallback([](int error, const char* description) {
    throw runtime_error{"GLFW Error " + to_string(error) + ": " + description};
  });

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);

  auto window = glfwCreateWindow(width, height, "Delaunay Triangulation Viewer",
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
  glfwSetScrollCallback(window, [](GLFWwindow* window, double x, double y) {
    camera.x *= exp(-0.1f * float(y));
    // camera = exp(-0.1f * float(y)) * (camera - origin) + origin;
  });

  GLuint vertex_array;
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);

  GLuint element_buffer;
  glGenBuffers(1, &element_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(uint32_t),
               elements.data(), GL_STATIC_DRAW);

  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * points.size(),
               points.data(), GL_STATIC_DRAW);

  auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  const char* vertex_shader_text =
      "#version 330\n"
      "uniform mat4 MVP;"
      "attribute vec3 vPos;"
      "void main() {"
      "  gl_Position = MVP * vec4(vPos, 1.0);"
      "}";
  glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
  glCompileShader(vertex_shader);
  auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  const char* fragment_shader_text =
      "#version 330\n"
      "void main() {"
      "  gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);"
      "}";
  glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
  glCompileShader(fragment_shader);
  auto program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  auto mvp_location = glGetUniformLocation(program, "MVP");
  auto vpos_location = glGetAttribLocation(program, "vPos");

  glEnableVertexAttribArray(vpos_location);
  glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                        (void*)0);

  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glPointSize(5.0f);
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_POINT_SPRITE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glm::vec2 old_mouse_pos{};
  glm::vec2 mouse_pos{};

  while (!glfwWindowShouldClose(window)) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    old_mouse_pos = mouse_pos;
    mouse_pos = glm::vec2(xpos, ypos);

    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (state == GLFW_PRESS) {
      const auto mouse_move = mouse_pos - old_mouse_pos;
      camera.z += mouse_move.x * 0.01;
      camera.y += mouse_move.y * 0.01;
      const constexpr float eye_altitude_max_abs = M_PI_2 - 0.0001f;
      camera.y = clamp(camera.y, -eye_altitude_max_abs, eye_altitude_max_abs);
    }

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    glm::mat4x4 m{1.0f};
    // m = rotate(m, (float)glfwGetTime(), glm::vec3(1, 1, 1));
    const auto v = glm::lookAt(
        origin + camera.x * glm::vec3{cos(camera.y) * cos(camera.z),  //
                                      sin(camera.y),                  //
                                      cos(camera.y) * sin(camera.z)},
        origin, up);
    const auto p = glm::perspective(fov.y, width / height, 0.1f, 100.f);
    glm::mat4 mvp = p * v * m;

    glUseProgram(program);
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));

    glPointSize(1.0f);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * points.size(),
                 points.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_POINTS, 0, points.size());

    glPointSize(5.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * pareto_points.size(),
                 pareto_points.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_POINTS, 0, pareto_points.size());
    glDrawElements(GL_TRIANGLES, elements.size(), GL_UNSIGNED_INT, nullptr);

    glPointSize(3.0f);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * projected_points.size(),
                 projected_points.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_POINTS, 0, projected_points.size());
    glDrawElements(GL_TRIANGLES, elements.size(), GL_UNSIGNED_INT, nullptr);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
}