#pragma once

#include <array>
#include <algorithm>

namespace goop
{
  static constexpr std::array permutation = { 151,160,137,91,90,15,                 // Hash lookup table as defined by Ken Perlin.  This is a randomly
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,    // arranged array of all numbers from 0-255 inclusive.
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
  };

  template<size_t S0, size_t S1, typename T>
  constexpr auto array_concat(std::array<T, S0> const& a, std::array<T, S1> const& b)
  {
    std::array<T, S0 + S1> result{};
    T* insert = result.data();
    for (auto const& item : a)
      (*(insert++)) = item;
    for (auto const& item : b)
      (*(insert++)) = item;
    return result;
  }

  static constexpr auto permutation_doubled = array_concat(permutation, permutation);

  constexpr float fade(float t)
  {
    return t * t * t * (t * (t * 6 - 15) + 10);
  }

  // Source: http://riven8192.blogspot.com/2010/08/calculate-perlinnoise-twice-as-fast.html
  constexpr float grad(int hash, float x, float y, float z)
  {
    switch (hash & 0xF)
    {
    case 0x0: return  x + y;
    case 0x1: return -x + y;
    case 0x2: return  x - y;
    case 0x3: return -x - y;
    case 0x4: return  x + z;
    case 0x5: return -x + z;
    case 0x6: return  x - z;
    case 0x7: return -x - z;
    case 0x8: return  y + z;
    case 0x9: return -y + z;
    case 0xA: return  y - z;
    case 0xB: return -y - z;
    case 0xC: return  y + x;
    case 0xD: return -y + z;
    case 0xE: return  y - x;
    case 0xF: return -y - z;
    default: return 0; // never happens
    }
  }

  constexpr float lerp(float a, float b, float t)
  {
    return a + t * (b - a);
  }

  constexpr float perlin_noise(float x, float y, float z)
  {
    x += 1000;
    y += 1000;
    z += 1000;

    constexpr auto fract = [](auto f) {
      auto const val = rnu::fmod(f, 1.0f);
      if (val < 0)
        return -val;
      return val;
    };

    constexpr auto cube = [](auto f)->uint8_t
    {
      return uint8_t(f);
    };
    
    auto const xi = cube(x);
    auto const yi = cube(y);
    auto const zi = cube(z);

    auto const xf = fract(x);
    auto const yf = fract(y);
    auto const zf = fract(z);


    auto const u = fade(xf);
    auto const v = fade(yf);
    auto const w = fade(zf);

    constexpr auto inc = [](auto i) { return i + 1; };

    auto const& p = permutation_doubled;
    auto const aaa = p[p[p[xi] + yi] + zi];
    auto const aba = p[p[p[xi] + inc(yi)] + zi];
    auto const aab = p[p[p[xi] + yi] + inc(zi)];
    auto const abb = p[p[p[xi] + inc(yi)] + inc(zi)];
    auto const baa = p[p[p[inc(xi)] + yi] + zi];
    auto const bba = p[p[p[inc(xi)] + inc(yi)] + zi];
    auto const bab = p[p[p[inc(xi)] + yi] + inc(zi)];
    auto const bbb = p[p[p[inc(xi)] + inc(yi)] + inc(zi)];

    // The gradient function calculates the dot product between a pseudorandom
    // gradient vector and the vector from the input coordinate to the 8
    // surrounding points in its unit cube.
    // This is all then lerped together as a sort of weighted average based on the faded (u,v,w)
    // values we made earlier.

    float x1, x2, y1, y2;
    x1 = lerp(grad(aaa, xf, yf, zf), grad(baa, xf - 1, yf, zf), u);
    x2 = lerp(grad(aba, xf, yf - 1, zf), grad(bba, xf - 1, yf - 1, zf), u);
    y1 = lerp(x1, x2, v);
    x1 = lerp(grad(aab, xf, yf, zf - 1), grad(bab, xf - 1, yf, zf - 1), u);
    x2 = lerp(grad(abb, xf, yf - 1, zf - 1), grad(bbb, xf - 1, yf - 1, zf - 1), u);
    y2 = lerp(x1, x2, v);

    return (lerp(y1, y2, w) + 1) / 2;
  }

  constexpr float perlin_noise(float x, float y, float z, int octaves, float persistence)
  {
    float val = 0.0f;
    float frequency = 1;
    float amplitude = 1;
    float maxValue = 0;
    for(int i=0; i<octaves; ++i)
    {
      val += perlin_noise(frequency * x, frequency * y, frequency * z) * amplitude;

      maxValue += amplitude;
      amplitude *= persistence;
      frequency *= 2;
    }
    return val / maxValue;
  }
}