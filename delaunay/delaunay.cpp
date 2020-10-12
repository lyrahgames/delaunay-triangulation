#include <SFML/Graphics.hpp>
#include <iostream>
#include <random>
#include <vector>

struct point {
  float x, y;
};

struct triangle {
  size_t pid[3];
};

struct circle {
  point center;
  float radius;
};

int main() {
  using namespace std;

  mt19937 rng{random_device{}()};
  uniform_real_distribution<float> dist{-1, 1};

  constexpr size_t point_count = 4;
  vector<point> points(point_count);
  for (auto& p : points) {
    p.x = dist(rng);
    p.y = dist(rng);
  }

  vector<triangle> triangles{{0, 1, 2}};

  const auto circumcircle = [&points](const triangle& t) {
    const point edge1{points[t.pid[1]].x - points[t.pid[0]].x,
                      points[t.pid[1]].y - points[t.pid[0]].y};
    const point edge2{points[t.pid[2]].x - points[t.pid[0]].x,
                      points[t.pid[2]].y - points[t.pid[0]].y};
    const auto d = 2.0f * (edge1.x * edge2.y - edge1.y * edge2.x);
    const auto inv_d = 1.0f / d;
    const auto norm_edge1 = edge1.x * edge1.x + edge1.y * edge1.y;
    const auto norm_edge2 = edge2.x * edge2.x + edge2.y * edge2.y;
    const point center{inv_d * (edge2.y * norm_edge1 - edge1.y * norm_edge2),
                       inv_d * (edge1.x * norm_edge2 - edge2.x * norm_edge1)};
    return circle{
        {center.x + points[t.pid[0]].x, center.y + points[t.pid[0]].y},
        sqrt(center.x * center.x + center.y * center.y)};
  };

  size_t width = 500;
  size_t height = 500;
  float origin_x = 0;
  float origin_y = 0;
  float fov_y = 3.0f;
  float fov_x = fov_y * width / height;
  const auto projection = [&origin_x, &origin_y, &fov_y, &width, &height](
                              float x, float y) {
    const auto scale = height / fov_y;
    return sf::Vector2f((x - origin_x) * scale + width / 2.0f,
                        (y - origin_y) * scale + height / 2.0f);
  };

  sf::RenderWindow window(sf::VideoMode(width, height),
                          "Delaunay Triangulation");
  window.setVerticalSyncEnabled(true);
  vector<sf::Vertex> vertices{};

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      switch (event.type) {
        case sf::Event::Closed:
          window.close();
          break;

        case sf::Event::Resized:
          width = event.size.width;
          height = event.size.height;
          fov_x = fov_y * width / height;
          window.setView(sf::View(sf::FloatRect(0, 0, width, height)));
          break;

        case sf::Event::KeyPressed:
          switch (event.key.code) {
            case sf::Keyboard::Escape:
              window.close();
              break;
          }
          break;
      }
    }

    // Render
    window.clear(sf::Color::White);
    vertices.clear();
    for (const auto& t : triangles) {
      vertices.push_back(
          sf::Vertex(projection(points[t.pid[0]].x, points[t.pid[0]].y),
                     sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(points[t.pid[1]].x, points[t.pid[1]].y),
                     sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(points[t.pid[1]].x, points[t.pid[1]].y),
                     sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(points[t.pid[2]].x, points[t.pid[2]].y),
                     sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(points[t.pid[2]].x, points[t.pid[2]].y),
                     sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(points[t.pid[0]].x, points[t.pid[0]].y),
                     sf::Color::Black));

      const auto c = circumcircle(t);
      const auto radius = c.radius * height / fov_y;
      sf::CircleShape shape(radius);
      shape.setFillColor(sf::Color(0, 0, 0, 0));
      shape.setOutlineColor(sf::Color::Black);
      shape.setOutlineThickness(1.5f);
      shape.setOrigin(radius, radius);
      shape.setPosition(projection(c.center.x, c.center.y));
      window.draw(shape);
    }
    window.draw(vertices.data(), vertices.size(), sf::Lines);

    for (const auto& p : points) {
      constexpr float radius = 2.5f;
      sf::CircleShape shape(radius);
      shape.setFillColor(sf::Color::Black);
      shape.setOrigin(radius, radius);
      shape.setPosition(projection(p.x, p.y));
      window.draw(shape);
    }
    window.display();
  }
}