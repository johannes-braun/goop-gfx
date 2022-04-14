#pragma once

#include "vectors.hpp"

namespace goop
{
	namespace detail
	{
		using namespace rnu;
		template<typename T>
		T sign(T x)
		{
			return x < 0 ? -1 : (x > 0 ? 1 : 0);
		}
		vec2 sign(vec2 x)
		{
			return vec2{ sign(x.x), sign(x.y) };
		}
	}

  inline float distance(rnu::vec2d point, rnu::vec2d start, line_data_t const& line, bool relative)
  {

    const auto& p0 = start;
    auto p1 = line.value;
    if (relative)
      p1 += start;

    const auto AB = point - p0;
    const auto CD = p1 - p0;

    const auto dot = rnu::dot(AB, CD);
    const auto len_sq = rnu::dot(CD, CD);
    auto param = -1.0;
    if (len_sq != 0) //in case of 0 length line
      param = dot / len_sq;

    auto xxyy = p0 + param * CD;

    if (param <= 0) {
      xxyy = p0;
    }
    else if (param >= 1) {
      xxyy = p1;
    }

    const auto x = rnu::dot(AB, rnu::vec2d(-CD.y, CD.x));
    const auto sign = x < 0 ? -1 : (x>0?1:1000000);

    const auto d = point - xxyy;
    return sign * rnu::norm(d);
  }

  namespace detail
  {
    template<typename T>
    T mix(T a, T b, float t)
    {
      return (1 - t) * a + t * b;
    }
    
    vec2 mix(vec2 a, vec2 b, vec2 t)
    {
      return vec2{ mix(a.x, b.x, float(t.x==1)), mix(a.y, b.y, float(t.y == 1)) };
    }
  }

  inline float distance(rnu::vec2d point, rnu::vec2d start, curve_to_data_t const& x, bool relative)
  {
    auto control = rnu::vec2(x.x1, x.y1);
		auto control2 = rnu::vec2(x.x2, x.y2);
    auto end = rnu::vec2(x.x, x.y);
    if (relative) {
      control += rnu::vec2(start);
      control2 += rnu::vec2(start);
      end += rnu::vec2(start);
    }

		auto const cas = [&](float t) {
			auto const l1 = detail::mix(start, control, t);
			auto const l2 = detail::mix(control, control2, t);
			auto const l3 = detail::mix(control2, end, t);
			auto const p1 = detail::mix(l1, l2, t);
			auto const p2 = detail::mix(l2, l3, t);
			return detail::mix(p1, p2, t);
		};

		rnu::vec2 last(start);
		float dmin = std::numeric_limits<float>::max();
		float num = 20;
		for (int i = 1; i <= num; ++i)
		{
			rnu::vec2 const current = cas(i / num);
			auto const d = distance(point, last, line_data_t{ current }, false);
			if(abs(d) < abs(dmin))
				dmin = d;
			last = current;
		}
		return dmin;
  }

  inline float distance(rnu::vec2d point, rnu::vec2d start, quad_bezier_data_t const& x, bool relative)
  {
		auto control = rnu::vec2(x.x1, x.y1);
		auto end = rnu::vec2(x.x, x.y);
		if (relative) {
			control += rnu::vec2(start);
			end += rnu::vec2(start);
		}

		auto const cas = [&](float t) {
			auto const l1 = detail::mix(start, control, t);
			auto const l2 = detail::mix(control, end, t);
			return detail::mix(l1, l2, t);
		};

		rnu::vec2 last(start);
		float dmin = std::numeric_limits<float>::max();
		float num = 20;
		for (int i = 1; i <= num; ++i)
		{
			rnu::vec2 const current = cas(i / num);
			auto const d = distance(point, last, line_data_t{ current }, false);
			if (abs(d) < abs(dmin))
				dmin = d;
			last = current;
		}
		return dmin;
  }

  inline float distance(rnu::vec2d point, rnu::vec2d start, vertical_data_t const& vertical, bool relative)
  {
    line_data_t line;
    line.value.x = relative ? 0.0 : start.x;
    line.value.y = vertical.value;
    return distance(point, start, line, relative);
  }

  inline float distance(rnu::vec2d point, rnu::vec2d start, horizontal_data_t const& horizontal, bool relative)
  {
    line_data_t line;
    line.value.x = horizontal.value;
    line.value.y = relative ? 0.0 : start.y;
    return distance(point, start, line, relative);
  }

  template<typename T>
  inline float distance(rnu::vec2d point, rnu::vec2d start, T const& x, bool relative)
  {
    return std::numeric_limits<float>::max();
  }

  inline float distance(rnu::vec2d point, rnu::vec2d start, path_action_t const& action)
  {
    const auto relative = is_relative(action.type);
    return visit(action, [&](auto val) { return distance(point, start, val, relative); });
  }
}