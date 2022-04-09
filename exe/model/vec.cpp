#pragma once

#include <array>
#include <numeric>

namespace rnuX
{
  template<typename T>
  class basic_value
  {
  public:
    using stored_type = std::conditional_t<std::is_reference_v<T>, std::remove_reference_t<T>*, T>;
    using returned_type = std::add_lvalue_reference_t<T>;
    using returned_const_type = std::add_lvalue_reference_t<std::add_const_t<T>>;

    constexpr basic_value() : value{} {}
    constexpr basic_value(T element)
      : value{ store(element) } {}

    constexpr operator returned_type() { return load(value); }
    constexpr operator returned_const_type() const { return load(value); }

    stored_type value;

  private:
    static stored_type store(T value)
    {
      if constexpr (std::is_reference_v<T>)
        return &value;
      else
        return value;
    }

    static decltype(auto) load(auto&& value)
    {
      if constexpr (std::is_reference_v<T>)
        return *value;
      else
        return value;
    }
  };

  template<typename T, size_t Elements>
  class basic_storage_t
  {
  public:
    using returned_type = typename basic_value<T>::returned_type;
    using returned_const_type = typename basic_value<T>::returned_const_type;

    template<typename... V>
    constexpr basic_storage_t(V&&... elements) requires(std::is_convertible_v<V, T> && ...) : storage{ basic_value<T>{ static_cast<T>(elements) }... } { }

    constexpr returned_type at(size_t index) { return storage[index]; }
    constexpr returned_const_type at(size_t index) const { return storage[index]; }

    std::array<basic_value<T>, Elements> storage{};
  };

#define construct_storage template<typename... V> constexpr storage_t(V&&... elements) requires(std::is_convertible_v<V, T> && ...) : storage{ elements... } { }

  template<typename T, size_t Columns, size_t Rows>
  class storage_t
  {
  protected:
    construct_storage

      basic_storage_t<T, Rows* Columns> storage{};
  };

  template<typename T>
  class storage_t<T, 1, 1>
  {
  protected:
    construct_storage

  public:
    union {
      struct {
        basic_value<T> x;
      };
      struct {
        basic_value<T> r;
      };
      basic_storage_t<T, 1> storage{};
    };
  };

  template<typename T>
  class storage_t<T, 1, 2>
  {
  protected:
    construct_storage

  public:
    union {
      struct {
        basic_value<T> x;
        basic_value<T> y;
      };
      struct {
        basic_value<T> r;
        basic_value<T> g;
      };
      basic_storage_t<T, 2> storage{};
    };
  };

  template<typename T>
  class storage_t<T, 1, 3>
  {
  protected:
    construct_storage

  public:
    union {
      struct {
        basic_value<T> x;
        basic_value<T> y;
        basic_value<T> z;
      };
      struct {
        basic_value<T> r;
        basic_value<T> g;
        basic_value<T> b;
      };
      basic_storage_t<T, 3> storage{};
    };
  };

  template<typename T>
  class storage_t<T, 1, 4>
  {
  protected:
    construct_storage

  public:
    union {
      struct {
        basic_value<T> x;
        basic_value<T> y;
        basic_value<T> z;
        basic_value<T> w;
      };
      struct {
        basic_value<T> r;
        basic_value<T> g;
        basic_value<T> b;
        basic_value<T> a;
      };
      basic_storage_t<T, 4> storage{};
    };
  };

  template<typename T, size_t C, size_t R>
  class mat : public storage_t<T, C, R>
  {
  public:
    using base = storage_t<T, C, R>;
    using base::storage;
    using returned_type = typename basic_value<T>::returned_type;
    using returned_const_type = typename basic_value<T>::returned_const_type;


    template<typename... V> requires((std::is_convertible_v<V, T> && ...) && sizeof...(V) <= C * R)
    constexpr mat(V&&... elements)
      : base{ elements... }
    {
    }

    template<typename... V>
    constexpr mat& operator=(V&&... elements) requires(std::is_reference_v<T>)
    {
      return assign(std::make_index_sequence<sizeof...(V)>{}, std::forward<V>(elements)...);
    }

  private:
    template<typename... V, size_t... S>
      constexpr mat& assign(std::index_sequence<S...>, V&&... elements) requires(std::is_reference_v<T>)
    {
      ((element_at(S) = std::forward<V>(elements)), ...);
      return *this;
    }

  public:
    template<typename V>
    constexpr operator mat<V, C, R>() const {
      return mget<V>(std::make_index_sequence<C* R>{});
    }

    template<typename V>
    constexpr operator mat<V, C, R>() {
      return mget<V>(std::make_index_sequence<C* R>{});
    }

