#pragma once

#include <graphics.hpp>
#include <vectors/vectors.hpp>

namespace goop
{
	class vector_graphics_holder_base
	{
	public:
		struct set_symbol_t
		{
			rnu::rect2f bounds;
			rnu::rect2f uvs;
		};

		void load(int atlas_width, float sdf_width, std::span<goop::vector_image const> graphics);
		set_symbol_t get(std::size_t index) const;
		goop::texture const& atlas_texture();

	private:
		float _base_size = 0;
		float _sdf_width = 0;
		int _width = 0;
		int _height = 0;
		std::vector<std::uint8_t> _image;
		std::optional<goop::texture> _texture;
		std::vector<rnu::rect2f> _packed_bounds;
		std::vector<rnu::rect2f> _default_bounds;
	};

	class vector_graphics_holder : public handle<vector_graphics_holder_base, vector_graphics_holder_base>
	{
	public:
		vector_graphics_holder(int atlas_width, float sdf_width, std::span<goop::vector_image const> graphics)
		{
			(*this)->load(atlas_width, sdf_width, graphics);
		}
	};
}