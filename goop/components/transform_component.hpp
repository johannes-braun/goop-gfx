#pragma once

#include <rnu/ecs/component.hpp>
#include <rnu/math/math.hpp>

namespace goop
{
	struct transform_component : rnu::component<transform_component>
	{
		rnu::mat4 create_matrix() const 
		{
			return rnu::translation(position) * rnu::rotation(rotation) * rnu::scale(scale);
		}

		rnu::vec3 position{0,0,0};
		rnu::quat rotation{1,0,0,0};
		rnu::vec3 scale{1,1,1};
	};
}