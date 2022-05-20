#pragma once

#include <rnu/math/math.hpp>
#include <variant>
#include <rnu/math/cx_fun.hpp>
#include "vectors.hpp"

namespace goop::lines
{
  struct line
  {
    rnu::vec2 start;
    rnu::vec2 end;

    constexpr void precompute() {}

    constexpr rnu::vec2 interpolate(float t) const {
      return rnu::cx::mix(start, end, t);
    }

    constexpr float curvature() const {
      return 0.0f;
    }
  };

  struct bezier
  {
    rnu::vec2 start;
    rnu::vec2 control;
    rnu::vec2 end;

    constexpr void precompute() {}

    constexpr rnu::vec2 interpolate(float t) const {
      auto const l0 = rnu::cx::mix(start, control, t);
      auto const l1 = rnu::cx::mix(control, end, t);
      return rnu::cx::mix(l0, l1, t);
    }

    constexpr float curvature() const {
      auto const conn0 = control - start;
      auto const conn1 = end - control;
      auto const conndir = end - start;

      auto const len0 = dot(conn0, conn0);
      auto const len1 = dot(conn1, conn1);
      auto const lendir = dot(conndir, conndir);

      return rnu::cx::sqrt(rnu::cx::abs((len0 + len1) / lendir));
    }
  };

  struct arc
  {
    rnu::vec2 start;
    rnu::vec2 radii;
    rnu::vec2 end;

    float rotation;
    bool large_arc;
    bool sweep;

    struct
    {
      double center_x, center_y, start_angle, end_angle, delta_angle, rotated_angle;
    } precomputed;

    constexpr void precompute() {

      precomputed.rotated_angle = std::numbers::pi_v<float> -(rotation / 360.0f) * std::numbers::pi_v<float>;
      bool real_sweep = sweep;
      if (large_arc) real_sweep = !real_sweep;

      double e = radii.x / radii.y;
      double c = rnu::cx::cos(precomputed.rotated_angle);
      double s = rnu::cx::sin(precomputed.rotated_angle);
      double ax = start.x * c - start.y * s;
      double ay = (start.x * s + start.y * c) * e;
      double bx = end.x * c - end.y * s;
      double by = (end.x * s + end.y * c) * e;

      precomputed.center_x = 0.5 * (ax + bx);     // mid point between A,B
      precomputed.center_y = 0.5 * (ay + by);
      double vx = (ay - by);
      double vy = (bx - ax);
      double const l = rnu::cx::sqrt(std::max(0.0, radii.x * radii.x / (vx * vx + vy * vy) - 0.25));
      vx *= l;
      vy *= l;

      if (real_sweep) {
        precomputed.center_x += vx;
        precomputed.center_y += vy;
      }
      else {
        precomputed.center_x -= vx;
        precomputed.center_y -= vy;
      }

      precomputed.start_angle = rnu::cx::atan2(ay - precomputed.center_y, ax - precomputed.center_x);
      precomputed.end_angle = rnu::cx::atan2(by - precomputed.center_y, bx - precomputed.center_x);
      precomputed.center_y = precomputed.center_y / e;

      auto const ACC_ZERO_ANG = 0.000001 * std::numbers::pi_v<float> / 180.0;
      auto const tau = std::numbers::pi_v<float>*2;
      precomputed.delta_angle = precomputed.end_angle - precomputed.start_angle;
      if (rnu::cx::abs(rnu::cx::abs(precomputed.delta_angle) - std::numbers::pi_v<float>) <= ACC_ZERO_ANG) {     // half arc is without larc and sweep is not working instead change a0,a1
        double db = (0.5 * (precomputed.start_angle + precomputed.end_angle)) - rnu::cx::atan2(by - ay, bx - ax);
        while (db < -std::numbers::pi_v<float>) {
          db += tau;     // db<0 CCW ... sweep=1
        }
        while (db > std::numbers::pi_v<float>) {
          db -= tau;     // db>0  CW ... sweep=0
        }
        real_sweep = false;
        if ((db < 0.0) && (!sweep)) {
          real_sweep = true;
        }
        if ((db > 0.0) && (sweep)) {
          real_sweep = true;
        }
        if (real_sweep) {
          if (precomputed.delta_angle >= 0.0) {
            precomputed.end_angle -= tau;
          }
          if (precomputed.delta_angle < 0.0) {
            precomputed.start_angle -= tau;
          }
        }
      }
      else if (large_arc) {            // big arc
        if ((precomputed.delta_angle < std::numbers::pi_v<float>) && (precomputed.delta_angle >= 0.0)) {
          precomputed.end_angle -= tau;
        }
        if ((precomputed.delta_angle > -std::numbers::pi_v<float>) && (precomputed.delta_angle < 0.0)) {
          precomputed.start_angle -= tau;
        }
      }
      else {                      // small arc
        if (precomputed.delta_angle > std::numbers::pi_v<float>) {
          precomputed.end_angle -= tau;
        }
        if (precomputed.delta_angle < -std::numbers::pi_v<float>) {
          precomputed.start_angle -= tau;
        }
      }
      precomputed.delta_angle = precomputed.end_angle - precomputed.start_angle;
    }

