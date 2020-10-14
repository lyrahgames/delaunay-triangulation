#include <SFML/Graphics.hpp>
#include <delaunay/delaunay.hpp>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

int main() {
  vector<geometry::point> points{{0, 0}, {1, 0}, {0, 1}, {1, 1}};

  size_t width = 500;
  size_t height = 500;
  float origin_x = 0;
  float origin_y = 0;
  float fov_y = 10.0f;
  float fov_x = fov_y * width / height;
  const auto projection = [&origin_x, &origin_y, &fov_y, &width, &height](
                              float x, float y) {
    const auto scale = height / fov_y;
    return sf::Vector2f((x - origin_x) * scale + width / 2.0f,
                        (y - origin_y) * scale + height / 2.0f);
  };
  sf::RenderWindow window(sf::VideoMode(width, height),
                          "Interactive Geometry Tests");
  window.setVerticalSyncEnabled(true);

  float old_mouse_x = 0;
  float old_mouse_y = 0;
  float mouse_x = 0;
  float mouse_y = 0;

  size_t select_pid = points.size();

  while (window.isOpen()) {
    const auto mouse_pos = sf::Mouse::getPosition(window);
    old_mouse_x = mouse_x;
    old_mouse_y = mouse_y;
    mouse_x = mouse_pos.x;
    mouse_y = mouse_pos.y;
    const auto scale = fov_y / height;
    const auto mouse_pos_x = (mouse_x - 0.5f * width) * scale + origin_x;
    const auto mouse_pos_y = (mouse_y - 0.5f * height) * scale + origin_y;
    const auto mouse_move_x = scale * (mouse_x - old_mouse_x);
    const auto mouse_move_y = scale * (mouse_y - old_mouse_y);

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

    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
      if (select_pid < points.size()) {
        points[select_pid].x += mouse_move_x;
        points[select_pid].y += mouse_move_y;
      }
    } else {
      // Compute selected point.
      select_pid = 0;
      for (const auto& p : points) {
        const auto mouse_distance =
            sqrt((mouse_pos_x - p.x) * (mouse_pos_x - p.x) +
                 (mouse_pos_y - p.y) * (mouse_pos_y - p.y));
        const auto threshold = scale * 10.0f;
        if (mouse_distance < threshold) break;
        ++select_pid;
      }
    }

    // Render
    window.clear(sf::Color::White);

    const auto draw_triangle = [&](const geometry::triangle& t) {
      vector<sf::Vertex> vertices{};
      vertices.push_back(sf::Vertex(projection(t.vertex[0].x, t.vertex[0].y),
                                    sf::Color::Black));
      vertices.push_back(sf::Vertex(projection(t.vertex[1].x, t.vertex[1].y),
                                    sf::Color::Black));
      vertices.push_back(sf::Vertex(projection(t.vertex[1].x, t.vertex[1].y),
                                    sf::Color::Black));
      vertices.push_back(sf::Vertex(projection(t.vertex[2].x, t.vertex[2].y),
                                    sf::Color::Black));
      vertices.push_back(sf::Vertex(projection(t.vertex[2].x, t.vertex[2].y),
                                    sf::Color::Black));
      vertices.push_back(sf::Vertex(projection(t.vertex[0].x, t.vertex[0].y),
                                    sf::Color::Black));
      window.draw(vertices.data(), vertices.size(), sf::Lines);
    };

    const auto draw_point = [&](const geometry::point& p) {
      constexpr float radius = 3.5f;
      sf::CircleShape shape(radius);
      shape.setFillColor(sf::Color::Black);
      shape.setOrigin(radius, radius);
      shape.setPosition(projection(p.x, p.y));
      window.draw(shape);
    };

    const auto draw_circle = [&](const geometry::circle& c) {
      const float radius = c.radius / scale;
      sf::CircleShape shape(radius);
      shape.setFillColor(sf::Color(0, 0, 0, 0));
      shape.setOrigin(radius, radius);
      shape.setPosition(projection(c.center.x, c.center.y));
      shape.setOutlineThickness(3.0f);
      shape.setOutlineColor(sf::Color::Red);
      shape.setPointCount(1000);
      window.draw(shape);
      // draw_point(c.center);
    };

    const auto draw_aabb = [&](const geometry::aabb& b) {
      vector<sf::Vertex> vertices{};
      vertices.push_back(
          sf::Vertex(projection(b.min.x, b.min.y), sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(b.max.x, b.min.y), sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(b.max.x, b.min.y), sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(b.max.x, b.max.y), sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(b.max.x, b.max.y), sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(b.min.x, b.max.y), sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(b.min.x, b.max.y), sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(b.min.x, b.min.y), sf::Color::Black));
      window.draw(vertices.data(), vertices.size(), sf::Lines);
    };

    const geometry::triangle t{points[0], points[1], points[2]};
    const auto c = geometry::circumcircle(t);
    draw_triangle(t);
    draw_circle(c);
    draw_aabb(geometry::bounding_box(t));
    draw_aabb(geometry::bounding_box(c));
    draw_circle(geometry::circumcircle(geometry::bounding_box(t)));

    // Draw all points.
    for (const auto& p : points) {
      draw_point(p);
    }

    // Draw selected point.
    if (select_pid < points.size()) {
      constexpr float radius = 5.5f;
      sf::CircleShape shape(radius);
      shape.setFillColor(sf::Color(0, 0, 0, 0));
      shape.setOrigin(radius, radius);
      shape.setPosition(projection(points[select_pid].x, points[select_pid].y));
      shape.setOutlineThickness(3.0f);

      if (geometry::intersection(t, points[3])) {
        shape.setOutlineColor(sf::Color::Blue);
      } else if (geometry::circumcircle_intersection(t, points[3])) {
        shape.setOutlineColor(sf::Color::Green);
      } else {
        shape.setOutlineColor(sf::Color::Red);
      }
      window.draw(shape);
    }

    window.display();
  }
}