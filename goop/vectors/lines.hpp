#pragma once

#include <rnu/math/math.hpp>
#include <variant>
#include <rnu/math/cx_fun.hpp>

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

      precomputed.rotated_angle = std::numbers::pi_v<float> - (rotation / 360.0f) * std::numbers::pi_v<float>;
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
  constexpr void subsample(T c, float baseline_samples, std::vector<line>& output)
  {
    c.precompute();
    auto const num_samples = 1 + c.curvature() * baseline_samples;
    auto const steps = std::ceil(num_samples);
    auto const step_size = 1.0 / steps;
    rnu::vec2 start = c.start;
    for (int i = 1; i <= steps; ++i)
    {
      auto const end = c.interpolate(step_size * i);
      output.push_back(line{
        .start = start,
        .end = end
        });
      start = end;
    }
  }

  using line_segment = std::variant<line, curve, bezier, arc>;
}