#include <SFML/Graphics.hpp>
#include <iostream>
#include <random>
#include <vector>

struct point {
  float x, y;
};

int main() {
  using namespace std;

  mt19937 rng{random_device{}()};
  uniform_real_distribution<float> dist{-1, 1};

  constexpr size_t point_count = 505;
  vector<point> points(point_count);
  for (auto& p : points) {
    p.x = dist(rng);
    p.y = dist(rng);
  }

  size_t width = 500;
  size_t height = 500;
  float origin_x = 0;
  float origin_y = 0;
  float fov_y = 3.0f;
  float fov_x = fov_y * width / height;
  sf::RenderWindow window(sf::VideoMode(width, height),
                          "Delaunay Triangulation");
  window.setVerticalSyncEnabled(true);

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
    for (const auto& p : points) {
      constexpr float radius = 2.5f;
      sf::CircleShape shape(radius);
      shape.setFillColor(sf::Color::Black);
      shape.setOrigin(radius, radius);
      const auto scale = width / fov_x;
      shape.setPosition((p.x - origin_x) * scale + width / 2.0f,
                        (p.y - origin_y) * scale + height / 2.0f);
      window.draw(shape);
    }
    window.display();
  }
}