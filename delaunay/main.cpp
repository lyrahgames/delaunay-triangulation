#include <SFML/Graphics.hpp>
#include <chrono>
#include <delaunay/delaunay.hpp>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

int main() {
  using namespace std;

  mt19937 rng{random_device{}()};
  uniform_real_distribution<float> dist{-1, 1};

  delaunay::triangulation triangulation{};

  size_t width = 800;
  size_t height = 800;
  float origin_x = 0;
  float origin_y = 0;
  float fov_y = 7.0f;
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
  float old_mouse_x = 0;
  float old_mouse_y = 0;
  float mouse_x = 0;
  float mouse_y = 0;

  while (window.isOpen()) {
    // Mouse movement.
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

    // Handle events.
    sf::Event event;
    while (window.pollEvent(event)) {
      switch (event.type) {
        case sf::Event::Closed:
          window.close();
          break;

        case sf::Event::Resized:
          width = event.size.width;
          height = event.size.height;
          window.setView(sf::View(sf::FloatRect(0, 0, width, height)));
          break;

        case sf::Event::MouseWheelMoved:
          fov_y *= exp(-event.mouseWheel.delta * 0.05f);
          fov_y = clamp(fov_y, 1e-6f, 100.f);
          break;

        case sf::Event::MouseButtonPressed:
          if (event.mouseButton.button == sf::Mouse::Right) {
            triangulation.add({mouse_pos_x, mouse_pos_y});
            cout << "triangulation:" << setw(20) << triangulation.points.size()
                 << " points" << setw(20) << triangulation.triangles.size()
                 << " triangles" << '\n';
          }
          break;

        case sf::Event::KeyPressed:
          switch (event.key.code) {
            case sf::Keyboard::Escape:
              window.close();
              break;

            case sf::Keyboard::Space:
              triangulation.add({0.5f * dist(rng) * fov_x + origin_x,
                                 0.5f * dist(rng) * fov_y + origin_y});
              cout << "triangulation:" << setw(20)
                   << triangulation.points.size() << " points" << setw(20)
                   << triangulation.triangles.size() << " triangles" << '\n';
              break;
          }
          break;
      }
    }

    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
      origin_x -= mouse_move_x;
      origin_y -= mouse_move_y;
    }

    // Update view.
    fov_x = fov_y * width / height;

    // Render.
    window.clear(sf::Color::White);

    // Draw hovered triangle.
    for (const auto& t : triangulation.triangles) {
      if (geometry::intersection({{{triangulation.points[t.pid[0]].x,
                                    triangulation.points[t.pid[0]].y},
                                   {triangulation.points[t.pid[1]].x,
                                    triangulation.points[t.pid[1]].y},
                                   {triangulation.points[t.pid[2]].x,
                                    triangulation.points[t.pid[2]].y}}},
                                 {mouse_pos_x, mouse_pos_y})) {
        vertices.clear();
        vertices.push_back(
            sf::Vertex(projection(triangulation.points[t.pid[0]].x,
                                  triangulation.points[t.pid[0]].y),
                       sf::Color(200, 200, 200)));
        vertices.push_back(
            sf::Vertex(projection(triangulation.points[t.pid[1]].x,
                                  triangulation.points[t.pid[1]].y),
                       sf::Color(200, 200, 200)));
        vertices.push_back(
            sf::Vertex(projection(triangulation.points[t.pid[2]].x,
                                  triangulation.points[t.pid[2]].y),
                       sf::Color(200, 200, 200)));
        window.draw(vertices.data(), vertices.size(), sf::Triangles);

        // Draw circumcircle.
        const auto c =
            geometry::circumcircle({{{triangulation.points[t.pid[0]].x,
                                      triangulation.points[t.pid[0]].y},
                                     {triangulation.points[t.pid[1]].x,
                                      triangulation.points[t.pid[1]].y},
                                     {triangulation.points[t.pid[2]].x,
                                      triangulation.points[t.pid[2]].y}}});
        const float radius = c.radius / scale;
        sf::CircleShape shape(radius);
        shape.setFillColor(sf::Color(0, 0, 0, 0));
        shape.setOrigin(radius, radius);
        shape.setPosition(projection(c.center.x, c.center.y));
        shape.setOutlineThickness(3.0f);
        shape.setOutlineColor(sf::Color::Red);
        shape.setPointCount(1000);
        window.draw(shape);

        break;
      }
    }

    // Draw wireframe of all triangles.
    vertices.clear();
    for (const auto& t : triangulation.triangles) {
      vertices.push_back(
          sf::Vertex(projection(triangulation.points[t.pid[0]].x,
                                triangulation.points[t.pid[0]].y),
                     sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(triangulation.points[t.pid[1]].x,
                                triangulation.points[t.pid[1]].y),
                     sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(triangulation.points[t.pid[1]].x,
                                triangulation.points[t.pid[1]].y),
                     sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(triangulation.points[t.pid[2]].x,
                                triangulation.points[t.pid[2]].y),
                     sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(triangulation.points[t.pid[2]].x,
                                triangulation.points[t.pid[2]].y),
                     sf::Color::Black));
      vertices.push_back(
          sf::Vertex(projection(triangulation.points[t.pid[0]].x,
                                triangulation.points[t.pid[0]].y),
                     sf::Color::Black));
    }
    window.draw(vertices.data(), vertices.size(), sf::Lines);

    // Draw all points.
    for (const auto& p : triangulation.points) {
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