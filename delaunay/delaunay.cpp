#include <SFML/Graphics.hpp>
#include <iomanip>
#include <iostream>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct point {
  float x, y;
};

struct edge {
  struct hash {
    constexpr size_t operator()(const edge& e) const noexcept {
      return e.pid[0] ^ (e.pid[1] << 1);
    }
  };

  edge(size_t pid1, size_t pid2)
      : pid{std::min(pid1, pid2), std::max(pid1, pid2)} {}

  size_t pid[2];
};

constexpr bool operator==(const edge& e1, const edge& e2) noexcept {
  return (e1.pid[0] == e2.pid[0]) && (e1.pid[1] == e2.pid[1]);
}

struct triangle {
  struct hash {
    constexpr size_t operator()(const triangle& t) const noexcept {
      return t.pid[0] ^ (t.pid[1] << 1) ^ (t.pid[2] << 2);
    }
  };

  triangle(size_t pid0, size_t pid1, size_t pid2) : pid{pid0, pid1, pid2} {
    if (pid[0] > pid[1]) std::swap(pid[0], pid[1]);
    if (pid[1] > pid[2]) std::swap(pid[1], pid[2]);
    if (pid[0] > pid[1]) std::swap(pid[0], pid[1]);
  }

  size_t pid[3];
};

constexpr bool operator==(const triangle& t1, const triangle& t2) noexcept {
  return (t1.pid[0] == t2.pid[0]) && (t1.pid[1] == t2.pid[1]) &&
         (t1.pid[2] == t2.pid[2]);
}

struct circle {
  point center;
  float radius;
};