    constexpr float curvature() const
    {
      return 5;
    }

    constexpr rnu::vec2 interpolate(float t) const {
      t = precomputed.start_angle + precomputed.delta_angle * t;
      double const x = precomputed.center_x + radii.x * rnu::cx::cos(t);
      double const y = precomputed.center_y + radii.y * rnu::cx::sin(t);
      double const c = rnu::cx::cos(-precomputed.rotated_angle);
      double const s = rnu::cx::sin(-precomputed.rotated_angle);

      rnu::vec2 result{};
      result.x = x * c - y * s;
      result.y = x * s + y * c;

      return result;
    }
  };

  struct curve
  {
    rnu::vec2 start;
    rnu::vec2 control_start;
    rnu::vec2 control_end;
    rnu::vec2 end;

    constexpr void precompute() {}

    constexpr rnu::vec2 interpolate(float t) const {
      auto const l0 = rnu::cx::mix(start, control_start, t);
      auto const l1 = rnu::cx::mix(control_start, control_end, t);
      auto const l2 = rnu::cx::mix(control_end, end, t);
      auto const p0 = rnu::cx::mix(l0, l1, t);
      auto const p1 = rnu::cx::mix(l1, l2, t);
      return rnu::cx::mix(p0, p1, t);
    }

    constexpr float curvature() const {
      auto const conn0 = control_start - start;
      auto const conninter = control_end - control_start;
      auto const conn1 = end - control_end;
      auto const conndir = end - start;

      auto const len0 = dot(conn0, conn0);
      auto const leninter = dot(conninter, conninter);
      auto const len1 = dot(conn1, conn1);
      auto const lendir = dot(conndir, conndir);

      return rnu::cx::sqrt(rnu::cx::abs((len0 + leninter + len1) / lendir));
    }
  };

  template<typename T>
  constexpr void subsample(T c, float baseline_samples, std::vector<line>& output);

  template<typename T, typename Fun>
  constexpr void subsample(T c, float baseline_samples, Fun&& yield)
  {
    c.precompute();
    auto const num_samples = 1 + c.curvature() * baseline_samples;
    auto const steps = std::ceil(num_samples);
    auto const step_size = 1.0 / steps;
    rnu::vec2 start = c.start;
    for (int i = 1; i <= steps; ++i)
    {
      auto const end = c.interpolate(step_size * i);
      yield(line{ .start = start, .end = end });
      start = end;
    }
  }

  template<typename T>
  constexpr void subsample(T c, float baseline_samples, std::vector<line>& output)
  {
    subsample(c, baseline_samples, [&](auto&& l) { output.push_back(l); });
  }

  using line_segment = std::variant<line, curve, bezier, arc>;

  constexpr float minimum_distance(rnu::vec2 v, rnu::vec2 w, rnu::vec2 p) {
    // Return minimum distance between line segment vw and point p
    auto const diff = w - v;
    const float l2 = dot(diff, diff);  // i.e. |w-v|^2 -  avoid a sqrt
    if (l2 == 0.0) return rnu::norm(p - v);   // v == w case
    // Consider the line extending the segment, parameterized as v + t (w - v).
    // We find projection of point p onto the line. 
    // It falls where t = [(p-v) . (w-v)] / |w-v|^2
    // We clamp t from [0,1] to handle points outside the segment vw.
    const float t = rnu::max(0, rnu::min(1, dot(p - v, w - v) / l2));
    const rnu::vec2 projection = v + t * (w - v);  // Projection falls on the segment
    return rnu::norm(p - projection);
  }

  constexpr int const scanline_test(rnu::vec2 a, rnu::vec2 b, rnu::vec2 r)
  {
    auto const s = b - a;
    auto const n = rnu::vec2(-s.y, s.x);
    auto const k = a - r;
    auto const t = (-k.y) / (s.y);

    auto const cross = [](auto x, auto y) { return x.x * y.y - x.y * y.x; };
    auto const u = (cross(k, s)) / s.y;

    bool const intersects = u > 0 && t >= 0 && t <= 1;

    if (!intersects)
      return 0;

    return rnu::sign(dot(n, k));
  }

