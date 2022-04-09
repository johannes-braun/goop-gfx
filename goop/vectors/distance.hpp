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

		//float minimum_distance(vec2 v, vec2 w, vec2 p) {
		//	// Return minimum distance between line segment vw and point p
		//	auto const diff = w - v;
		//	const float l2 = dot(diff, diff);  // i.e. |w-v|^2 -  avoid a sqrt
		//	if (l2 == 0.0) return rnu::norm(p - v);   // v == w case
		//	// Consider the line extending the segment, parameterized as v + t (w - v).
		//	// We find projection of point p onto the line. 
		//	// It falls where t = [(p-v) . (w-v)] / |w-v|^2
		//	// We clamp t from [0,1] to handle points outside the segment vw.
		//	const float t = max(0, min(1, dot(p - v, w - v) / l2));
		//	const vec2 projection = v + t * (w - v);  // Projection falls on the segment
		//	return rnu::norm(p - projection);
		//}

		//float sign(vec2 v, vec2 w, vec2 p)
		//{
		//	vec2 const diff = w - v;
		//	vec2 const normal(diff.y, -diff.x);
		//	return dot(normal, p - v) > 0 ? 1 : -1;
		//}
	}

  inline float distance(rnu::vec2d point, rnu::vec2d start, line_data_t const& line, bool relative)
  {

    const auto& p0 = start;
    auto p1 = line.value;
    if (relative)
      p1 += start;
		//auto const s = detail::sign(p0, p1, point);
		//auto const d = detail::minimum_distance(p0, p1, point);
		//return d * s;

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
//
  namespace detail
  {
//    
    template<typename T>
    T mix(T a, T b, float t)
    {
      return (1 - t) * a + t * b;
    }
    
    vec2 mix(vec2 a, vec2 b, vec2 t)
    {
      return vec2{ mix(a.x, b.x, float(t.x==1)), mix(a.y, b.y, float(t.y == 1)) };
    }
//    float step(float x, float th)
//    {
//      return x < th ? 0.f : 1.f;
//    }
//
//    // Test if point p crosses line (a, b), returns sign of result
//    float testCross(vec2 a, vec2 b, vec2 p) {
//      return sign((b.y - a.y) * (p.x - a.x) - (b.x - a.x) * (p.y - a.y));
//    }
//
//    // Determine which side we're on (using barycentric parameterization)
//    float signBezier(vec2 A, vec2 B, vec2 C, vec2 p)
//    {
//      vec2 a = C - A, b = B - A, c = p - A;
//      vec2 bary = vec2(c.x * b.y - b.x * c.y, a.x * c.y - c.x * a.y) / (a.x * b.y - b.x * a.y);
//      vec2 d = vec2(bary.y * 0.5, 0.0) + 1.0f - bary.x - bary.y;
//
//      auto s = sign(d.x * d.x - d.y);
//      auto e = mix(-1.0f, 1.0f, step(testCross(A, B, p) * testCross(B, C, p), 0.0));
//      auto t = step((d.x - d.y), 0.0f);
//
//      return mix(s, e, 0)* testCross(A, C, B);
//    }
//
//    // Solve cubic equation for roots
//    vec3 solveCubic(float a, float b, float c)
//    {
//      float p = b - a * a / 3.0, p3 = p * p * p;
//      float q = a * (2.0 * a * a - 9.0 * b) / 27.0 + c;
//      float d = q * q + 4.0 * p3 / 27.0;
//      float offset = -a / 3.0;
//      if (d >= 0.0) {
//        float z = sqrt(d);
//        vec2 x = (vec2(z, -z) - q) / 2.0f;
//        vec2 uv = sign(x) * pow(abs(x), vec2(1.0 / 3.0));
//        return vec3(offset + uv.x + uv.y);
//      }
//      float v = acos(-sqrt(-27.0 / p3) * q / 2.0) / 3.0;
//      float m = cos(v), n = sin(v) * 1.732050808;
//      return vec3(m + m, -n - m, n - m) * sqrt(-p / 3.0f) + offset;
//    }
//
//    // Find the signed distance from a point to a bezier curve
//    float sdBezier(vec2 A, vec2 B, vec2 C, vec2 p)
//    {
//      B = mix(B + vec2(1e-4), B, abs(sign(B * 2.0f - A - C)));
//      vec2 a = B - A, b = A - B * 2.0f + C, c = a * 2.0f, d = A - p;
//      vec3 k = vec3(3. * dot(a, b), 2. * dot(a, a) + dot(d, b), dot(d, a)) / dot(b, b);
//      vec3 t = clamp(solveCubic(k.x, k.y, k.z), 0.0, 1.0);
//      vec2 pos = A + (c + b * t.x) * t.x;
//      float dis = norm(pos - p);
//      pos = A + (c + b * t.y) * t.y;
//      dis = rnu::min(dis, norm(pos - p));
//      pos = A + (c + b * t.z) * t.z;
//      dis = rnu::min(dis, norm(pos - p));
//      return -dis * signBezier(A, B, C, p);
//    }
//
//		const int halley_iterations = 8;
//		const float eps = .000005;
//
//		//lagrange positive real root upper bound
//		//see for example: https://doi.org/10.1016/j.jsc.2014.09.038
//		float upper_bound_lagrange5(float a0, float a1, float a2, float a3, float a4) {
//
//			vec4 coeffs1 = vec4(a0, a1, a2, a3);
//
//			vec4 neg1 = max(-coeffs1, vec4(0));
//			float neg2 = max(-a4, 0.);
//
//			const vec4 indizes1 = vec4(0, 1, 2, 3);
//			const float indizes2 = 4.;
//
//			vec4 bounds1 = pow(neg1, 1. / (5. - indizes1));
//			float bounds2 = pow(neg2, 1. / (5. - indizes2));
//
//			vec2 min1_2 = rnu::min(rnu::vec2(bounds1.x, bounds1.z), rnu::vec2(bounds1.y, bounds1.w));
//			vec2 max1_2 = rnu::max(rnu::vec2(bounds1.x, bounds1.z), rnu::vec2(bounds1.y, bounds1.w));
//
//			float maxmin = max(min1_2.x, min1_2.y);
//			float minmax = min(max1_2.x, max1_2.y);
//
//			float max3 = max(max1_2.x, max1_2.y);
//
//			float max_max = max(max3, bounds2);
//			float max_max2 = max(min(max3, bounds2), max(minmax, maxmin));
//
//			return max_max + max_max2;
//		}
//
//		//lagrange upper bound applied to f(-x) to get lower bound
//		float lower_bound_lagrange5(float a0, float a1, float a2, float a3, float a4) {
//
//			vec4 coeffs1 = vec4(-a0, a1, -a2, a3);
//
//			vec4 neg1 = max(-coeffs1, vec4(0));
//			float neg2 = max(-a4, 0.);
//
//			const vec4 indizes1 = vec4(0, 1, 2, 3);
//			const float indizes2 = 4.;
//
//			vec4 bounds1 = pow(neg1, 1. / (5. - indizes1));
//			float bounds2 = pow(neg2, 1. / (5. - indizes2));
//
//			vec2 min1_2 = rnu::min(rnu::vec2(bounds1.x, bounds1.z), rnu::vec2(bounds1.y, bounds1.w));
//			vec2 max1_2 = rnu::max(rnu::vec2(bounds1.x, bounds1.z), rnu::vec2(bounds1.y, bounds1.w));
//
//			float maxmin = max(min1_2.x, min1_2.y);
//			float minmax = min(max1_2.x, max1_2.y);
//
//			float max3 = max(max1_2.x, max1_2.y);
//
//			float max_max = max(max3, bounds2);
//			float max_max2 = max(min(max3, bounds2), max(minmax, maxmin));
//
//			return -max_max - max_max2;
//		}
//
//		vec2 parametric_cub_bezier(float t, vec2 p0, vec2 p1, vec2 p2, vec2 p3) {
//			vec2 a0 = (-p0 + 3. * p1 - 3. * p2 + p3);
//			vec2 a1 = (3. * p0 - 6. * p1 + 3. * p2);
//			vec2 a2 = (-3. * p0 + 3. * p1);
//			vec2 a3 = p0;
//
//			return (((a0 * t) + a1) * t + a2) * t + a3;
//		}
//
//		void sort_roots3(vec3& roots) {
//			vec3 tmp;
//
//			tmp[0] = min(roots[0], min(roots[1], roots[2]));
//			tmp[1] = max(roots[0], min(roots[1], roots[2]));
//			tmp[2] = max(roots[0], max(roots[1], roots[2]));
//
//			roots = tmp;
//		}
//
//		void sort_roots4( vec4& roots) {
//			vec4 tmp;
//
//			vec2 min1_2 = rnu::min(rnu::vec2(roots.x, roots.z), rnu::vec2(roots.y, roots.w));
//			vec2 max1_2 = rnu::max(rnu::vec2(roots.x, roots.z), rnu::vec2(roots.y, roots.w));
//
//			float maxmin = max(min1_2.x, min1_2.y);
//			float minmax = min(max1_2.x, max1_2.y);
//
//			tmp[0] = min(min1_2.x, min1_2.y);
//			tmp[1] = min(maxmin, minmax);
//			tmp[2] = max(minmax, maxmin);
//			tmp[3] = max(max1_2.x, max1_2.y);
//
//			roots = tmp;
//		}
//
//		float eval_poly5(float a0, float a1, float a2, float a3, float a4, float x) {
//
//			float f = ((((x + a4) * x + a3) * x + a2) * x + a1) * x + a0;
//
//			return f;
//		}
//
//		//halley's method
//		//basically a variant of newton raphson which converges quicker and has bigger basins of convergence
//		//see http://mathworld.wolfram.com/HalleysMethod.html
//		//or https://en.wikipedia.org/wiki/Halley%27s_method
//		float halley_iteration5(float a0, float a1, float a2, float a3, float a4, float x) {
//
//			float f = ((((x + a4) * x + a3) * x + a2) * x + a1) * x + a0;
//			float f1 = (((5. * x + 4. * a4) * x + 3. * a3) * x + 2. * a2) * x + a1;
//			float f2 = ((20. * x + 12. * a4) * x + 6. * a3) * x + 2. * a2;
//
//			return x - (2. * f * f1) / (2. * f1 * f1 - f * f2);
//		}
//
//		float halley_iteration4(vec4 coeffs, float x) {
//
//			float f = (((x + coeffs[3]) * x + coeffs[2]) * x + coeffs[1]) * x + coeffs[0];
//			float f1 = ((4. * x + 3. * coeffs[3]) * x + 2. * coeffs[2]) * x + coeffs[1];
//			float f2 = (12. * x + 6. * coeffs[3]) * x + 2. * coeffs[2];
//
//			return x - (2. * f * f1) / (2. * f1 * f1 - f * f2);
//		}
//
//		// Modified from http://tog.acm.org/resources/GraphicsGems/gems/Roots3And4.c
//		// Credits to Doublefresh for hinting there
//		int solve_quadric(vec2 coeffs,  vec2& roots) {
//
//			// normal form: x^2 + px + q = 0
//			float p = coeffs[1] / 2.;
//			float q = coeffs[0];
//
//			float D = p * p - q;
//
//			if (D < 0.) {
//				return 0;
//			}
//			else if (D >= 0.) {
//				roots[0] = -sqrt(D) - p;
//				roots[1] = sqrt(D) - p;
//
//				return 2;
//			}
//		}
//
//		//From Trisomie21
//		//But instead of his cancellation fix i'm using a newton iteration
//		int solve_cubic(vec3 coeffs,  vec3& r) {
//
//			float a = coeffs[2];
//			float b = coeffs[1];
//			float c = coeffs[0];
//
//			float p = b - a * a / 3.0;
//			float q = a * (2.0 * a * a - 9.0 * b) / 27.0 + c;
//			float p3 = p * p * p;
//			float d = q * q + 4.0 * p3 / 27.0;
//			float offset = -a / 3.0;
//			if (d >= 0.0) { // Single solution
//				float z = sqrt(d);
//				float u = (-q + z) / 2.0;
//				float v = (-q - z) / 2.0;
//				u = sign(u) * pow(abs(u), 1.0 / 3.0);
//				v = sign(v) * pow(abs(v), 1.0 / 3.0);
//				r[0] = offset + u + v;
//
//				//Single newton iteration to account for cancellation
//				float f = ((r[0] + a) * r[0] + b) * r[0] + c;
//				float f1 = (3. * r[0] + 2. * a) * r[0] + b;
//
//				r[0] -= f / f1;
//
//				return 1;
//			}
//			float u = sqrt(-p / 3.0);
//			float v = acos(-sqrt(-27.0 / p3) * q / 2.0) / 3.0;
//			float m = cos(v), n = sin(v) * 1.732050808;
//
//			//Single newton iteration to account for cancellation
//			//(once for every root)
//			r[0] = offset + u * (m + m);
//			r[1] = offset - u * (n + m);
//			r[2] = offset + u * (n - m);
//
//			vec3 f = ((r + a) * r + b) * r + c;
//			vec3 f1 = (3. * r + 2. * a) * r + b;
//
//			r -= f / f1;
//
//			return 3;
//		}
//
//		// Modified from http://tog.acm.org/resources/GraphicsGems/gems/Roots3And4.c
//		// Credits to Doublefresh for hinting there
//		int solve_quartic(vec4 coeffs,  vec4& s) {
//
//			float a = coeffs[3];
//			float b = coeffs[2];
//			float c = coeffs[1];
//			float d = coeffs[0];
//
//			/*  substitute x = y - A/4 to eliminate cubic term:
//		x^4 + px^2 + qx + r = 0 */
//
//			float sq_a = a * a;
//			float p = -3. / 8. * sq_a + b;
//			float q = 1. / 8. * sq_a * a - 1. / 2. * a * b + c;
//			float r = -3. / 256. * sq_a * sq_a + 1. / 16. * sq_a * b - 1. / 4. * a * c + d;
//
//			int num;
//
//			/* doesn't seem to happen for me */
//				//if(abs(r)<eps){
//			//	/* no absolute term: y(y^3 + py + q) = 0 */
//
//			//	vec3 cubic_coeffs;
//
//			//	cubic_coeffs[0] = q;
//			//	cubic_coeffs[1] = p;
//			//	cubic_coeffs[2] = 0.;
//
//			//	num = solve_cubic(cubic_coeffs, s.xyz);
//
//			//	s[num] = 0.;
//			//	num++;
//				//}
//			{
//				/* solve the resolvent cubic ... */
//
//				vec3 cubic_coeffs;
//
//				cubic_coeffs[0] = 1.0 / 2. * r * p - 1.0 / 8. * q * q;
//				cubic_coeffs[1] = -r;
//				cubic_coeffs[2] = -1.0 / 2. * p;
//
//				rnu::vec3 rs{ 0 };
//				solve_cubic(cubic_coeffs, rs);
//				s.x = rs.x;
//				s.y = rs.y;
//				s.z = rs.z;
//
//				/* ... and take the one real solution ... */
//
//				float z = s[0];
//
//				/* ... to build two quadric equations */
//
//				float u = z * z - r;
//				float v = 2. * z - p;
//
//				if (u > -eps) {
//					u = sqrt(abs(u));
//				}
//				else {
//					return 0;
//				}
//
//				if (v > -eps) {
//					v = sqrt(abs(v));
//				}
//				else {
//					return 0;
//				}
//
//				vec2 quad_coeffs;
//
//				quad_coeffs[0] = z - u;
//				quad_coeffs[1] = q < 0. ? -v : v;
//
//				rnu::vec2 qr;
//				num = solve_quadric(quad_coeffs, qr);
//				s.x = qr.x;
//				s.y = qr.y;
//
//				quad_coeffs[0] = z + u;
//				quad_coeffs[1] = q < 0. ? v : -v;
//
//				vec2 tmp = vec2(1e38);
//				int old_num = num;
//
//				num += solve_quadric(quad_coeffs, tmp);
//				if (old_num != num) {
//					if (old_num == 0) {
//						s[0] = tmp[0];
//						s[1] = tmp[1];
//					}
//					else {//old_num == 2
//						s[2] = tmp[0];
//						s[3] = tmp[1];
//					}
//				}
//			}
//
//			/* resubstitute */
//
//			float sub = 1. / 4. * a;
//
//			/* single halley iteration to fix cancellation */
//			for (int i = 0; i < 4; i += 2) {
//				if (i < num) {
//					s[i] -= sub;
//					s[i] = halley_iteration4(coeffs, s[i]);
//
//					s[i + 1] -= sub;
//					s[i + 1] = halley_iteration4(coeffs, s[i + 1]);
//				}
//			}
//
//			return num;
//		}
//
//		//Sign computation is pretty straightforward:
//		//I'm solving a cubic equation to get the intersection count
//		//of a ray from the current point to infinity and parallel to the x axis
//		//Also i'm computing the intersection count with the tangent in the end points of the curve
//		float cubic_bezier_sign(vec2 uv, vec2 p0, vec2 p1, vec2 p2, vec2 p3) {
//
//			float cu = (-p0.y + 3. * p1.y - 3. * p2.y + p3.y);
//			float qu = (3. * p0.y - 6. * p1.y + 3. * p2.y);
//			float li = (-3. * p0.y + 3. * p1.y);
//			float co = p0.y - uv.y;
//
//			vec3 roots = vec3(1e38);
//			int n_roots = solve_cubic(vec3(co / cu, li / cu, qu / cu), roots);
//
//			int n_ints = 0;
//
//			for (int i = 0; i < 3; i++) {
//				if (i < n_roots) {
//					if (roots[i] >= 0. && roots[i] <= 1.) {
//						float x_pos = -p0.x + 3. * p1.x - 3. * p2.x + p3.x;
//						x_pos = x_pos * roots[i] + 3. * p0.x - 6. * p1.x + 3. * p2.x;
//						x_pos = x_pos * roots[i] + -3. * p0.x + 3. * p1.x;
//						x_pos = x_pos * roots[i] + p0.x;
//
//						if (x_pos < uv.x) {
//							n_ints++;
//						}
//					}
//				}
//			}
//
//			auto const xy = [](auto v) { return rnu::vec2(v.x, v.y); };
//
//			vec2 tang1 = xy(p0) - xy(p1);
//			vec2 tang2 = xy(p2) - xy(p3);
//
//			vec2 nor1 = vec2(tang1.y, -tang1.x);
//			vec2 nor2 = vec2(tang2.y, -tang2.x);
//
//			if (p0.y < p1.y) {
//				if ((uv.y <= p0.y) && (dot(uv - xy(p0), nor1) < 0.)) {
//					n_ints++;
//				}
//			}
//			else {
//				if (!(uv.y <= p0.y) && !(dot(uv - xy(p0), nor1) < 0.)) {
//					n_ints++;
//				}
//			}
//
//			if (p2.y < p3.y) {
//				if (!(uv.y <= p3.y) && dot(uv - xy(p3), nor2) < 0.) {
//					n_ints++;
//				}
//			}
//			else {
//				if ((uv.y <= p3.y) && !(dot(uv - xy(p3), nor2) < 0.)) {
//					n_ints++;
//				}
//			}
//
//			if (n_ints == 0 || n_ints == 2 || n_ints == 4) {
//				return 1.;
//			}
//			else {
//				return -1.;
//			}
//		}
//
//		float cubic_bezier_dis(vec2 uv, vec2 p0, vec2 p1, vec2 p2, vec2 p3) {
//
//			//switch points when near to end point to minimize numerical error
//			//only needed when control point(s) very far away
//#if 0
//			vec2 mid_curve = parametric_cub_bezier(.5, p0, p1, p2, p3);
//			vec2 mid_points = (p0 + p3) / 2.;
//
//			vec2 tang = mid_curve - mid_points;
//			vec2 nor = vec2(tang.y, -tang.x);
//
//			if (sign(dot(nor, uv - mid_curve)) != sign(dot(nor, p0 - mid_curve))) {
//				vec2 tmp = p0;
//				p0 = p3;
//				p3 = tmp;
//
//				tmp = p2;
//				p2 = p1;
//				p1 = tmp;
//			}
//#endif
//
//			vec2 a3 = (-p0 + 3. * p1 - 3. * p2 + p3);
//			vec2 a2 = (3. * p0 - 6. * p1 + 3. * p2);
//			vec2 a1 = (-3. * p0 + 3. * p1);
//			vec2 a0 = p0 - uv;
//
//			//compute polynomial describing distance to current pixel dependent on a parameter t
//			float bc6 = dot(a3, a3);
//			float bc5 = 2. * dot(a3, a2);
//			float bc4 = dot(a2, a2) + 2. * dot(a1, a3);
//			float bc3 = 2. * (dot(a1, a2) + dot(a0, a3));
//			float bc2 = dot(a1, a1) + 2. * dot(a0, a2);
//			float bc1 = 2. * dot(a0, a1);
//			float bc0 = dot(a0, a0);
//
//			bc5 /= bc6;
//			bc4 /= bc6;
//			bc3 /= bc6;
//			bc2 /= bc6;
//			bc1 /= bc6;
//			bc0 /= bc6;
//
//			//compute derivatives of this polynomial
//
//			float b0 = bc1 / 6.;
//			float b1 = 2. * bc2 / 6.;
//			float b2 = 3. * bc3 / 6.;
//			float b3 = 4. * bc4 / 6.;
//			float b4 = 5. * bc5 / 6.;
//
//			vec4 c1 = vec4(b1, 2. * b2, 3. * b3, 4. * b4) / 5.;
//			vec3 c2 = vec3(c1[1], 2. * c1[2], 3. * c1[3]) / 4.;
//			vec2 c3 = vec2(c2[1], 2. * c2[2]) / 3.;
//			float c4 = c3[1] / 2.;
//
//			vec4 roots_drv = vec4(1e38);
//
//			int num_roots_drv = solve_quartic(c1, roots_drv);
//			sort_roots4(roots_drv);
//
//			float ub = upper_bound_lagrange5(b0, b1, b2, b3, b4);
//			float lb = lower_bound_lagrange5(b0, b1, b2, b3, b4);
//
//			vec3 a = vec3(1e38);
//			vec3 b = vec3(1e38);
//
//			vec3 roots = vec3(1e38);
//
//			int num_roots = 0;
//
//			//compute root isolating intervals by roots of derivative and outer root bounds
//				//only roots going form - to + considered, because only those result in a minimum
//			if (num_roots_drv == 4) {
//				if (eval_poly5(b0, b1, b2, b3, b4, roots_drv[0]) > 0.) {
//					a[0] = lb;
//					b[0] = roots_drv[0];
//					num_roots = 1;
//				}
//
//				if (sign(eval_poly5(b0, b1, b2, b3, b4, roots_drv[1])) != sign(eval_poly5(b0, b1, b2, b3, b4, roots_drv[2]))) {
//					if (num_roots == 0) {
//						a[0] = roots_drv[1];
//						b[0] = roots_drv[2];
//						num_roots = 1;
//					}
//					else {
//						a[1] = roots_drv[1];
//						b[1] = roots_drv[2];
//						num_roots = 2;
//					}
//				}
//
//				if (eval_poly5(b0, b1, b2, b3, b4, roots_drv[3]) < 0.) {
//					if (num_roots == 0) {
//						a[0] = roots_drv[3];
//						b[0] = ub;
//						num_roots = 1;
//					}
//					else if (num_roots == 1) {
//						a[1] = roots_drv[3];
//						b[1] = ub;
//						num_roots = 2;
//					}
//					else {
//						a[2] = roots_drv[3];
//						b[2] = ub;
//						num_roots = 3;
//					}
//				}
//			}
//			else {
//				if (num_roots_drv == 2) {
//					if (eval_poly5(b0, b1, b2, b3, b4, roots_drv[0]) < 0.) {
//						num_roots = 1;
//						a[0] = roots_drv[1];
//						b[0] = ub;
//					}
//					else if (eval_poly5(b0, b1, b2, b3, b4, roots_drv[1]) > 0.) {
//						num_roots = 1;
//						a[0] = lb;
//						b[0] = roots_drv[0];
//					}
//					else {
//						num_roots = 2;
//
//						a[0] = lb;
//						b[0] = roots_drv[0];
//
//						a[1] = roots_drv[1];
//						b[1] = ub;
//					}
//
//				}
//				else {//num_roots_drv==0
//					vec3 roots_snd_drv = vec3(1e38);
//					int num_roots_snd_drv = solve_cubic(c2, roots_snd_drv);
//
//					vec2 roots_trd_drv = vec2(1e38);
//					int num_roots_trd_drv = solve_quadric(c3, roots_trd_drv);
//					num_roots = 1;
//
//					a[0] = lb;
//					b[0] = ub;
//				}
//
//				//further subdivide intervals to guarantee convergence of halley's method
//		//by using roots of further derivatives
//				vec3 roots_snd_drv = vec3(1e38);
//				int num_roots_snd_drv = solve_cubic(c2, roots_snd_drv);
//				sort_roots3(roots_snd_drv);
//
//				int num_roots_trd_drv = 0;
//				vec2 roots_trd_drv = vec2(1e38);
//
//				if (num_roots_snd_drv != 3) {
//					num_roots_trd_drv = solve_quadric(c3, roots_trd_drv);
//				}
//
//				for (int i = 0; i < 3; i++) {
//					if (i < num_roots) {
//						for (int j = 0; j < 3; j += 2) {
//							if (j < num_roots_snd_drv) {
//								if (a[i] < roots_snd_drv[j] && b[i] > roots_snd_drv[j]) {
//									if (eval_poly5(b0, b1, b2, b3, b4, roots_snd_drv[j]) > 0.) {
//										b[i] = roots_snd_drv[j];
//									}
//									else {
//										a[i] = roots_snd_drv[j];
//									}
//								}
//							}
//						}
//						for (int j = 0; j < 2; j++) {
//							if (j < num_roots_trd_drv) {
//								if (a[i] < roots_trd_drv[j] && b[i] > roots_trd_drv[j]) {
//									if (eval_poly5(b0, b1, b2, b3, b4, roots_trd_drv[j]) > 0.) {
//										b[i] = roots_trd_drv[j];
//									}
//									else {
//										a[i] = roots_trd_drv[j];
//									}
//								}
//							}
//						}
//					}
//				}
//			}
//
//			float d0 = 1e38;
//
//			//compute roots with halley's method
//
//			for (int i = 0; i < 3; i++) {
//				if (i < num_roots) {
//					roots[i] = .5 * (a[i] + b[i]);
//
//					for (int j = 0; j < halley_iterations; j++) {
//						roots[i] = halley_iteration5(b0, b1, b2, b3, b4, roots[i]);
//					}
//
//
//					//compute squared distance to nearest point on curve
//					roots[i] = clamp(roots[i], 0., 1.);
//					vec2 to_curve = uv - parametric_cub_bezier(roots[i], p0, p1, p2, p3);
//					d0 = min(d0, dot(to_curve, to_curve));
//				}
//			}
//
//			return sqrt(d0);
//		}
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