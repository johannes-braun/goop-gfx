#include "symbol.hpp"

namespace goop::gui
{
	void symbol::set_holder(vector_graphics_holder holder)
	{
		_holder = std::move(holder);
		set_atlas(_holder.value()->atlas_texture());
		set_sdf_width(_holder.value()->sdf_width());
	}
	void symbol::set_icon(std::size_t index)
	{
		auto const item = _holder.value()->get(index);
		_icon.offset = item.bounds.position;
		_icon.scale = item.bounds.size;
		_icon.uv_offset = item.uvs.position;
		_icon.uv_scale = item.uvs.size;
		set_instances(std::span(&_icon, 1));
		set_default_size(_icon.scale);
	}
}