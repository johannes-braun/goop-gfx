#pragma once

#include <cstdint>
#include <cinttypes>

namespace goop
{
	template<typename T, typename C>
	static constexpr std::ptrdiff_t offset_of(T C::* member)
	{
		return (std::ptrdiff_t) & (((C*)nullptr)->*member) - (std::ptrdiff_t)nullptr;
	}

	template<typename T, typename C, size_t K>
	static constexpr std::ptrdiff_t offset_of(T(C::* member)[K], size_t index)
	{
		T(* const x)[K] = &(((C*)nullptr)->*(member));
		auto const p = (std::ptrdiff_t)(index * sizeof(T[2]) >> 1);
		return (std::ptrdiff_t)(x)-(std::ptrdiff_t)nullptr + p;
	}
}