int main() {
  using namespace std;

  mt19937 rng{random_device{}()};
  uniform_real_distribution<float> dist{-2, 2};

  constexpr size_t point_count = 4;
  vector<point> points(point_count);

  // Set bounding quad.
  points[0] = {-3.0f, -3.0f};
  points[1] = {3.0f, -3.0f};
  points[2] = {3.0f, 3.0f};
  points[3] = {-3.0f, 3.0f};
  unordered_set<triangle, triangle::hash> triangles{{0, 1, 2}, {2, 3, 0}};

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

  const auto intersection = [&points](const triangle& t, size_t pid) {
    const point p{points[pid].x - points[t.pid[0]].x,
                  points[pid].y - points[t.pid[0]].y};
    const point edge1{points[t.pid[1]].x - points[t.pid[0]].x,
                      points[t.pid[1]].y - points[t.pid[0]].y};
    const point edge2{points[t.pid[2]].x - points[t.pid[0]].x,
                      points[t.pid[2]].y - points[t.pid[0]].y};
    const auto d = (edge1.x * edge2.y - edge1.y * edge2.x);
    const auto inv_d = 1.0f / d;
    const auto u = inv_d * (edge2.y * p.x - edge2.x * p.y);
    const auto v = inv_d * (edge1.x * p.y - edge1.y * p.x);
    return ((u >= 0.0f) && (v >= 0.0f) && (u + v <= 1.0f));
  };

  const auto circumcircle_intersection = [&points](const triangle& t,
                                                   const point& p) {
    const auto axdx = points[t.pid[0]].x - p.x;
    const auto aydy = points[t.pid[0]].y - p.y;
    const auto bxdx = points[t.pid[1]].x - p.x;
    const auto bydy = points[t.pid[1]].y - p.y;
    const auto cxdx = points[t.pid[2]].x - p.x;
    const auto cydy = points[t.pid[2]].y - p.y;
    const auto sqsum_a = axdx * axdx + aydy * aydy;
    const auto sqsum_b = bxdx * bxdx + bydy * bydy;
    const auto sqsum_c = cxdx * cxdx + cydy * cydy;
    const auto det = axdx * (bydy * sqsum_c - cydy * sqsum_b) -
                     aydy * (bxdx * sqsum_c - cxdx * sqsum_b) +
                     sqsum_a * (bxdx * cydy - cxdx * bydy);
    const point edge1{points[t.pid[1]].x - points[t.pid[0]].x,
                      points[t.pid[1]].y - points[t.pid[0]].y};
    const point edge2{points[t.pid[2]].x - points[t.pid[0]].x,
                      points[t.pid[2]].y - points[t.pid[0]].y};
    const auto d = (edge1.x * edge2.y - edge1.y * edge2.x);
    return (d * det) > 0.0f;
  };

  // const auto add_point = [&]() {
  //   const auto pid = points.size();
  //   points.push_back({dist(rng), dist(rng)});
  //   triangle nt[3];
  //   for (size_t tid = 0; tid < triangles.size(); ++tid) {
  //     auto& t = triangles[tid];
  //     if (intersection(t, pid)) {
  //       nt[0].pid[0] = t.pid[0];
  //       nt[0].pid[1] = t.pid[1];
  //       nt[0].pid[2] = pid;

  //       nt[0].tnid[0] = t.tnid[0];
  //       nt[0].tnid[1] = (triangles.size() + 0 << 2) | 0b01;
  //       nt[0].tnid[2] = (triangles.size() + 1 << 2) | 0b00;

  //       nt[1].pid[0] = t.pid[1];
  //       nt[1].pid[1] = t.pid[2];
  //       nt[1].pid[2] = pid;

  //       nt[1].tnid[0] = t.tnid[1];
  //       nt[1].tnid[1] = (triangles.size() + 1 << 2) | 0b01;
  //       nt[1].tnid[2] = (tid << 2) | 0b00;

  //       nt[2].pid[0] = t.pid[2];
  //       nt[2].pid[1] = t.pid[0];
  //       nt[2].pid[2] = pid;

  //       nt[2].tnid[0] = t.tnid[2];
  //       nt[2].tnid[1] = (tid << 2) | 0b01;
  //       nt[2].tnid[2] = (triangles.size() << 2) | 0b00;

  //       constexpr int npid_id_buffer[] = {0, 1, 2, 0, 1};

  //       // neighbor 0
  //       if (t.tnid[0] != 0b11) {
  //         const auto npid_id = t.tnid[0] & 0b11;
  //         const auto ntid = t.tnid[0] >> 2;
  //         triangles[ntid].tnid[npid_id_buffer[npid_id + 1]] = (tid << 2) |
  //         0b10; if (circumcircle_intersection(triangles[ntid], pid)) {
  //           // Do the edge flip.
  //           const auto npid = triangles[ntid].pid[npid_id];
  //           triangle ntt;
  //           ntt.pid[0] = pid;
  //           ntt.pid[1] = npid;
  //           ntt.pid[2] = t.pid[1];
  //           ntt.tnid[0] = (tid << 2) | 0b10;
  //           ntt.tnid[1] = triangles[ntid].tnid[npid_id];
  //           ntt.tnid[2] = nt[0].tnid[1];

  //           nt[0].pid[0] = npid;
  //           nt[0].pid[1] = pid;
  //           nt[0].pid[2] = t.pid[0];
  //           nt[0].tnid[0] = (ntid << 2) | 0b10;
  //           nt[0].tnid[1] = nt[0].tnid[2];
  //           nt[0].tnid[2] = triangles[ntid].tnid[(npid_id + 2) % 3];

  //           triangles[ntid] = ntt;
  //         }
  //       }
  //       // neighbor 1
  //       if (t.tnid[1] != 0b11) {
  //         const auto npid_id = t.tnid[1] & 0b11;
  //         const auto ntid = t.tnid[1] >> 2;
  //         triangles[ntid].tnid[npid_id_buffer[npid_id + 1]] =
  //             (triangles.size() << 2) | 0b10;
  //         if (circumcircle_intersection(triangles[ntid], pid)) {
  //           // Do the edge flip.
  //           const auto npid = triangles[ntid].pid[npid_id];
  //           triangle ntt;
  //           ntt.pid[0] = pid;
  //           ntt.pid[1] = npid;
  //           ntt.pid[2] = t.pid[2];
  //           ntt.tnid[0] = (tid << 2) | 0b10;
  //           ntt.tnid[1] = triangles[ntid].tnid[npid_id];
  //           ntt.tnid[2] = nt[1].tnid[1];

  //           nt[1].pid[0] = npid;
  //           nt[1].pid[1] = pid;
  //           nt[1].pid[2] = t.pid[1];
  //           nt[1].tnid[0] = (ntid << 2) | 0b10;
  //           nt[1].tnid[1] = nt[1].tnid[2];
  //           nt[1].tnid[2] = triangles[ntid].tnid[(npid_id + 2) % 3];

  //           triangles[ntid] = ntt;
  //         }
  //       }
  //       // neighbor 2
  //       if (t.tnid[2] != 0b11) {
  //         const auto npid_id = t.tnid[2] & 0b11;
  //         const auto ntid = t.tnid[2] >> 2;
  //         triangles[ntid].tnid[npid_id_buffer[npid_id + 1]] =
  //             ((triangles.size() + 1) << 2) | 0b10;
  //         if (circumcircle_intersection(triangles[ntid], pid)) {
  //           // Do the edge flip.
  //           const auto npid = triangles[ntid].pid[npid_id];
  //           triangle ntt;
  //           ntt.pid[0] = pid;
  //           ntt.pid[1] = npid;
  //           ntt.pid[2] = t.pid[0];
  //           ntt.tnid[0] = (tid << 2) | 0b10;
  //           ntt.tnid[1] = triangles[ntid].tnid[npid_id];
  //           ntt.tnid[2] = nt[2].tnid[1];

  //           nt[2].pid[0] = npid;
  //           nt[2].pid[1] = pid;
  //           nt[2].pid[2] = t.pid[2];
  //           nt[2].tnid[0] = (ntid << 2) | 0b10;
  //           nt[2].tnid[1] = nt[2].tnid[2];
  //           nt[2].tnid[2] = triangles[ntid].tnid[(npid_id + 2) % 3];

  //           triangles[ntid] = ntt;
  //         }
  //       }

  //       t = nt[0];
  //       break;
  //     }
  //   }
  //   triangles.push_back(nt[1]);
  //   triangles.push_back(nt[2]);
  // };

  const auto bw_add_point = [&]() {
    point p{dist(rng), dist(rng)};
    size_t pid = points.size();
    points.push_back(p);

    unordered_map<edge, int, edge::hash> polygon{};

    for (auto it = triangles.begin(); it != triangles.end();) {
      auto& t = *it;
      if (circumcircle_intersection(t, p)) {
        ++polygon[{t.pid[0], t.pid[1]}];
        ++polygon[{t.pid[1], t.pid[2]}];
        ++polygon[{t.pid[2], t.pid[0]}];
        it = triangles.erase(it);
      } else {
        ++it;
      }
    }

    for (auto& [e, i] : polygon) {
      if (i != 1) continue;
      triangles.insert({e.pid[0], e.pid[1], pid});
    }
  };

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

  size_t tid_select = 0;

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

            case sf::Keyboard::Num0:
            case sf::Keyboard::Num1:
            case sf::Keyboard::Num2:
            case sf::Keyboard::Num3:
            case sf::Keyboard::Num4:
            case sf::Keyboard::Num5:
            case sf::Keyboard::Num6:
            case sf::Keyboard::Num7:
            case sf::Keyboard::Num8:
            case sf::Keyboard::Num9:
              tid_select = event.key.code - sf::Keyboard::Num0;
              break;

            case sf::Keyboard::Space:
              bw_add_point();
              break;

            case sf::Keyboard::Enter:
              for (int i = 0; i < 100; ++i) bw_add_point();
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

      // const auto c = circumcircle(t);
      // const auto radius = c.radius * height / fov_y;
      // sf::CircleShape shape(radius);
      // shape.setFillColor(sf::Color(0, 0, 0, 128));
      // shape.setOutlineColor(sf::Color::Red);
      // shape.setOutlineThickness(1.5f);
      // shape.setOrigin(radius, radius);
      // shape.setPosition(projection(c.center.x, c.center.y));
      // shape.setPointCount(1000);
      // window.draw(shape);
    }
    window.draw(vertices.data(), vertices.size(), sf::Lines);

    {
      // const auto& t = triangles[tid_select];

      // vertices.clear();
      // vertices.push_back(
      //     sf::Vertex(projection(points[t.pid[0]].x, points[t.pid[0]].y),
      //                sf::Color::Black));
      // vertices.push_back(
      //     sf::Vertex(projection(points[t.pid[1]].x, points[t.pid[1]].y),
      //                sf::Color::Black));
      // vertices.push_back(
      //     sf::Vertex(projection(points[t.pid[1]].x, points[t.pid[1]].y),
      //                sf::Color::Black));
      // vertices.push_back(
      //     sf::Vertex(projection(points[t.pid[2]].x, points[t.pid[2]].y),
      //                sf::Color::Black));
      // vertices.push_back(
      //     sf::Vertex(projection(points[t.pid[2]].x, points[t.pid[2]].y),
      //                sf::Color::Black));
      // vertices.push_back(
      //     sf::Vertex(projection(points[t.pid[0]].x, points[t.pid[0]].y),
      //                sf::Color::Black));

      // const auto c = circumcircle(t);
      // const auto radius = c.radius * height / fov_y;
      // sf::CircleShape shape(radius);
      // shape.setFillColor(sf::Color(0, 0, 0, 0));
      // shape.setOutlineColor(sf::Color::Red);
      // shape.setOutlineThickness(1.5f);
      // shape.setOrigin(radius, radius);
      // shape.setPosition(projection(c.center.x, c.center.y));
      // shape.setPointCount(1000);
      // window.draw(shape);

      // window.draw(vertices.data(), vertices.size(), sf::Lines);

      // for (size_t nid = 0; nid < 3; ++nid) {
      //   if (t.tnid[nid] == 0b11) continue;
      //   const auto npid_id = t.tnid[nid] & 0b11;
      //   const auto ntid = t.tnid[nid] >> 2;
      //   const auto npid = triangles[ntid].pid[npid_id];
      //   const auto& nt = triangles[ntid];

      //   vertices.clear();
      //   vertices.push_back(
      //       sf::Vertex(projection(points[nt.pid[0]].x, points[nt.pid[0]].y),
      //                  sf::Color(0, 0, 0, 50)));
      //   vertices.push_back(
      //       sf::Vertex(projection(points[nt.pid[1]].x, points[nt.pid[1]].y),
      //                  sf::Color(0, 0, 0, 50)));
      //   vertices.push_back(
      //       sf::Vertex(projection(points[nt.pid[2]].x, points[nt.pid[2]].y),
      //                  sf::Color(0, 0, 0, 50)));
      //   window.draw(vertices.data(), vertices.size(), sf::Triangles);

      //   constexpr float radius = 3.0f;
      //   sf::CircleShape shape(radius);
      //   shape.setFillColor(sf::Color(0, 0, 0, 0));
      //   shape.setOutlineColor(sf::Color::Red);
      //   shape.setOutlineThickness(2.0f);
      //   shape.setOrigin(radius, radius);
      //   shape.setPosition(projection(points[npid].x, points[npid].y));
      //   window.draw(shape);
      // }
    }

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