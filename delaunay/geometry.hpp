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

using aabb_t = struct aabb {
  point min;
  point max;
};

// using aabb_t = aabb;

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

constexpr auto circumcircle(const aabb& b) noexcept {
  using namespace std;
  const point r{0.5f * (b.max.x - b.min.x), 0.5f * (b.max.y - b.min.y)};
  return circle{{0.5f * (b.min.x + b.max.x), 0.5f * (b.min.y + b.max.y)},
                sqrt(r.x * r.x + r.y * r.y)};
}

constexpr auto aabb(const triangle& t) noexcept { return bounding_box(t); }

}  // namespace geometry