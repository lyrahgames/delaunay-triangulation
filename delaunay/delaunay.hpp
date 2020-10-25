#pragma once
#include <delaunay/geometry.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace delaunay {

struct point {
  float x, y;
};

struct circle {
  point center;
  float radius;
};

struct triangulation {
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

      // Make sure clockwise and counterclockwise orders are preserved.
      // Move minimal index to start and cyclically interchange others.
      // if (pid[0] < pid[1]) {
      //   if (pid[2] < pid[0]) {
      //     pid[0] = pid2;
      //     pid[1] = pid0;
      //     pid[2] = pid1;
      //   }
      // } else {
      //   if (pid[1] < pid[2]) {
      //     pid[0] = pid1;
      //     pid[1] = pid2;
      //     pid[2] = pid0;
      //   } else {
      //     pid[0] = pid2;
      //     pid[1] = pid0;
      //     pid[2] = pid1;
      //   }
      // }
    }

    size_t pid[3];
  };

  void add(const point& p);

  std::vector<uint32_t> triangle_data(std::vector<point>& data) {
    for (const auto& p : data) add(p);
    std::vector<uint32_t> result{};
    for (auto it = triangles.begin(); it != triangles.end(); ++it) {
      auto& t = *it;
      const auto mask = (~size_t{0}) << 2;
      if ((t.pid[0] & mask) && (t.pid[1] & mask) && (t.pid[2] & mask)) {
        result.push_back(t.pid[0] - 4);
        result.push_back(t.pid[1] - 4);
        result.push_back(t.pid[2] - 4);
      }
    }
    return result;
  }

  template <typename Vector>
  std::vector<uint32_t> triangle_data(const std::vector<Vector>& data) {
    for (const auto& p : data) add({p.x, p.y});
    std::vector<uint32_t> result{};
    for (auto it = triangles.begin(); it != triangles.end(); ++it) {
      auto& t = *it;
      const auto mask = (~size_t{0}) << 2;
      if ((t.pid[0] & mask) && (t.pid[1] & mask) && (t.pid[2] & mask)) {
        result.push_back(t.pid[0] - 4);
        result.push_back(t.pid[1] - 4);
        result.push_back(t.pid[2] - 4);
      }
    }
    return result;
  }

  std::vector<point> points{
      {-300.0f, -300.0f},
      {300.0f, -300.0f},
      {300.0f, 300.0f},
      {-300.0f, 300.0f},
  };
  std::unordered_set<triangle, triangle::hash> triangles{{0, 1, 2}, {2, 3, 0}};
  std::unordered_map<edge, int, edge::hash> polygon{};
};

constexpr bool operator==(const triangulation::edge& e1,
                          const triangulation::edge& e2) noexcept {
  return (e1.pid[0] == e2.pid[0]) && (e1.pid[1] == e2.pid[1]);
}

constexpr bool operator==(const triangulation::triangle& t1,
                          const triangulation::triangle& t2) noexcept {
  return (t1.pid[0] == t2.pid[0]) && (t1.pid[1] == t2.pid[1]) &&
         (t1.pid[2] == t2.pid[2]);
}

void triangulation::add(const point& p) {
  size_t pid = points.size();
  points.push_back(p);

  polygon.clear();

  for (auto it = triangles.begin(); it != triangles.end();) {
    auto& t = *it;
    if (geometry::circumcircle_intersection(
            {{{points[t.pid[0]].x, points[t.pid[0]].y},
              {points[t.pid[1]].x, points[t.pid[1]].y},
              {points[t.pid[2]].x, points[t.pid[2]].y}}},
            {p.x, p.y})) {
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
}

}  // namespace delaunay