    constexpr operator returned_type() requires(C == 1 && R == 1) {
      return element_at(0);
    }

    constexpr operator returned_const_type() const requires(C == 1 && R == 1) {
      return element_at(0);
    }

    constexpr decltype(auto) element_at(size_t element)
    {
      return storage.at(element);
    }

    constexpr decltype(auto) element_at(size_t element) const
    {
      return storage.at(element);
    }

    constexpr mat<T, 1, R>& at(size_t column)
    {
      return reinterpret_cast<mat<T, 1, R>&>(storage.at(column * R));
    }

    constexpr mat<T, 1, R> const& at(size_t column) const
    {
      return reinterpret_cast<mat<T, 1, R> const&>(storage.at(column * R));
    }

    constexpr decltype(auto) operator[](size_t column)
    {
      return at(column);
    }

    constexpr decltype(auto) operator[](size_t column) const
    {
      return at(column);
    }

    constexpr mat<T&, C, 1> row(size_t row)
    {
      return rget<T&>(std::make_index_sequence<C>{}, row);
    }

    constexpr mat<T const&, C, 1> row(size_t row) const
    {
      return rget<T const&>(std::make_index_sequence<C>{}, row);
    }

    constexpr decltype(auto) col(size_t column)
    {
      return at(column);
    }

    constexpr decltype(auto) col(size_t column) const
    {
      return at(column);
    }

  private:
    template<typename V, size_t... Elems>
    constexpr mat<V, C, R> mget(std::index_sequence<Elems...>) const
    {
      return mat<V, C, R>{ storage.at(Elems)... };
    }
    template<typename V, size_t... Elems>
    constexpr mat<V, C, R> mget(std::index_sequence<Elems...>)
    {
      return mat<V, C, R>{ storage.at(Elems)... };
    }
    template<typename V, size_t... Elems>
    constexpr mat<V, C, 1> rget(std::index_sequence<Elems...>, size_t off) const
    {
      return mat<V, C, 1>{ storage.at(Elems* R + off)... };
    }
    template<typename V, size_t... Elems>
    constexpr mat<V, C, 1> rget(std::index_sequence<Elems...>, size_t off)
    {
      return mat<V, C, 1>{ storage.at(Elems* R + off)... };
    }
  };

  template<typename T, size_t C, size_t R>
  constexpr mat<T, R, C> transpose(mat<T, C, R> src) {
    mat<T, R, C> result{};
    for (size_t c = 0; c < C; ++c)
      for (size_t r = 0; r < R; ++r)
        result.at(r).row(c) = src.at(c).row(r);
    return result;
  }

  template<typename T>
  constexpr auto operator*(mat<T, 1, 1> const& lhs, mat<T, 1, 1> const& rhs) {
    return lhs.element_at(0) * rhs.element_at(0);
  }

  template<typename T, size_t C>
  constexpr auto operator*(mat<T, C, 1> const& lhs, mat<T, C, 1> const& rhs) {
    using Ty = decltype(lhs.element_at(0) * rhs.element_at(0));
    mat<Ty, C, 1> result{};
    for (size_t c = 0; c < C; ++c)
      result[c] = lhs[c] * rhs[c];
    return result;
  }

  template<typename T, size_t C, size_t R, size_t R1>
  constexpr auto operator*(mat<T, C, R> const& lhs, mat<T, R1, C> const& rhs) requires(R != 1 || C != 1) {
    constexpr size_t result_columns = R;
    constexpr size_t result_rows = R1;
    using Ty = decltype(lhs.element_at(0)* rhs.element_at(0));

    mat<Ty, result_columns, result_rows> result{};

    for (size_t c = 0; c < result_columns; ++c)
      for (size_t r = 0; r < result_rows; ++r)
        for (size_t r1 = 0; r1 < C; ++r1)
          result.at(c).row(r) = lhs.at(r1).row(c) * rhs.at(r).row(r1);

    return result;
  }

  inline void XIOPASX() {
    float a = 1;
    float b = 3;
    float c = 4;
    float d = 6;

    mat<float&, 1, 1> x = a;

    x = b;

    using vec4 = mat<float, 1, 4>;
    using vec4_ref = mat<float&, 1, 4>;

    mat<float, 4, 1> base{ 1, 2, 3, 4 };

    mat<float, 4, 4> mat{
      1,0,0,5,
      0,1,0,5,
      0,0,1,5,
      0,0,0,1
    };

    auto ro = mat.row(2);
    auto co = mat.at(1);
    auto cn = transpose(co);
    auto cnmat = transpose(mat);

    auto mm = cn * co;
    auto sm = co * cn;
    auto ao = cn * cn;

    return;
  }
}