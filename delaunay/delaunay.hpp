#pragma once
#include <cmath>

namespace geometry {

struct point {
  float x, y;
};

struct triangle {
  point vertex[3];
};

struct circle {
  point center;
  float radius;
};

struct aabb {
  point min;
  point max;
};

constexpr auto circumcircle(const triangle& t) noexcept {
  using namespace std;

  const point edge1{t.vertex[1].x - t.vertex[0].x,
                    t.vertex[1].y - t.vertex[0].y};
  const point edge2{t.vertex[2].x - t.vertex[0].x,
                    t.vertex[2].y - t.vertex[0].y};
  const auto d = 2.0f * (edge1.x * edge2.y - edge1.y * edge2.x);
  const auto inv_d = 1.0f / d;
  const auto sqnorm_edge1 = edge1.x * edge1.x + edge1.y * edge1.y;
  const auto sqnorm_edge2 = edge2.x * edge2.x + edge2.y * edge2.y;
  const point center{inv_d * (edge2.y * sqnorm_edge1 - edge1.y * sqnorm_edge2),
                     inv_d * (edge1.x * sqnorm_edge2 - edge2.x * sqnorm_edge1)};
  return circle{{center.x + t.vertex[0].x, center.y + t.vertex[0].y},
                sqrt(center.x * center.x + center.y * center.y)};
};

constexpr auto circumcircle(const aabb& b) noexcept {
  using namespace std;
  const point r{0.5f * (b.max.x - b.min.x), 0.5f * (b.max.y - b.min.y)};
  return circle{{0.5f * (b.min.x + b.max.x), 0.5f * (b.min.y + b.max.y)},
                sqrt(r.x * r.x + r.y * r.y)};
}

constexpr auto intersection(const triangle& t, const point& p) noexcept {
  const point tp{p.x - t.vertex[0].x, p.y - t.vertex[0].y};
  const point edge1{t.vertex[1].x - t.vertex[0].x,
                    t.vertex[1].y - t.vertex[0].y};
  const point edge2{t.vertex[2].x - t.vertex[0].x,
                    t.vertex[2].y - t.vertex[0].y};
  const auto d = (edge1.x * edge2.y - edge1.y * edge2.x);
  const auto inv_d = 1.0f / d;
  const auto u = inv_d * (edge2.y * tp.x - edge2.x * tp.y);
  const auto v = inv_d * (edge1.x * tp.y - edge1.y * tp.x);
  return ((u >= 0.0f) && (v >= 0.0f) && (u + v <= 1.0f));
};

constexpr auto circumcircle_intersection(const triangle& t,
                                         const point& p) noexcept {
  const auto axdx = t.vertex[0].x - p.x;
  const auto aydy = t.vertex[0].y - p.y;
  const auto bxdx = t.vertex[1].x - p.x;
  const auto bydy = t.vertex[1].y - p.y;
  const auto cxdx = t.vertex[2].x - p.x;
  const auto cydy = t.vertex[2].y - p.y;
  const auto sqsum_a = axdx * axdx + aydy * aydy;
  const auto sqsum_b = bxdx * bxdx + bydy * bydy;
  const auto sqsum_c = cxdx * cxdx + cydy * cydy;
  const auto det = axdx * (bydy * sqsum_c - cydy * sqsum_b) -
                   aydy * (bxdx * sqsum_c - cxdx * sqsum_b) +
                   sqsum_a * (bxdx * cydy - cxdx * bydy);
  const point edge1{t.vertex[1].x - t.vertex[0].x,
                    t.vertex[1].y - t.vertex[0].y};
  const point edge2{t.vertex[2].x - t.vertex[0].x,
                    t.vertex[2].y - t.vertex[0].y};
  const auto d = (edge1.x * edge2.y - edge1.y * edge2.x);
  return (d * det) > 0.0f;
};

constexpr auto bounding_box(const circle& c) noexcept {
  return aabb{{c.center.x - c.radius, c.center.y - c.radius},
              {c.center.x + c.radius, c.center.y + c.radius}};
}

constexpr auto bounding_box(const triangle& t) noexcept {
  using namespace std;
  return aabb{{min(min(t.vertex[0].x, t.vertex[1].x), t.vertex[2].x),
               min(min(t.vertex[0].y, t.vertex[1].y), t.vertex[2].y)},
              {max(max(t.vertex[0].x, t.vertex[1].x), t.vertex[2].x),
               max(max(t.vertex[0].y, t.vertex[1].y), t.vertex[2].y)}};
}

}  // namespace geometry

namespace delaunay::bowyer_watson {

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

class triangulation {
 public:
 private:
};

}  // namespace delaunay::bowyer_watson