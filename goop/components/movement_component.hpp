#pragma once

#include <rnu/ecs/component.hpp>
#include <rnu/ecs/system.hpp>
#include <rnu/math/math.hpp>

#include "transform_component.hpp"
#include "collider_component.hpp"

namespace goop
{
  struct movement_component : rnu::component<movement_component>
  {
    rnu::vec3 move_by;
    rnu::quat rotate_by;
  };
  

}