  template<typename T>
  concept line_segment_sequence = std::ranges::forward_range<T> && requires(T range) {
    { *range.begin() } -> std::convertible_to<line_segment>;
  };

  template<line_segment_sequence T>
  constexpr float signed_distance(T&& polygon, rnu::vec2 point, float subsampling_factor = 3.0f)
  {
    float dmin = std::numeric_limits<float>::max();
    int intersections = 0;

    auto const consume = [&](line const& l)
    {
      auto const f = point.y - static_cast<int>(point.y);
      if (f == 0)
        point.y += 1e-3f;

      auto const d = minimum_distance(l.start, l.end, point);
      intersections += scanline_test(l.start, l.end, point);

      if (rnu::cx::abs(d) < rnu::cx::abs(dmin))
        dmin = d;
    };

    for (auto const& segment : polygon)
    {
      if constexpr (std::is_same_v<std::decay_t<decltype(segment)>, line_segment>)
      {
        std::visit([&](auto const& segment_part) {
            subsample(segment_part, subsampling_factor, consume);
          }, segment);
      }
      else
      {
        subsample(segment, subsampling_factor, consume);
      }
    }
    return (intersections == 0 ? 1 : -1) * dmin;
  }


  std::vector<line_segment> to_line_segments(goop::vector_image const& image)
  {
    struct path_visitor
    {
      std::vector<line_segment> segments;
      rnu::vec2d cursor{ 0, 0 };
      bool relative = false;
      rnu::vec2 last_curve_control;
      rnu::vec2 last_move_target;

      rnu::vec2d offset() const {
        return relative ? cursor : rnu::vec2d(0, 0);
      }

      void operator()(goop::line_data_t const& data) {
        segments.push_back(line{ .start = cursor, .end = data.value + offset() });
      }

      void operator()(goop::vertical_data_t const& data) {
        auto end = cursor;
        end.y = offset().y + data.value;
        segments.push_back(line{ .start = cursor, .end = end });
      }

      void operator()(goop::horizontal_data_t const& data) {
        auto end = cursor;
        end.x = offset().x + data.value;
        segments.push_back(line{ .start = cursor, .end = end });
      }

      void operator()(goop::smooth_quad_bezier_data_t const& data)
      {
        auto to_last = last_curve_control - cursor;
        auto next = cursor - to_last;

        auto const off = offset();
        bezier curve{
          .start = cursor,
          .control = next,
          .end = {off.x + data.value.x, off.y + data.value.y}
        };
        segments.push_back(curve);
        last_curve_control = curve.control;
      }

      void operator()(goop::smooth_curve_to_data_t const& data)
      {
        auto to_last = last_curve_control - cursor;
        auto next = cursor - to_last;

        auto const off = offset();
        goop::lines::curve curve{
          .start = cursor,
          .control_start = next,
          .control_end = {off.x + data.x2, off.y + data.y2},
          .end = {off.x + data.x, off.y + data.y}
        };
        segments.push_back(curve);
        last_curve_control = curve.control_end;
      }

      void operator()(goop::quad_bezier_data_t const& data)
      {
        auto const off = offset();
        goop::lines::bezier curve{
          .start = cursor,
          .control = {off.x + data.x1, off.y + data.y1},
          .end = {off.x + data.x, off.y + data.y}
        };
        segments.push_back(curve);
        last_curve_control = curve.control;
      }

      void operator()(goop::curve_to_data_t const& data)
      {
        auto const off = offset();
        goop::lines::curve curve{
          .start = cursor,
          .control_start = {off.x + data.x1, off.y + data.y1},
          .control_end = {off.x + data.x2, off.y + data.y2},
          .end = {off.x + data.x, off.y + data.y}
        };
        segments.push_back(curve);
        last_curve_control = curve.control_end;
      }

      void operator()(goop::arc_data_t const& data)
      {
        auto const off = offset();
        goop::lines::arc curve{
          .start = cursor,
          .radii = {data.rx, data.ry},
          .end = {off.x + data.x, off.y + data.y},
          .rotation = float(data.x_axis_rotation),
          .large_arc = data.large_arc_flag != 0,
          .sweep = data.sweep_flag != 0
        };
        segments.push_back(curve);
      }

      void operator()(goop::close_data_t const& close)
      {
        segments.push_back(line{ .start = cursor, .end = last_move_target });
      }

      void operator()(goop::move_data_t const& move)
      {
        last_move_target = move.value + offset();
      }
    } visitor;

    for (auto const& seg : image.path())
    {
      visitor.relative = is_relative(seg.type);
      goop::visit(seg, visitor);
      goop::move_cursor(seg, visitor.cursor);
    }

    return visitor.segments;
  }
}